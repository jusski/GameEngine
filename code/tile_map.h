#if !defined(TILE_MAP_H)

struct entity_block
{
    u32 EntityCount;
    u32 EntityIndex[16];
    
    entity_block *Next;
};

struct tile
{
    u32 X;
    u32 Y;

    entity_block FirstBlock;
    
    tile *NextInHash;
};

struct tile_map
{
    r32 TileSizeInMeters;
    u32 TileSizeInPixels;
    r32 PixelsPerMeter;
    tile *ChunkHashMap[4096];

    entity_block *FirstFree;
};

struct tile_map_position
{
    s32 X;
    s32 Y;

    v2 Offset;
};

#define TILE_MAP_H
#endif
