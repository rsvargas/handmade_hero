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
#include "handmade_tile.h"

struct world
{
    tile_map* TileMap;
};

struct loaded_bitmap
{
    int32 Width;
    int32 Height;
    uint32* Pixels;
};

struct hero_bitmaps
{
    int32 AlignX;
    int32 AlignY;
    loaded_bitmap Head;
    loaded_bitmap Cape;
    loaded_bitmap Torso;
};

struct high_entity
{
    bool Exists;
    v2 P; //NOTE: relative to the camera
    v2 dP;
    uint32 FacingDirection;
};

struct low_entity
{
};

struct dormant_entity
{
    tile_map_position P;
    real32 Height;
    real32 Width;
};

enum entity_residence
{
    EntityResidence_Nonexistent = 0,
    EntityResidence_Dormant=  0x1,
    EntityResidence_Low = 0x2,
    EntityResidence_High = 0x4,
};

struct entity
{
    uint32 Residence;
    low_entity *Low;
    dormant_entity *Dormant;
    high_entity *High;
};

struct game_state
{
    memory_arena WorldArena;
    world * World;

    uint32 CameraFollowingEntityIndex;
    tile_map_position CameraP;

    uint32 PlayerIndexForController[ARRAY_COUNT(((game_input*)0)->Controllers)];

    uint32 EntityCount;
    entity_residence EntityResidence[256];
    high_entity HighEntities[256];
    low_entity LowEntities[256];
    dormant_entity DormantEntities[256];

    loaded_bitmap Backdrop;
    hero_bitmaps HeroBitmaps[4];
};

#endif// HANDMADE_H
