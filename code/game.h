#pragma once
//#pragma clang diagnostic ignored "-Wunused-variable"
//#pragma clang diagnostic ignored "-Wmissing-braces"
//#pragma clang diagnostic ignored "-Wunused-function"
#include "platform.h"
#include "intrinsic.h"
#include "math.h"
#include "random.h"
#include "tile_map.h"
#include "sim_region.h"
//#include "render_group.h"

struct memory_arena
{
    u8 *Memory;
    u32 AvailableSize;
    memory_index Index;
};

struct temporary_memory
{
    memory_arena *Arena;
    u32 Used;
    s32 Counter;
};

struct normal_map
{
    u32 Width;
    u32 Height;
    v4 *Bytes;
};

struct loaded_bitmap
{
    u32 Width;
    u32 Height;
    u8 *Bytes;

    u32 Pitch;

    v2 Align;
};  

struct enviroment_map
{
    loaded_bitmap* LOD[4];
};

struct ground_buffer
{
    //NOTE This is the center of Bitmap
    tile_map_position P; 
    loaded_bitmap *Bitmap;
    
};

struct work_queue;
struct transient_state
{
    bool32 IsInitialized;
    memory_arena Arena;
    u32 GroundBufferCount;
    ground_buffer *GroundBuffers;

    work_queue *HighPriorityQueue;

    enviroment_map Sky;
    enviroment_map Ground;
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

struct render_group;
struct render_work_data
{
    render_group *Group;
    loaded_bitmap *Target;
    rectangle2i ClipRect;

    r32 PixelsPerMeter;
};
typedef void (*callback_function)(void *);

struct work_queue;
loaded_file ReadFileToMemory(const char* );
bool32 AddToQueue_(work_queue *, callback_function, void *);
bool32 RemoveAndExecuteTask_(work_queue *, u32 );
void WaitForAllToFinish_(work_queue *);
    
typedef decltype(ReadFileToMemory) read_file_to_memory;
typedef decltype(AddToQueue_) add_to_queue;
typedef decltype(RemoveAndExecuteTask_) remove_and_execute_task;
typedef decltype(WaitForAllToFinish_) wait_for_all_to_finish;


struct game_memory
{
    u32 PersistentStorageSize;
    void *PersistentStorage;
    void *TransientStorage;
    u32 TransientStorageSize;

    read_file_to_memory *ReadFileToMemory;
    add_to_queue *AddToQueue;
    remove_and_execute_task *RemoveAndExecuteTask;
    wait_for_all_to_finish *WaitForAllToFinish;
    
    work_queue *WorkQueue;

    debug_cycle_counter DebugCycleCounters[DebugCycleCounterType_Size];
    
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
    f32 dtForFrame;
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
    f32 SecondsPerFrame;
    controlled_hero Player;
    tile_map TileMap;
    f32 WallSize;

    tile_map_position CameraPosition;
    u32 CameraFollowingEntityIndex;

    loaded_bitmap Grass[2];
    loaded_bitmap Ground[4];
    loaded_bitmap Rock[4];
    loaded_bitmap Tree[3];
    loaded_bitmap Tuft[3];

    normal_map SphereNormal;
    normal_map PyramidNormal;
    normal_map TestNormal;

    loaded_bitmap Reflection;
    
    memory_arena Arena;
    bool32 IsInitialized;
    
    union {
        hero_bitmaps HeroBitmaps[2];
        struct {
            hero_bitmaps HeroFacingLeft;
            hero_bitmaps HeroFacingRight;
        };
    };
    u32 EntityCount;
    entity Entities[1024];
    f32 PixelsPerMeter;
    f32 MetersPerPixel;
};

extern "C" void __declspec(dllexport) GameEngine(game_memory *Memory, game_input *Input, game_sound_output_buffer *Sound, game_offscreen_bitmap *Video);


#define PushStruct(Arena, Type) (Type *)PushArray_(Arena, 1, sizeof(Type))
#define PushArray(Arena, Count, Type) (Type *)PushArray_(Arena, Count, sizeof(Type))
#define PushSize(Arena, Count) PushArray(Arena, Count, u8)

internal void *
PushArray_(memory_arena *Arena, u32 Count, u32 Size)
{
    Assert(Arena);
    Assert((Arena->Index += Count * Size) <= Arena->AvailableSize);
    void *Result = Arena->Memory + Arena->Index;
    Arena->Index += Count * Size;

    return(Result);
}

internal temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    Assert(Arena);
    temporary_memory Result = {};

    Result.Arena = Arena;
    Assert(Arena->Index < Arena->AvailableSize);
    Result.Used = Arena->Index;
    Result.Counter = 1;

    return(Result);
}

internal void
EndTemporaryMemory(temporary_memory *TempMemory)
{
    memory_arena *Arena = TempMemory->Arena;
    Assert(Arena->Index >= TempMemory->Used);
    Arena->Index = TempMemory->Used;
    Assert(TempMemory->Counter > 0);
    --TempMemory->Counter;
}
