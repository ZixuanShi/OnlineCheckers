#include "GameState.h"

#include "CheckersBoard.h"
#include "Piece.h"

#include <unordered_map>

// When set to 1, only spawn two pieces
#define TESTING 0

GameState::GameState()
	: m_tiles{}
	, m_myPieces{}
	, m_otherPieces{}
	, m_currentPlayer{ CheckersColor::kDark }
	, m_doneInit{ false }
{
}

//---------------------------------------------------------------------------------------------------------------------
// Reset game map
//		-pRenderer: I need this renderer to create piece texture
//		-isClient: If this is an client, then this board should place light pieces at the bottom, vice-versa
//---------------------------------------------------------------------------------------------------------------------
void GameState::Init(SDL_Renderer* pRenderer, bool isClient)
{
	if (isClient)
		m_currentPlayer = CheckersColor::kLight;
	else
	{
		m_doneInit = true;
		m_currentPlayer = CheckersColor::kDark;
	}

	// Set up game map
	InitMap(pRenderer, false);
}

//---------------------------------------------------------------------------------------------------------------------
// Draw tiles and pieces on it
//---------------------------------------------------------------------------------------------------------------------
void GameState::Render(SDL_Renderer* pRenderer) const
{
	// Walk through tiles and render each of them
	for (const Tile& tile : m_tiles)
		tile.Render(pRenderer);
}

//---------------------------------------------------------------------------------------------------------------------
// Returns the game state winner or continue
//---------------------------------------------------------------------------------------------------------------------
CheckersColor GameState::CheckerWinner() const
{
	// If there is no single piece on board, means it's the client's end and it's not done initialing, return continue
	if (!m_doneInit)
		return CheckersColor::kContinue;

	// I won
	if (m_myPieces.size() <= 0)
		return (m_currentPlayer == CheckersColor::kDark) ? (CheckersColor::kLight) : (CheckersColor::kDark);

	// other won
	if (m_otherPieces.size() <= 0)
		return m_currentPlayer;

	// Continue
	return CheckersColor::kContinue;
}

//---------------------------------------------------------------------------------------------------------------------
// Called whenever we get a tile select event. Returns selected tile index if it's valid, kInvalidIndex if not
//      -mouseX: The X pixel position on the screen where the mouse clicked.
//      -mouseY: The Y pixel position on the screen where the mouse clicked.
//---------------------------------------------------------------------------------------------------------------------
size_t GameState::OnSelected(Sint32 mouseX, Sint32 mouseY)
{
	size_t index = GetIndexFromPixel(mouseX, mouseY);

	// Get the piece on it
	Piece* pSelectedPiece = m_tiles[index].GetPiece();

	// if there is no piece on it, or it's not my piece, return invalid index
	if (!pSelectedPiece || m_myPieces.find(index) == m_myPieces.end())
		return kInvalidIndex;

	// If we reach this point, means the selected piece is valid and is mine.
	// Perform on selected behavior of this piece
	pSelectedPiece->OnSelected();

	// High light all possible moves
	HighLightAllPossibleTiles(index);

	return index;
}

//---------------------------------------------------------------------------------------------------------------------
// Called when the movement was invalid
//		-tileIndex: The selected piece's tile index
//---------------------------------------------------------------------------------------------------------------------
void GameState::ResetSelectedPiece(size_t tileIndex)
{
	m_tiles[tileIndex].GetPiece()->UnSelect();
}

//---------------------------------------------------------------------------------------------------------------------
// Return if we have a valid move and the removing piece
//		-fromIndex: The current tile index of the selected piece standing on
//      -mouseX: The X pixel position on the screen where the mouse clicked.
//      -mouseY: The Y pixel position on the screen where the mouse clicked.
//---------------------------------------------------------------------------------------------------------------------
MoveResult GameState::IsValidMove(Sint32 mouseX, Sint32 mouseY)
{
	size_t destIndex = GetIndexFromPixel(mouseX, mouseY);
	MoveResult result;

	if (m_tiles[destIndex].HighLighted())
	{
		result.m_destIndex = destIndex;

		// If we are killing anyone
		if (m_pieceInDanger.find(destIndex) != m_pieceInDanger.end())
			result.m_piecesToKill = m_pieceInDanger[destIndex];
	}
	return result;
}

