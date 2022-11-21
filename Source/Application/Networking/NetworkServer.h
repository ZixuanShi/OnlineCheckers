#pragma once

#include "Network.h"

//--------------------------------------------------------------------------------------------------------------
// The host is authoritative over the game state and is the one listening for connections.
// Also the dark piece player
//--------------------------------------------------------------------------------------------------------------
class NetworkServer final : public NetworkingBase
{
private:
    struct Connection
    {
        SOCKET socket;
        std::string nickname;
        std::vector<char> incomingBuffer;
        std::vector<char> outgoingBuffer;
    };

    // Connections
    SOCKET m_listener;
    fd_set m_socketFDs;
    std::vector<Connection> m_connections;

public:
    NetworkServer(App* _pApp);
    virtual void Initialize() override;
    virtual void Shutdown() override;
    virtual void Update(bool gameRunning) override;
    virtual void HandleInput(const std::string& instruction) override;

private:
    virtual void OnMessage(const std::string& message) override;
    virtual void WinsockUpdate() override;
    virtual void GameUpdate(bool gameRunning) override;
    void SendToAll(const std::string& message);
    void OnConnectionEstablished(Connection& conn);
};