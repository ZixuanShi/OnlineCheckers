#include "NetworkServer.h"

#include "Application/Application.h"
#include "Utils/Log/Log.h"
#include "Checkers/CheckersConstants.h"

NetworkServer::NetworkServer(App* _pApp)
    : NetworkingBase{ _pApp }
    , m_connections{}
{
}

void NetworkServer::Initialize()
{
    WSAData wsadata;
    int result = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (result != 0)
        return;

    m_listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(kServerPort);
    remoteAddr.sin_addr.s_addr = inet_addr(pServerIp);
    memset(remoteAddr.sin_zero, 0, sizeof(remoteAddr.sin_zero));

    bind(m_listener, reinterpret_cast<const sockaddr*>(&remoteAddr),
        sizeof(remoteAddr));

    result = listen(m_listener, 10);
    if (result < 0)
    {
        int socketError = WSAGetLastError();
        printf("Socket error: %d\n", socketError);
    }
    
    FD_ZERO(&m_socketFDs);
    FD_SET(m_listener, &m_socketFDs);

    Log::Get().PrintInColor(Log::Color::kLightGray, "You can press '");
    Log::Get().PrintInColor(Log::Color::kLightCyan, "%c", kRestartKey);
    Log::Get().PrintInColor(Log::Color::kLightGray, "' to restart\n");
    Log::Get().PrintInColor(Log::Color::kLightCyan, "Waiting for connections...\n");
}

void NetworkServer::Shutdown()
{
    for (auto& conn : m_connections)
        closesocket(conn.socket);

    closesocket(m_listener);
    
    WSACleanup();
}

void NetworkServer::Update(bool gameRunning)
{
    if (m_logTurn)
    {
        if (m_active)
            Log::Get().PrintInColor(Log::Color::kYellow, "Host's turn\n");
        else
            Log::Get().PrintInColor(Log::Color::kYellow, "Client's turn\n");
        m_logTurn = false;
    }

    WinsockUpdate();
    GameUpdate(gameRunning);
}

//--------------------------------------------------------------------------------------------------------------
// Processes the events we care about.  Returns true if we want to exit the program, false if not.
//--------------------------------------------------------------------------------------------------------------
void NetworkServer::HandleInput(const std::string& message)
{
    OnMessage(message);

    // Send message to the client that to set active
    if (message.compare(kRestart) != 0)
    {
        char activeMsg[kLimit];
        m_active = false;
        sprintf_s(activeMsg, kActive.c_str());
        SendToAll(activeMsg);
        m_logTurn = true;
    }
}

