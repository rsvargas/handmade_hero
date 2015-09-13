
#define WORLD_SAFE_MARGIN (INT32_MAX/64)
#define WORLD_UNINITIALIZED INT32_MAX

inline world_chunk* GetWorldChunk(world * World, int32 ChunkX, int32 ChunkY, 
    int32 ChunkZ, memory_arena* Arena = 0)
{
    world_chunk *Chunk = 0;

    ASSERT(ChunkX > -WORLD_SAFE_MARGIN);
    ASSERT(ChunkY > -WORLD_SAFE_MARGIN);
    ASSERT(ChunkZ > -WORLD_SAFE_MARGIN);
    ASSERT(ChunkX < WORLD_SAFE_MARGIN);
    ASSERT(ChunkZ < WORLD_SAFE_MARGIN);
    ASSERT(ChunkZ < WORLD_SAFE_MARGIN);

    uint32 HashValue = 19*ChunkX + 7*ChunkY + 3*ChunkZ;
    uint32 HashSlot = HashValue & (ARRAY_COUNT(World->WorldChunkHash) - 1);
    ASSERT(HashSlot < ARRAY_COUNT(World->WorldChunkHash));

    Chunk = World->WorldChunkHash + HashSlot;
    do
    {
        if ((ChunkX == Chunk->ChunkX) && 
            (ChunkY == Chunk->ChunkY) && 
            (ChunkZ == Chunk->ChunkZ))
        {
            break;
        }

        if (Arena && (Chunk->ChunkX != 0) && (!Chunk->NextInHash))
        {
            Chunk->NextInHash = PushStruct(Arena, world_chunk);
            Chunk = Chunk->NextInHash;
            Chunk->ChunkX = WORLD_UNINITIALIZED;
        }

        if (Arena && ((Chunk->ChunkX == WORLD_UNINITIALIZED)))
        {
            uint32 TileCount = World->ChunkDim * World->ChunkDim;
            Chunk->ChunkX = ChunkX;
            Chunk->ChunkY = ChunkY;
            Chunk->ChunkZ = ChunkZ;

            Chunk->NextInHash = 0;
            break;
        }
        Chunk = Chunk->NextInHash;
    }
    while (Chunk);

    return Chunk;
}

#if 0
inline tile_chunk_position GetChunkPositionFor(world * World, int32 AbsTileX, int32 AbsTileY,
    int32 AbsTileZ)
{
    tile_chunk_position Result;
    Result.WorldChunkX = AbsTileX >> World->ChunkShift;
    Result.WorldChunkY = AbsTileY >> World->ChunkShift;
    Result.WorldChunkZ = AbsTileZ;
    Result.RelTileX = AbsTileX & World->ChunkMask;
    Result.RelTileY =  AbsTileY & World->ChunkMask;

    return Result;
}
#endif



//
/////
//

inline void RecanonicalizeCoord(world* World, int32* Tile, real32* TileRel)
{
    //NOTE: World is assumed to be toroidal topology , if you step off one
    // and you come back in the other
    int32 Offset = RoundReal32ToInt32(*TileRel / World->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*World->TileSideInMeters;

    ASSERT(*TileRel > -0.5f*World->TileSideInMeters);
    ASSERT(*TileRel < 0.5f*World->TileSideInMeters);
}

internal world_position MapIntoTileSpace(world* World, world_position BasePos, v2 Offset)
{
    world_position Result = BasePos;

    Result.Offset_ += Offset;
    RecanonicalizeCoord(World, &Result.AbsTileX, &Result.Offset_.X);
    RecanonicalizeCoord(World, &Result.AbsTileY, &Result.Offset_.Y);

    return Result;
}


inline bool32 AreOnSameTile(world_position* A, world_position* B)
{
    bool Result = ((A->AbsTileX == B->AbsTileX) &&
        (A->AbsTileY == B->AbsTileY) &&
        (A->AbsTileZ == B->AbsTileZ));
    return Result;
}

world_difference Subtract(world* World, world_position* A, world_position* B)
{
    world_difference Result;

    v2 dTileXY = { (real32)A->AbsTileX - (real32)B->AbsTileX,
        (real32)A->AbsTileY - (real32)B->AbsTileY };
    real32 dTileZ = (real32)A->AbsTileZ - (real32)B->AbsTileZ;

    Result.dXY = World->TileSideInMeters*dTileXY + (A->Offset_ - B->Offset_);

    Result.dZ = World->TileSideInMeters*dTileZ;

    return  Result;
}

inline world_position CenteredTilePoint(int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ)
{
    world_position Result = {};
    Result.AbsTileX = AbsTileX;
    Result.AbsTileY = AbsTileY;
    Result.AbsTileZ = AbsTileZ;

    return Result;
}

internal void InitializeWorld(world* World, real32 TileSideInMeters)
{
    World->ChunkShift = 4;
    World->ChunkMask = (1 << World->ChunkShift) - 1;
    World->ChunkDim = (1 << World->ChunkShift);
    World->TileSideInMeters = TileSideInMeters;

    for (uint32 WorldChunkIndex = 0;
        WorldChunkIndex < ARRAY_COUNT(World->WorldChunkHash);
        ++WorldChunkIndex)
    {
        World->WorldChunkHash[WorldChunkIndex].ChunkX = WORLD_UNINITIALIZED;

    }

}

