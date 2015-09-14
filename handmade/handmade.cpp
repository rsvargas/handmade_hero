#include "handmade.h"

#include "handmade_world.cpp"
#include "handmade_random.h"

internal void GameOutputSound(game_state* GameState, game_sound_output_buffer* SoundBuffer, int ToneHz)
{

    int16 ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16* SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
#if 0
        real32 SineValue = sinf(GameState.tSine);
        int16 SampleValue = (int16)(SineValue*ToneVolume);
#else
        int16 SampleValue = 0;
#endif

        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

#if 0
        GameState.tSine += 2.0f * Pi32 * (1.0f / (real32)WavePeriod);
        if (GameState.tSine >(2.0f * Pi32))
        {
            GameState.tSine -= (2.0f * Pi32);
        }
#endif
    }
}

internal void DrawRectangle(game_offscreen_buffer *Buffer,
    v2 vMin, v2 vMax, real32 R, real32 G, real32 B)
{
    int32 MinX = RoundReal32ToInt32(vMin.X);
    int32 MinY = RoundReal32ToInt32(vMin.Y);
    int32 MaxX = RoundReal32ToInt32(vMax.X);
    int32 MaxY = RoundReal32ToInt32(vMax.Y);

    if (MinX < 0)
    {
        MinX = 0;
    }

    if (MinY < 0)
    {
        MinY = 0;
    }

    if (MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if (MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint32 Color = ((RoundReal32ToUInt32(R*255.0f) << 16) |
        (RoundReal32ToUInt32(G*255.0f) << 8) |
        (RoundReal32ToUInt32(B*255.0f) << 0));

    uint8* Row = (((uint8*)Buffer->Memory) +
        MinX * Buffer->BytesPerPixel +
        MinY * Buffer->Pitch);

    for (int Y = MinY; Y < MaxY; ++Y)
    {
        uint32 *Pixel = (uint32*)Row;
        for (int X = MinX; X < MaxX; ++X)
        {
            *Pixel++ = Color;
        }
        Row += Buffer->Pitch;
    }

}

internal void DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap* Bitmap, 
    real32 RealX, real32 RealY, int32 AlignX = 0, int32 AlignY = 0,
    real32 CAlpha = 1.0f)
{
    RealX -= (real32)AlignX;
    RealY -= (real32)AlignY;

    int32 MinX = RoundReal32ToInt32(RealX);
    int32 MinY = RoundReal32ToInt32(RealY);
    int32 MaxX = MinX + Bitmap->Width;
    int32 MaxY = MinY + Bitmap->Height;

    int32 SourceOffsetX = 0;
    if (MinX < 0)
    {
        SourceOffsetX = -MinX;
        MinX = 0;
    }

    int32 SourceOffsetY = 0;
    if (MinY < 0)
    {
        SourceOffsetY = -MinY;
        MinY = 0;
    }

    if (MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }

    if (MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    uint32* SourceRow = Bitmap->Pixels + (Bitmap->Width*(Bitmap->Height - 1));
    SourceRow += -Bitmap->Width *SourceOffsetY + SourceOffsetX;
    uint8* DestRow = (((uint8*)Buffer->Memory) +
        MinX * Buffer->BytesPerPixel +
        MinY * Buffer->Pitch);
    for (int Y = MinY; Y < MaxY; ++Y)
    {
        uint32 *Dest = (uint32*)DestRow;
        uint32 *Source = SourceRow;
        for (int X = MinX; X < MaxX; ++X)
        {
            real32 A = (real32)((*Source >> 24) & 0xFF) / 255.0f;
            A *= CAlpha;
            real32 SR = (real32)((*Source >> 16) & 0xFF);
            real32 SG = (real32)((*Source >> 8) & 0xFF);
            real32 SB = (real32)((*Source >> 0) & 0xFF);

            real32 DR = (real32)((*Dest >> 16) & 0xFF);
            real32 DG = (real32)((*Dest >> 8) & 0xFF);
            real32 DB = (real32)((*Dest >> 0) & 0xFF);

            real32 R = (1.0f - A)*DR + A*SR;
            real32 G = (1.0f - A)*DG + A*SG;
            real32 B = (1.0f - A)*DB + A*SB;

            *Dest = (((uint32)(R + 0.5f) << 16) |
                     ((uint32)(G + 0.5f) << 8) |
                     ((uint32)(B + 0.5f) << 0));

            ++Dest;
            ++Source;
        }
        DestRow += Buffer->Pitch;
        SourceRow -= Bitmap->Width;
    }
}



#pragma pack(push,1)
struct bitmap_header
{
    uint16 FileType;
    uint32 FileSize;
    uint16 Reserved1;
    uint16 Reserved2;
    uint32 BitmapOffset;
    uint32 Size;
    int32 Width;
    int32 Height;
    uint16 Planes;
    uint16 BitsPerPixel;
    uint32 Compression;
    uint32 SizeOfBitmap;
    int32 HorzResolution;
    int32 VertResolution;
    uint32 ColorsUsed;
    uint32 ColorsImportant;

    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;
};
#pragma pack(pop)

internal loaded_bitmap DEBUGLoadBMP(thread_context* Thread, debug_platform_read_entire_file *ReadEntireFile,
    char* FileName)
{
    loaded_bitmap Result = {};

    debug_read_file_result ReadResult= ReadEntireFile(Thread, FileName);
    if (ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header*)ReadResult.Contents;
        uint32 *Pixels = (uint32*)((uint8*)ReadResult.Contents + Header->BitmapOffset);
        Result.Pixels = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;

        ASSERT(Header->Compression == 3);

        uint32 RedMask = Header->RedMask;
        uint32 GreenMask = Header->GreenMask;
        uint32 BlueMask = Header->BlueMask;
        uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

        bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);

        ASSERT(RedScan.Found);
        ASSERT(GreenScan.Found);
        ASSERT(BlueScan.Found);
        ASSERT(AlphaScan.Found);

        int32 RedShift = 16 - (int32)RedScan.Index;
        int32 GreenShift = 8 - (int32)GreenScan.Index;
        int32 BlueShift = 0 - (int32)BlueScan.Index;
        int32 AlphaShift = 24 - (int32)AlphaScan.Index;


        uint32* SourceDest = Pixels;
        for (int32 Y = 0; Y < Header->Height; ++Y)
        {
            for (int32 X = 0; X < Header->Width; ++X)
            {
                uint32 C = *SourceDest;
                *SourceDest = (RotateLeft(C & RedMask, RedShift) |
                    RotateLeft(C & GreenMask, GreenShift) |
                    RotateLeft(C & BlueMask, BlueShift) |
                    RotateLeft(C & AlphaMask, AlphaShift));
                ++SourceDest;
            }
        }
    }

    return Result;
}