void NetworkServer::WinsockUpdate()
{
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000; // 1ms

    fd_set reads = m_socketFDs, writes = m_socketFDs, excepts = m_socketFDs;
    if (select(0, &reads, &writes, &excepts, &tv) <= 0)
        return; // nothing to do with sockets this update

    // Do we have a pending connection?
    if (FD_ISSET(m_listener, &reads))
    {
        sockaddr_in remoteAddr;
        int remoteAddrLen = sizeof(remoteAddr);

        Connection conn;
        conn.socket = accept(m_listener,
            reinterpret_cast<sockaddr*>(&remoteAddr), &remoteAddrLen);
        if (conn.socket != INVALID_SOCKET)
        {
            conn.nickname = inet_ntoa(remoteAddr.sin_addr);
            m_connections.emplace_back(conn);
            FD_SET(conn.socket, &m_socketFDs);
            printf("Accepted new connection from %s:%u\n",
                inet_ntoa(remoteAddr.sin_addr), ntohs(remoteAddr.sin_port));

            OnConnectionEstablished(*m_connections.rbegin());
        }
    }

    for (size_t i = 0; i < m_connections.size();)
    {
        auto& conn = m_connections[i];

        // Read
        if (FD_ISSET(conn.socket, &reads))
        {
            char buffer[8];
            int readBytes = recv(conn.socket, buffer, sizeof(buffer), 0);
            if (readBytes <= 0) // disconnect or error
            {
                if (readBytes < 0)
                {
                    printf("Socket error: %u\n", ::WSAGetLastError());
                }

                Log::Get().PrintInColor(Log::Color::kMagenta, "Connection lost.\n");
                closesocket(conn.socket);
                FD_CLR(conn.socket, &m_socketFDs);
                m_connections.erase(m_connections.begin() + i);
                continue;
            }

#if LOG_DATA
            printf("Received: %d bytes. (prev %d bytes)\n", readBytes, (int)conn.incomingBuffer.size());
#endif
            conn.incomingBuffer.insert(conn.incomingBuffer.end(),
                &buffer[0], &buffer[readBytes]);

            auto newline = std::find(conn.incomingBuffer.begin(), conn.incomingBuffer.end(), '\n');
            if (newline == conn.incomingBuffer.end())
                continue; // no full message yet

            std::string msg;
            msg.assign(conn.incomingBuffer.begin(), newline);
            conn.incomingBuffer.erase(conn.incomingBuffer.begin(), newline + 1);

            OnMessage(msg);
        }

        // Send
        if (FD_ISSET(conn.socket, &writes) && !conn.outgoingBuffer.empty())
        {
            char buffer[8];
            int bufferSize = min(sizeof(buffer), (int)conn.outgoingBuffer.size());
            memcpy(buffer, conn.outgoingBuffer.data(), bufferSize);

            int sentBytes = send(conn.socket, buffer, bufferSize, 0);
            if (sentBytes > 0)
            {
#if LOG_DATA
                printf("Sent %d/%d bytes.\n", sentBytes, (int)conn.outgoingBuffer.size());
#endif
                conn.outgoingBuffer.erase(conn.outgoingBuffer.begin(),
                    conn.outgoingBuffer.begin() + sentBytes);
            }
            else
            {
                if (sentBytes < 0)
                {
                    printf("Socket error: %u\n", ::WSAGetLastError());
                }

                Log::Get().PrintInColor(Log::Color::kMagenta, "Connection lost.\n");
                closesocket(conn.socket);
                FD_CLR(conn.socket, &m_socketFDs);
                m_connections.erase(m_connections.begin() + i);
                continue;
            }
        }

        ++i;
    }
}

//--------------------------------------------------------------------------------------------------------------
// Server:
//    On SPAWNREQUEST:
//      SpawnEntity()
//      Send SPAWN to Clients
//--------------------------------------------------------------------------------------------------------------
void NetworkServer::GameUpdate(bool gameRunning)
{
    if (!gameRunning)
        m_active = false;

    while (gameRunning)
    {
        char message[kLimit];

        Message* msg = GetNextMessage();
        if (!msg)
        {
            break;
        }

        // Remove
        if (msg->type == Message::Type::Kill)
        {
            auto* pKill = static_cast<KillMessage*>(msg);

            // Kill command sent by host/myself
            if (pKill->m_isHostCalling)
            {
                m_pApp->Remove(pKill->m_index);
                sprintf_s(message, kKill.c_str(), RevertedIndex((int)pKill->m_index), pKill->m_isHostCalling);
                Log::Get().PrintInColor(Log::Color::kLightGray, "REMOVED ");
                Log::Get().PrintInColor(Log::Color::kLightGreen, "%zd\n", pKill->m_index);
            }
            // Kill command sent by the client
            else
            {
                m_pApp->Remove(RevertedIndex((int)pKill->m_index));
                sprintf_s(message, kKill.c_str(), pKill->m_index, pKill->m_isHostCalling);
                Log::Get().PrintInColor(Log::Color::kLightGray, "REMOVED ");
                Log::Get().PrintInColor(Log::Color::kLightGreen, "%zd\n", RevertedIndex((int)pKill->m_index));
            }

            SendToAll(message);
        }

        // Move
        if (msg->type == Message::Type::Move)
        {
            // Move for this server's application
            auto* pMove = static_cast<MoveMessage*>(msg);

            // We need to adjust the moving piece's coords by 
            if (pMove->m_isHostCalling)
            {
                m_pApp->Move(pMove->m_fromIndex, pMove->m_destIndex);
                sprintf_s(message, kMove.c_str(), RevertedIndex((int)pMove->m_fromIndex), RevertedIndex((int)pMove->m_destIndex), pMove->m_isHostCalling);
                Log::Get().PrintInColor(Log::Color::kLightGray, "MOVED ");
                Log::Get().PrintInColor(Log::Color::kLightGreen, "%zd", pMove->m_fromIndex);
                Log::Get().PrintInColor(Log::Color::kLightGray, " TO ");
                Log::Get().PrintInColor(Log::Color::kLightGreen, "%zd\n", pMove->m_destIndex);
            }
            else
            {
                m_pApp->Move(RevertedIndex((int)pMove->m_fromIndex), RevertedIndex((int)pMove->m_destIndex));
                sprintf_s(message, kMove.c_str(), pMove->m_fromIndex, pMove->m_destIndex, pMove->m_isHostCalling);
                Log::Get().PrintInColor(Log::Color::kLightGray, "MOVED ");
                Log::Get().PrintInColor(Log::Color::kLightGreen, "%zd", RevertedIndex((int)pMove->m_fromIndex));
                Log::Get().PrintInColor(Log::Color::kLightGray, " TO ");
                Log::Get().PrintInColor(Log::Color::kLightGreen, "%zd\n", RevertedIndex((int)pMove->m_destIndex));
            }

            SendToAll(message);
        }

        // Active
        if (msg->type == Message::Type::Active)
        {
            m_active = true;
            m_logTurn = true;
        }

        // Restart
        if (msg->type == Message::Type::Restart)
        {
            m_pApp->Restart();
            Log::Get().PrintInColor(Log::Color::kLightCyan, "Game Restarted\n");
            sprintf_s(message, kRestart.c_str());
            SendToAll(message);
        }

        delete msg;
        msg = nullptr;
    }
}

