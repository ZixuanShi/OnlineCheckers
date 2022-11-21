#pragma once

#include "CheckersConstants.h"

#include <SDL.h>

//---------------------------------------------------------------------------------------------------------------------
// Represents a piece/token/chess depending how you call them in game
//---------------------------------------------------------------------------------------------------------------------
class Piece
{
private:
	static constexpr SDL_Color kDarkTileColor = { 55,55,55,1 };
	static constexpr SDL_Color kLightTileColor = { 225,225,225,1 };
	static constexpr int kOnSelectedYOffset = kTileHeight / 3;

	// Gameplay
	CheckersColor m_color;
	bool m_isKing;

	// Drawing
	SDL_Rect m_pieceRect;
	SDL_Texture* m_pTexture;
	static constexpr const char* kLightPieceSpritePath = "Assets/Sprites/Pikachu.png";
	static constexpr const char* kDarkPieceSpritePath = "Assets/Sprites/Charmander.png";

public:
	Piece(CheckersColor side, SDL_Renderer* pRenderer);
	~Piece();

	void Render(SDL_Renderer* pRenderer) const;

	void OnSelected();
	void UnSelect();
	void SetPosition(SDL_Rect transform) { m_pieceRect = transform; }
	void ToKing() { m_isKing = true; }
	bool IsKing() const { return m_isKing; }
	CheckersColor GetCheckerColor() const { return m_color; }
};

