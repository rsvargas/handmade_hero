

inline tile_chunk* GetTileChunk(tile_map * TileMap, uint32 TileChunkX, uint32 TileChunkY, 
    uint32 TileChunkZ)
{
    tile_chunk * TileChunk = 0;

    if ((TileChunkX >= 0) && (TileChunkX < TileMap->TileChunkCountX) &&
        (TileChunkY >= 0) && (TileChunkY < TileMap->TileChunkCountY) &&
        (TileChunkZ >= 0) && (TileChunkZ < TileMap->TileChunkCountZ))
    {
        TileChunk = &TileMap->TileChunks[
            TileChunkZ*TileMap->TileChunkCountY * TileMap->TileChunkCountX +
            TileChunkY*TileMap->TileChunkCountX + 
            TileChunkX];

    }
    return TileChunk;
}

inline uint32 GetTileValueUnchecked(tile_map* TileMap, tile_chunk * TileChunk, uint32 TileX, uint32 TileY)
{
    ASSERT(TileChunk);
    ASSERT(TileX < TileMap->ChunkDim);
    ASSERT(TileY < TileMap->ChunkDim);

    uint32 TileChunkValue = TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX];
    return TileChunkValue;
}

inline void SetTileValueUnchecked(tile_map* TileMap, tile_chunk * TileChunk, uint32 TileX, uint32 TileY,
    uint32 TileValue)
{
    ASSERT(TileChunk);
    ASSERT(TileX < TileMap->ChunkDim);
    ASSERT(TileY < TileMap->ChunkDim);

    TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX] = TileValue;
}

internal uint32 GetTileValue(tile_map* TileMap, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY)
{
    uint32 TileChunkValue = 0;
    if (TileChunk && TileChunk->Tiles)
    {
        TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
    }
    return TileChunkValue;
}

internal void SetTileValue(tile_map* TileMap, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY,
    uint32 TileValue)
{
    if (TileChunk && TileChunk->Tiles)
    {
        SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
    }
}

inline tile_chunk_position GetChunkPositionFor(tile_map * TileMap, uint32 AbsTileX, uint32 AbsTileY,
    uint32 AbsTileZ)
{
    tile_chunk_position Result;
    Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
    Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
    Result.TileChunkZ = AbsTileZ;
    Result.RelTileX = AbsTileX & TileMap->ChunkMask;
    Result.RelTileY =  AbsTileY & TileMap->ChunkMask;

    return Result;
}

internal uint32 GetTileValue(tile_map * TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
    uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX,  ChunkPos.RelTileY);

    return TileChunkValue;
}

internal uint32 GetTileValue(tile_map * TileMap, tile_map_position Pos)
{
    uint32 TileChunkValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);
    return TileChunkValue;
}

internal bool32 IsTileMapPointEmpty(tile_map * TileMap, tile_map_position CanPos)
{
    uint32 TilechunkEmpty = GetTileValue(TileMap, CanPos.AbsTileX, CanPos.AbsTileY, CanPos.AbsTileZ);
    bool Empty = ((TilechunkEmpty == 1) ||
        (TilechunkEmpty == 3) ||
        (TilechunkEmpty == 4 ));

    return Empty;
}
internal void SetTileValue(memory_arena* Arena, tile_map* TileMap, uint32 AbsTileX,
    uint32 AbsTileY, uint32 AbsTileZ, uint32 TileValue)
{
    tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, 
        ChunkPos.TileChunkZ);
    ASSERT(TileChunk);

    if (!TileChunk->Tiles)
    {
        uint32 TileCount = TileMap->ChunkDim*TileMap->ChunkDim;
        TileChunk->Tiles = PushArray(Arena, TileCount, uint32);
        for (uint32 TileIndex = 0; TileIndex < TileCount; ++TileIndex)
        {
            TileChunk->Tiles[TileIndex] = 1;
        }

    }
    SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX,  ChunkPos.RelTileY, TileValue);
}

//
/////
//

inline void RecanonicalizeCoord(tile_map* TileMap, uint32* Tile, real32* TileRel)
{
    //NOTE: TileMap is assumed to be toroidal topology , if you step off one
    // and you come back in the other
    int32 Offset = RoundReal32ToInt32(*TileRel / TileMap->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*TileMap->TileSideInMeters;

    ASSERT(*TileRel >= -0.5f*TileMap->TileSideInMeters);
    ASSERT(*TileRel <= 0.5f*TileMap->TileSideInMeters);
}

internal tile_map_position RecanonicalizePosition(tile_map * TileMap, tile_map_position Pos)
{
    tile_map_position Result = Pos;

    RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.OffsetX);
    RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.OffsetY);

    return Result;
}


inline bool32 AreOnSameTile(tile_map_position* A, tile_map_position* B)
{
    bool Result = ((A->AbsTileX == B->AbsTileX) &&
        (A->AbsTileY == B->AbsTileY) &&
        (A->AbsTileZ == B->AbsTileZ));
    return Result;
}

tile_map_diference Subtract(tile_map* TileMap, tile_map_position* A, tile_map_position* B)
{
    tile_map_diference Result;

    real32 dTileX = (real32)A->AbsTileX - (real32)B->AbsTileX;
    real32 dTileY = (real32)A->AbsTileY - (real32)B->AbsTileY;
    real32 dTileZ = (real32)A->AbsTileZ - (real32)B->AbsTileZ;

    Result.dX = TileMap->TileSideInMeters*dTileX + (A->OffsetX - B->OffsetX);
    Result.dY = TileMap->TileSideInMeters*dTileY + (A->OffsetY - B->OffsetY);
    Result.dZ = TileMap->TileSideInMeters*dTileZ;

    return  Result;
}
