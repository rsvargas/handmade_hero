#pragma once
#ifndef HANDMADE_H
#define HANDMADE_H

/*
NOTE:

HANDMADE_INTERNAL
0 = Build for public release
1 = Build for developer only

HANDMADE_SLOW
0 = No slow code allowed
1 = Slow code allowed!
*/

#include "handmade_platform.h"


#define MINIMUM(A, B) ((A < B) ? (A) : (B))
#define MAXIMUM(A, B) ((A > B) ? (A) : (B))

//
//
//

struct memory_arena
{
    memory_index Size;
    uint8* Base;
    memory_index Used;
};

internal void InitializeArena(memory_arena* Arena, memory_index Size, uint8* Base)
{
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

#define PushStruct(Arena, Type) (Type*)PushSize_(Arena, sizeof(Type))
#define PushArray(Arena, Count, Type) (Type*)PushSize_(Arena, (Count)*sizeof(Type))
internal void* PushSize_(memory_arena* Arena, memory_index Size)
{
    ASSERT((Arena->Used + Size) <= Arena->Size);
    void* Result = Arena->Base + Arena->Used;
    Arena->Used += Size;
    return Result;
}

#include "handmade_math.h"
#include "handmade_intrinsics.h"
#include "handmade_world.h"

struct loaded_bitmap
{
    int32 Width;
    int32 Height;
    uint32* Pixels;
};

struct hero_bitmaps
{
    v2 Align;
    loaded_bitmap Head;
    loaded_bitmap Cape;
    loaded_bitmap Torso;
};

struct high_entity
{
    v2 P; //NOTE: relative to the camera
    v2 dP;
    uint32 ChunkZ;
    uint32 FacingDirection;

    real32 tBob;

    real32 Z;
    real32 dZ;

    uint32 LowEntityIndex;
};

enum entity_type
{
    EntityType_Null,

    EntityType_Hero,
    EntityType_Wall,
    EntityType_Familiar,
    EntityType_Monstar,
    EntityType_Sword,
};

#define HIT_POINT_SUB_COUNT 4
struct hit_point
{
    uint8 Flags;
    uint8 FilledAmount;
};

struct low_entity
{
    entity_type Type;

    world_position P;
    real32 Height;
    real32 Width;

    bool32 Collides;
    int32 dAbsTileZ;

    uint32 HighEntityIndex;

    uint32 HitPointMax;
    hit_point HitPoint[16];

    uint32 SwordLowIndex;
    real32 DistanceRemaining;
};

struct entity
{
    uint32 LowIndex;
    low_entity *Low;
    high_entity *High;
};

struct entity_visible_piece
{
    loaded_bitmap *Bitmap;
    v2 Offset;
    real32 OffsetZ;
    real32 EntityZC;

    real32 R, G, B, A;
    v2 Dim;
};

struct game_state
{
    memory_arena WorldArena;
    world * World;

    uint32 CameraFollowingEntityIndex;
    world_position CameraP;

    uint32 PlayerIndexForController[ARRAY_COUNT(((game_input*)0)->Controllers)];

    uint32 LowEntityCount;
    low_entity LowEntities[100000];

    uint32 HighEntityCount;
    high_entity HighEntities_[256];

    loaded_bitmap Backdrop;
    loaded_bitmap Shadow;
    hero_bitmaps HeroBitmaps[4];

    loaded_bitmap Tree;
    loaded_bitmap Sword;
    real32 MetersToPixels;
};

struct entity_visible_piece_group
{
    game_state *GameState;
    uint32 PieceCount;
    entity_visible_piece Pieces[32];
};


#endif// HANDMADE_H
