#include "handmade.h"

#include "handmade_world.cpp"
#include "handmade_random.h"
#include "handmade_sim_region.cpp"
#include "handmade_entity.cpp"

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
    real32 RealX, real32 RealY, real32 ShadowAlpha= 1.0f)
{
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
            A *= ShadowAlpha;
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

        Assert(Header->Compression == 3);

        uint32 RedMask = Header->RedMask;
        uint32 GreenMask = Header->GreenMask;
        uint32 BlueMask = Header->BlueMask;
        uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

        bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);

        Assert(RedScan.Found);
        Assert(GreenScan.Found);
        Assert(BlueScan.Found);
        Assert(AlphaScan.Found);

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

inline v2 GetCameraSpaceP(game_state *GameState, low_entity* EntityLow)
{
    world_difference Diff = Subtract(GameState->World, &EntityLow->P, &GameState->CameraP);
    v2 Result = Diff.dXY;

    return Result;
}

struct add_low_entity_result
{
    low_entity *Low;
    uint32 LowIndex;
};

internal add_low_entity_result AddLowEntity(game_state* GameState, entity_type Type, world_position P)
{
    Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntities))
    uint32 EntityIndex = GameState->LowEntityCount++;

    low_entity *EntityLow = GameState->LowEntities + EntityIndex;
    *EntityLow = {};
    EntityLow->Sim.Type = Type;
    EntityLow->P = NullPosition();

    ChangeEntityLocation(&GameState->WorldArena, GameState->World, EntityIndex, EntityLow, P);

    add_low_entity_result Result;
    Result.Low = EntityLow;
    Result.LowIndex = EntityIndex;

    return Result;
}

internal add_low_entity_result AddWall(game_state *GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Wall, P);

    Entity.Low->Sim.Height = GameState->World->TileSideInMeters;
    Entity.Low->Sim.Width = Entity.Low->Sim.Height;
    AddFlag(&Entity.Low->Sim, EntityFlag_Collides);

    return Entity;
}

internal void InitHitPoints(low_entity *EntityLow, uint32 HitPointCount)
{
    Assert(HitPointCount <= ArrayCount(EntityLow->Sim.HitPoint));
    EntityLow->Sim.HitPointMax = HitPointCount;
    for(uint32 HitPointIndex = 0;
            HitPointIndex <  EntityLow->Sim.HitPointMax;
            ++HitPointIndex)
    {
        hit_point *HitPoint = EntityLow->Sim.HitPoint + HitPointIndex;
        HitPoint->Flags = 0;
        HitPoint->FilledAmount = HIT_POINT_SUB_COUNT;
    }
}

internal add_low_entity_result AddSword(game_state* GameState)
{
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Sword, NullPosition());

    Entity.Low->Sim.Height = 0.5f;// 1.4f;
    Entity.Low->Sim.Width = 1.0f;// *Entity->Height;

    return Entity;
}

internal add_low_entity_result AddPlayer(game_state *GameState)
{
    world_position P = GameState->CameraP;
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Hero, P);

    Entity.Low->Sim.Height = 0.5f;// 1.4f;
    Entity.Low->Sim.Width = 1.0f;// *Entity->Height;
    AddFlag(&Entity.Low->Sim, EntityFlag_Collides);

    InitHitPoints(Entity.Low, 3);

    add_low_entity_result Sword = AddSword(GameState);
    Entity.Low->Sim.Sword.Index = Sword.LowIndex;

    if (GameState->CameraFollowingEntityIndex == 0)
    {
        GameState->CameraFollowingEntityIndex = Entity.LowIndex;
    }

    return Entity;
}

internal add_low_entity_result AddMonstar(game_state* GameState, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Monstar, P);

    Entity.Low->Sim.Height = 0.5f;// 1.4f;
    Entity.Low->Sim.Width = 1.0f;// *Entity->Height;
    AddFlag(&Entity.Low->Sim, EntityFlag_Collides);

    InitHitPoints(Entity.Low, 3);

    return Entity;
}