inline low_entity* GetLowEntity(game_state *GameState, uint32 Index)
{
    low_entity* Result = 0;

    if ((Index > 0) && (Index < GameState->LowEntityCount))
    {
        Result = GameState->LowEntities + Index;
    }

    return Result;
}

inline v2 GetCameraSpaceP(game_state *GameState, low_entity* EntityLow)
{
    world_difference Diff = Subtract(GameState->World, &EntityLow->P, &GameState->CameraP);
    v2 Result = Diff.dXY;

    return Result;
}

inline high_entity* MakeEntityHighFrequency(game_state *GameState, low_entity* EntityLow, 
    uint32 LowIndex, v2 CameraSpaceP)
{
    high_entity* EntityHigh = 0;

    ASSERT(EntityLow->HighEntityIndex == 0);
    if (EntityLow->HighEntityIndex == 0)
    {
        if (GameState->HighEntityCount < ARRAY_COUNT(GameState->HighEntities_))
        {
            uint32 HighIndex = GameState->HighEntityCount++;
            EntityHigh = GameState->HighEntities_ + HighIndex;

            EntityHigh->P = CameraSpaceP;
            EntityHigh->dP = V2(0, 0);
            EntityHigh->ChunkZ = EntityLow->P.ChunkZ;
            EntityHigh->FacingDirection = 0;
            EntityHigh->LowEntityIndex = LowIndex;

            EntityLow->HighEntityIndex = HighIndex;
        }
        else
        {
            INVALID_CODE_PATH;
        }
    }

    return EntityHigh;
}


