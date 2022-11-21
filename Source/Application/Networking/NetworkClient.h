#pragma once

#include "Network.h"

//--------------------------------------------------------------------------------------------------------------
// TCP client
// Also the light piece player
//--------------------------------------------------------------------------------------------------------------
class NetworkClient final : public NetworkingBase
{
private:
    // Connections
    bool m_connected;
    SOCKET m_connection;

    // Client data
    std::vector<char> m_incomingBuffer;
    std::vector<char> m_outgoingBuffer;

public:
    NetworkClient(App* _pApp);
    virtual void Initialize() override;
    virtual void Shutdown() override;
    virtual void Update(bool gameRunning) override;
    virtual void HandleInput(const std::string& instruction) override;

private:
    virtual void WinsockUpdate() override;
    virtual void GameUpdate(bool gameRunning) override;
    virtual void OnMessage(const std::string& message) override;
};