internal add_low_entity_result AddFamiliar(game_state* GameState, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ)
{
    world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
    add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Familiar, P);

    Entity.Low->Sim.Height = 0.5f;// 1.4f;
    Entity.Low->Sim.Width = 1.0f;// *Entity->Height;
    AddFlag(&Entity.Low->Sim, EntityFlag_Collides);

    return Entity;
}



inline void PushPiece(entity_visible_piece_group *Group, loaded_bitmap *Bitmap,
    v2 Offset, real32 OffsetZ, v2 Align, v2 Dim, v4 Color, real32 EntityZC = 1.0f)
{
    Assert(Group->PieceCount < ArrayCount(Group->Pieces));
    entity_visible_piece *Piece = Group->Pieces + Group->PieceCount++;
    Piece->Bitmap = Bitmap;
    Piece->Offset = Group->GameState->MetersToPixels*V2(Offset.X, -Offset.Y) - Align;
    Piece->OffsetZ = Group->GameState->MetersToPixels*OffsetZ;
    Piece->EntityZC = EntityZC;
    Piece->R = Color.R;
    Piece->G = Color.G;
    Piece->B = Color.B;
    Piece->A = Color.A;
    Piece->Dim = Dim;
}

inline void PushBitmap(entity_visible_piece_group *Group, loaded_bitmap *Bitmap,
    v2 Offset, real32 OffsetZ, v2 Align, real32 Alpha = 1.0f, real32 EntityZC = 1.0f)
{
    PushPiece(Group, Bitmap, Offset, OffsetZ, Align, V2(0,0), V4(1.0f, 1.0f, 1.0f, Alpha), EntityZC);
}

inline void PushRect(entity_visible_piece_group *Group, v2 Offset, real32 OffsetZ,
    v2 Dim, v4 Color, real32 EntityZC = 1.0f)
{
    PushPiece(Group, 0, Offset, OffsetZ, V2(0, 0), Dim, Color, EntityZC);
}

