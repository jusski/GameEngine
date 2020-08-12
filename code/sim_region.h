#pragma once

struct move_spec
{
    bool32 UnitMaxAccelVector;
    real32 Speed;
    real32 Drag;
    real32 DistanceRemaining;
};

enum entity_type
{
    EntityType_Null,
    EntityType_Hero,
    EntityType_Wall,
    EntityType_Sword,
    EntityType_Monstar,
    EntityType_Tree,
};

struct sim_entity;
union entity_reference
{
    sim_entity *Ptr;
    u32 Index;
};

enum entity_flag
{
    EntityFlag_NonSpatial = (1 << 1),
    EntityFlag_Collides   = (1 << 2),
    EntityFlag_Moves      = (1 << 3),

    EntityFlag_SimSpace   = (1 << 30),
};

struct sim_entity
{
    v2 P;
    v2 dP;
    u32 FacingDirection;
    r32 Width, Height;

    entity_type Type;
    u32 Flags;
    bool32 Updatable;

    entity_reference Sword;
    r32 DistanceRemaining;
    u32 StorageIndex;
    
};


struct sim_entity_hash
{
    sim_entity *Ptr;
    u32 Index;
};

struct sim_region
{
    tile_map *TileMap;
    tile_map_position Origin;
    rectangle2 Bounds;
    rectangle2 UpdatableBounds;
    
    u32 EntityCount;
    u32 MaxEntityCount;
    sim_entity *Entities;
    sim_entity_hash Hash[4096];
};

