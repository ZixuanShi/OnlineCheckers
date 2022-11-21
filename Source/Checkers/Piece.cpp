#include "Piece.h"

#include "Utils/Log/Log.h"

#include <SDL_image.h>

//---------------------------------------------------------------------------------------------------------------------
// Ctor
//  - Checkers side
//  - Set SDL rect position
//  - SDL image texture
//---------------------------------------------------------------------------------------------------------------------
Piece::Piece(CheckersColor side, SDL_Renderer* pRenderer)
	: m_color{ side }
    , m_pTexture{ nullptr }
    , m_isKing{ false }
{
    // SDL rect
    m_pieceRect.w = kTileWidth - 1;
    m_pieceRect.h = kTileHeight - 1;

    // SDL surface
    SDL_Surface* pSurface = IMG_Load(side == CheckersColor::kDark ? kDarkPieceSpritePath: kLightPieceSpritePath);
    if (!pSurface)
    {
        LOG("Error", "Unable to load %s", side == CheckersColor::kDark ? kDarkPieceSpritePath : kLightPieceSpritePath);
        return;
    }

    // SDL texture
    m_pTexture = SDL_CreateTextureFromSurface(pRenderer, pSurface);
    if (!m_pTexture)
    {
        LOG("Error", "Unable to create texture.");
        return;
    }

    // Free surface.
    SDL_FreeSurface(pSurface);
}

//---------------------------------------------------------------------------------------------------------------------
// Dtor
//  - Destroy SDL Texture
//---------------------------------------------------------------------------------------------------------------------
Piece::~Piece()
{
    SDL_DestroyTexture(m_pTexture);
}

//---------------------------------------------------------------------------------------------------------------------
// Renders a circle according to this piece's color and position
//      -pRenderer: The SDL renderer to use.
//	    -x: x position
//		-y: y position
//---------------------------------------------------------------------------------------------------------------------
void Piece::Render(SDL_Renderer* pRenderer) const
{
    SDL_RenderCopy(pRenderer, m_pTexture, nullptr, &m_pieceRect);
}

//---------------------------------------------------------------------------------------------------------------------
// Decrease SDL_rect's y position to make it looks like "Selected"
//---------------------------------------------------------------------------------------------------------------------
void Piece::OnSelected()
{
    m_pieceRect.y -= kOnSelectedYOffset;
}

//---------------------------------------------------------------------------------------------------------------------
// Reset y pos
//---------------------------------------------------------------------------------------------------------------------
void Piece::UnSelect()
{
    m_pieceRect.y += kOnSelectedYOffset;
}
