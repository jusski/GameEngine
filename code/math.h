#pragma once

inline r32
Lerp(r32 A, r32 t, r32 B)
{
    r32 Result = (1.0f - t)*A + t*B;

    return(Result);
}


inline rectangle2
AddRadiusTo(rectangle2 A, r32 Width, r32 Height)
{
    rectangle2 Result;
    Result.Min = A.Min - v2{Width, Height};
    Result.Max = A.Max + v2{Width, Height};

    return(Result);
}


inline bool32
IsInRectangle(rectangle2 &Rectangle, v2 &P)
{
    bool32 Result = false;

    if((Rectangle.Min.X <= P.X) &&
       (Rectangle.Min.Y <= P.Y) &&
       (Rectangle.Max.X > P.X) &&
       (Rectangle.Max.Y > P.Y))
    {
        Result = true;
    }
    
    return(Result);
}



inline rectangle2
RectMinMax(v2 Min, v2 Max)
{
    rectangle2 Result;

    Result.Min = Min;
    Result.Max = Max;
    
    return(Result);
}

inline rectangle2
RectCentHalfDim(v2 Center, v2 HalfDim)
{
    rectangle2 Result;

    Result.Min = Center - HalfDim;
    Result.Max = Center + HalfDim;
    
    return(Result);
}

inline rectangle2
RectCentDim(v2 Center, v2 Dim)
{
    rectangle2 Result = RectCentHalfDim(Center, 0.5f*Dim);
    
    return(Result);
}

inline v2
GetCenter(rectangle2 Rect)
{
    v2 Result;

    Result = Rect.Min + 0.5f*(Rect.Max - Rect.Min);
    
    return(Result);
}

inline v2
GetMinCorner(rectangle2 Rect)
{
    v2 Result = Rect.Min;

    return(Result);
}

inline v2
GetMaxCorner(rectangle2 Rect)
{
    v2 Result = Rect.Max;

    return(Result);
}

