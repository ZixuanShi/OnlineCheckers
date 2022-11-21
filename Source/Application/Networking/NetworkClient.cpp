#include "NetworkClient.h"

#include "Application/Application.h"

NetworkClient::NetworkClient(App* _pApp)
    : NetworkingBase{ _pApp }
    , m_connection{}
    , m_connected{ false }
{
    m_logTurn = false;
}

void NetworkClient::Initialize()
{
    WSAData wsadata;
    int result = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (result > 0)
        return;
    
    m_connection = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    u_long on = 1;
    ioctlsocket(m_connection, FIONBIO, &on);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)kServerPort);
    addr.sin_addr.s_addr = inet_addr(pServerIp);

    connect(m_connection, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
}

void NetworkClient::Shutdown()
{
    WSACleanup();
}

void NetworkClient::Update(bool gameRunning)
{
    if (m_logTurn)
    {
        if (m_active)
            Log::Get().PrintInColor(Log::Color::kYellow, "Client's turn\n");
        else
            Log::Get().PrintInColor(Log::Color::kYellow, "Host's turn\n");
        m_logTurn = false;
    }

    WinsockUpdate();
    GameUpdate(gameRunning);
}

//--------------------------------------------------------------------------------------------------------------
// Processes the events we care about.  Returns true if we want to exit the program, false if not.
//--------------------------------------------------------------------------------------------------------------
void NetworkClient::HandleInput(const std::string& message)
{
    m_outgoingBuffer.insert(m_outgoingBuffer.end(), message.begin(), message.end());

    m_active = false;
    m_outgoingBuffer.insert(m_outgoingBuffer.end(), kActive.begin(), kActive.end());
    m_logTurn = true;
}

void NetworkClient::WinsockUpdate()
{
    fd_set reads, writes, excepts;
    FD_ZERO(&reads);
    FD_ZERO(&writes);
    FD_ZERO(&excepts);
    FD_SET(m_connection, &reads);
    FD_SET(m_connection, &writes);
    FD_SET(m_connection, &excepts);
    if (select(0, &reads, &writes, &excepts, nullptr) <= 0)
        return;

    // If there is no server when this client launch, quit application
    if (!FD_ISSET(m_connection, &writes))
    {
        Log::Get().PrintInColor(Log::Color::kMagenta, "Couldn't found server, quiting...\n");
        m_pApp->Stop();
        return;
    }

    // Init connection
    if (!m_connected && FD_ISSET(m_connection, &writes))
    {
        // Receive a message see if the game is full or not
        Log::Get().PrintInColor(Log::Color::kLightCyan, "Connection established!\n");
        m_connected = true;
        return;
    }

    // Sending
    if (m_connected && FD_ISSET(m_connection, &writes) && !m_outgoingBuffer.empty())
    {
        char buffer[64];
        int bufferSize = min(sizeof(buffer), (int)m_outgoingBuffer.size());
        memcpy(buffer, m_outgoingBuffer.data(), bufferSize);

        int sentBytes = send(m_connection, buffer, bufferSize, 0);
        if (sentBytes > 0)
        {
#if LOG_DATA
            printf("Sent %d/%d bytes.\n", sentBytes, (int)m_outgoingBuffer.size());
#endif
            m_outgoingBuffer.erase(m_outgoingBuffer.begin(),
                m_outgoingBuffer.begin() + sentBytes);
        }
        else
        {
            if (sentBytes < 0 && !m_active)
                printf("Socket error: %u\n", ::WSAGetLastError());

            Log::Get().PrintInColor(Log::Color::kMagenta, "Connection lost.\n");
            closesocket(m_connection);
            m_connected = false;
        }
    }

    // Readings
    if (FD_ISSET(m_connection, &reads))
    {
        char buffer[8];
        int readBytes = recv(m_connection, buffer, sizeof(buffer), 0);
        if (readBytes <= 0 && !m_active) // disconnect or error
        {
            if (readBytes < 0)
                printf("Socket error: %u\n", ::WSAGetLastError());

            Log::Get().PrintInColor(Log::Color::kMagenta, "Connection lost.\n");
            closesocket(m_connection);
            m_connected = false;
            return;
        }

#if LOG_DATA
        printf("Received: %d bytes. (prev %d bytes)\n", readBytes, (int)m_incomingBuffer.size());
#endif
        m_incomingBuffer.insert(m_incomingBuffer.end(),
            &buffer[0], &buffer[readBytes]);

        auto newline = std::find(m_incomingBuffer.begin(), m_incomingBuffer.end(), '\n');
        if (newline == m_incomingBuffer.end())
            return; // no full message yet

        std::string msg;
        msg.assign(m_incomingBuffer.begin(), newline);
        m_incomingBuffer.erase(m_incomingBuffer.begin(), newline + 1);

        OnMessage(msg);
    }
}

