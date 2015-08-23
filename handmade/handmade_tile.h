#pragma once

struct tile_map_position
{
    uint32 AbsTileX;
    uint32 AbsTileY;

    //NOTE: X and Y relative to the tile we are in in this tilemap
    real32 TileRelX;
    real32 TileRelY;
};

struct tile_chunk_position
{
    uint32 TileChunkX;
    uint32 TileChunkY;

    uint32 RelTileX;
    uint32 RelTileY;
};

struct tile_chunk
{
    uint32* Tiles;
};

struct tile_map
{
    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;

    real32 TileSideInMeters;
    int32 TileSideInPixels;
    real32 MetersToPixels;

    uint32 TileChunkCountX;
    uint32 TileChunkCountY;
    tile_chunk *TileChunks;
};