inline high_entity* MakeEntityHighFrequency(game_state *GameState, uint32 LowIndex)
{
    high_entity * EntityHigh = 0;

    low_entity* EntityLow = GameState->LowEntities + LowIndex;
    if (EntityLow->HighEntityIndex)
    {
        EntityHigh = GameState->HighEntities_ + EntityLow->HighEntityIndex;
    }
    else
    {
        v2 CameraSpaceP = GetCameraSpaceP(GameState, EntityLow);
        EntityHigh = MakeEntityHighFrequency(GameState, EntityLow, LowIndex, CameraSpaceP);
    }

    return EntityHigh;
}

inline entity GetHighEntity(game_state *GameState, uint32 LowIndex)
{
    entity Result = {};
    
    if ((LowIndex > 0) && (LowIndex < GameState->LowEntityCount))
    {
        Result.LowIndex = LowIndex;
        Result.Low = GameState->LowEntities + LowIndex;
        Result.High = MakeEntityHighFrequency(GameState, LowIndex);
    }

    return Result;
}


inline void MakeEntityLowFrequency(game_state *GameState, uint32 LowIndex)
{
    low_entity* EntityLow = &GameState->LowEntities[LowIndex];
    uint32 HighIndex = EntityLow->HighEntityIndex;
    if (HighIndex)
    {
        uint32 LastHighIndex = GameState->HighEntityCount - 1;
        if (HighIndex != LastHighIndex)
        {
            high_entity *LastEntity = GameState->HighEntities_ + LastHighIndex;
            high_entity *DelEntity = GameState->HighEntities_ + HighIndex;

            *DelEntity = *LastEntity;
            GameState->LowEntities[LastEntity->LowEntityIndex].HighEntityIndex = HighIndex;
        }
        GameState->HighEntityCount--;

        EntityLow->HighEntityIndex = 0;
    }
}

inline bool32 ValidateEntityPairs(game_state *GameState)
{
    bool32 Valid = true;
    for (uint32 HighEntityIndex = 1; HighEntityIndex < GameState->HighEntityCount; ++HighEntityIndex)
    {
        high_entity *High = GameState->HighEntities_ + HighEntityIndex;
        Valid = Valid &&
            (GameState->LowEntities[High->LowEntityIndex].HighEntityIndex == HighEntityIndex);
    }
    return Valid;
}

inline void OffsetAndCheckFrequencyByArea(game_state *GameState, v2 Offset, rectangle2 HighFrequencyBounds)
{
    for (uint32 HighEntityIndex = 1; HighEntityIndex < GameState->HighEntityCount;)
    {
        high_entity *High = GameState->HighEntities_ + HighEntityIndex;
        High->P += Offset;

        if (IsInRectangle(HighFrequencyBounds, High->P))
        {
            ++HighEntityIndex;
        }
        else
        {
            ASSERT(GameState->LowEntities[High->LowEntityIndex].HighEntityIndex == HighEntityIndex);
            //NOTE: this assumes that the MakeEntityLowFrequency will change the HighEntities array!
            MakeEntityLowFrequency(GameState, High->LowEntityIndex);
        }
    }
}


internal uint32 AddLowEntity(game_state* GameState, entity_type Type, world_position *P)
{
    ASSERT(GameState->LowEntityCount < ARRAY_COUNT(GameState->LowEntities))
    uint32 EntityIndex = GameState->LowEntityCount++;
        
    low_entity *EntityLow = GameState->LowEntities + EntityIndex;
    *EntityLow = {};
    EntityLow->Type = Type;

    if (P)
    {
        EntityLow->P = *P;
        ChangeEntityLocation(&GameState->WorldArena, GameState->World, EntityIndex, 0, P);
    }

    return EntityIndex;
}

internal uint32 AddWall(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    uint32 EntityIndex = AddLowEntity(GameState, EntityType_Wall, &P);
    low_entity *EntityLow = GetLowEntity(GameState, EntityIndex);

    EntityLow->Height = GameState->World->TileSideInMeters;
    EntityLow->Width = EntityLow->Height;
    EntityLow->Collides = true;

    return EntityIndex;
}

