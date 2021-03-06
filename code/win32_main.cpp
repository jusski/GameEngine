#include "pch.h"
#include "win32_main.h"
#include "stb_easy_font.h"

global_variable volatile bool Running = true;

global_variable win32_offscreen_bitmap GlobalBitmap = {};
global_variable game_offscreen_bitmap Video = {};
global_variable long ClientWidth;
global_variable long ClientHeight;
//global_variable char OutputDebugMessage[200];
global_variable GLuint TextureHandle;

void print_string(float x, float y, char *text, float r, float g, float b)
{
  static char buffer[99999]; // ~500 chars
  int num_quads;

  num_quads = stb_easy_font_print(x, y, text, NULL, buffer, sizeof(buffer));

  glColor3f(r,g,b);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_FLOAT, 16, buffer);
  glDrawArrays(GL_QUADS, 0, num_quads*4);
  glDisableClientState(GL_VERTEX_ARRAY);
}

internal inline void
Win32TogleFullScreen(HWND Window)
{
    
    local_persist WINDOWPLACEMENT WindowPosition = { sizeof(WindowPosition) };
                        
                        
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
        if (GetWindowPlacement(Window, &WindowPosition) &&
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo)) {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &WindowPosition);
        SetWindowPos(Window, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
                     
}

loaded_file
ReadFileToMemory(const char* FileName)
{
    loaded_file Result = {};
    HANDLE FileHandle = CreateFile(FileName, GENERIC_READ,
                                   0, 0, OPEN_EXISTING, 0 ,0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    { 
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            Assert(FileSize.QuadPart < GigaBytes(4));
            Result.Bytes = VirtualAlloc(0, (SIZE_T)FileSize.QuadPart,
                                        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (Result.Bytes)
            {
                DWORD BytesRead;
                ReadFile(FileHandle, Result.Bytes, (DWORD)FileSize.QuadPart, &BytesRead, 0);
                if (BytesRead == (DWORD)FileSize.QuadPart)
                {
                    Result.Size = BytesRead;
                }
                else
                {
                    VirtualFree(Result.Bytes, 0, MEM_RELEASE);
                    Result.Bytes = 0;
                }
            }
        }
        CloseHandle(FileHandle);
    }
    return(Result);
}

internal inline u32
StringLength(char *String)
{
    u32 Result = 0; 
    while(*String++)
    {
        Result++;
    }
    return(Result);
}

internal void
CatStrings(char *SourceA, u32 SizeA,
           char *SourceB, u32 SizeB,
           char *Destination, u32 SizeD)
{ 
    Assert(SizeA + SizeB < SizeD);

    while(*SourceA != 0 && SizeA--)
    {
        *Destination++ = *SourceA++;
    }

    while(*SourceB != 0 && SizeB--)
    {
        *Destination++ = *SourceB++;
    }

    *Destination = 0;
}

internal inline void
Win32GetExecutableDirectory()
{
    char FileName[MAX_PATH];
    if (GetModuleFileName(0, FileName, sizeof(FileName)))
    {
        char *Character;
        char *LastSlash = FileName;
        for (Character = FileName; *Character != '\0'; Character++)
        {
            if (*Character == '\\')
            {
                LastSlash = Character;
            }
        }
        Character = LastSlash + 1;
        *Character = '\0'; 
    }
//    return FileName;
}

internal win32_game_code
Win32LoadGameCode(char *GameLibraryFileName)
{
    win32_game_code Result = {};
    
    char *SourceFile = "game.dll";
    char *TargetFile = "game_dll_copy.dll";
    
    while(!CopyFile(SourceFile, TargetFile, FALSE))
    {
        Sleep(1);
    }

    Result.Library = LoadLibraryA(TargetFile);
    if (Result.Library)
    {
        WIN32_FIND_DATAA FindFileData;
        HANDLE FindHandle = FindFirstFileA(SourceFile, &FindFileData);
        if (FindHandle != INVALID_HANDLE_VALUE)
        {
            Result.LastWriteTime = FindFileData.ftLastWriteTime;
        }
        Result.GameEngine =
            (game_engine *)GetProcAddress(Result.Library, "GameEngine");
    }
    
    return(Result);
}

internal void
Win32UnloadGameCode(win32_game_code *GameCode)
{
    if (GameCode->Library)
    {
        FreeLibrary(GameCode->Library);
        GameCode->Library = 0;
        GameCode->GameEngine = 0;
    }
}

internal inline FILETIME
Win32LastWriteTime(char *FileName)
{
    FILETIME Result = {};
    WIN32_FILE_ATTRIBUTE_DATA FileData;
    if(GetFileAttributesExA(FileName, GetFileExInfoStandard, &FileData))
    {
        Result = FileData.ftLastWriteTime;
    }
    return(Result);
}

internal void
Win32DebugSoundDrawVerticalLine(DWORD Position, s32 LineHeight, s32 LineOffset, u32 Color, u32 SoundBufferSize)
{
    LineHeight = LineHeight > GlobalBitmap.Height ? GlobalBitmap.Height : LineHeight;

    u32 x = (u32)(Position * ((float)GlobalBitmap.Width / (float)SoundBufferSize));
    uint8 *Memory = (uint8 *)GlobalBitmap.Memory + x * GlobalBitmap.Stride + LineOffset * GlobalBitmap.Pitch;
    for (s32 i = LineOffset; i < LineHeight; i++)
    {
        *(u32 *)Memory = Color;
        Memory += GlobalBitmap.Pitch;
    }
}

internal void
Win32DebugSoundSync(win32_debug_sound_markers *Markers, u32 Count, u32 SoundBufferSize)
{
    for (u32 i = 0; i < Count; i++)
    {
        win32_debug_sound_markers *Marker = &Markers[i];
        Win32DebugSoundDrawVerticalLine(Marker->PlayCursor, 400, 0, 0xff00ff, SoundBufferSize);
        Win32DebugSoundDrawVerticalLine(Marker->WriteCursor, 450, 40, 0xffffff, SoundBufferSize);
    }
}

internal void
Win32InitDirectSound(HWND Window, win32_sound_output *SoundOutput)
{
    LPDIRECTSOUND DirectSound;
    if (DirectSoundCreate(0, &DirectSound, 0) == DS_OK)
    {
        if (DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY) == DS_OK)
        {
            HRESULT Result;

            WAVEFORMATEX WaveFormat = {};
            WaveFormat.cbSize = 0;
            WaveFormat.nChannels = (WORD)SoundOutput->Channels;
            WaveFormat.nBlockAlign = (WORD)SoundOutput->BytesPerSample;
            WaveFormat.nAvgBytesPerSec = SoundOutput->SamplesPerSecond * WaveFormat.nBlockAlign;
            WaveFormat.nSamplesPerSec = SoundOutput->SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;

            DSBUFFERDESC BufferDescription = { };
            BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
            BufferDescription.dwSize = sizeof(BufferDescription);

            LPDIRECTSOUNDBUFFER PrimaryBuffer;
            Result = DirectSound->CreateSoundBuffer(&BufferDescription,
                                                    &PrimaryBuffer, 0);
            if (Result == DS_OK)
            {
                Result = PrimaryBuffer->SetFormat(&WaveFormat);
                if (Result != DS_OK)
                {
                    Trace("FAILED: SetSoundBufferFormat\n");
                }
            }
            else
            {
                Trace("FAILED: CreeateSoundBuffer\n");
            }

            BufferDescription.dwBufferBytes = SoundOutput->BufferSize;
            BufferDescription.dwFlags = 0;
            BufferDescription.lpwfxFormat = &WaveFormat;

            Result = DirectSound->CreateSoundBuffer(&BufferDescription,
                                                    &SoundOutput->Buffer, 0);
            if (Result != DS_OK)
            {
                Trace("FAILED: CreateSoundBuffer\n");
            }
        }
        else
        {
            Trace("FAILED: SetCooperativeLevel\n");
        }
    }
    else
    {
        Trace("DirectSoundCreate failed\n");
    }
}

internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput,
                     game_sound_output_buffer *SoundData,
                     DWORD ByteToLock, DWORD Bytes)
{
    void *WriteCursor1;
    DWORD AudioBytes1;
    void *WriteCursor2;
    DWORD AudioBytes2;
    SoundOutput->Buffer->Lock(ByteToLock, Bytes,
                              &WriteCursor1, &AudioBytes1,
                              &WriteCursor2, &AudioBytes2, 0);

    int16 *Target = (int16 *) WriteCursor1;
    int16 *Source = SoundData->Buffer;
    u32 SamplesToWrite = AudioBytes1 / SoundOutput->BytesPerSample;
    while (SamplesToWrite)
    {

        *Target++ = *Source++;
        *Target++ = *Source++;
        SamplesToWrite--;
        SoundOutput->RunningSampleIndex += 1;
    }

    SamplesToWrite = AudioBytes2 / SoundOutput->BytesPerSample;
    Target = (int16 *) WriteCursor2;
    while (SamplesToWrite)
    {
        *Target++ = *Source++;
        *Target++ = *Source++;
        SamplesToWrite--;
        SoundOutput->RunningSampleIndex += 1;
    }
    SoundOutput->Buffer->Unlock(WriteCursor1, AudioBytes1,
                                WriteCursor2, AudioBytes2);
}