internal void DrawHitpoints(sim_entity *SimEntity, entity_visible_piece_group *PieceGroup)
{
    if (SimEntity->HitPointMax >= 1 )
    {
        v2 HealthDim = { 0.2f, 0.2f };
        real32 SpacingX = 1.5f*HealthDim.X;
        v2 HitP = { -0.5f*(SimEntity->HitPointMax - 1) * SpacingX, -0.25f };
        v2 dHitP = { SpacingX, 0.0f };
        for (uint32 HealthIndex = 0;
                HealthIndex < SimEntity->HitPointMax;
                ++HealthIndex)
        {
            hit_point *HitPoint = SimEntity->HitPoint + HealthIndex;
            v4 Color = { 1.0f, 0.0f, 0.0f, 1.0f };
            if (HitPoint->FilledAmount == 0)
            {
                Color = V4(0.2f, 0.2f, 0.2f, 1.0f);
            }

            PushRect(PieceGroup, HitP, 0, HealthDim, Color, 0.0f);
            HitP += dHitP;
        }
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
           ArrayCount(Input->Controllers->Buttons) )
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state*)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        //NOTE: reserve entity slot 0 for the null entity
        AddLowEntity(GameState, EntityType_Null, NullPosition());

        GameState->Backdrop =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");
        GameState->Shadow =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_shadow.bmp");
        GameState->Tree =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test2/tree00.bmp");
        GameState->Sword =
            DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test2/rock03.bmp");

        hero_bitmaps* Bitmap;

        Bitmap = GameState->HeroBitmaps;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_torso.bmp");
        Bitmap->Align = V2( 72, 182);
        ++Bitmap;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_torso.bmp");
        Bitmap->Align = V2(72, 182);
        ++Bitmap;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_torso.bmp");
        Bitmap->Align = V2(72, 182);
        ++Bitmap;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_torso.bmp");
        Bitmap->Align = V2(72, 182);
        ++Bitmap;

        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(GameState),
            (uint8*)Memory->PermanentStorage + sizeof(game_state));

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world * World = GameState->World;
        InitializeWorld(World, 1.4f);

        int32 TileSideInPixels = 60;
        GameState->MetersToPixels = (real32)TileSideInPixels / (real32)World->TileSideInMeters;

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
            Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));

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
        int32 CameraTileX = ScreenBaseX * TilesPerWidth + 17 / 2;
        int32 CameraTileY = ScreenBaseY * TilesPerHeight + 9 / 2;
        int32 CameraTileZ = ScreenBaseZ;
        NewCameraP = ChunkPositionFromTilePosition(World,
            CameraTileX,
            CameraTileY,
            CameraTileZ);
        GameState->CameraP = NewCameraP;

        AddMonstar(GameState, CameraTileX + 2, CameraTileY + 2, CameraTileZ);
        for (int FamiliarIndex = 0;
            FamiliarIndex < 1;
            ++FamiliarIndex)
        {
            int32 FamiliarOffsetX = (RandomNumberTable[RandomNumberIndex++] % 14) - 7;
            int32 FamiliarOffsetY = (RandomNumberTable[RandomNumberIndex++] % 6) - 3;
            AddFamiliar(GameState, CameraTileX + FamiliarOffsetX, CameraTileY + FamiliarOffsetY, CameraTileZ);
        }


        Memory->IsInitialized = true;
    }

    world * World = GameState->World;

    real32 MetersToPixels = GameState->MetersToPixels;

    //
    // NOTE:
    //
    for (int ControllerIndex = 0;
        ControllerIndex < ArrayCount(Input->Controllers);
        ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        controlled_hero *ConHero = GameState->ControlledHeroes + ControllerIndex;
        if (ConHero->EntityIndex == 0)
        {
            if (Controller->Start.EndedDown)
            {
                *ConHero = {};
                ConHero->EntityIndex = AddPlayer(GameState).LowIndex;
            }
        }
        else
        {
            ConHero->dZ = 0.0f;
            ConHero->ddP = {};
            ConHero->dSword = {};

            if (Controller->IsAnalog)
            {
                //NOTE: Use analog movement tuning
                ConHero->ddP = V2( Controller->StickAverageX, Controller->StickAverageY );
            }
            else
            {
                //NOTE: Use digital movement tuning
                if (Controller->MoveUp.EndedDown)
                {
                    ConHero->ddP.Y = 1.0f;
                }
                if (Controller->MoveDown.EndedDown)
                {
                    ConHero->ddP.Y = -1.0f;
                }
                if (Controller->MoveLeft.EndedDown)
                {
                    ConHero->ddP.X = -1.0f;
                }
                if (Controller->MoveRight.EndedDown)
                {
                    ConHero->ddP.X = 1.0f;
                }
            }

            if (Controller->Start.EndedDown)
            {
                ConHero->dZ = 3.0f;
            }

            if (Controller->ActionUp.EndedDown)
            {
                ConHero->dSword  = V2(0, 1);
            }
            if (Controller->ActionDown.EndedDown)
            {
                ConHero->dSword  = V2(0, -1);
            }
            if (Controller->ActionLeft.EndedDown)
            {
                ConHero->dSword  = V2(-1, 0);
            }
            if (Controller->ActionRight.EndedDown)
            {
                ConHero->dSword  = V2(1, 0);
            }


        }

    }


    uint32 TileSpanX = 17 * 3;
    uint32 TileSpanY = 9 * 3;
    rectangle2 CameraBounds = RectCenterDim(V2(0, 0),
        World->TileSideInMeters * V2((real32)TileSpanX, (real32)TileSpanY));

    memory_arena SimArena;
    InitializeArena(&SimArena, Memory->TransientStorageSize, Memory->TransientStorage);
    sim_region *SimRegion = BeginSim(&SimArena, GameState, GameState->World,
        GameState->CameraP, CameraBounds);

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

    entity_visible_piece_group PieceGroup;
    PieceGroup.GameState = GameState;
    sim_entity *Entity = SimRegion->Entities;
    for (uint32 EntityIndex = 0;
        EntityIndex < SimRegion->EntityCount;
        ++EntityIndex, ++Entity)
    {
        if(Entity->Updatable)
        {
            PieceGroup.PieceCount = 0;

            real32 dt = Input->dtForFrame;
            real32 ShadowAlpha = 1.0f - 0.5f*Entity->Z;
            if (ShadowAlpha< 0)
            {
                ShadowAlpha = 0.0f;
            }

            move_spec MoveSpec = DefaultMoveSpec();
            v2 ddP = {};

            hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[Entity->FacingDirection];
            switch (Entity->Type)
            {
            case EntityType_Hero:
                {
                    for(uint32 ControlIndex = 0;
                        ControlIndex < ArrayCount(GameState->ControlledHeroes);
                        ++ControlIndex)
                    {
                        controlled_hero *ConHero = GameState->ControlledHeroes + ControlIndex;
                        if(Entity->StorageIndex == ConHero->EntityIndex)
                        {
                            if(ConHero->dZ != 0.0f)
                            {
                                Entity->dZ = ConHero->dZ;
                            }

                            MoveSpec.UnitMaxAccelVector = true;
                            MoveSpec.Speed = 50.0f;
                            MoveSpec.Drag = 8.0f;
                            ddP = ConHero->ddP;

                            if((ConHero->dSword.X != 0.0f) || (ConHero->dSword.Y != 0.0f))
                            {
                                sim_entity *Sword = Entity->Sword.Ptr;
                                if(Sword && IsSet(Sword, EntityFlag_Nonspatial))
                                {
                                    Sword->DistanceLimit = 5.0f;
                                    MakeEntitySpatial(Sword, Entity->P, 5.0f*ConHero->dSword);
                                }

                            }
                        }
                    }


                    PushBitmap(&PieceGroup, &GameState->Shadow, V2(0, 0), 0, HeroBitmaps->Align, ShadowAlpha, 0.0f);
                    PushBitmap(&PieceGroup, &HeroBitmaps->Torso, V2(0, 0), 0, HeroBitmaps->Align);
                    PushBitmap(&PieceGroup, &HeroBitmaps->Cape, V2(0, 0), 0, HeroBitmaps->Align);
                    PushBitmap(&PieceGroup, &HeroBitmaps->Head, V2(0, 0), 0, HeroBitmaps->Align);

                    DrawHitpoints(Entity, &PieceGroup);

                } break;

                case EntityType_Wall:
                {
                    PushBitmap(&PieceGroup, &GameState->Tree, V2(0, 0), 0, V2(40, 80));
                } break;

                case EntityType_Sword:
                {
                    MoveSpec.UnitMaxAccelVector = false;
                    MoveSpec.Speed = 0.0f;
                    MoveSpec.Drag = 0.0f;

                    v2 OldP = Entity->P;
                    if(Entity->DistanceLimit == 0.0f)
                    {
                        MakeEntityNonSpatial(Entity);
                    }

                    PushBitmap(&PieceGroup, &GameState->Shadow, V2(0, 0), 0, HeroBitmaps->Align, ShadowAlpha, 0.0f);
                    PushBitmap(&PieceGroup, &GameState->Sword, V2(0, 0), 0, V2(29, 10));
                } break;

                case EntityType_Familiar:
                {
                    sim_entity *ClosestHero = 0;
                    real32 ClosestHeroDSq = Square(10.0f); //NOTE: Ten meter maximum Search

                    sim_entity *TestEntity = SimRegion->Entities;
                    for (uint32 TestEntityIndex = 0;
                        TestEntityIndex < SimRegion->EntityCount;
                        ++TestEntityIndex, ++TestEntity)
                    {
                        if (TestEntity->Type == EntityType_Hero)
                        {
                            real32 TestDSq = LengthSq(TestEntity->P - Entity->P);
                            if (TestEntity->Type == EntityType_Hero)
                            {
                                TestDSq *= 0.5f;
                            }
                            if (ClosestHeroDSq > TestDSq)
                            {
                                ClosestHero = TestEntity;
                                ClosestHeroDSq = TestDSq;
                            }
                        }
                    }

                    if (ClosestHero && ClosestHeroDSq > Square(3.0f) )
                    {
                        real32 Acceleration = 0.5f;
                        real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroDSq);
                        ddP = OneOverLength *(ClosestHero->P - Entity->P);
                    }


                    MoveSpec.UnitMaxAccelVector = true;
                    MoveSpec.Speed = 50.0f;
                    MoveSpec.Drag = 8.0f;

                    Entity->tBob += dt;
                    if (Entity->tBob > 2.0f*Pi32)
                    {
                        Entity->tBob -= (2.0f*Pi32);
                    }
                    real32 BobSin = Sin(2.0f*Entity->tBob);
                    PushBitmap(&PieceGroup, &GameState->Shadow, V2(0, 0), 0, HeroBitmaps->Align, (0.5f*ShadowAlpha) + 0.2f*BobSin, 0.0f);
                    PushBitmap(&PieceGroup, &HeroBitmaps->Head, V2(0, 0), 0.25f*BobSin, HeroBitmaps->Align);
                } break;

                case EntityType_Monstar:
                {
                    PushBitmap(&PieceGroup, &GameState->Shadow, V2(0, 0), 0, HeroBitmaps->Align, ShadowAlpha, 0.0f);
                    PushBitmap(&PieceGroup, &HeroBitmaps->Torso, V2(0, 0), 0, HeroBitmaps->Align);
                    DrawHitpoints(Entity, &PieceGroup);
                } break;

                default:
                {
                    INVALID_CODE_PATH;
                } break;
            }

            if(!IsSet(Entity, EntityFlag_Nonspatial))
            {
                MoveEntity(SimRegion, Entity, Input->dtForFrame, &MoveSpec, ddP);
            }

            real32 EntityGroundPointX = ScreenCenterX + MetersToPixels * Entity->P.X;
            real32 EntityGroundPointY = ScreenCenterY - MetersToPixels * Entity->P.Y;
            real32 EntityZ = -MetersToPixels * Entity->Z;
    #if 0
            v2 PlayerLeftTop = { PlayerGroundPointX - 0.5f*MetersToPixels*LowEntity->Width,
                PlayerGroundPointY - 0.5f*MetersToPixels*LowEntity->Height };
            v2 EntityWidthHeight = { LowEntity->Width, LowEntity->Height };
            DrawRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + 0.9f*EntityWidthHeight * MetersToPixels,
                1.0f, 1.0f, 0.0f);
    #endif


            for (uint32 PieceIndex = 0;
                PieceIndex < PieceGroup.PieceCount;
                ++PieceIndex)
            {
                entity_visible_piece *Piece = PieceGroup.Pieces + PieceIndex;
                v2 Center = { EntityGroundPointX + Piece->Offset.X,
                    EntityGroundPointY + Piece->Offset.Y + Piece->OffsetZ + Piece->EntityZC* EntityZ };
                if (Piece->Bitmap)
                {
                    DrawBitmap(Buffer, Piece->Bitmap,
                        Center.X,
                        Center.Y,
                        Piece->A);
                }
                else
                {
                    v2 HalfDim = 0.5f*MetersToPixels * Piece->Dim;
                    DrawRectangle(Buffer, Center - HalfDim, Center + HalfDim,
                        Piece->R, Piece->G, Piece->B);
                }
            }
        }
    }

    world_position WorldOrigin = {};
    world_difference  Diff = Subtract(SimRegion->World, &WorldOrigin, &SimRegion->Origin);
    DrawRectangle(Buffer, Diff.dXY, V2(10.0f, 10.0f), 1.0f, 1.0f, 0.0f);

    EndSim(SimRegion, GameState);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state*)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundOutput, 400);
}