internal uint32 AddPlayer(game_state *GameState)
{
    world_position P = GameState->CameraP;
    uint32 EntityIndex = AddLowEntity(GameState, EntityType_Hero, &P);
    low_entity *EntityLow = GetLowEntity(GameState, EntityIndex);
    
    EntityLow->Height = 0.5f;// 1.4f;
    EntityLow->Width = 1.0f;// *Entity->Height;
    EntityLow->Collides = true;

    if (GameState->CameraFollowingEntityIndex == 0)
    {
        GameState->CameraFollowingEntityIndex = EntityIndex;
    }

    return EntityIndex;
}

internal bool32 TestWall(real32 WallX, real32 RelX, real32 RelY, real32 PlayerDeltaX, real32 PlayerDeltaY,
    real32* tMin, real32 MinY, real32 MaxY)
{
    bool32 Hit = false;
    real32 tEpsilon = 0.001f;
    if (PlayerDeltaX != 0.0f)
    {
        real32 tResult = (WallX - RelX) / PlayerDeltaX;
        real32 Y = RelY + tResult * PlayerDeltaY;
        if ((tResult >= 0.0f) && (*tMin > tResult))
        {
            if ((Y >= MinY) && (Y <= MaxY))
            {
                *tMin = MAXIMUM(0.0f, tResult - tEpsilon);
                Hit = true;
            }
        }
    }
    return Hit;
}

internal void MovePlayer(game_state* GameState, entity Entity, real32 dt, v2 ddP)
{
    world *World = GameState->World;

    real32 ddPLength = LengthSq(ddP);
    if (ddPLength > 1.0f)
    {
        ddP *= 1.0f / SquareRoot(ddPLength);
    }

    real32 PlayerSpeed = 50.0f; // m/s^2
    ddP *= PlayerSpeed;

    ddP += -8.0f* Entity.High->dP;

    v2 OldPlayerP = Entity.High->P;
    v2 PlayerDelta = (0.5f * ddP * Square(dt) +
        Entity.High->dP * dt);
    Entity.High->dP = ddP*dt + Entity.High->dP;
    //newPos = 1/2*accel*dTime^2 + vel*dTime + pos
    // newVelocity = accel * dTime + vel
    v2 NewPlayerP = OldPlayerP + PlayerDelta;

    /*
    uint32 MinTileX = MINIMUM(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
    uint32 MinTileY = MINIMUM(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);
    uint32 MaxTileX = MAXIMUM(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
    uint32 MaxTileY = MAXIMUM(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);

    uint32 EntityTileWidth = CeilReal32ToInt32(Entity.High->Width / World->TileSideInMeters);
    uint32 EntityTileHeight = CeilReal32ToInt32(Entity.High->Height / World->TileSideInMeters);

    MinTileX -= EntityTileWidth;
    MinTileY -= EntityTileHeight;
    MaxTileX += EntityTileWidth;
    MaxTileY += EntityTileHeight;

    uint32 AbsTileZ = Entity.High->P.AbsTileZ;
    */

    for (uint32 Iteration = 0; (Iteration < 4); ++Iteration)
    {
        real32 tMin = 1.0f;
        v2 WallNormal = {};
        uint32 HitHighEntityIndex = 0;

        v2 DesiredPosition = Entity.High->P + PlayerDelta;

        for (uint32 TestHighEntityIndex = 1; TestHighEntityIndex < GameState->HighEntityCount; ++TestHighEntityIndex)
        {
            if (TestHighEntityIndex != Entity.Low->HighEntityIndex)
            {
                entity TestEntity;
                TestEntity.High = GameState->HighEntities_ + TestHighEntityIndex;
                TestEntity.LowIndex = TestEntity.High->LowEntityIndex;
                TestEntity.Low = GameState->LowEntities + TestEntity.LowIndex;
                if (TestEntity.Low->Collides)
                {

                    real32 DiameterW = TestEntity.Low->Width + Entity.Low->Width;
                    real32 DiameterH = TestEntity.Low->Height + Entity.Low->Height;

                    v2 MinCorner = -0.5f * V2( DiameterW, DiameterH );
                    v2 MaxCorner = 0.5f * V2( DiameterW, DiameterH );

                    v2 Rel = Entity.High->P - TestEntity.High->P;

                    if (TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                        &tMin, MinCorner.Y, MaxCorner.Y))
                    {
                        WallNormal = V2( -1, 0 );
                        HitHighEntityIndex = TestHighEntityIndex;
                    }
                    if (TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X, PlayerDelta.Y,
                        &tMin, MinCorner.Y, MaxCorner.Y))
                    {
                        WallNormal = V2( 1, 0 );
                        HitHighEntityIndex = TestHighEntityIndex;
                    }
                    if (TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                        &tMin, MinCorner.X, MaxCorner.X))
                    {
                        WallNormal = V2( 0, -1 );
                        HitHighEntityIndex = TestHighEntityIndex;
                    }
                    if (TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y, PlayerDelta.X,
                        &tMin, MinCorner.X, MaxCorner.X))
                    {
                        WallNormal = V2( 0, 1 );
                        HitHighEntityIndex = TestHighEntityIndex;
                    }
                }
            }
        }

        Entity.High->P += tMin * PlayerDelta;
        if (HitHighEntityIndex)
        {
            Entity.High->dP = Entity.High->dP - Inner(Entity.High->dP, WallNormal)*WallNormal;
            PlayerDelta = DesiredPosition - Entity.High->P;
            PlayerDelta = PlayerDelta - Inner(PlayerDelta, WallNormal) * WallNormal;

            high_entity* HitHigh = GameState->HighEntities_ + HitHighEntityIndex;
            low_entity* HitLow= GameState->LowEntities + HitHigh->LowEntityIndex;
            //Entity.High->ChunkZ += HitLow->dAbsTileZ;
        }
        else
        {
            break;
        }
    }

    if ((Entity.High->dP.X == 0.0f) && (Entity.High->dP.Y == 0.0f))
    {
        //NOTE: leaving faceDirection whatever it was
    }
    else if (AbsoluteValue(Entity.High->dP.X) > AbsoluteValue(Entity.High->dP.Y))
    {
        if (Entity.High->dP.X > 0)
        {
            Entity.High->FacingDirection = 0;
        }
        else
        {
            Entity.High->FacingDirection = 2;
        }
    }
    else
    {
        if (Entity.High->dP.Y > 0)
        {
            Entity.High->FacingDirection = 1;
        }
        else
        {
            Entity.High->FacingDirection = 3;
        }
    }

    world_position NewP = MapIntoChunkSpace(GameState->World, GameState->CameraP, Entity.High->P);

    ChangeEntityLocation(&GameState->WorldArena, GameState->World, Entity.LowIndex, 
        &Entity.Low->P, &NewP);
    Entity.Low->P = NewP;

}

