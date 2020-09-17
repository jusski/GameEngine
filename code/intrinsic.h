#pragma once

#pragma warning(push, 0)
#include <math.h>
#pragma warning(pop)

internal inline s32
CeillR32ToS32(r32 Value)
{
    s32 Result;
    Result = (s32)ceilf(Value);

    return(Result);
}

internal inline s32
RoundR32ToS32(r32 Value)
{
    s32 Result;
    Result = (s32)roundf(Value);

    return(Result);
}

internal inline u32
RoundR32ToU32(r32 Value)
{
    Assert(Value >= 0.0f);
    return (u32)(Value + 0.5f);
}

internal inline s32
TruncateR32ToS32(r32 Value)
{
    return (s32)(Value);
}

internal inline u32
TruncateR32ToU32(r32 Value)
{
    Assert(Value >= 0.0f);
    return (u32)(Value);
}

internal inline r32
Square(r32 Value)
{
    return Value*Value;
}

internal inline r32
SquareRoot(r32 Value)
{
    r32 Result;
    Result = sqrtf(Value);

    return(Result);
}

internal inline r32
LengthSquared(v2 Value)
{
    r32 Result = Square(Value.x) + Square(Value.y);

    return(Result);
}

internal inline r32
LengthSquared(v3 Value)
{
    r32 Result = Square(Value.x) + Square(Value.y) + Square(Value.z);

    return(Result);
}

internal inline r32
LengthSquared(v4 Value)
{
    r32 Result = Square(Value.x) + Square(Value.y) +
        Square(Value.z) + Square(Value.w);

    return(Result);
}

internal inline r32
Length(v2 Value)
{
    r32 Result = SquareRoot(LengthSquared(Value));

    return(Result);
}

internal inline r32
Length(v3 Value)
{
    r32 Result = SquareRoot(LengthSquared(Value));

    return(Result);
}

internal inline r32
Length(v4 Value)
{
    r32 Result = SquareRoot(LengthSquared(Value));

    return(Result);
}


internal inline r32
Sin(r32 A)
{
    r32 Result = (r32)sin(A);

    return(Result);
}

internal inline r32
Tan(r32 A)
{
    r32 Result = (r32)tan(A);

    return(Result);
}

internal inline r32
Cos(r32 A)
{
    r32 Result = (r32)cos(A);

    return(Result);
}


internal inline s32
Floor(r32 A)
{
    s32 Result = (s32)floor(A);

    return(Result);
}

internal inline s32
Ceill(r32 A)
{
    s32 Result = (s32)ceill(A);

    return(Result);
}

r32
internal inline Clamp(r32 A, r32 Min, r32 Max)
{
    r32 Result = Minimum(Maximum(A, Min), Max);
    
    return(Result);
}

internal inline r32
Clamp01(r32 A)
{
    r32 Result = Clamp(A, 0.0f, 1.0f);

    return(Result);
}

internal inline v3
Clamp01(v3 A)
{
    v3 Result = V3(Clamp01(A.r), Clamp01(A.g), Clamp01(A.b));
        
    return(Result);
}

internal inline v4
Clamp01(v4 A)
{
    v4 Result = V4(Clamp01(A.r), Clamp01(A.g), Clamp01(A.b), Clamp01(A.a));
        
    return(Result);
}

