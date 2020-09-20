#pragma once

#include <stdint.h>
#include <intrin.h>

#define global_variable static
#define internal static
#define local_persist static

#define KilloBytes(Size) (Size*1024LL)
#define MegaBytes(Size) (KilloBytes(Size)*1024LL)
#define GigaBytes(Size) (MegaBytes(Size)*1024LL)
#define BITMAP_BYTES_PER_PIXEL 4

#define Assert(Statement) if(!(Statement)) {*(volatile int *)0 = 0;}
#define INVALID_CODE_PATH Assert(!"InvalidCodePath")
#define ArrayCount(Array) (sizeof(Array) / (sizeof(Array[0])))

#define InvalidDefaultCase default: {INVALID_CODE_PATH;} break;

#define Minimum(A, B) (A) < (B) ? (A) : (B)
#define Maximum(A, B) (A) > (B) ? (A) : (B)

#define ZeroArray(Array) ZeroMemory(sizeof(Array), Array)
#define ZeroStruct(Struct) ZeroMemory(sizeof(Struct), &(Struct))

#define MonitorHz 30
#define NUM_OF_PROCESSORS 4
typedef size_t memory_index;

typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;

typedef unsigned __int8 uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;

typedef int32 bool32;
typedef bool32 b32;

typedef int8 s8;
typedef int16 s16;
typedef int32 s32;
typedef int64 s64;

typedef uint8 u8;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;

typedef float real32;
typedef double real64;
typedef real32 r32;
typedef real64 r64;
typedef real32 f32;
typedef real64 f64;

//TODO Put Compiler/Memory barier?
#define BEGIN_TIMED_BLOCK(ID) u64 CycleCountStart_##ID = __rdtsc();
#define END_TIMED_BLOCK(ID) GlobalMemory->DebugCycleCounters[DebugCycleCounterType_##ID].Cycles += __rdtsc() - CycleCountStart_##ID; ++GlobalMemory->DebugCycleCounters[DebugCycleCounterType_##ID].Hits;GlobalMemory->DebugCycleCounters[DebugCycleCounterType_##ID].Name = __FUNCTION__;
#define END_TIMED_BLOCK_COUNTED(ID, Count) GlobalMemory->DebugCycleCounters[DebugCycleCounterType_##ID].Cycles += __rdtsc() - CycleCountStart_##ID; GlobalMemory->DebugCycleCounters[DebugCycleCounterType_##ID].Hits += (Count);GlobalMemory->DebugCycleCounters[DebugCycleCounterType_##ID].Name = __FUNCTION__;

#define TIMED_FUNCTION_(Name, Counter) timed_block DebugCounter_(Name, Counter);
#define TIMED_FUNCTION() TIMED_FUNCTION_(__FUNCTION__, __COUNTER__)


enum 
{
    DebugCycleCounterType_GameEngine,
    DebugCycleCounterType_PixelHit,
    DebugCycleCounterType_Size,
};

struct debug_cycle_counter
{
    u64 Cycles;
    u32 Hits;
    const char *Name;
};


debug_cycle_counter DebugCycles[10];

struct timed_block
{
    u32 Index;
    u64 StartCycles;
    const char *Name;
    timed_block(const char *FunctionName, u32 DebugIndex)
    {
        Index = DebugIndex;
        Name = FunctionName;
        StartCycles = __rdtsc();
    }
    ~timed_block()
    {
        DebugCycles[Index].Cycles += __rdtsc() - StartCycles;
        DebugCycles[Index].Hits++;
        DebugCycles[Index].Name = Name;
    }
};

#pragma pack(push, 1)
struct alignas(1) bitmap_format
{
    u16 FileType;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 HeaderSize;
    s32 Width;
    s32 Height;
    u16 Planes;
    u16 BitsPerPixel;
    u32 Compression;
    u32 SizeOfBitmap;
    u32 HorizontalResolution;
    u32 VerticalResolution;
    u32 colorsUsed;
    u32 Importantcolors;
    u32 RedMask;
    u32 GreenMask;
    u32 BlueMask;
    u32 AlphaMask;
};
#pragma pack(pop)

struct v2i
{
    s32 X, Y;
};

struct v3i
{
    s32 X, Y, Z;
};

struct v2
{
    union
    {
        struct
        {
            r32 x, y;
        };
        r32 E[2];        
    };
    r32& operator[](int i) {return E[i];}
};

union v3
{
    struct
    {
        r32 r,g,b;    
    };
    struct
    {
        r32 x,y,z;
    };
    struct
    {
        v2 xy;
        r32 _Ignore0;
    };
    
};

union v4
{
    struct
    {
        r32 x,y,z,w;
     };
    struct
    {
        r32 r,g,b,a; 
    };
    struct
    {
        v3 rgb;
        r32 _Ignore0;
    };
    struct
    {
        v3 xyz;
        r32 _Ignore1;
    };
    struct
    {
        v2 xy;
        v2 _Ignore2;
    };
    struct
    {
        r32 _Ignore3;
        v2 yz;
        r32 _Ignore4;
    };
};


struct rectangle2
{
    union
    {
        v2 Min;
        struct
        {
            f32 MinX, MinY;
        };
    };
    union
    {
        v2 Max;
        struct
        {
            f32 MaxX, MaxY;
        };
    };    
};


struct rectangle2i
{
    union
    {
        v2i Min;
        struct
        {
            s32 MinX, MinY;
        };
    };
    union
    {
        v2i Max;
        struct
        {
            s32 MaxX, MaxY;
        };
    };
};