internal void
Win32ResizeBitmap(win32_offscreen_bitmap *Bitmap, s32 Width, s32 Height)
{
    Bitmap->BytesPerPixel = 4;
    Bitmap->Width = Width;
    Bitmap->Height = Height;
    Bitmap->Pitch =  Bitmap->BytesPerPixel * Width;
    Bitmap->Stride = Bitmap->BytesPerPixel;

    BITMAPINFOHEADER *BitmapInfoHeader = &Bitmap->BitmapInfo.bmiHeader;

    BitmapInfoHeader->biSize = sizeof(*BitmapInfoHeader);
    BitmapInfoHeader->biWidth = Bitmap->Width;
    BitmapInfoHeader->biHeight = ((int64)Bitmap->Height);
    BitmapInfoHeader->biPlanes = 1;
    BitmapInfoHeader->biBitCount = (WORD)(Bitmap->BytesPerPixel * 8);
    BitmapInfoHeader->biCompression = BI_RGB;

    if (Bitmap->Memory)
    {
        VirtualFree(Bitmap->Memory, 0, MEM_RELEASE);
    }
    SIZE_T BitmapMemorySize = (SIZE_T)(Bitmap->BytesPerPixel * Bitmap->Width * Bitmap->Height);
    if (BitmapMemorySize == 0)
    {
        BitmapMemorySize = BITMAP_BYTES_PER_PIXEL * 100 * 100;
    }
    Bitmap->Memory = VirtualAlloc(0, BitmapMemorySize,
                                  MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);

    Assert(Bitmap->Memory);
    
}


#if 1
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include "stb_truetype.h"

unsigned char ttf_buffer[1<<20];
unsigned char temp_bitmap[512*512];

stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs
GLuint ftex;

void my_stbtt_initfont(void)
{
   fread(ttf_buffer, 1, 1<<20, fopen("c:/windows/fonts/times.ttf", "rb"));
   stbtt_BakeFontBitmap(ttf_buffer,0, 32.0, temp_bitmap,512,512, 32,96, cdata); // no guarantee this fits!
   // can free ttf_buffer at this point
   glGenTextures(1, &ftex);
   glBindTexture(GL_TEXTURE_2D, ftex);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512,512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
   // can free temp_bitmap at this point
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void my_stbtt_print(float x, float y, char *text)
{
   // assume orthographic projection with units = screen pixels, origin at top left
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, ftex);
   glBegin(GL_QUADS);
   while (*text) {
      if (*text >= 32 && *text < 128) {
         stbtt_aligned_quad q;
         stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1);//1=opengl & d3d10+,0=d3d9
         glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y0);
         glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y0);
         glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y1);
         glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y1);
      }
      ++text;
   }
   glEnd();
}
#endif


internal void
Win32UpdateWindow(HDC DeviceContext, win32_offscreen_bitmap *Bitmap, long x, long y, long Width, long Height)
{
#if 0
    //PatBlt(DeviceContext, 0, 0, 100, 100, BLACKNESS);
    StretchDIBits(DeviceContext,
                  0, 0, Width, Height,
                  0, 0, Bitmap->Width, Bitmap->Height,
                  Bitmap->Memory, &Bitmap->BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
#else
    
    glViewport(0, 0, ClientWidth, ClientHeight);
    glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glBindTexture(GL_TEXTURE_2D, TextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, Bitmap->Width, Bitmap->Height,
                 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, Bitmap->Memory);
        
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glBegin(GL_TRIANGLES);


    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1,-1);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(1,-1);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1,1);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1,-1);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(1,1);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-1,1);
    
    
    glEnd();

    //glFrustum(-.1, 0.1, -0.1, 100.0, 100.0, 200.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 100.0, 100.0, -100.0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    print_string(0.f, 0.f, "Hello World 0123\nliaw\nliaw\n1234\n", 0.5f, 0.5f, 1.f);
    //my_stbtt_print(0.1f, 0.1f, "k u p");
    
    SwapBuffers(DeviceContext);
