/*
 * win32_hello.h
 *
 *  Created on: Jun 9, 2020
 *      Author: desktop
 */

#ifndef WIN32_HELLO_H_
#define WIN32_HELLO_H_


#pragma warning(push, 0)
#include <windows.h>
#include <dsound.h>
#include <stdio.h>
#include <math.h>
#pragma warning(pop)

#include "game.h"

typedef  decltype(GameEngine) game_engine;

struct win32_state
{
    void *MemoryArea;
    SIZE_T TotalMemorySize;

    HANDLE RecordingHandle;
    HANDLE PlayBackHandle;

    void *RecordingMemoryArea;

    bool32 IsRecording;
    bool32 IsPlaying;
};
struct win32_game_code
{
    HMODULE Library;
    game_engine *GameEngine;
    FILETIME LastWriteTime;
};


struct win32_offscreen_bitmap
{
    BITMAPINFO BitmapInfo;
    void *Memory;
    int32 BytesPerPixel;
    int32 Width;
    int32 Height;
    int32 Pitch;
    int32 Stride;
};

struct win32_sound_output
{
    LPDIRECTSOUNDBUFFER Buffer;
    uint32 BufferSize;

    uint32 BytesPerSample;
    uint32 Channels;
    int32 RunningSampleIndex;
    uint32 SamplesPerSecond;
};

struct win32_debug_sound_markers
{
    DWORD PlayCursor;
    DWORD WriteCursor;
    DWORD ByteToLock;

};




#endif /* WIN32_HELLO_H_ */
