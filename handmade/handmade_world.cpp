
#define WORLD_SAFE_MARGIN (INT32_MAX/64)
#define WORLD_UNINITIALIZED INT32_MAX
#define TILES_PER_CHUNK 16


inline bool32 IsCanonical(world* World, real32 TileRel)
{
    bool32 Result = ((TileRel >= -0.5f*World->ChunkSideInMeters) &&
        (TileRel <= 0.5f*World->ChunkSideInMeters));

    return Result;
}

inline bool32 IsCanonical(world* World, v2 Offset)
{
    bool32 Result = (IsCanonical(World, Offset.X) && IsCanonical(World, Offset.Y));

    return Result;
}



inline bool32 AreInSameChunk(world* World, world_position* A, world_position* B)
{
    ASSERT(IsCanonical(World, A->Offset_));
    ASSERT(IsCanonical(World, B->Offset_));

    bool Result = ((A->ChunkX == B->ChunkX) &&
                   (A->ChunkY == B->ChunkY) &&
                   (A->ChunkZ == B->ChunkZ));
    return Result;
}

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
    uint32 HashSlot = HashValue & (ARRAY_COUNT(World->ChunkHash) - 1);
    ASSERT(HashSlot < ARRAY_COUNT(World->ChunkHash));

    Chunk = World->ChunkHash + HashSlot;
    do
    {
        if ((ChunkX == Chunk->ChunkX) && 
            (ChunkY == Chunk->ChunkY) && 
            (ChunkZ == Chunk->ChunkZ))
        {
            break;
        }

        if (Arena && (Chunk->ChunkX != WORLD_UNINITIALIZED) && (!Chunk->NextInHash))
        {
            Chunk->NextInHash = PushStruct(Arena, world_chunk);
            Chunk = Chunk->NextInHash;
            Chunk->ChunkX = WORLD_UNINITIALIZED;
        }

        if (Arena && ((Chunk->ChunkX == WORLD_UNINITIALIZED)))
        {
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

internal void InitializeWorld(world* World, real32 TileSideInMeters)
{
    World->TileSideInMeters = TileSideInMeters;
    World->ChunkSideInMeters = (real32)TILES_PER_CHUNK*TileSideInMeters;
    World->FirstFree = 0;

    for (uint32 ChunkIndex = 0;
        ChunkIndex < ARRAY_COUNT(World->ChunkHash);
        ++ChunkIndex)
    {
        World->ChunkHash[ChunkIndex].ChunkX = WORLD_UNINITIALIZED;
        World->ChunkHash[ChunkIndex].FirstBlock.EntityCount = 0;
    }

}

//
/////
//

inline void RecanonicalizeCoord(world* World, int32* Tile, real32* TileRel)
{
    //NOTE: World is assumed to be toroidal topology , if you step off one
    // and you come back in the other
    int32 Offset = RoundReal32ToInt32(*TileRel / World->ChunkSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*World->ChunkSideInMeters;

    ASSERT(IsCanonical(World, *TileRel));
}

internal world_position MapIntoChunkSpace(world* World, world_position BasePos, v2 Offset)
{
    world_position Result = BasePos;

    Result.Offset_ += Offset;
    RecanonicalizeCoord(World, &Result.ChunkX, &Result.Offset_.X);
    RecanonicalizeCoord(World, &Result.ChunkY, &Result.Offset_.Y);

    return Result;
}

inline world_position ChunkPositionFromTilePosition(world* World,
    int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ)
{
    world_position Result = {};
    Result.ChunkX = AbsTileX / TILES_PER_CHUNK;
    Result.ChunkY = AbsTileY / TILES_PER_CHUNK;
    Result.ChunkZ = AbsTileZ / TILES_PER_CHUNK;

    //
    Result.Offset_.X = (real32)(AbsTileX - (Result.ChunkX*TILES_PER_CHUNK)) * World->TileSideInMeters;
    Result.Offset_.Y = (real32)(AbsTileY - (Result.ChunkY*TILES_PER_CHUNK)) * World->TileSideInMeters;

    return Result;
}


world_difference Subtract(world* World, world_position* A, world_position* B)
{
    world_difference Result;

    v2 dTileXY = { (real32)A->ChunkX - (real32)B->ChunkX,
        (real32)A->ChunkY - (real32)B->ChunkY };
    real32 dTileZ = (real32)A->ChunkZ - (real32)B->ChunkZ;

    Result.dXY = World->ChunkSideInMeters*dTileXY + (A->Offset_ - B->Offset_);

    Result.dZ = World->ChunkSideInMeters*dTileZ;

    return  Result;
}

inline world_position CenteredChunkPoint(int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ)
{
    world_position Result = {};
    Result.ChunkX = AbsTileX;
    Result.ChunkY = AbsTileY;
    Result.ChunkZ = AbsTileZ;

    return Result;
}

inline void ChangeEntityLocation(memory_arena* Arena, world* World, uint32 LowEntityIndex, 
    world_position *OldP, world_position *NewP)
{
    if (OldP && AreInSameChunk(World, OldP, NewP))
    {
        //NOTE: Leave entity where it is
    }
    else
    {
        if (OldP)
        {
            world_chunk *Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ);
            ASSERT(Chunk);
            if (Chunk)
            {
                bool32 NotFound = true;
                world_entity_block *FirstBlock = &Chunk->FirstBlock;
                for (world_entity_block *Block = FirstBlock;
                    Block && NotFound;
                    Block = Block->Next)
                {
                    for (uint32 Index = 0; (Index < Block->EntityCount) && NotFound; ++Index)
                    {
                        if (Block->LowEntityIndex[Index] == LowEntityIndex)
                        {
                            ASSERT(FirstBlock->EntityCount > 0);
                            Block->LowEntityIndex[Index] =
                                FirstBlock->LowEntityIndex[--FirstBlock->EntityCount];

                            if (FirstBlock->EntityCount == 1)
                            {
                                if (FirstBlock->Next)
                                {
                                    world_entity_block *NextBlock = FirstBlock->Next;
                                    *FirstBlock = *NextBlock;

                                    NextBlock->Next = World->FirstFree;
                                    World->FirstFree = NextBlock;
                                }
                            }

                            NotFound = false;;
                        }
                    }
                }
            }
        }

        world_chunk *Chunk = GetWorldChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, Arena);
        ASSERT(Chunk);

        world_entity_block* Block = &Chunk->FirstBlock;
        if (Block->EntityCount == ARRAY_COUNT(Block->LowEntityIndex))
        {
            world_entity_block *OldBlock = World->FirstFree;
            if (OldBlock)
            {
                World->FirstFree = OldBlock->Next;
            }
            else
            {
                OldBlock = PushStruct(Arena, world_entity_block);
            }

            *OldBlock = *Block;
            Block->Next = OldBlock;
            Block->EntityCount = 0;
        }

        ASSERT(Block->EntityCount < ARRAY_COUNT(Block->LowEntityIndex));
        Block->LowEntityIndex[Block->EntityCount++] = LowEntityIndex;
    }
}