internal void SetCamera(game_state *GameState, world_position NewCameraP)
{
    world *World = GameState->World;

    ASSERT(ValidateEntityPairs(GameState));

    world_difference dCameraP = Subtract(World, &NewCameraP, &GameState->CameraP);
    GameState->CameraP = NewCameraP;

    uint32 TileSpanX = 17 * 3;
    uint32 TileSpanY = 9 * 3;
    rectangle2 CameraBounds = RectCenterDim(V2(0, 0),
        World->TileSideInMeters * V2((real32)TileSpanX, (real32)TileSpanY));

    v2 EntityOffsetForFrame = -dCameraP.dXY;
    OffsetAndCheckFrequencyByArea(GameState, EntityOffsetForFrame, CameraBounds);

    ASSERT(ValidateEntityPairs(GameState));

    world_position MinChunkP = MapIntoChunkSpace(World, NewCameraP, GetMinCorner(CameraBounds));
    world_position MaxChunkP = MapIntoChunkSpace(World, NewCameraP, GetMaxCorner(CameraBounds));
    for (int32 ChunkY = MinChunkP.ChunkY; 
         ChunkY <= MaxChunkP.ChunkY; 
         ++ChunkY)
    {
        for (int32 ChunkX = MinChunkP.ChunkX; 
             ChunkX <= MaxChunkP.ChunkX; 
             ++ChunkX)
        {
            world_chunk *Chunk = GetWorldChunk(World, ChunkX, ChunkY, NewCameraP.ChunkZ);
            if (Chunk)
            {
                for (world_entity_block *Block = &Chunk->FirstBlock; 
                    Block;
                    Block = Block->Next)
                {
                    for (uint32 EntityIndexIndex = 0; 
                        EntityIndexIndex < Block->EntityCount; 
                        ++EntityIndexIndex)
                    {
                        uint32 LowEntityIndex = Block->LowEntityIndex[EntityIndexIndex];
                        low_entity *Low = GameState->LowEntities + LowEntityIndex;

                        if (Low->HighEntityIndex == 0)
                        {
                            v2 CameraSpaceP = GetCameraSpaceP(GameState, Low);
                            if (IsInRectangle(CameraBounds, CameraSpaceP))
                            {
                                MakeEntityHighFrequency(GameState, Low, LowEntityIndex, CameraSpaceP);
                            }
                        }
                    }
                }
            }
        }
    }

    ASSERT(ValidateEntityPairs(GameState));
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    ASSERT((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == 
           ARRAY_COUNT(Input->Controllers->Buttons) )
    ASSERT(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state*)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        //NOTE: reserve entity slot 0 for the null entity
        AddLowEntity(GameState, EntityType_Null, 0);
        GameState->HighEntityCount = 1;

        GameState->Backdrop =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");
        GameState->Shadow =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_shadow.bmp");
        GameState->Tree =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test2/tree00.bmp");

        hero_bitmaps* Bitmap;

        Bitmap = GameState->HeroBitmaps;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_torso.bmp");
        Bitmap->AlignX = 72;
        Bitmap->AlignY = 182;
        ++Bitmap;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_torso.bmp");
        Bitmap->AlignX = 72;
        Bitmap->AlignY = 182;
        ++Bitmap;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_torso.bmp");
        Bitmap->AlignX = 72;
        Bitmap->AlignY = 182;
        ++Bitmap;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_torso.bmp");
        Bitmap->AlignX = 72;
        Bitmap->AlignY = 182;
        ++Bitmap;

        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(GameState),
            (uint8*)Memory->PermanentStorage + sizeof(game_state));

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world * World = GameState->World;
        InitializeWorld(World, 1.4f);

        uint32 RandomNumberIndex = 0;

        uint32 TilesPerWidth = 17;
        uint32 TilesPerHeight = 9;
        uint32 ScreenBaseX = 0;
        uint32 ScreenBaseY = 0;
        uint32 ScreenBaseZ = 0;
        uint32 ScreenX = ScreenBaseX;
        uint32 ScreenY = ScreenBaseY;
        uint32 AbsTileZ = ScreenBaseZ;

        bool32 DoorLeft = false;
        bool32 DoorRight = false;
        bool32 DoorTop = false;
        bool32 DoorBottom = false;
        bool32 DoorUp = false;
        bool32 DoorDown = false;

        for (uint32 ScreenIndex = 0; ScreenIndex < 200; ++ScreenIndex)
        {
            ASSERT(RandomNumberIndex < ARRAY_COUNT(RandomNumberTable));

            uint32 RandomChoice;
            //if (DoorUp || DoorDown)
            {
                RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
            }
            //else
            //{
            //    RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
            //}

            bool32 CreatedZDoor = false;
            if (RandomChoice == 2)
            {
                CreatedZDoor = true;
                if (AbsTileZ == ScreenBaseZ)
                {
                    DoorUp = true;
                }
                else
                {
                    DoorDown = true;
                }
            }
            else if (RandomChoice == 1)
            {
                DoorRight = true;
            }
            else
            {
                DoorTop = true;
            }

            for (uint32 TileY = 0; TileY < TilesPerHeight; ++TileY)
            {
                for (uint32 TileX = 0; TileX < TilesPerWidth; ++TileX)
                {
                    uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
                    uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;

                    uint32 TileValue = 1;
                    if (TileX == 0 && (!DoorLeft || (TileY != (TilesPerHeight / 2))))
                    {
                        TileValue = 2;
                    }

                    if ((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight / 2))))
                    {
                        TileValue = 2;
                    }

                    if (TileY == 0 && (!DoorBottom || (TileX != (TilesPerWidth / 2))))
                    {
                        TileValue = 2;
                    }

                    if ((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth / 2))))
                    {
                        TileValue = 2;
                    }

                    if (TileX == 10 && TileY == 6)
                    {
                        if (DoorUp)
                        {
                            TileValue = 3;
                        }
                        if (DoorDown)
                        {
                            TileValue = 4;
                        }
                    }

                    if (TileValue == 2)
                    {
                        AddWall(GameState, AbsTileX, AbsTileY, AbsTileZ);
                    }
                }
            }

            DoorLeft = DoorRight;
            DoorBottom = DoorTop;

            if (CreatedZDoor)
            {
                DoorDown = !DoorDown;
                DoorUp = !DoorUp;
            }
            else
            {
                DoorUp = false;
                DoorDown = false;
            }

            DoorRight = false;
            DoorTop = false;

            if (RandomChoice == 2)
            {
                if (AbsTileZ == ScreenBaseZ)
                    AbsTileZ = ScreenBaseZ + 1;
                else
                {
                    AbsTileZ = ScreenBaseZ;
                }

            }
            else if (RandomChoice == 1)
            {
                ScreenX += 1;
            }
            else
            {
                ScreenY += 1;
            }
        }

        world_position NewCameraP = {};
        NewCameraP = ChunkPositionFromTilePosition(World, ScreenBaseX * TilesPerWidth + 17 / 2,
            ScreenBaseY * TilesPerHeight + 9 / 2,
            ScreenBaseZ);
        SetCamera(GameState, NewCameraP);

        Memory->IsInitialized = true;
    }

    world * World = GameState->World;

    int32 TileSideInPixels = 60;
    real32 MetersToPixels = (real32)TileSideInPixels / (real32)World->TileSideInMeters;

    //
    // NOTE: 
    //
    for (int ControllerIndex = 0;
        ControllerIndex < ARRAY_COUNT(Input->Controllers);
        ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        uint32 LowIndex = GameState->PlayerIndexForController[ControllerIndex];
        if (LowIndex == 0)
        {
            if (Controller->Start.EndedDown)
            {
                uint32 EntityIndex = AddPlayer(GameState);
                GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
            }
        }
        else
        {
            entity ControllingEntity = GetHighEntity(GameState, LowIndex);
            v2 ddP = {};
            if (Controller->IsAnalog)
            {
                //NOTE: Use analog movement tuning
                ddP = V2( Controller->StickAverageX, Controller->StickAverageY );
            }
            else
            {
                //NOTE: Use digital movement tuning
                if (Controller->MoveUp.EndedDown)
                {
                    ddP.Y = 1.0f;
                }
                if (Controller->MoveDown.EndedDown)
                {
                    ddP.Y = -1.0f;
                }
                if (Controller->MoveLeft.EndedDown)
                {
                    ddP.X = -1.0f;
                }
                if (Controller->MoveRight.EndedDown)
                {
                    ddP.X = 1.0f;
                }
            }

            if (Controller->ActionUp.EndedDown)
            {
                ControllingEntity.High->dZ = 3.0f;
            }

            MovePlayer(GameState, ControllingEntity, Input->dtForFrame, ddP);
        }

    }

    v2 EntityOffsetForFrame = {};
    entity CameraFollowingEntity = GetHighEntity(GameState, GameState->CameraFollowingEntityIndex);
    if (CameraFollowingEntity.High)
    {
        world_position NewCameraP = GameState->CameraP;
        NewCameraP.ChunkZ = CameraFollowingEntity.Low->P.ChunkZ;

#if 0
        if (CameraFollowingEntity.High->P.X > (9.0f*World->TileSideInMeters))
        {
            NewCameraP.AbsTileX += 17;
        }
        if (CameraFollowingEntity.High->P.X < -(9.0f*World->TileSideInMeters))
        {
            NewCameraP.AbsTileX -= 17;
        }
        if (CameraFollowingEntity.High->P.Y >(5.0f*World->TileSideInMeters))
        {
            NewCameraP.AbsTileY += 9;
        }
        if (CameraFollowingEntity.High->P.Y < -(5.0f*World->TileSideInMeters))
        {
            NewCameraP.AbsTileY -= 9;
        }
#else
        NewCameraP = CameraFollowingEntity.Low->P;
#endif

        SetCamera(GameState, NewCameraP);
    }

    //
    // NOTE : render
    //