//--------------------------------------------------------------------------------------------------------------
// Add the message to our incoming messages if it's reasonable
//--------------------------------------------------------------------------------------------------------------
void NetworkServer::OnMessage(const std::string& message)
{
    size_t fromIndex = kInvalidIndex;
    size_t destIndex = kInvalidIndex;
    size_t isHostCalling = 0;

    if (2 == sscanf_s(message.c_str(), (--kKill).c_str(), &destIndex, &isHostCalling))
        m_incomingMessages.emplace(new KillMessage(destIndex, isHostCalling));

    else if (3 == sscanf_s(message.c_str(), (--kMove).c_str(), &fromIndex, &destIndex, &isHostCalling))
        m_incomingMessages.emplace(new MoveMessage(fromIndex, destIndex, isHostCalling));

    else if (message.compare(--kActive) == 0)
        m_incomingMessages.emplace(new ActiveMessage());

    else if (message.compare(kRestart) == 0)
        m_incomingMessages.emplace(new RestartMessage());

    else
        Log::Get().PrintInColor(Log::Color::kMagenta, "Unhandled message.\n");
}

//--------------------------------------------------------------------------------------------------------------
// Called when we have a new established connection
//      - conn: the new connection we established
//--------------------------------------------------------------------------------------------------------------
void NetworkServer::OnConnectionEstablished(Connection& conn)
{
    // If connection is more than one, pop the extras with Game is Full message to that connection.
    if (m_connections.size() > 1)
    {
        conn.outgoingBuffer.insert(conn.outgoingBuffer.end(), kGameFull.begin(), kGameFull.end());
    }
    // If this is the only connection, send the checkers board data to it, and the current turn
    else
    {
        char message[kLimit];
        std::string msg;
        AllPiecesIndex allPiecesIndex = m_pApp->GetAllPiecesIndex();

        // All opponent/host/dark pieces from client/light POV
        for (size_t index : allPiecesIndex[(size_t)CheckersColor::kDark])
        {
            sprintf_s(message, kPiece.c_str(), (size_t)CheckersColor::kDark, RevertedIndex((int)index));
            msg = message;
            conn.outgoingBuffer.insert(conn.outgoingBuffer.end(), msg.begin(), msg.end());
        }

        // All ally/client/light pieces from client/light POV
        for (size_t index : allPiecesIndex[(size_t)CheckersColor::kLight])
        {
            sprintf_s(message, kPiece.c_str(), (size_t)CheckersColor::kLight, RevertedIndex((int)index));
            msg = message;
            conn.outgoingBuffer.insert(conn.outgoingBuffer.end(), msg.begin(), msg.end());
        }

        // Turn
        sprintf_s(message, kTurn.c_str(), !m_active);
        msg = message;
        conn.outgoingBuffer.insert(conn.outgoingBuffer.end(), msg.begin(), msg.end());
    }
}

void NetworkServer::SendToAll(const std::string& message)
{
    for (auto& conn : m_connections)
    {
        conn.outgoingBuffer.insert(conn.outgoingBuffer.end(),
            message.begin(), message.end());
    }
}