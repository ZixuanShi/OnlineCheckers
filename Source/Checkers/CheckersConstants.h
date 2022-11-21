#pragma once

#include "Utils/Math/Vector2.h"
#include <string>
#include <numeric>
#include <utility>
#include <vector>
#include <array>
#include <SDL.h>

//--------------------------------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------------------------------
// Application
static constexpr int kWindowWidth = 512;
static constexpr int kWindowHeight = 512;

// Game map
static constexpr size_t kBoardWidth = 8;
static constexpr size_t kBoardHeight = 8;
static constexpr size_t kBoardSize = kBoardWidth * kBoardHeight;
static constexpr int kTileWidth = kWindowWidth / kBoardWidth;
static constexpr int kTileHeight = kWindowHeight / kBoardHeight;

// Gameplay
static constexpr size_t kInvalidIndex = (std::numeric_limits<size_t>::max)();
static constexpr SDL_KeyCode kRestartKey = SDLK_r;		// Server press this char to restart

// Networking messages
static constexpr size_t kLimit = 128;
inline static const std::string kKill = "KILL %zd %zd\n";		// Kill index, isHostcCalling
inline static const std::string kMove = "MOVE %zd %zd %zd\n";	// From index, dest Index, isHostCalling
inline static const std::string kGameFull = "GAME IS FULL\n";	
inline static const std::string kActive = "ACTIVE\n";	
inline static const std::string kPiece = "PIECE %zd AT %zd\n";	// zd for piece's side (0 for Host/Dark or 1 for Client/Light), zd for index
inline static const std::string kTurn = "TURN %zd\n";			// zd for turn (0 for Host/Dark or 1 for Client/Light)
inline static const std::string kRestart = "RESTART\n";		

//--------------------------------------------------------------------------------------------------------------
// Enums
//--------------------------------------------------------------------------------------------------------------
// Can be used for both player and tile color, kContinue is for checking winners
enum class CheckersColor : size_t
{
	kDark = 0,		// Host
	kLight = 1,		// Client

	kCount = 2,		// Count for sides
	kContinue = 3	// If current game state has no winner, return this in check winner function
};

//--------------------------------------------------------------------------------------------------------------
// Structs
//--------------------------------------------------------------------------------------------------------------
// Used when moving a piece, bool is if it's a valid move, size_t is the opponent's piece index if jump occured, kInvalidIndex if the move didn't involve killing
struct MoveResult
{
	size_t m_destIndex = kInvalidIndex;
	std::vector<size_t> m_piecesToKill;

	MoveResult() 
		: m_destIndex{ kInvalidIndex }
		, m_piecesToKill{  }
	{}
};

//--------------------------------------------------------------------------------------------------------------
// Alias
//--------------------------------------------------------------------------------------------------------------
using AllPiecesIndex = std::array<std::vector<size_t>, (size_t)CheckersColor::kCount>;	// two vectors of index, one for dark and the other for light

//--------------------------------------------------------------------------------------------------------------
// Helper functions
//--------------------------------------------------------------------------------------------------------------
// Return a string without "\n". 
inline std::string operator--(const std::string& input)
{
    return input.substr(0, input.find_last_of("\n"));
}

// Returns a dark side tile index to light side tile index
inline size_t RevertedIndex(int indexToRevert)
{
	return std::abs(indexToRevert - ((int)kBoardSize - 1));
}

// Return the tile's index of x and y pos
//      -x: The X horizontal position on the board
//      -y: The Y vertical position on the board
constexpr size_t GetIndexFromPos(size_t x, size_t y) 
{ 
	return (y * kBoardWidth) + x; 
}

// Return the tile's index of pixelX, pixelY coords
//      -pixelX: The X pixel position on the screen where the mouse clicked.
//      -pixelY: The Y pixel position on the screen where the mouse clicked.
constexpr size_t GetIndexFromPixel(int pixelX, int pixelY)
{
	int gridX = pixelX / kTileWidth;
	int gridY = pixelY / kTileHeight;
	return GetIndexFromPos(gridX, gridY);
}

// Return a Vector2 by index
//		-index: The index to get Vector2
inline Vector2 GetVec2FromIndex(size_t index)
{
	size_t x = index % kBoardWidth;
	size_t y = index / kBoardWidth;
	return Vector2((float)x, (float)y);
}