#endif
}

internal void
Win32InitializeOpenGL(HWND Window)
{
    HDC DeviceContext = GetDC(Window);
    
    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
    DesiredPixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    DesiredPixelFormat.nVersion = 1;
    DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
    DesiredPixelFormat.cColorBits = 24;
    DesiredPixelFormat.cAlphaBits = 8;
    DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
    
    int PixelFormatIndex = ChoosePixelFormat(DeviceContext,
                                             &DesiredPixelFormat);

    PIXELFORMATDESCRIPTOR SuggestedPixelFormat = {};
    DescribePixelFormat(DeviceContext, PixelFormatIndex,
                        sizeof(PIXELFORMATDESCRIPTOR), &SuggestedPixelFormat);
    SetPixelFormat(DeviceContext, PixelFormatIndex, &SuggestedPixelFormat);

    HGLRC RenderingContext = wglCreateContext(DeviceContext);
    if (RenderingContext)
    {
        if(wglMakeCurrent(DeviceContext, RenderingContext))
        {
            glGenTextures(0, &TextureHandle);
        }
        else
        {
            Trace("FAILED: Make Current OpenGL Context\n");
        }
    }
    else
    {
        Trace("FAILED: Create OpenGL Rendering Context\n");
    }
    
    ReleaseDC(Window, DeviceContext);
}

internal void
Win32ProcessButtonState(game_button_state *Button, bool32 IsDown)
{
    Assert(Button->IsDown != IsDown);
    
    Button->HalfTransitionCount++;
    Button->IsDown = IsDown;
}

internal void
Win32BeginPlayBackRecording(win32_state *State)
{
    HANDLE File = CreateFileA("replay.out", GENERIC_WRITE,
                              0, 0, CREATE_ALWAYS, 0, 0);
    if (File != INVALID_HANDLE_VALUE)
    {
        State->IsRecording = true;
        State->RecordingHandle = File;

        CopyMemory(State->RecordingMemoryArea, State->MemoryArea,
                   State->TotalMemorySize);
    }
}

internal void
Win32RecordInput(win32_state *State, game_input *Input)
{
    DWORD BytesWritten;
    WriteFile(State->RecordingHandle, (void *)Input, sizeof(*Input),
              &BytesWritten, 0);
    Assert(BytesWritten == sizeof(*Input));
}

internal void
Win32StopPlayBackRecording(win32_state *State)
{
    CloseHandle(State->RecordingHandle);
    State->RecordingHandle = 0;
    State->IsRecording = false;
}

