#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>

#include "Utils/Log/Log.h"

#include <string>
#include <vector>
#include <queue>
#include <SDL.h>
#include <assert.h>

//--------------------------------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------------------------------
static constexpr const char* pServerIp = "127.0.0.1";
static constexpr unsigned int kServerPort = 6565;

//--------------------------------------------------------------------------------------------------------------
// Events for connections
//--------------------------------------------------------------------------------------------------------------
struct Message
{
    enum class Type
    {
        Unknown,
        Kill,
        Move,
        GameFull,
        Active,
        Piece,
        Turn,
        Restart
    };

    Type type;
};

// Base for message event
template <Message::Type MessageType>
struct MessageBase : Message
{
    MessageBase() { type = MessageType; }
};

// Remove a piece event
struct KillMessage : MessageBase<Message::Type::Kill>
{
    size_t m_index = 0;
    size_t m_isHostCalling = 0;
    KillMessage(size_t index, size_t isHostCalling)
        : m_index{ index }
        , m_isHostCalling{ isHostCalling }
    {        
        assert(m_isHostCalling == 0 || m_isHostCalling == 1);
    }
};

// Move a piece to the destindex event
struct MoveMessage : MessageBase<Message::Type::Move>
{
    size_t m_fromIndex = 0;
    size_t m_destIndex = 0;
    size_t m_isHostCalling = 0;    // 0 for client calling, 1 for host calling
    MoveMessage(size_t fromIndex, size_t destIndex, size_t isHostCalling)
        : m_fromIndex{ fromIndex }
        , m_destIndex{ destIndex }
        , m_isHostCalling{ isHostCalling }
    {
        assert(m_isHostCalling == 0 || m_isHostCalling == 1);
    }
};

// Game is Full event
struct GameFullMessage : MessageBase<Message::Type::GameFull>
{
};

// Set active event
struct ActiveMessage : MessageBase<Message::Type::Active>
{
};

struct RestartMessage : MessageBase<Message::Type::Restart>
{
};

// Set active event
struct PieceMessage : MessageBase<Message::Type::Piece>
{
    size_t m_side = 0;  // 0 for Host/Dark, 1 for Client/Light
    size_t m_destIndex = 0;
    PieceMessage(size_t side, size_t destIndex) 
        : m_side{ side }
        , m_destIndex { destIndex } 
    {
        assert(m_side == 0 || side == 1);
    }
};

// Turn
struct TurnMessage : MessageBase<Message::Type::Turn>
{
    size_t m_side = 0;  // 0 for Host/Dark, 1 for Client/Light
    TurnMessage(size_t side)
        : m_side{ side }
    {
        assert(m_side == 0 || side == 1);
    }
};

//--------------------------------------------------------------------------------------------------------------
// Base class for networking
//--------------------------------------------------------------------------------------------------------------
#define LOG_DATA 0
class App;

class NetworkingBase
{
protected:
    std::queue<Message*> m_incomingMessages;
    App* m_pApp;
    bool m_active;      // Whether or not this network should handle input
    bool m_logTurn;

public:
    NetworkingBase(App* _pApp) 
        : m_pApp{ _pApp }
        , m_active{ true }
        , m_logTurn{ true }
    {}

    virtual ~NetworkingBase()
    {
        while (!m_incomingMessages.empty())
        {
            Message* pMsg = m_incomingMessages.front();
            delete pMsg;
            pMsg = nullptr;
            m_incomingMessages.pop();
        }
    }

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Update(bool gameRunning) = 0;
    virtual void HandleInput(const std::string& instruction) = 0;

    bool Active() const { return m_active; }
    Message* GetNextMessage()
    {
        if (m_incomingMessages.empty())
            return nullptr;
        auto msg = m_incomingMessages.front();
        m_incomingMessages.pop();
        return msg;
    }

protected:
    virtual void OnMessage(const std::string& message) = 0;
    virtual void WinsockUpdate() = 0;
    virtual void GameUpdate(bool gameRunning) = 0;
};