#if 1
    DrawRectangle(Buffer, V2(0.0f, 0.0f), V2((real32)Buffer->Width, (real32)Buffer->Height), 0.5f, 0.5f, 0.5f);
#else
    DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);
#endif

    real32 ScreenCenterX = 0.5f*(real32)Buffer->Width;
    real32 ScreenCenterY = 0.5f*(real32)Buffer->Height;

#if 0
    for (int32 RelRow = -10;
        RelRow < 10;
        ++RelRow)
    {
        for (int32 RelColumn = -20;
            RelColumn < 20;
            ++RelColumn)
        {
            uint32  Column = GameState->CameraP.AbsTileX + RelColumn;
            uint32 Row = GameState->CameraP.AbsTileY + RelRow;

            uint32 TileID = GetTileValue(World, Column, Row, GameState->CameraP.AbsTileZ);

            if (TileID > 1)
            {
                real32 Gray = -0.5f;
                if (TileID == 2)
                {
                    Gray = 1.0f;
                }

                if (TileID == 3)
                {
                    Gray = 0.25f;
                }

                if (TileID == 4)
                {
                    Gray = 0.75f;
                }

                v2 TileSide = {0.5f*TileSideInPixels, 0.5f*TileSideInPixels};
                v2 Cen = { (ScreenCenterX - MetersToPixels*GameState->CameraP.Offset_.X) + ((real32)RelColumn)*TileSideInPixels,
                    (ScreenCenterY + MetersToPixels*GameState->CameraP.Offset_.Y) - ((real32)RelRow)*TileSideInPixels };
                v2 Min = Cen - 0.9f *TileSide;
                v2 Max = Cen + 0.9f *TileSide;

                DrawRectangle(Buffer, Min, Max, Gray, Gray, Gray);
            }
        }
    }
