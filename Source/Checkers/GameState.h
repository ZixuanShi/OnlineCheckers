#pragma once

#include "Tile.h"
#include "Checkers/CheckersConstants.h"
#include "Utils/Math/Vector2.h"

#include <unordered_set>
#include <unordered_map>
#include <SDL.h>
#include <queue>

//--------------------------------------------------------------------------------------------------------------
// Represents Checkers game state
//--------------------------------------------------------------------------------------------------------------
class GameState
{
	enum class Pos
	{
		kLeftFwd,
		kRightFwd,
		kLeftBack,
		kRightBack,
	};

	// Constants
	static constexpr size_t kSpawnBottomPieceRow = 4;
	static constexpr size_t kSpawnTopPieceRow = 3;

	// Game map array
	Tile m_tiles[kBoardSize];	

	// Used for tracking winners
	CheckersColor m_currentPlayer;
	bool m_doneInit;

	// Store pieces index
	std::unordered_set<size_t> m_myPieces;
	std::unordered_set<size_t> m_otherPieces;
	std::unordered_map<size_t, std::vector<size_t>> m_pieceInDanger;	// For removing other's piece when jump occurs, first size_t for storing the destination tile index, second size_t for storing the other's piece in danger

public:
	GameState();

	void Init(SDL_Renderer* pRenderer, bool isClient);
	void Render(SDL_Renderer* pRenderer) const;

	void MovePiece(size_t fromIndex, size_t destIndex);
	void KillPieceAt(size_t index);
	void ResetSelectedPiece(size_t tileIndex);
	void ResetHighlightedTiles();
	void Restart(SDL_Renderer* pRenderer);
	void PlacePiece(CheckersColor side, size_t index, SDL_Renderer* pRenderer);
	size_t OnSelected(Sint32 mouseX, Sint32 mouseY);
	MoveResult IsValidMove(Sint32 mouseX, Sint32 mouseY);
	CheckersColor CheckerWinner() const;
	CheckersColor GetPlayer() const { return m_currentPlayer; }
	AllPiecesIndex GetAllPiecesIndex();

private:
	void InitMap(SDL_Renderer* pRenderer, bool isRestarting);
	void HighLightAllPossibleTiles(size_t beginIndex);
	bool IsInDanger(size_t index) const;
	void ProcessHighLightTile(bool moveCondition, size_t targetTileIndex, bool jumpCondition, size_t jumpIndex, std::unordered_map<Pos, size_t>& moveMap, size_t currentIndex, std::queue<size_t>& startPoints, Pos dir);
};

