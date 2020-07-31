#ifndef HELLO_H_

#include "platform.h"
#include "tile_map.h"
#include "intrinsic.h"
#include "sim_region.h"

struct memory_arena
{
    u8 *Memory;
    u32 AvailableSize;
    u32 Index;
};

#define PushStruct(Arena, Type) (Type *)PushArray_(Arena, 1, sizeof(Type))
#define PushArray(Arena, Count, Type) (Type *)PushArray_(Arena, Count, sizeof(Type))

internal void *
PushArray_(memory_arena *Arena, u32 Count, u32 Size)
{
    Assert((Arena->Index += Count * Size) <= Arena->AvailableSize);
    void *Result = Arena->Memory + Arena->Index;
    Arena->Index += Count * Size;

    return(Result);
}

struct loaded_bitmap
{
    u32 Width;
    u32 Height;
    u8* Bytes;

    v2 Offset;
};  

struct loaded_file
{ 
    void *Bytes;
    u32 Size;
};

struct game_sound_output_buffer
{
    s16 *Buffer;
    u32 Samples;
};

struct game_offscreen_bitmap
{
    void *Memory;
    s32 Width;
    s32 Height;
    s32 Pitch;
    s32 Stride;

};

loaded_file ReadFileToMemory(char* FileName); 
typedef decltype(ReadFileToMemory) read_file_to_memory;

struct game_memory
{
    u32 PersistentStorageSize;
    void *PersistentStorage;
    void *TransientStorage;
    u32 TransientStorageSize;
    read_file_to_memory *ReadFileToMemory;
};

struct entity
{
    tile_map_position P;
    sim_entity Sim;
};

struct game_button_state
{
    u32 HalfTransitionCount;
    bool32 IsDown;
};

struct game_controller_input
{
    union
    {
        game_button_state Buttons[6];
        struct
        {
            game_button_state Left;
            game_button_state Right;
            game_button_state Up;
            game_button_state Down;
            game_button_state Space;
            game_button_state DoubleSpeed;
        };
    };
};

struct game_input
{
    game_controller_input Keyboard;
    r32 dtForFrame;
};

struct controlled_hero
{
    uint32 EntityIndex;
    v2 ddP;
    v2 dSword;
    real32 dZ;
};

struct hero_bitmaps
{
    union{
        loaded_bitmap Hero[4];
        struct
        {
            loaded_bitmap Body;
            loaded_bitmap Shadow;
        };
    };
};

struct game_state
{
    float t;
    r32 SecondsPerFrame;
    controlled_hero Player;
    tile_map TileMap;

    tile_map_position CameraPosition;
    u32 CameraFollowingEntityIndex;
    
    memory_arena Arena;
    bool32 Initialized;
    
    union {
        hero_bitmaps HeroBitmaps[2];
        struct {
            hero_bitmaps HeroFacingLeft;
            hero_bitmaps HeroFacingRight;
        };
    };
    u32 EntityCount;
    entity Entities[1024];
    r32 PixelsPerMeter; 
};

struct entity_visible_piece
{
    loaded_bitmap *Bitmap;
    v2 Offset;
    r32 R, G, B, A;
    v2 Dim;
};

struct entity_visible_piece_group
{
    game_state *GameState;
    u32 PieceCount;
    entity_visible_piece Pieces[32];
};

extern "C" void GameEngine(game_memory *Memory, game_input *Input, game_sound_output_buffer *Sound, game_offscreen_bitmap *Video);


#define HELLO_H_
#endif
