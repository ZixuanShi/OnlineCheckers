#include "Networking/Network.h"
#include "Checkers/CheckersBoard.h"
#include "Checkers/CheckersConstants.h"

#include <stdio.h>
#include <vector>
#include <SDL.h>

class NetworkingBase;

class App
{
private:
    // Constants
    static constexpr int kDelay = 0;

    // SDL
    SDL_Window* m_pWindow = nullptr;
    SDL_Renderer* m_pRenderer = nullptr;

    // Networking
    NetworkingBase* m_pNetwork = nullptr;   // Could be client or server

    // Game
    CheckersBoard m_board;
    bool m_running = true;

public:
    bool Initialize();
    void Shutdown();
    void Run();

    // Networking
    void Remove(size_t index) { m_board.Remove(index); }
    void Move(size_t fromIndex, size_t destIndex) { m_board.Move(fromIndex, destIndex); }
    AllPiecesIndex GetAllPiecesIndex() { return m_board.GetAllPiecesIndex(); }
    void PlacePiece(CheckersColor side, size_t index) { m_board.PlacePiece(side, index, m_pRenderer); }
    void Restart() { m_board.Restart(m_pRenderer); }
    bool Running() const { return m_running; }
    void Stop() { m_running = false; }  // This is called when I want to stop running but not deleting network stuff yet

private:
    bool InitSDL(bool isClient);
    void RenderWorld();
    void HandleInput();
};