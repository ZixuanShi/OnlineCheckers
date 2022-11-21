#include "Tile.h"

#include "Piece.h"

//---------------------------------------------------------------------------------------------------------------------
// Default ctor
//---------------------------------------------------------------------------------------------------------------------
Tile::Tile()
    : m_pPiece{ nullptr }
    , m_side{ CheckersColor::kDark }
    , m_highLighted{ false }
{
    // We will set tile rect's x and y later in GameState Init()
    m_tileRect.x = 0; 
    m_tileRect.y = 0; 
    m_tileRect.w = kTileWidth - 1;
    m_tileRect.h = kTileHeight - 1;
}

//---------------------------------------------------------------------------------------------------------------------
// Dtor, delete piece if there is one
//---------------------------------------------------------------------------------------------------------------------
Tile::~Tile()
{
    RemovePiece();
}

//---------------------------------------------------------------------------------------------------------------------
// Renders a single tile by this tile's color, Render the piece on this tile if there is one
//     -pRenderer: The SDL renderer to use.
//---------------------------------------------------------------------------------------------------------------------
void Tile::Render(SDL_Renderer* pRenderer) const
{
    // if highlighted
    if (m_highLighted)
        SDL_SetRenderDrawColor(pRenderer, kHighLightTileColor.r, kHighLightTileColor.g, kHighLightTileColor.b, kHighLightTileColor.a);

    // Render tile according to it's color
    else if (m_side == CheckersColor::kDark)
        SDL_SetRenderDrawColor(pRenderer, kDarkTileColor.r, kDarkTileColor.g, kDarkTileColor.b, kDarkTileColor.a);
   
    else if (m_side == CheckersColor::kLight)
        SDL_SetRenderDrawColor(pRenderer, kLightTileColor.r, kLightTileColor.g, kLightTileColor.b, kLightTileColor.a);

    SDL_RenderFillRect(pRenderer, &m_tileRect);

    // Render piece if any
    if (m_pPiece)
        m_pPiece->Render(pRenderer);
}

//---------------------------------------------------------------------------------------------------------------------
// Remove the piece on this tile if there is any
//---------------------------------------------------------------------------------------------------------------------
void Tile::RemovePiece()
{
    if (m_pPiece)
    {
        delete m_pPiece;
        m_pPiece = nullptr;
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Set the piece pointer on this tile, if it's not a nullptr, adjust it's position to match this tile's rect
//---------------------------------------------------------------------------------------------------------------------
void Tile::SetPiece(Piece* pPiece)
{
    m_pPiece = pPiece;
    if (m_pPiece)
        pPiece->SetPosition(m_tileRect);
}
