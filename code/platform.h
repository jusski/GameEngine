#pragma once

#include <stdint.h>
#define global_variable static
#define internal static
#define local_persist static

#define KilloBytes(Size) (Size*1024LL)
#define MegaBytes(Size) (KilloBytes(Size)*1024LL)
#define GigaBytes(Size) (MegaBytes(Size)*1024LL)
#define BITMAP_BYTES_PER_PIXEL 4

#define Assert(Statement) if(!(Statement)) {*(int *)0 = 0;}
#define INVALID_CODE_PATH Assert(!"InvalidCodePath")
#define ArrayCount(Array) (sizeof(Array) / (sizeof(Array[0])))
#define Trace(Format, ...) {_snprintf_s(OutputDebugMessage, sizeof(OutputDebugMessage), Format, ## __VA_ARGS__); OutputDebugStringA(OutputDebugMessage);}

#define Minimum(A, B) (A) < (B) ? (A) : (B)
#define Maximum(A, B) (A) > (B) ? (A) : (B)

#define ZeroArray(Array) ZeroMemory(sizeof(Array), Array)
#define ZeroStruct(Struct) ZeroMemory(sizeof(Struct), &(Struct))

#define MonitorHz 30

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


#pragma pack(push, 1)
struct bitmap_format
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
    u32 ColorsUsed;
    u32 ImportantColors;
    u32 RedMask;
    u32 GreenMask;
    u32 BlueMask;
    u32 AlphaMask;
};
#pragma pack(pop)

struct v2
{
    union
    {
        struct
        {
            r32 X, Y;
        };
        r32 E[2];        
    };
    r32& operator[](int i) {return E[i];}
};

union v3
{
    struct
    {
        r32 R,G,B;    
    };
    struct
    {
        r32 X,Y,Z;
    };
    struct
    {
        v2 XY;
        r32 _Ignore;
    };
    
};


inline v2
V2(s32 X, s32 Y)
{
    v2 Result = {(r32)X, (r32)Y};

    return(Result);
}

inline v3
V3(s32 X, s32 Y, s32 Z)
{
    v3 Result = {(r32)X, (r32)Y, (r32)Z};

    return(Result);
}


inline v3
V3(u32 X, u32 Y, u32 Z)
{
    v3 Result = {(r32)X, (r32)Y, (r32)Z};

    return(Result);
}


inline v2
V2(u32 X, u32 Y)
{
    v2 Result = {(r32)X, (r32)Y};

    return(Result);
}


inline v2
V2(r32 X, r32 Y)
{
    v2 Result = {(r32)X, (r32)Y};

    return(Result);
}



union v4
{
    struct
    {
        r32 R,G,B,A;
    };
    struct
    {
        r32 X,Y,Z,W;
    };
    r32 E[4];
};


struct rectangle2
{
    v2 Min;
    v2 Max;
};

inline v2
operator+(v2 A, v2 B)
{
    v2 Result;
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;

    return(Result);
}

inline v2
operator*(r32 C, v2 A)
{
    v2 Result;
    Result.X = C * A.X;
    Result.Y = C * A.Y;

    return(Result);
}

inline v2
operator*(v2 A, r32 C)
{
    v2 Result;
    Result.X = C * A.X;
    Result.Y = C * A.Y;

    return(Result);
}

inline v2
operator-(v2 A, v2 B)
{
    v2 Result;
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;

    return(Result);
}

inline v2&
operator-=(v2 &A, v2 B)
{
    A.X -= B.X;
    A.Y -= B.Y;
    return(A);
}

inline v2&
operator+=(v2 &A, v2 B)
{
    A.X += B.X;
    A.Y += B.Y;
    return A;
}

inline v2&
operator*=(v2 &A, r32 C)
{
    A.X = C * A.X;
    A.Y = C * A.Y;

    return(A);
}

inline v2
operator-(v2 A)
{
    v2 Result;
    Result.X = -A.X;
    Result.Y = -A.Y;

    return(Result);
}

inline r32
operator*(v2 A, v2 B)
{
    r32 Result;
    Result = A.X*B.X + A.Y*B.Y;

    return(Result);
}

inline void
ZeroMemory(u32 Size, void *Ptr)
{
    u8 *Byte = (u8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}
