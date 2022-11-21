#include "Application.h"

#include "Networking/NetworkClient.h"
#include "Networking/NetworkServer.h"
#include "Checkers/CheckersBoard.h"
#include "Utils/Log/Log.h"

#include <SDL_mixer.h>
#include <SDL_image.h>

//--------------------------------------------------------------------------------------------------------------
// Initialize SDL, Create client or server based on user's choice
//--------------------------------------------------------------------------------------------------------------
bool App::Initialize()
{
    bool isClient = (IDYES == ::MessageBoxA(NULL, "Would you like to run as a client?", "Client or Server?", MB_YESNO | MB_ICONQUESTION));

    // SDL
    if (!InitSDL(isClient))
        return false;

    // Networking
    if (isClient)
        m_pNetwork =  new NetworkClient(this);
    else
        m_pNetwork = new NetworkServer(this);
    m_pNetwork->Initialize();

    // Game
    m_board.Init(m_pRenderer, isClient);

    return true;
}

//--------------------------------------------------------------------------------------------------------------
// Shutdown Game, SDL, Destroy client or server
//--------------------------------------------------------------------------------------------------------------
void App::Shutdown()
{
    // Game
    m_running = false;
    m_board.Shutdown();

    // Networking
    if (m_pNetwork)
    {
        m_pNetwork->Shutdown();
        delete m_pNetwork;
        m_pNetwork = nullptr;
    }

    // SDL
    if (m_pRenderer) SDL_DestroyRenderer(m_pRenderer);
    if (m_pWindow) SDL_DestroyWindow(m_pWindow);
    Mix_CloseAudio(); 
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
}

//--------------------------------------------------------------------------------------------------------------
// Run game loop
//--------------------------------------------------------------------------------------------------------------
void App::Run()
{
    while (m_running)
    {
        HandleInput();
        RenderWorld(); 
        m_pNetwork->Update(m_board.ShouldContinue());
        SDL_Delay(kDelay);
    }
}

//--------------------------------------------------------------------------------------------------------------
// Init SDL
//  - Window
//  - Renderer
//  - Music
//  - IMG
//--------------------------------------------------------------------------------------------------------------
bool App::InitSDL(bool isClient)
{
    // SDL
    SDL_Init(SDL_INIT_VIDEO);

    // Window and renderer
    if (-1 == SDL_CreateWindowAndRenderer(kWindowWidth, kWindowHeight, SDL_WINDOW_SHOWN, &m_pWindow, &m_pRenderer))
    {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return false;
    }
    SDL_SetWindowTitle(m_pWindow, isClient ? "Client" : "Server");

    // Music
    if (SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        SDL_Log("Failed to initialize SDL Audio: %s", Mix_GetError());
        return false;
    }
    Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024);
    if (0 == Mix_Init(MIX_INIT_FLAC | MIX_INIT_MOD | MIX_INIT_MP3 | MIX_INIT_OGG))
    {
        SDL_Log("Failed to create SDL Mixer: %s", SDL_GetError());
        return false;
    }

    // Image
    if (0 == IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG))
    {
        SDL_Log("Failed to create Image: %s", SDL_GetError());
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------------------
// Draw entities
//--------------------------------------------------------------------------------------------------------------
void App::RenderWorld()
{
    SDL_SetRenderDrawColor(m_pRenderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(m_pRenderer);
    m_board.Render(m_pRenderer);
    SDL_RenderPresent(m_pRenderer);
}

//--------------------------------------------------------------------------------------------------------------
// Let networking handle SDL event
//--------------------------------------------------------------------------------------------------------------
void App::HandleInput()
{
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent))
    {
        if (sdlEvent.type == SDL_QUIT)
        {
            m_running = false;
            m_board.Shutdown();
        }

        if (m_pNetwork->Active())
        {
            m_running = m_board.HandleInput(&sdlEvent, m_pNetwork);
        }
    }
}