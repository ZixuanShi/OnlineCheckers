#pragma once

#include "GameState.h"
#include "CheckersConstants.h"

#include <SDL.h>

class NetworkingBase;

//--------------------------------------------------------------------------------------------------------------
// Represents the checkers world
//--------------------------------------------------------------------------------------------------------------
class CheckersBoard
{
private:
	GameState m_currentState;
	bool m_running;
	bool m_isSelecting;	// If current player should select or drop a piece
	size_t m_holdingPieceIndex;
	
public:
	CheckersBoard();

	void Init(SDL_Renderer* pRenderer, bool isClient);
	void Render(SDL_Renderer* pRenderer) const;
	void Shutdown();
	bool HandleInput(SDL_Event* pEvent, NetworkingBase* pNetwork);

	void Remove(size_t index);
	void Move(size_t fromIndex, size_t destIndex);
	void Restart(SDL_Renderer* pRenderer) { m_currentState.Restart(pRenderer); }
	bool ShouldContinue();
	AllPiecesIndex GetAllPiecesIndex() { return m_currentState.GetAllPiecesIndex(); }
	void PlacePiece(CheckersColor side, size_t index, SDL_Renderer* pRenderer) { m_currentState.PlacePiece(side, index, pRenderer); }
};