//--------------------------------------------------------------------------------------------------------------
// Client:
//    On spacebar:
//      Request entity spawn on server:
//        Create a NetworkClient->RequestSpawnEntity(...)
//    On SPAWN:
//        Add entity to m_entities
//--------------------------------------------------------------------------------------------------------------
void NetworkClient::GameUpdate(bool gameRunning)
{
    if (!gameRunning)
        m_active = false;

    while (m_pApp->Running() && gameRunning)
    {
        Message* msg = GetNextMessage();
        if (!msg)
        {
            break;
        }

        // Remove
        if (msg->type == Message::Type::Kill)
        {
            auto* pKill = static_cast<KillMessage*>(msg);
            m_pApp->Remove(pKill->m_index);
            Log::Get().PrintInColor(Log::Color::kLightGray, "REMOVED ");
            Log::Get().PrintInColor(Log::Color::kLightGreen, "%zd\n", pKill->m_index);
        }

        // Move
        if (msg->type == Message::Type::Move)
        {
            auto* pMove = static_cast<MoveMessage*>(msg);
            m_pApp->Move(pMove->m_fromIndex, pMove->m_destIndex);
            Log::Get().PrintInColor(Log::Color::kLightGray, "MOVED ");
            Log::Get().PrintInColor(Log::Color::kLightGreen, "%zd", pMove->m_fromIndex);
            Log::Get().PrintInColor(Log::Color::kLightGray, " TO ");
            Log::Get().PrintInColor(Log::Color::kLightGreen, "%zd\n", pMove->m_destIndex);
        }

        // Full
        if (msg->type == Message::Type::GameFull)
        {
            // Shut down local application
            m_pApp->Stop();
            Log::Get().PrintInColor(Log::Color::kMagenta, "Game is full, quiting...\n");
        }

        // Active
        if (msg->type == Message::Type::Active)
        {
            m_active = true;
            m_logTurn = true;
        }

        // Place Piece
        if (msg->type == Message::Type::Piece)
        {
            auto* pPiece = static_cast<PieceMessage*>(msg);
            m_pApp->PlacePiece((CheckersColor)pPiece->m_side, pPiece->m_destIndex);
        }

        // Turn
        if (msg->type == Message::Type::Turn)
        {
            auto* pTurn = static_cast<TurnMessage*>(msg);
            m_active = pTurn->m_side;
            m_logTurn = true;
        }

        // Restart
        if (msg->type == Message::Type::Restart)
        {
            m_pApp->Restart();
            Log::Get().PrintInColor(Log::Color::kLightCyan, "Game Restarted\n");
        }

        delete msg;
        msg = nullptr;
    }
}

void NetworkClient::OnMessage(const std::string& message)
{
    size_t side = kInvalidIndex;
    size_t fromIndex = kInvalidIndex;
    size_t destIndex = kInvalidIndex;
    size_t isHostCalling = 0;

    if (2 == sscanf_s(message.c_str(), (--kKill).c_str(), &destIndex, &isHostCalling))
        m_incomingMessages.emplace(new KillMessage(destIndex, isHostCalling));

    else if (3 == sscanf_s(message.c_str(), (--kMove).c_str(), &fromIndex, &destIndex, &isHostCalling))
        m_incomingMessages.emplace(new MoveMessage(fromIndex, destIndex, isHostCalling));

    else if (message.compare(--kGameFull) == 0)
        m_incomingMessages.emplace(new GameFullMessage());

    else if (message.compare(--kActive) == 0)
        m_incomingMessages.emplace(new ActiveMessage());

    else if (2 == sscanf_s(message.c_str(), (--kPiece).c_str(), &side, &destIndex))
        m_incomingMessages.emplace(new PieceMessage(side, destIndex));

    else if (1 == sscanf_s(message.c_str(), (--kTurn).c_str(), &side))
        m_incomingMessages.emplace(new TurnMessage(side));

    else if (message.compare(--kRestart) == 0)
        m_incomingMessages.emplace(new RestartMessage());

    else
        Log::Get().PrintInColor(Log::Color::kMagenta, "Unhandled message.\n");
}