internal void
Win32BeginInputPlayBack(win32_state *State)
{
    HANDLE PlayBackHandle = CreateFileA("replay.out", GENERIC_READ,
                                        FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (PlayBackHandle != INVALID_HANDLE_VALUE)
    {
        State->PlayBackHandle = PlayBackHandle;
        State->IsPlaying = true;

        CopyMemory(State->MemoryArea, State->RecordingMemoryArea,
                   State->TotalMemorySize);          
    }
}

internal void
Win32StopInputPlayBack(win32_state *State)
{
    CloseHandle(State->PlayBackHandle);
    State->PlayBackHandle = 0;
    State->IsPlaying = false;
}

internal void
Win32PlayBackInput(win32_state *State, game_input *Input)
{
    DWORD BytesRead;
    ReadFile(State->PlayBackHandle, Input, sizeof(*Input),
             &BytesRead, 0);
    if(BytesRead == 0)
    {
        Win32StopInputPlayBack(State);
        Win32BeginInputPlayBack(State);
        ReadFile(State->RecordingHandle, Input, sizeof(*Input),
                 &BytesRead, 0);
    }
}

internal void
Win32ProcessKeyboardInput(HWND Window, win32_state *State, game_controller_input *Input)
{
    MSG Message = {};
    while (PeekMessage(&Message, Window, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_KEYUP:
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            {
                u32 KeyCode = Message.wParam;
                bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                bool32 WasDown =  ((Message.lParam & (1 << 30)) != 0);
                if (IsDown)
                {
                    if (KeyCode == VK_F4 )
                    {
                        OutputDebugStringA("F4\n");
                        bool AltPressed = ((Message.lParam & (1 << 29)) !=0);
                        if (AltPressed)
                        {
                            OutputDebugStringA("Alt+F4\n");
                            Running = false;
                        }
                    }
                    if (KeyCode == 'Q' )
                    {
                        Running = false;
                    }
                    if (KeyCode == 'F')
                    {
                        Win32TogleFullScreen(Window);
                    }

                }
                if (WasDown != IsDown)
                {
                    if (KeyCode == VK_LEFT || KeyCode == 'A' || KeyCode == 'K')
                    {
                        Win32ProcessButtonState(&Input->Left, IsDown);
                        OutputDebugStringA("Left\n");
                    }
                    else if (KeyCode == VK_RIGHT || KeyCode == 'D' || KeyCode == VK_OEM_1)
                    {
                        Win32ProcessButtonState(&Input->Right, IsDown);
                        OutputDebugStringA("Right\n");
                    }
                    else if (KeyCode == VK_UP || KeyCode == 'W' || KeyCode == 'O')
                    {
                        Win32ProcessButtonState(&Input->Up, IsDown);
                        OutputDebugStringA("Up\n");
                    }
                    else if (KeyCode == VK_DOWN || KeyCode == 'S' || KeyCode == 'L')
                    {
                        Win32ProcessButtonState(&Input->Down, IsDown);
                        OutputDebugStringA("Down\n");
                    }
                    else if (KeyCode == VK_SPACE)
                    {
                        Win32ProcessButtonState(&Input->Space, IsDown);
                        OutputDebugStringA("Space\n");
                    }
#if _DEBUG
                    if (KeyCode == 'R')
                    {
                        if(IsDown)
                        {
                            if(State->IsPlaying)
                            {
                                Win32StopInputPlayBack(State);
                            }
                            else
                            {
                                if(State->IsRecording)
                                {
                                    Win32StopPlayBackRecording(State);
                                    Win32BeginInputPlayBack(State);
                                }
                                else
                                {
                                    Win32BeginPlayBackRecording(State);
                                }
                            }
                        }
                    }
                    if (KeyCode == 'D')
                    {
                        Win32ProcessButtonState(&Input->DoubleSpeed, IsDown);
                    }
#endif                    
                }
            } break;
            default:
            {
                if (Message.message == WM_QUIT)
                {
                    Running = false;
                }
                TranslateMessage(&Message);
                DispatchMessageA(&Message);                
            }
        }
    }
}

LRESULT CALLBACK
WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    switch(Message)
    {
        case WM_ACTIVATEAPP:
        {
            if (WParam == TRUE)
            {
                SetLayeredWindowAttributes(Window, RGB(125,0,0), 255, LWA_ALPHA);
            }
            else
            {
                SetLayeredWindowAttributes(Window, RGB(125,125,125), 150, LWA_ALPHA);
            } 
        } break;
        case WM_KEYUP:
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            Trace("ALERT: Processing key in WindowProc\n");
        } break;
        case WM_SIZE:
        {
            RECT Rectangle = { };
            GetClientRect(Window, &Rectangle);
            ClientWidth = Rectangle.right - Rectangle.left;
            ClientHeight = Rectangle.bottom - Rectangle.top;

            Win32ResizeBitmap(&GlobalBitmap, ClientWidth, ClientHeight);

            Video.Memory = GlobalBitmap.Memory;
            Video.Width  = GlobalBitmap.Width;
            Video.Height = GlobalBitmap.Height;
            Video.Stride = GlobalBitmap.Stride;
            Video.Pitch = GlobalBitmap.Pitch;

            InvalidateRect(Window, 0, false); // NOTE Doesn't work?
            OutputDebugStringA("WM_SIZE\n");

        } break;
        case WM_DESTROY:
        {
            Running = false;
            OutputDebugStringA("WM_DESTROY\n");
        } break;
        case WM_CLOSE:
        {
            Running = false;
            OutputDebugStringA("WM_CLOSE\n");
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);

            Win32UpdateWindow(DeviceContext, &GlobalBitmap, 0, 0, ClientWidth, ClientHeight); // Whole client area paint?

            EndPaint(Window, &Paint);
            OutputDebugStringA("WM_PAINT\n");
        } break;
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