//---------------------------------------------------------------------------------------------------------------------
// Reset all the pieces on map
//---------------------------------------------------------------------------------------------------------------------
void GameState::Restart(SDL_Renderer* pRenderer)
{
	// Delete all pieces
	for (Tile& tile : m_tiles)
		tile.RemovePiece();

	InitMap(pRenderer, true);
}

//---------------------------------------------------------------------------------------------------------------------
// Called when client initialize game board, client need the host to send all pieces info from here
// This can be only called from Server
//---------------------------------------------------------------------------------------------------------------------
AllPiecesIndex GameState::GetAllPiecesIndex()
{
	// Assert this is only called by the Dark/Host player
	assert(m_currentPlayer == CheckersColor::kDark);

	// Grab all the piece we want
	AllPiecesIndex allPiecesInfo;
	allPiecesInfo[(size_t)CheckersColor::kDark].reserve(m_myPieces.size());
	allPiecesInfo[(size_t)CheckersColor::kLight].reserve(m_otherPieces.size());

	// Get all dark pieces
	for (size_t index : m_myPieces)
		allPiecesInfo[(size_t)CheckersColor::kDark].emplace_back(index);

	// All light pieces
	for (size_t index : m_otherPieces)
		allPiecesInfo[(size_t)CheckersColor::kLight].emplace_back(index);

	return allPiecesInfo;
}

//---------------------------------------------------------------------------------------------------------------------
// Place a Piece at index for the input side. This should be only called from Client
//		-side: Indicates whose piece it is. 
//		-index: Where to place it
//		-pRenderer: I need this renderer to create the a piece
//---------------------------------------------------------------------------------------------------------------------
void GameState::PlacePiece(CheckersColor side, size_t index, SDL_Renderer* pRenderer)
{
	assert(m_currentPlayer == CheckersColor::kLight);

	// Place piece
	m_tiles[index].SetPiece(new Piece(side, pRenderer));

	// Insert index to pieces
	if (side == m_currentPlayer)
		m_myPieces.emplace(index);
	else
		m_otherPieces.emplace(index);

	if (m_myPieces.size() > 0 && m_otherPieces.size() > 0)
		m_doneInit = true;
}

//---------------------------------------------------------------------------------------------------------------------
// Set up checker game map and pieces
//		-pRenderer: Used when creating pieces
//		-isRestarting: If we are restarting by the host's command
//---------------------------------------------------------------------------------------------------------------------
void GameState::InitMap(SDL_Renderer* pRenderer, bool isRestarting)
{
	bool isDarkCell = false;
	size_t index = 0;

	// Row
	for (size_t row = 0; row < kBoardHeight; ++row)
	{
		// Col
		for (size_t col = 0; col < kBoardWidth; ++col)
		{
			// Set tile's position
			m_tiles[index].SetPosition(col, row);

			// If it's a dark cell, I might wanna place pieces on them
			if (isDarkCell)
			{
				// If it's a client, we want light pieces at bottom and dark pieces at top, and follow the host
				if (m_currentPlayer == CheckersColor::kLight && isRestarting)
				{
					// Spawn dark piece
					if (row < kSpawnTopPieceRow)
					{
						m_tiles[index].SetPiece(new Piece(CheckersColor::kDark, pRenderer));
						m_otherPieces.emplace(index);
					}
					// Light
					else if (row > kSpawnBottomPieceRow)
					{
						m_tiles[index].SetPiece(new Piece(CheckersColor::kLight, pRenderer));
						m_myPieces.emplace(index);
					}
				}
				// If it's a host, we want dark pieces at bottom and light pieces at top when Init
				else if (m_currentPlayer == CheckersColor::kDark)
				{
#if TESTING

#else
					// Spawn light piece
					if (row < kSpawnTopPieceRow)
					{
						m_tiles[index].SetPiece(new Piece(CheckersColor::kLight, pRenderer));
						m_otherPieces.emplace(index);
					}
					else if (row > kSpawnBottomPieceRow)
					{
						m_tiles[index].SetPiece(new Piece(CheckersColor::kDark, pRenderer));
						m_myPieces.emplace(index);
					}
#endif
				}
			}
			else
				m_tiles[index].SetSide(CheckersColor::kLight);

			// Update bool flags
			isDarkCell = !isDarkCell;
			++index;
		}
		isDarkCell = !isDarkCell;
	}
#if TESTING
	size_t lightPiece = 53;
	size_t lightPiece2 = 35;
	size_t lightPiece3 = 19;
	size_t darkPiece = 62;

	if (m_currentPlayer == CheckersColor::kDark)
	{
		m_tiles[lightPiece].SetPiece(new Piece(CheckersColor::kLight, pRenderer));
		m_otherPieces.emplace(lightPiece);

		m_tiles[lightPiece2].SetPiece(new Piece(CheckersColor::kLight, pRenderer));
		m_otherPieces.emplace(lightPiece2);

		m_tiles[lightPiece3].SetPiece(new Piece(CheckersColor::kLight, pRenderer));
		m_otherPieces.emplace(lightPiece3);

		m_tiles[darkPiece].SetPiece(new Piece(CheckersColor::kDark, pRenderer));
		m_myPieces.emplace(darkPiece);
	}
#endif
}

