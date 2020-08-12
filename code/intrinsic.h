#if !defined(INTRINSIC_H)

#pragma warning(push, 0)
#include <math.h>
#pragma warning(pop)

inline s32
CeillR32ToS32(r32 Value)
{
    s32 Result;
    Result = (s32)ceilf(Value);

    return(Result);
}

inline s32
RoundR32ToS32(r32 Value)
{
    s32 Result;
    Result = (s32)roundf(Value);

    return(Result);
}

inline u32
RoundR32ToU32(r32 Value)
{
    Assert(Value >= 0.0f);
    return (u32)(Value + 0.5f);
}

inline s32
TruncateR32ToS32(r32 Value)
{
    return (s32)(Value);
}

inline u32
TruncateR32ToU32(r32 Value)
{
    Assert(Value >= 0.0f);
    return (u32)(Value);
}

inline r32
Square(r32 Value)
{
    return Value*Value;
}

inline r32
SquareRoot(r32 Value)
{
    r32 Result;
    Result = sqrtf(Value);

    return(Result);
}

inline r32
LengthSquared(v2 Value)
{
    r32 Result = Square(Value.x) + Square(Value.y);

    return(Result);
}

inline r32
Length(v2 Value)
{
    r32 Result = SquareRoot(LengthSquared(Value));

    return(Result);
}

#define INTRINSIC_H
#endif