internal void
Win32ResetKeyboardTransitionCount(game_controller_input *Keyboard)
{
    for (int i = 0; i < ArrayCount(Keyboard->Buttons); i++)
    {
        Keyboard->Buttons[i].HalfTransitionCount = 0;
    }
}

struct work_item
{
    void *Data;
    callback_function CallBack;

    std::atomic<u32> Sequence;
};

struct work_queue
{
    std::atomic<u32> ConsumerIndex;
    std::atomic<u32> ProducerIndex;
    std::atomic<u32> FinishedJobs;
    work_item WorkItems[1 << 8];
    u32 Mask;

    HANDLE Semaphore;
};

struct thread_creation_data
{
    u32 Index;
    work_queue *WorkQueue;
};

void
WaitForAllToFinish(work_queue *WorkQueue)
{
    while (WorkQueue->FinishedJobs < WorkQueue->ProducerIndex)
    {
        //spin
    }
}

bool32
RemoveFromQueue(work_queue *WorkQueue, callback_function *CallBack, void **Data)
{
    work_item *Item;
    u32 Mask = WorkQueue->Mask;
    
    size_t Index = WorkQueue->ConsumerIndex.load(std::memory_order_relaxed);
    for (;;)
    {
      Item = &WorkQueue->WorkItems[Index & Mask];
      size_t Sequence = Item->Sequence.load(std::memory_order_acquire);
      intptr_t Diff = (intptr_t)Sequence - (intptr_t)(Index + 1);
      if (Diff == 0)
      {
        if (WorkQueue->ConsumerIndex.compare_exchange_weak
            (Index, Index + 1, std::memory_order_relaxed))
          break;
      }
      else if (Diff < 0)
        return false;
      else
        Index = WorkQueue->ConsumerIndex.load(std::memory_order_relaxed);
    }
    *CallBack = Item->CallBack;
    *Data = Item->Data;
    Item->Sequence.store(Index + Mask + 1, std::memory_order_release);

    return(true);
}

bool32
RemoveAndExecuteTask(work_queue *WorkQueue, u32 ThreadIndex)
{
    void *Data = 0;
    callback_function CallBack = 0;
    if (RemoveFromQueue(WorkQueue, &CallBack, &Data))
    {
        //Trace("Thread %d works\n", ThreadIndex);    

        CallBack(Data);
        ++WorkQueue->FinishedJobs;

        //Trace("Thread %d finished on %d\n", ThreadIndex,
        //WorkQueue->FinishedJobs.load(std::memory_order_relaxed));
        return(true);
    }
    return(false);
}

DWORD CALLBACK
ThreadProc(LPVOID Parameter)
{
    thread_creation_data *ThreadCreationData = (thread_creation_data *)Parameter;
    u32 ThreadIndex = ThreadCreationData->Index;
    work_queue *WorkQueue = ThreadCreationData->WorkQueue;

    Trace("Thread %d created\n", ThreadIndex);

    for(;;)
    {
        if (!RemoveAndExecuteTask(WorkQueue, ThreadIndex))
        {
            //Trace("Thread %d sleeps\n", ThreadIndex);    
            WaitForSingleObject(WorkQueue->Semaphore, INFINITE);
        }
        
    }
}

bool32
AddToQueue(work_queue *WorkQueue, callback_function CallBack, void *Data)
{
    work_item *Item;
    u32 Mask = WorkQueue->Mask;
    std::atomic<u32> *ProducerIndex = &WorkQueue->ProducerIndex;
    
    size_t Index = ProducerIndex->load(std::memory_order_relaxed);
    for (;;)
    {
        Item = &WorkQueue->WorkItems[Index & Mask];
        size_t Sequence = Item->Sequence.load(std::memory_order_acquire);
        intptr_t Diff = (intptr_t)Sequence - (intptr_t)Index;
        if (Diff == 0)
        {
            if (ProducerIndex->compare_exchange_weak
                (Index, Index + 1, std::memory_order_relaxed))
                break;
        }
        else if (Diff < 0)
            return(false);
        else
            Index = ProducerIndex->load(std::memory_order_relaxed);
    }
    Item->CallBack = CallBack;
    Item->Data = Data;
    Item->Sequence.store(Index + 1, std::memory_order_release);

    ReleaseSemaphore(WorkQueue->Semaphore, 1, 0);
    return(true);
}