void GameState::HighLightAllPossibleTiles(size_t beginIndex)
{
	std::unordered_map<Pos, size_t> moveMap;
	moveMap.insert({Pos::kLeftFwd, kInvalidIndex});
	moveMap.insert({Pos::kRightFwd, kInvalidIndex});
	moveMap.insert({Pos::kLeftBack, kInvalidIndex});
	moveMap.insert({Pos::kRightBack, kInvalidIndex});

	// High-light all possible, movable tiles for the selected piece
	std::queue<size_t> startPoints;
	startPoints.emplace(beginIndex);

	while (!startPoints.empty())
	{
		size_t currentIndex = startPoints.front();
		startPoints.pop();
		Vector2 currPos = GetVec2FromIndex(currentIndex);

		// Left Forward
		ProcessHighLightTile((currPos.x - 1 >= 0 && currPos.y - 1 >= 0),	// Move condition
			GetIndexFromPos((size_t)currPos.x - 1, (size_t)currPos.y - 1),	// Target index
			(currPos.x - 2 >= 0 && currPos.y - 2 >= 0),						// Jump condition
			GetIndexFromPos((size_t)currPos.x - 2, (size_t)currPos.y - 2),	// Jump index
			moveMap,
			currentIndex,
			startPoints,
			Pos::kLeftFwd);

		// left backwards
		ProcessHighLightTile((currPos.x - 1 >= 0 && currPos.y + 1 < kBoardHeight && m_tiles[beginIndex].GetPiece()->IsKing()),		// Move condition
			GetIndexFromPos((size_t)currPos.x - 1, (size_t)currPos.y + 1),								// Target index
			(currPos.x - 2 >= 0 && currPos.y + 2 < kBoardHeight && m_tiles[beginIndex].GetPiece()->IsKing()),		// Jump condition
			GetIndexFromPos((size_t)currPos.x - 2, (size_t)currPos.y + 2),								// Jump index
			moveMap,
			currentIndex,
			startPoints,
			Pos::kLeftBack);

		// Right Forward
		ProcessHighLightTile((currPos.x + 1 < kBoardWidth && currPos.y - 1 >= 0),						// Move condition
			GetIndexFromPos((size_t)currPos.x + 1, (size_t)currPos.y - 1),	// Target index
			(currPos.x + 2 < kBoardWidth && currPos.y - 2 >= 0),						// Jump condition
			GetIndexFromPos((size_t)currPos.x + 2, (size_t)currPos.y - 2),	// Jump index
			moveMap,
			currentIndex,
			startPoints,
			Pos::kRightFwd);

		// Right backwards
		ProcessHighLightTile((currPos.x + 1 < kBoardWidth && currPos.y + 1 < kBoardHeight && m_tiles[beginIndex].GetPiece()->IsKing()),						// Move condition
			GetIndexFromPos((size_t)currPos.x + 1, (size_t)currPos.y + 1),	// Target index
			(currPos.x + 2 < kBoardWidth && currPos.y + 2 < kBoardHeight && m_tiles[beginIndex].GetPiece()->IsKing()),						// Jump condition
			GetIndexFromPos((size_t)currPos.x + 2, (size_t)currPos.y + 2),	// Jump index
			moveMap,
			currentIndex,
			startPoints,
			Pos::kRightBack);
	}

	// If there is no hop, set movable tiles highlighted
	if (m_pieceInDanger.empty())
	{
		for (auto& [pos, index] : moveMap)
		{
			if (index != kInvalidIndex)
				m_tiles[index].SetHighLighted();
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------
// Return if the index is in m_pieceInDanger's value
//---------------------------------------------------------------------------------------------------------------------
bool GameState::IsInDanger(size_t index) const
{
	for (const auto& [key, inDangerVec] : m_pieceInDanger)
	{
		for (size_t inDangerIndex : inDangerVec)
		{
			if (inDangerIndex == index)
				return true;
		}
	}
	return false;
}

void GameState::ProcessHighLightTile(bool moveCondition, size_t targetTileIndex, bool jumpCondition, size_t jumpIndex, std::unordered_map<Pos, size_t>& moveMap, size_t currentIndex, std::queue<size_t>& startPoints, Pos dir)
{
	// Move
	if (moveCondition && !m_tiles[targetTileIndex].GetPiece())
	{
		moveMap[dir] = targetTileIndex;
	}
	// Jump
	if (jumpCondition && !m_tiles[jumpIndex].GetPiece() && 	// There is a spot for me to jump
		m_tiles[targetTileIndex].GetPiece() && m_tiles[targetTileIndex].GetPiece()->GetCheckerColor() != m_currentPlayer &&
		!IsInDanger(targetTileIndex))	// There is a piece on the dest tile that is not mine
	{
		// High-light jump spot 
		m_tiles[jumpIndex].SetHighLighted();

		// Mark that jumpped over piece in danger
		m_pieceInDanger[jumpIndex].emplace_back(targetTileIndex);

		// Multiple hop
		if (m_pieceInDanger.find(currentIndex) != m_pieceInDanger.end())
		{
			m_tiles[currentIndex].Reset();
			for (size_t index : m_pieceInDanger[currentIndex])
				m_pieceInDanger[jumpIndex].emplace_back(index);
			m_pieceInDanger.erase(currentIndex);
		}

		startPoints.emplace(jumpIndex);
	}
}

//---------------------------------------------------------------------------------------------------------------------
// Reset high-lighted tiles
//---------------------------------------------------------------------------------------------------------------------
void GameState::ResetHighlightedTiles()
{
	for (Tile& tile : m_tiles)
		tile.Reset();
	m_pieceInDanger.clear();
}

//---------------------------------------------------------------------------------------------------------------------
// Remove a piece at index, update pieces recording sets
//---------------------------------------------------------------------------------------------------------------------
void GameState::KillPieceAt(size_t index)
{
	m_tiles[index].RemovePiece();

	if (m_myPieces.find(index) != m_myPieces.end())
		m_myPieces.erase(index);
	else if (m_otherPieces.find(index) != m_otherPieces.end())
		m_otherPieces.erase(index);
}

//---------------------------------------------------------------------------------------------------------------------
// Move fromIndex's tile's m_pPiece destIndex's tile's m_pPiece, update m_myPieces if it's a local made movement
//---------------------------------------------------------------------------------------------------------------------
void GameState::MovePiece(size_t fromIndex, size_t destIndex)
{
	m_tiles[destIndex].SetPiece(m_tiles[fromIndex].GetPiece());
	m_tiles[fromIndex].SetPiece(nullptr);

	// If this move is made by myself, update my pieces index
	if (m_myPieces.find(fromIndex) != m_myPieces.end())
	{
		m_myPieces.erase(fromIndex);
		m_myPieces.emplace(destIndex);
	}
	// If this move is made by the opponent, update m_otherPieces
	else if(m_otherPieces.find(fromIndex) != m_otherPieces.end())
	{
		m_otherPieces.erase(fromIndex);
		m_otherPieces.emplace(destIndex);
	}

	// If dest is at the other's bottom, upgrade that piece to king
	if (destIndex / kBoardWidth == 0 && m_myPieces.find(destIndex) != m_myPieces.end())
		m_tiles[destIndex].GetPiece()->ToKing();
}
