#pragma once

struct tile_map_diference
{
    v2 dXY;
    real32 dZ;
};

struct tile_map_position
{
    uint32 AbsTileX;
    uint32 AbsTileY;
    uint32 AbsTileZ;

    //NOTE: Offset from the tile center
    v2 Offset;
};

struct tile_chunk_position
{
    uint32 TileChunkX;
    uint32 TileChunkY;
    uint32 TileChunkZ;

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

    uint32 TileChunkCountX;
    uint32 TileChunkCountY;
    uint32 TileChunkCountZ;
    tile_chunk *TileChunks;
};
