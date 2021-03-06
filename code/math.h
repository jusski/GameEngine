#pragma once

internal inline bool32
IsEmptySet(rectangle2i A)
{
    bool32 Result = false;
    
    Result = (A.MaxX < A.MinX) || (A.MaxY < A.MinY);
    return(Result);
}

internal inline r32
SafeRatio0(r32 A, r32 B)
{
    r32 Result = A / B;
    
    if (B == 0.0f)
    {
        Result = 0;
    }
    return(Result);
}

internal inline r32
Inner(v2 A, v2 B)
{
    r32 Result = A.x * B.x + A.y * B.y;

    return(Result);
}

internal inline r32
Lerp(r32 A, r32 t, r32 B)
{
    r32 Result = (1.0f - t)*A + t*B;

    return(Result);
}

internal inline v3
Lerp(v3 A, r32 t, v3 B)
{
    v3 Result = (1.0f - t) * A + t*B;
        
    return(Result);
}

internal inline v4
Lerp(v4 A, r32 t, v4 B)
{
    v4 Result = (1.0f - t) * A + t*B;
        
    return(Result);
}

internal inline v2
Perp(v2 A)
{
    v2 Result = {-A.y, A.x};

    return(Result);
}

internal inline rectangle2
AddRadiusTo(rectangle2 A, r32 Width, r32 Height)
{
    rectangle2 Result;
    Result.Min = A.Min - v2{Width, Height};
    Result.Max = A.Max + v2{Width, Height};

    return(Result);
}

internal inline bool32
IsInRectangle(rectangle2 &Rectangle, v2 &P)
{
    bool32 Result = false;

    if((Rectangle.Min.x <= P.x) &&
       (Rectangle.Min.y <= P.y) &&
       (Rectangle.Max.x > P.x) &&
       (Rectangle.Max.y > P.y))
    {
        Result = true;
    }
    
    return(Result);
}



internal inline rectangle2
RectMinMax(v2 Min, v2 Max)
{
    rectangle2 Result;

    Result.Min = Min;
    Result.Max = Max;
    
    return(Result);
}

internal inline rectangle2
RectCentHalfDim(v2 Center, v2 HalfDim)
{
    rectangle2 Result;

    Result.Min = Center - HalfDim;
    Result.Max = Center + HalfDim;
    
    return(Result);
}

internal inline rectangle2
RectCentDim(v2 Center, v2 Dim)
{
    rectangle2 Result = RectCentHalfDim(Center, 0.5f*Dim);
    
    return(Result);
}

internal inline v2
GetCenter(rectangle2 Rect)
{
    v2 Result;

    Result = Rect.Min + 0.5f*(Rect.Max - Rect.Min);
    
    return(Result);
}

internal inline v2
GetMinCorner(rectangle2 Rect)
{
    v2 Result = Rect.Min;

    return(Result);
}

internal inline v2
GetMaxCorner(rectangle2 Rect)
{
    v2 Result = Rect.Max;

    return(Result);
}

internal inline v4
SRGBA255ToLinear1(v4 A)
{
    v4 Result = 1.0f/255.0f * A;
    Result.r = Square(Result.r);
    Result.g = Square(Result.g);
    Result.b = Square(Result.b);

    return(Result);
}

internal inline v4
UnpackBGRA(u32 *Ptr)
{
    v4 Result = V4(((*Ptr >> 16) & 0xFF),
                   ((*Ptr >> 8)  & 0xFF),
                   ((*Ptr >> 0)  & 0xFF),
                   ((*Ptr >> 24) & 0xFF));

    return(Result);
}

internal inline u32
PackBGRA(v4 Color)
{
    u32 Result =
        ((u32)Color.a << 24) |
        ((u32)Color.r << 16) |
        ((u32)Color.g << 8)  |
        ((u32)Color.b << 0);

    return(Result);
}

internal inline v2
Normalize(v2 A)
{
    r32 InvLength = SafeRatio0(1.0f, Length(A));
    v2 Result = InvLength * A;

    return(Result);
}

internal inline v3
Normalize(v3 A)
{
    r32 InvLength = SafeRatio0(1.0f, Length(A));
    v3 Result = InvLength * A;

    return(Result);
}

internal inline v4
Normalize(v4 A)
{
    r32 InvLength = SafeRatio0(1.0f, Length(A));
    v4 Result = InvLength * A;

    return(Result);
}

internal inline v4
Linear1ToSRGBA255(v4 A)
{
    v4 Result = {255.0f * SquareRoot(A.r),
                 255.0f * SquareRoot(A.g),
                 255.0f * SquareRoot(A.b),
                 255.0f * A.a};

    return(Result);
}

internal inline v3
Hadamard(v3 A, v3 B)
{
    v3 Result = {A.x * B.x,
                 A.y * B.y,
                 A.z * B.z};

    return(Result);
}

internal inline v4
Hadamard(v4 A, v4 B)
{
    v4 Result = {A.x * B.x,
                 A.y * B.y,
                 A.z * B.z,
                 A.w * B.w};

    return(Result);
}

internal inline rectangle2i
Intersection(rectangle2i A, rectangle2i B)
{
    rectangle2i Result;

    Result.MinX = Maximum(A.MinX, B.MinX);
    Result.MaxX = Minimum(A.MaxX, B.MaxX);

    Result.MinY = Maximum(A.MinY, B.MinY);
    Result.MaxY = Minimum(A.MaxY, B.MaxY);
    
    return(Result);
}

internal inline rectangle2i
Union(rectangle2i A, rectangle2i B)
{
    rectangle2i Result;

    Result.MinX = Minimum(A.MinX, B.MinX);
    Result.MaxX = Maximum(A.MaxX, B.MaxX);

    Result.MinY = Minimum(A.MinY, B.MinY);
    Result.MaxY = Maximum(A.MaxY, B.MaxY);

    return(Result);
}
