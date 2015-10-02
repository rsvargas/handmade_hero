#pragma once

struct move_spec
{
    bool32 UnitMaxAccelVector;
    real32 Speed;
    real32 Drag;
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

struct sim_entity;
union entity_reference
{
        sim_entity *Ptr;
        uint32 Index;
};

enum sim_entity_flags
{
    EntityFlag_Collides =   (1 << 0),
    EntityFlag_Nonspatial = (1 << 1),

    EntityFlag_Simming = (1 << 30),
};

struct sim_entity
{
    //NOTE: There are only for the sim Region
    uint32 StorageIndex;
    bool32 Updatable;

    //

    entity_type Type;
    uint32 Flags;

    v2 P;
    v2 dP;

    real32 Z;
    real32 dZ;

    real32 DistanceLimit;

    uint32 ChunkZ;

    real32 Height;
    real32 Width;

    uint32 FacingDirection;
    real32 tBob;

    int32 dAbsTileZ;

    uint32 HitPointMax;
    hit_point HitPoint[16];

    entity_reference Sword;
};

struct sim_entity_hash
{
    sim_entity *Ptr;
    uint32 Index;
};

struct sim_region
{
    world *World;

    world_position Origin;
    rectangle2 Bounds;
    rectangle2 UpdatableBounds;

    uint32 MaxEntityCount;
    uint32 EntityCount;
    sim_entity *Entities;
    sim_entity_hash Hash[4096];
};
