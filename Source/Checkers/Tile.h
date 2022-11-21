#pragma once

#include "CheckersConstants.h"

#include <SDL.h>

class Piece; 

//--------------------------------------------------------------------------------------------------------------
// Represents a tile in the map
//--------------------------------------------------------------------------------------------------------------
class Tile
{
	// Constants
	static constexpr SDL_Color kDarkTileColor = { 148,118,65,1 };
	static constexpr SDL_Color kLightTileColor = { 206,174,0,1 };
	static constexpr SDL_Color kHighLightTileColor = { 15,217,55,1 };

	// SDL
	SDL_Rect m_tileRect;

	// Game
	Piece* m_pPiece;		// The piece on this tile currently
	CheckersColor m_side;
	bool m_highLighted;		// Whether this tile is available for a piece to move to

public:
	Tile();
	~Tile();

	void Render(SDL_Renderer* pRenderer) const;

	void RemovePiece();
	void SetHighLighted(){ m_highLighted = true; }
	void Reset() { m_highLighted = false; }
	void SetPiece(Piece* pPiece);
	void SetPosition(size_t x, size_t y) { m_tileRect.x = (int)x * kTileWidth; m_tileRect.y = (int)y * kTileHeight; }
	void SetSide(CheckersColor color) { m_side = color; }
	bool HighLighted() const { return m_highLighted; }
	Piece* GetPiece() const { return m_pPiece; }
};