int WINAPI
WinMain(HINSTANCE Instance, HINSTANCE _Ignore, LPSTR CommandLine, int ShowCommand)
{
    DWORD ThreadId;
    
    thread_creation_data Threads[2];
    work_queue WorkQueue = {};
    for (u32 Index = 0; Index < ArrayCount(WorkQueue.WorkItems); ++Index)
    {
        WorkQueue.WorkItems[Index].Sequence.store(Index, std::memory_order_release);
    }
    
    HANDLE Semaphore = CreateSemaphore(0, 0, ArrayCount(Threads), 0);
    WorkQueue.Semaphore = Semaphore;
    WorkQueue.Mask = ArrayCount(WorkQueue.WorkItems) - 1;
    
    for (u32 Index = 0; Index < ArrayCount(Threads); ++Index)
    {
        Threads[Index].Index = Index;
        Threads[Index].WorkQueue = &WorkQueue;
        LPVOID Parameter = (LPVOID) (Threads + Index);
        HANDLE Thread = CreateThread(0, 0, ThreadProc, Parameter, 0, &ThreadId);
        CloseHandle(Thread);    
    }

    
    WNDCLASS WindowClass = {};
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "Learning C Windows Class!";
    WindowClass.lpfnWndProc = WindowProc;

    RegisterClassA(&WindowClass);

    u32 WindowFlags = WS_EX_TOPMOST | WS_EX_LAYERED;
    if (IsDebuggerPresent())
    {
        WindowFlags = 0;
    }
    HWND Window = CreateWindowExA(
        WindowFlags,
        WindowClass.lpszClassName,
        "Learning C Hello World",
        WS_OVERLAPPEDWINDOW,
        750, 370,
        400, 400,
        0, 0,
        Instance,
        0);
    ShowWindow(Window, ShowCommand);

    LARGE_INTEGER PerformanceCounterFrequency;
    QueryPerformanceFrequency(&PerformanceCounterFrequency);
    LARGE_INTEGER LastCounter;
    QueryPerformanceCounter(&LastCounter);
    LONGLONG CountsPerFrame = PerformanceCounterFrequency.QuadPart / MonitorHz;

    if (timeBeginPeriod(1) != TIMERR_NOERROR)
    {
        Trace("FAILED: timeBeginPeriod(1)");
    }

    if (Window)
    {
        Win32InitializeOpenGL(Window);
        
        win32_state Win32State = {};
        //Win32GetExecutableDirectory(); //TODO
        
        win32_sound_output SoundOutput = {};
        SoundOutput.SamplesPerSecond = 48000;
        SoundOutput.Channels = 2;
        SoundOutput.BytesPerSample = sizeof(int16) * SoundOutput.Channels;
        SoundOutput.BufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
        Win32InitDirectSound(Window, &SoundOutput);
        //TODO CLEAR BUFFER
        SoundOutput.Buffer->Play(0, 0, DSBPLAY_LOOPING);

        game_sound_output_buffer SoundData = {};
        SoundData.Buffer = (int16 *)VirtualAlloc(0, SoundOutput.BufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        game_memory Memory = {};

        Memory.AddToQueue = AddToQueue;
        Memory.RemoveAndExecuteTask = RemoveAndExecuteTask;
        Memory.WorkQueue = &WorkQueue;
        Memory.WaitForAllToFinish = WaitForAllToFinish;
        
        Memory.ReadFileToMemory = ReadFileToMemory;
        Memory.PersistentStorageSize = MegaBytes(64);
        Memory.TransientStorageSize = MegaBytes(64);
        
        Win32State.TotalMemorySize = Memory.PersistentStorageSize + Memory.TransientStorageSize;
        LPVOID BaseMemoryAddress = 0;
        Win32State.MemoryArea = VirtualAlloc(BaseMemoryAddress, Win32State.TotalMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        Memory.PersistentStorage = Win32State.MemoryArea;
        Memory.TransientStorage = (uint8 *)Win32State.MemoryArea + Memory.PersistentStorageSize;

#if _DEBUG
        Win32State.RecordingMemoryArea = VirtualAlloc(0, Win32State.TotalMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#endif
        if (Win32State.MemoryArea)
        {
#if _DEBUG
            win32_debug_sound_markers Markers[15] = {};
            u32 Count = ArrayCount(Markers);
            u32 MarkerIndex = 0;
#endif
        
            LARGE_INTEGER Counter;

            char *GameLibraryFileName = "game.dll";
            win32_game_code GameCode = Win32LoadGameCode(GameLibraryFileName);
            game_input GameInput = {};
            GameInput.dtForFrame = 1.0f/MonitorHz;

            while (Running)
            {
#if _DEBUG            
                FILETIME LastWriteTime = Win32LastWriteTime(GameLibraryFileName);
                if (CompareFileTime(&LastWriteTime, &GameCode.LastWriteTime))
                {
                    Win32UnloadGameCode(&GameCode);
                    GameCode = Win32LoadGameCode(GameLibraryFileName);
                }
#endif
                Win32ResetKeyboardTransitionCount(&GameInput.Keyboard);
                Win32ProcessKeyboardInput(Window, &Win32State, &GameInput.Keyboard);
#if _DEBUG          
                if (Win32State.IsRecording)
                {
                    Win32RecordInput(&Win32State, &GameInput);
                }

                if (Win32State.IsPlaying)
                {
                    Win32PlayBackInput(&Win32State, &GameInput);
                }
#endif            
                DWORD PlayCursor;
                DWORD WriteCursor;
                SoundOutput.Buffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

                DWORD ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.BufferSize;

                DWORD Bytes = 0;
                if (ByteToLock > PlayCursor)
                {
                    Bytes = SoundOutput.BufferSize - ByteToLock;
                    Bytes += PlayCursor;
                }
                else
                {
                    Bytes = PlayCursor - ByteToLock;
                }

                SoundData.Samples = Bytes / SoundOutput.BytesPerSample;

                GameCode.GameEngine(&Memory, &GameInput, &SoundData, &Video);

                
                
                for (u32 Index = 0; Index < ArrayCount(Memory.DebugCycleCounters); ++Index)
                {
                    debug_cycle_counter *Cycles = Memory.DebugCycleCounters + Index;
                    if (Cycles->Hits > 0)
                    {
                        Trace("%s: %I64uc %uh %I64u c/h\n",
                              Cycles->Name, Cycles->Cycles,
                              Cycles->Hits,
                              Cycles->Cycles / (u64)Cycles->Hits);
                        Cycles->Cycles = 0;
                        Cycles->Hits = 0;
                    }
                }
                
                Win32FillSoundBuffer(&SoundOutput, &SoundData, ByteToLock, Bytes);

#if 0
                Markers[MarkerIndex++ % Count] = {PlayCursor, WriteCursor};
                Win32DebugSoundSync(Markers, Count, SoundOutput.BufferSize);
#endif
                QueryPerformanceCounter(&Counter);
                
                LONGLONG TargetCounterValue = LastCounter.QuadPart + CountsPerFrame;
                if (Counter.QuadPart < TargetCounterValue)
                {
                    DWORD TimeToSleep = (DWORD)(1000 * (TargetCounterValue - Counter.QuadPart) / (float)PerformanceCounterFrequency.QuadPart);
                    Sleep(TimeToSleep);
                    QueryPerformanceCounter(&Counter);
                    while (Counter.QuadPart < TargetCounterValue)
                    {
                        QueryPerformanceCounter(&Counter);
                    }
                }
                else
                {
                    //Trace("ALERT: Frame skipped!\n");
                }
                
                QueryPerformanceCounter(&Counter);
                HDC DeviceContext = GetDC(Window);
                Win32UpdateWindow(DeviceContext, &GlobalBitmap, 0, 0, ClientWidth, ClientHeight);
                ReleaseDC(Window, DeviceContext);

#if 0
                Trace("PC: %u, WC: %u, BT: %u, Bytes: %u, FPS: %f\n",
                      PlayCursor, WriteCursor, ByteToLock, Bytes,
                      (float)PerformanceCounterFrequency.QuadPart / (float)(Counter.QuadPart - LastCounter.QuadPart));
#endif
#if 0

                float Delta = (float)(Counter.QuadPart - LastCounter.QuadPart);
                float FPS = (float)PerformanceCounterFrequency.QuadPart / Delta;
                float MSPF = 1000.0f * Delta / (float)PerformanceCounterFrequency.QuadPart;
                Trace("FPS: %f, MS/Frame: %f\n", FPS, MSPF );
#endif
                LastCounter = Counter;
            }
        }
    }
    return(0);
}