internal inline v2
V2(r32 x, r32 y)
{
    v2 Result = {(r32)x, (r32)y};

    return(Result);
}


internal inline v2
V2(s32 x, s32 y)
{
    v2 Result = {(r32)x, (r32)y};

    return(Result);
}


internal inline v2
V2(u32 x, u32 y)
{
    v2 Result = {(r32)x, (r32)y};

    return(Result);
}

internal inline v3
V3(s32 x, s32 y, s32 z)
{
    v3 Result = {(r32)x, (r32)y, (r32)z};

    return(Result);
}

internal inline v3
V3(r32 x, r32 y, r32 z)
{
    v3 Result = {(r32)x, (r32)y, (r32)z};

    return(Result);
}


internal inline v3
V3(u32 x, u32 y, u32 z)
{
    v3 Result = {(r32)x, (r32)y, (r32)z};

    return(Result);
}



internal inline v4
V4(s32 x, s32 y, s32 z, s32 w)
{
    v4 Result = {(r32)x, (r32)y, (r32) z, (r32) w};

    return(Result);
}

internal inline v4
V4(u32 x, u32 y, u32 z, u32 w)
{
    v4 Result = {(r32)x, (r32)y, (r32) z, (r32) w};

    return(Result);
}

internal inline v4
V4(r32 x, r32 y, r32 z, r32 w)
{
    v4 Result = {(r32)x, (r32)y, (r32) z, (r32) w};

    return(Result);
}

internal inline v2
operator+(v2 a, v2 b)
{
    v2 Result;
    Result.x = a.x + b.x;
    Result.y = a.y + b.y;

    return(Result);
}

internal inline v2
operator*(v2 a, r32 c)
{
    v2 Result;
    Result.x = c * a.x;
    Result.y = c * a.y;

    return(Result);
}

internal inline v2
operator*(r32 c, v2 a)
{
    v2 Result;
    Result.x = c * a.x;
    Result.y = c * a.y;

    return(Result);
}

internal inline v2
operator-(v2 a, v2 b)
{
    v2 Result;
    Result.x = a.x - b.x;
    Result.y = a.y - b.y;

    return(Result);
}

internal inline v2&
operator-=(v2 &a, v2 b)
{
    a.x -= b.x;
    a.y -= b.y;
    return(a);
}

internal inline v2&
operator+=(v2 &a, v2 b)
{
    a.x += b.x;
    a.y += b.y;
    return a;
}

internal inline v2&
operator*=(v2 &a, r32 c)
{
    a.x = c * a.x;
    a.y = c * a.y;

    return(a);
}

internal inline v2
operator-(v2 a)
{
    v2 Result;
    Result.x = -a.x;
    Result.y = -a.y;

    return(Result);
}

internal inline r32
operator*(v2 a, v2 b)
{
    r32 Result;
    Result = a.x*b.x + a.y*b.y;

    return(Result);
}

internal inline v3
operator+(v3 a, v3 b)
{
    v3 Result;
    Result.x = a.x + b.x;
    Result.y = a.y + b.y;
    Result.z = a.z + b.z;

    return(Result);
}

internal inline v3
operator-(v3 a, v3 b)
{
    v3 Result;
    Result.x = a.x - b.x;
    Result.y = a.y - b.y;
    Result.z = a.z - b.z;

    return(Result);
}

internal inline v3
operator*(r32 c, v3 a)
{
    v3 Result;
    Result.x = c * a.x;
    Result.y = c * a.y;
    Result.z = c * a.z;

    return(Result);
}

internal inline v3
operator*(v3 a, r32 c)
{
    v3 Result;
    Result.x = c * a.x;
    Result.y = c * a.y;
    Result.z = c * a.z;

    return(Result);
}

internal inline v3&
operator*=(v3 &a, r32 c)
{
    a.x = c * a.x;
    a.y = c * a.y;
    a.z = c * a.z;

    return(a);
}

internal inline v3&
operator+=(v3 &a, v3 &b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;

    return(a);
}

internal inline v4
operator+(v4 a, v4 b)
{
    v4 Result;
    Result.x = a.x + b.x;
    Result.y = a.y + b.y;
    Result.z = a.z + b.z;
    Result.w = a.w + b.w;

    return(Result);
}

internal inline v4
operator-(v4 a, v4 b)
{
    v4 Result;
    Result.x = a.x - b.x;
    Result.y = a.y - b.y;
    Result.z = a.z - b.z;
    Result.w = a.w - b.w;

    return(Result);
}

internal inline v4
operator*(r32 c, v4 a)
{
    v4 Result;
    Result.x = c * a.x;
    Result.y = c * a.y;
    Result.z = c * a.z;
    Result.w = c * a.w;

    return(Result);
}

internal inline v4
operator*(v4 a, r32 c)
{
    v4 Result;
    Result.x = c * a.x;
    Result.y = c * a.y;
    Result.z = c * a.z;
    Result.w = c * a.w;

    return(Result);
}

internal inline v4&
operator*=(v4 &a, r32 c)
{
    a.x = c * a.x;
    a.y = c * a.y;
    a.z = c * a.z;
    a.w = c * a.w;

    return(a);
}

internal inline v4&
operator+=(v4 &a, v4 &b)
{
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    a.w += b.w;

    return(a);
}

internal inline void
ZeroMemory(u32 Size, void *Ptr)
{
    u8 *byte = (u8 *)Ptr;
    while(Size--)
    {
        *byte++ = 0;
    }
}
