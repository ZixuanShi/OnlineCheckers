#include "CheckersBoard.h"

#include "Application/Networking/NetworkClient.h"
#include "Application/Networking/NetworkServer.h"
#include "Utils/Log/Log.h"

#include "GameState.h"

CheckersBoard::CheckersBoard()
	: m_running{ true }
	, m_isSelecting{ true }
	, m_holdingPieceIndex{ kInvalidIndex }
{
}

//---------------------------------------------------------------------------------------------------------------------
// Init game state
//	-pRenderer: I need this renderer to create piece texture
//---------------------------------------------------------------------------------------------------------------------
void CheckersBoard::Init(SDL_Renderer* pRenderer, bool isClient)
{
	m_currentState.Init(pRenderer, isClient);
}

//---------------------------------------------------------------------------------------------------------------------
// Called every frame to render the world.
//---------------------------------------------------------------------------------------------------------------------
void CheckersBoard::Render(SDL_Renderer* pRenderer) const
{
	// SDL draw game board state
	m_currentState.Render(pRenderer);
}

void CheckersBoard::Shutdown()
{
	m_running = false;
}

void CheckersBoard::Remove(size_t index)
{
	m_currentState.KillPieceAt(index);
}

void CheckersBoard::Move(size_t fromIndex, size_t destIndex)
{
	m_currentState.MovePiece(fromIndex, destIndex);
}

//---------------------------------------------------------------------------------------------------------------------
// Checks to see if we should continue running the game
//      -return:    true if continue running, false if not.
//---------------------------------------------------------------------------------------------------------------------
bool CheckersBoard::ShouldContinue()
{
	// Continue game
	if (m_currentState.CheckerWinner() == CheckersColor::kContinue)
		return m_running;

	// Has a winner
	static bool s_logged = false;
	if (!s_logged)
	{
		Log::Get().PrintInColor(Log::Color::kLightGray, "Winner is ");
		Log::Get().PrintInColor(Log::Color::kLightCyan, "%s\n", (m_currentState.CheckerWinner() == CheckersColor::kDark ? ("Charmander") : ("Pikachu")));
		s_logged = true;
	}

	m_running = false;
	return m_running;
}

//--------------------------------------------------------------------------------------------------------------
// Processes the events we care about.  Returns true if we want to exit the program, false if not.
//--------------------------------------------------------------------------------------------------------------
bool CheckersBoard::HandleInput(SDL_Event* pEvent, NetworkingBase* pNetwork)
{
	char msg[kLimit];

	switch (pEvent->type)
	{
	case SDL_KEYDOWN:
		// Restart
		if (pEvent->key.keysym.sym == kRestartKey && m_currentState.GetPlayer() == CheckersColor::kDark)
		{
			sprintf_s(msg, kRestart.c_str());
			pNetwork->HandleInput(msg);
		}
		break;

	case SDL_MOUSEBUTTONDOWN:
		// Select a piece to move
		if (m_isSelecting)
		{
			m_holdingPieceIndex = m_currentState.OnSelected(pEvent->button.x, pEvent->button.y);

			// If the position we clicked has a piece on it, we are not moving this piece
			if (m_holdingPieceIndex != kInvalidIndex)
				m_isSelecting = false;
		}
		// Try to drop if the move is legit, notify networking to work
		else
		{
			// If we succeded moved the holding piece to the new destination, tell the network to work
			MoveResult moveResult = m_currentState.IsValidMove(pEvent->button.x, pEvent->button.y);

			// If it's a legit move, notify network to perform so
			if (moveResult.m_destIndex != kInvalidIndex)
			{
				sprintf_s(msg, kMove.c_str(), m_holdingPieceIndex, moveResult.m_destIndex, size_t(m_currentState.GetPlayer() == CheckersColor::kDark));
				pNetwork->HandleInput(msg);

				for (size_t pieceToKill : moveResult.m_piecesToKill)
				{
					sprintf_s(msg, kKill.c_str(), pieceToKill, m_currentState.GetPlayer() == CheckersColor::kDark);
					pNetwork->HandleInput(msg);
				}
			}
			// If it's not legit, make the selected piece back to original position
			else
			{
				Log::Get().PrintInColor(Log::Color::kMagenta, "Invalid move\n");
				m_currentState.ResetSelectedPiece(m_holdingPieceIndex);
			}

			// Set holding piece, high-lighted tiles, and selecting back
			m_holdingPieceIndex = kInvalidIndex;
			m_isSelecting = true;
			m_currentState.ResetHighlightedTiles();
		}

		break;
	}

	return m_running;
}