#endif

    for (uint32 HighEntityIndex = 1; HighEntityIndex < GameState->HighEntityCount; ++HighEntityIndex)
    {
        high_entity *HighEntity = GameState->HighEntities_ + HighEntityIndex;
        low_entity *LowEntity = GameState->LowEntities + HighEntity->LowEntityIndex;

        HighEntity->P += EntityOffsetForFrame;

        real32 dt = Input->dtForFrame;
        real32 ddZ = -9.8f;
        HighEntity->Z = 0.5f * ddZ * Square(dt) + HighEntity->dZ * dt + HighEntity->Z;
        HighEntity->dZ = ddZ*dt + HighEntity->dZ;
        if (HighEntity->Z < 0)
        {
            HighEntity->Z = 0;
        }

        real32 CAlpha = 1.0f - 0.5f*HighEntity->Z;
        if (CAlpha < 0)
        {
            CAlpha = 0.0f;
        }

        real32 PlayerR = 1.0f;
        real32 PlayerG = 1.0f;
        real32 PlayerB = 0.0f;
        real32 PlayerGroundPointX = ScreenCenterX + MetersToPixels * HighEntity->P.X;
        real32 PlayerGroundPointY = ScreenCenterY - MetersToPixels * HighEntity->P.Y;
        real32 Z = -MetersToPixels * HighEntity->Z;
        v2 PlayerLeftTop = { PlayerGroundPointX - 0.5f*MetersToPixels*LowEntity->Width,
                             PlayerGroundPointY - 0.5f*MetersToPixels*LowEntity->Height };
        v2 EntityWidthHeight = { LowEntity->Width, LowEntity->Height };

        if (LowEntity->Type == EntityType_Hero)
        {
            hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[HighEntity->FacingDirection];
            DrawBitmap(Buffer, &GameState->Shadow, PlayerGroundPointX, PlayerGroundPointY, HeroBitmaps->AlignX, HeroBitmaps->AlignY, CAlpha);
            DrawBitmap(Buffer, &HeroBitmaps->Torso, PlayerGroundPointX, PlayerGroundPointY + Z, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
            DrawBitmap(Buffer, &HeroBitmaps->Cape, PlayerGroundPointX, PlayerGroundPointY + Z, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
            DrawBitmap(Buffer, &HeroBitmaps->Head, PlayerGroundPointX, PlayerGroundPointY + Z, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
        }
        else
        {
#if 0
            DrawRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + 0.9f*EntityWidthHeight * MetersToPixels,
                1.0f, 1.0f, 0.0f);
#endif
            DrawBitmap(Buffer, &GameState->Tree, 
                PlayerGroundPointX, PlayerGroundPointY + Z, 
                40, 80);
        }
    }
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state*)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundOutput, 400);
}

