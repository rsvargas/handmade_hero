/*
    TODO:  THIS IS NOT A FINAL PLATFORM LAYER

    - Saved game locations
    - Getting a handle to our own executable file
    - Asset loading path
    - Threading (lauche a thread)
    - Raw Input (support for multiple keyboards)
    - Sleep/TimeBeginPeriod
    - ClipCursor() (for multimonitor support)
    - Fullscreen support
    - WM_SETCURDOR (control cursor visibility)
    - QueryCancelAutoplay
    - WM_ACTIVATEAPP (for when we are not the active application)
    - Blt speed improvements (BitBlt)
    - Hardware acceleration (OpenDL or Direct3D or BOTH??)
    - GetKeyboardLayout (for French keyboards, internation WASD support)

    Just a partial list of stuff.
    */

//TODO: Implement sine ourselves

#include "handmade.h"
 
#include <Windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <stdio.h>
#include <malloc.h>

#include "win32_handmade.h"

//TODO: global for now
global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;


//NOTE: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name( DWORD /*dwUserIndex*/, XINPUT_STATE* /*pState*/)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;

//NOTE: XINputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name( DWORD /*dwUserIndex*/, XINPUT_VIBRATION* /*pVibration*/ )
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

//NOTE: this works because XInput was included before.
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    VirtualFree(Memory, 0, MEM_RELEASE);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool32 Result = 0;

    HANDLE FileHandle = CreateFileA(Filename,
        GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {
            Result = (BytesWritten == MemorySize);
        }
        else
        {
            //TODO: logging
        }
        CloseHandle(FileHandle);
    }
    else
    {
        //TODO Logging
    }
    return Result;
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {};
    HANDLE FileHandle = CreateFileA(Filename,
        GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            Result.ContentsSize = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, Result.ContentsSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (Result.Contents)
            {
                DWORD BytesRead;
                if (ReadFile(FileHandle, Result.Contents, Result.ContentsSize, &BytesRead, 0) &&
                    (Result.ContentsSize== BytesRead))
                {

                }
                else
                {
                    DEBUGPlatformFreeFileMemory(Result.Contents);
                    Result.Contents = 0;
                }

            }
            else
            {
                //TODO: Logging
            }
        }
        else
        {
            //TODO: Logging
        }
        CloseHandle(FileHandle);
    }
    else
    {
        //TODO: Logging
    }
    return Result;
}

inline FILETIME Win32GetLastWriteTime(const char* Filename)
{
    FILETIME Result = {};
    WIN32_FIND_DATAA FileData;
    HANDLE FindHandle = FindFirstFileA(Filename, &FileData);
    if (FindHandle)
    {
        Result = FileData.ftLastWriteTime;
        FindClose(FindHandle);
    }
    return Result;
}

internal win32_game_code Win32LoadGameCode(char* SourceDLLName, char* TempDLLName)
{
    win32_game_code Result = {};

    Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

    CopyFileA(SourceDLLName, TempDLLName, FALSE);
    Result.GameCodeDLL = LoadLibraryA(TempDLLName);
    if (Result.GameCodeDLL)
    {
        Result.UpdateAndRender = (game_update_and_render*)
            GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
        Result.GetSoundSamples = (game_get_sound_samples*)
            GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");

        Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
    }

    if (!Result.IsValid)
    {
        Result.UpdateAndRender = GameUpdateAndRenderStub;
        Result.GetSoundSamples = GameGetSoundSamplesStub;
    }
    return Result;
}

internal void Win32UnloadGameCode(win32_game_code* GameCode)
{
    if (GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
    }

    GameCode->IsValid = false;
    GameCode->UpdateAndRender = GameUpdateAndRenderStub;
    GameCode->GetSoundSamples = GameGetSoundSamplesStub;

}

internal void Win32LoadXInput()
{
    //TODO: test in several OSs
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    //NOTE: Load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    //The primary buffer will only be used internally by direct sound
    //we'll only be writing audio to the secondary buffer.

    if (DSoundLibrary)
    {
        direct_sound_create *DirectSoundCreate_ = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        LPDIRECTSOUND DSound;
        if (DirectSoundCreate_ && SUCCEEDED(DirectSoundCreate_(0, &DSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            //NOTE: Get a DirectSound object! - cooperative
            if (SUCCEEDED(DSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescriptor = {};
                BufferDescriptor.dwSize = sizeof(BufferDescriptor);
                BufferDescriptor.dwFlags = DSBCAPS_PRIMARYBUFFER;

                //NOTE: Create a primary buffer
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(DSound->CreateSoundBuffer(&BufferDescriptor, &PrimaryBuffer, 0)))
                {
                    HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
                    if (SUCCEEDED(Error))
                    {
                        //NOTe: We have finally set the format
                        OutputDebugStringA("We've set format on primary buffer\n");
                    }
                    else
                    {
                        //TODO: Diagnostic
                    }
                }
                else
                {
                    //TODO: Diagnostic
                }
            }
            else
            {
                //TODO: Diagnostic
            }

            //NOTE: Create a secondary buffer
            DSBUFFERDESC BufferDescriptor = {};
            BufferDescriptor.dwSize = sizeof(BufferDescriptor);
            BufferDescriptor.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
            BufferDescriptor.dwBufferBytes = BufferSize;
            BufferDescriptor.lpwfxFormat = &WaveFormat;
            HRESULT Error = DSound->CreateSoundBuffer(&BufferDescriptor, &GlobalSecondaryBuffer, 0);
            if (SUCCEEDED(Error))
            {
                //NOTE: Start it playing!
                OutputDebugStringA("Created secondary buffer\n");
            }
            else
            {
            }


        }
    }
}

internal void Win32ClearSoundBuffer(win32_sound_output &Output)
{
    VOID* Region1;
    DWORD Region1Size;
    VOID* Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, Output.SecondaryBufferSize,
        &Region1, &Region1Size,
        &Region2, &Region2Size,
        0)))
    {
        uint8 * DestSample = (uint8*)Region1;
        for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }
        DestSample = (uint8*)Region2;
        for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }

}


internal void Win32FillSoundBuffer(win32_sound_output &Output, DWORD ByteToLock, 
    DWORD BytesToWrite, const game_sound_output_buffer &SourceBuffer)
{
    //TODO: More Test!
    VOID* Region1;
    DWORD Region1Size;
    VOID* Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
        &Region1, &Region1Size,
        &Region2, &Region2Size,
        0)))
    {
        //TODO: assert that region sizes are valid

        //TODO: Collapse these two loops
        DWORD Region1SampleCount = Region1Size / Output.BytesPerSample;
        int16* DestSample = (int16*)Region1;
        int16* SourceSample = SourceBuffer.Samples;

        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++Output.RunningSampleIndex;
        }

        DestSample = (int16*)Region2;
        DWORD Region2SampleCount = Region2Size / Output.BytesPerSample;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++Output.RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);

    }
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
    game_button_state* OldState,  DWORD ButtonBit,  game_button_state* NewState)
{
    NewState->EndedDown = (XInputButtonState & ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown == NewState->EndedDown) ? 1 : 0;
}

internal real32  Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
    real32 Result = 0;
    if (Value < -DeadZoneThreshold)
    {
        Result = (real32)((Value + DeadZoneThreshold) / (32768.0f + DeadZoneThreshold));
    }
    else if (Value > DeadZoneThreshold)
    {
        Result = (real32)((Value - DeadZoneThreshold) / (32767.0f + DeadZoneThreshold));
    }
    return Result;
}

internal void Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
    ASSERT(NewState->EndedDown != IsDown);  
    NewState->EndedDown = IsDown;
    ++(NewState->HalfTransitionCount);
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}


internal void Win32ResizeDIBSection(win32_offscreen_buffer& Buffer, int Width, int Height)
{
    //TODO: bulletproof this!
    // maybe dont free first, free after. then free first if that fails
    if (Buffer.Memory)
    {
        VirtualFree(Buffer.Memory, 0, MEM_RELEASE);
    }

    Buffer.Width = Width;
    Buffer.Height = Height;
    int BytesPerPixel = 4;
    Buffer.BytesPerPixel = BytesPerPixel;

    Buffer.Info.bmiHeader.biSize = sizeof(Buffer.Info.bmiHeader);
    Buffer.Info.bmiHeader.biWidth = Buffer.Width;
    Buffer.Info.bmiHeader.biHeight = -Buffer.Height; //negative is top-down, positive is bottom-up
    Buffer.Info.bmiHeader.biPlanes = 1;
    Buffer.Info.bmiHeader.biBitCount = 32;
    Buffer.Info.bmiHeader.biCompression = BI_RGB;

    //Note: casey thanks Chris Hecker!!
    int BitmapMemorySize = (Buffer.Width * Buffer.Height) * BytesPerPixel;
    Buffer.Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    Buffer.Pitch = Width * BytesPerPixel;

    //TODO: probably celar this to black
}

internal void Win32DisplayBufferInWindow(const win32_offscreen_buffer& Buffer,
    HDC DeviceContext, int WindowWidth, int WindowHeigth)
{
    //TODO: aspect ratio coeection
    //TODO: Play with strech modes
    StretchDIBits(DeviceContext,
        0, 0, WindowWidth, WindowHeigth,//X, Y, Width, Height,
        0, 0, Buffer.Width, Buffer.Height,//X, Y, Width, Height,
        Buffer.Memory,
        &Buffer.Info,
        DIB_RGB_COLORS,
        SRCCOPY);
}

LRESULT CALLBACK MainWindowCallback(HWND Window, UINT Message,
    WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
    case WM_SIZE:
    {
        //OutputDebugStringA("WM_SIZE\n");
    } break;

    case WM_CLOSE:
    {
        //TODO: handle this witha a message to the user?
        GlobalRunning = false;
    } break;

    case WM_ACTIVATEAPP:
    {
        OutputDebugStringA("WM_ACTIVATEAAPP\n");
    } break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        ASSERT(!"Keyboard input came in through a non-diaptch message");
    } break;

    case WM_DESTROY:
    {
        //TODO: handle this with a error - recreate window?
        GlobalRunning = false;
    } break;

    case WM_PAINT:
    {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(Window, &Paint);
        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
        Win32DisplayBufferInWindow(GlobalBackbuffer, DeviceContext,
            Dimension.Width, Dimension.Height);
        EndPaint(Window, &Paint);

    } break;

    default:
    {
        //		OutputDebugStringA("default\n");
        Result = DefWindowProcA(Window, Message, WParam, LParam);
    } break;
    }

    return Result;
}

internal void Win32ProcessPendingMessages(game_controller_input* KeyboardController)
{
    MSG Message;
    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch (Message.message)
        {
        case WM_QUIT:
            GlobalRunning = false;
            break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = (uint32)Message.wParam;
            bool32 WasDown = (Message.lParam & (1 << 30)) != 0;
            bool32 IsDown = (Message.lParam & (1 << 31)) == 0;

            if (WasDown != IsDown)
            {
                if (VKCode == 'W')
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
                }
                else if (VKCode == 'A')
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                }
                else if (VKCode == 'S')
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
                }
                else if (VKCode == 'D')
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
                }
                else if (VKCode == 'Q')
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
                }
                else if (VKCode == 'E')
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
                }
                else if (VKCode == VK_UP)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
                }
                else if (VKCode == VK_LEFT)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
                }
                else if (VKCode == VK_DOWN)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
                }
                else if (VKCode == VK_RIGHT)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
                }
                else if (VKCode == VK_ESCAPE)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                }
                else if (VKCode == VK_SPACE)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                }
#if HANDMADE_INTERNAL
                else if (VKCode == 'P')
                {
                    if (IsDown)
                    {
                        GlobalPause = !GlobalPause;
                    }
                }
#endif
            }

            bool32 AltKeyWasDown = (Message.lParam & (1 << 29)) != 0;
            if ((VKCode == VK_F4) && AltKeyWasDown)
            {
                GlobalRunning = false;
            }

        } break;
        default:
            TranslateMessage(&Message);
            DispatchMessageA(&Message);
            break;
        }

    }

}


inline LARGE_INTEGER Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline real32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result = ((real32)End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCountFrequency;
    return Result;
}

internal void Win32DebugDrawVertical(win32_offscreen_buffer *BackBuffer,
    int X, int Top, int Bottom, uint32 Color)
{
    if (Top <= 0)
    {
        Top = 0;
    }

    if (Bottom > BackBuffer->Height)
    {
        Bottom = BackBuffer->Height;
    }

    if ((X >= 0) && X < BackBuffer->Width)
    {
        uint8* Pixel = ((uint8*)BackBuffer->Memory +
        X* BackBuffer->BytesPerPixel +
        Top * BackBuffer->Pitch);

        for (int Y = Top; Y < Bottom; ++Y)
        {
            *(uint32*)Pixel = Color;
            Pixel += BackBuffer->Pitch;
        }
    }
}

inline void Win32DrawSoundBufferMarker(win32_offscreen_buffer *Backbuffer,
    win32_sound_output * SoundOutput, real32 C, int PadX, int Top, int Bottom,
    DWORD Value, uint32 Color)
{
    //ASSERT(Value < SoundOutput->SecondaryBufferSize);
    int X = PadX + (int)(C * (real32)Value);
    Win32DebugDrawVertical(Backbuffer, X, Top, Bottom, Color);
}


internal void Win32DebugSyncDisplay(win32_offscreen_buffer* Backbuffer,
    int MarkerCount, win32_debug_time_marker* Markers,
    int CurrentMarkerIndex,
    win32_sound_output* SoundOutput, real32 TargetSecondsPerFrame)
{
    int PadX = 16;
    int PadY = 16;

    int LineHeight = 64;

    real32 C = (real32)(Backbuffer->Width - 2*PadX) / (real32)SoundOutput->SecondaryBufferSize;
    for (int MarkerIndex = 0;
        MarkerIndex < MarkerCount;
        ++MarkerIndex)
    {
        win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
        ASSERT(ThisMarker->OutputPlayCursor  < SoundOutput->SecondaryBufferSize);
        ASSERT(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
        ASSERT(ThisMarker->OutputLocation    < SoundOutput->SecondaryBufferSize);
        ASSERT(ThisMarker->OutputByteCount   < SoundOutput->SecondaryBufferSize);
        ASSERT(ThisMarker->FlipPlayCursor    < SoundOutput->SecondaryBufferSize);
        ASSERT(ThisMarker->FlipWriteCursor   < SoundOutput->SecondaryBufferSize);
        //ASSERT(ThisMarker->ExpectedFlipPlayCursor < SoundOutput->SecondaryBufferSize);//this is an added value

        DWORD PlayColor = 0xFFFFFFFF;
        DWORD WriteColor = 0xFFFF0000;
        DWORD ExpectedFlipColor = 0xFF00FF00;
        DWORD PlayWindowColor = 0xFFFF00FF;

        int Top = PadY;
        int Bottom = PadY +LineHeight;

        if (MarkerIndex == CurrentMarkerIndex)
        {
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;

            int FirstTop = Top;
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom,
                ThisMarker->OutputPlayCursor, PlayColor);
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom,
                ThisMarker->OutputWriteCursor, WriteColor);
            
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;

            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom,
                ThisMarker->OutputLocation, PlayColor);
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom,
                ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);
            
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;
 
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, FirstTop, Bottom,
                ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);

            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom + 10,
                ThisMarker->FlipPlayCursor - (240 * SoundOutput->BytesPerSample), PlayWindowColor);
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom + 10,
                ThisMarker->FlipPlayCursor + (240 * SoundOutput->BytesPerSample), PlayWindowColor);
        }
        Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom,
            ThisMarker->FlipPlayCursor, PlayColor);
        Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom,
            ThisMarker->FlipWriteCursor, WriteColor);
    }

}

internal void CatStrings(
    size_t SourceACount, char* SourceA,
    size_t SourceBCount, char* SourceB,
    size_t DestCount, char*Dest)
{
    ASSERT(SourceACount + SourceBCount < DestCount)
    for (int Index = 0; Index < SourceACount; ++Index)
    {
        *Dest++ = *SourceA++;
    }
    for (int Index = 0; Index < SourceBCount; ++Index)
    {
        *Dest++ = *SourceB++;
    }
    *Dest++ = 0;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE /*PrevInstance*/,
    LPSTR /*CommandLine*/, int /*ShowCode*/)
{
    char EXEFileName[MAX_PATH];
    DWORD SizeOfFilename = GetModuleFileNameA(0, EXEFileName, sizeof(EXEFileName));
    char *OnePastLastSlash = EXEFileName;
    for (char *Scan = EXEFileName; *Scan; ++Scan)
    {
        if (*Scan == '\\')
        {
            OnePastLastSlash = Scan + 1;
        }
    }

    char SourceGameCodeDLLFilename[] = "handmade.dll";
    char SourceGameCodeDLLFullPath[MAX_PATH];
    CatStrings(OnePastLastSlash - EXEFileName, EXEFileName,
        sizeof(SourceGameCodeDLLFilename), SourceGameCodeDLLFilename,
        sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);


    char TempGameCodeDLLFilename[] = "handmade_temp.dll";
    char TempGameCodeDLLFullPath[MAX_PATH];
    CatStrings(OnePastLastSlash - EXEFileName, EXEFileName,
        sizeof(TempGameCodeDLLFilename), TempGameCodeDLLFilename,
        sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);



    LARGE_INTEGER PerfCounterFrequencyResult;
    QueryPerformanceFrequency(&PerfCounterFrequencyResult);
    GlobalPerfCountFrequency = PerfCounterFrequencyResult.QuadPart;

    char Msg[256];
    TIMECAPS caps;
    timeGetDevCaps(&caps, sizeof(caps));
    sprintf_s(Msg, "CAPS: min=%d, max=%d\n", caps.wPeriodMin, caps.wPeriodMax);
    OutputDebugStringA(Msg);

    UINT DesiredSchedulerMS = 1;
    bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    Win32LoadXInput();
    WNDCLASSA WindowClass = {}; //zero initialize

    Win32ResizeDIBSection(GlobalBackbuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; //always redraw when resize
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

#define MonitorRefreshHz (60)
#define GameUpdateHz (MonitorRefreshHz / 2)
    real32 TargetSecondsPerFrame = (1.0f / (real32)(GameUpdateHz));

    if (RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(
            0, //DWORD dwExStyle,
            WindowClass.lpszClassName, //LPCWSTR lpClassName,
            "Handmade Hero", //LPCWSTR lpWindowName,
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,//DWORD dwStyle,
            CW_USEDEFAULT, //int X,
            CW_USEDEFAULT, //int Y,
            CW_USEDEFAULT, //int nWidth,
            CW_USEDEFAULT, //int nHeight,
            0,//HWND hWndParent,
            0, //HMENU hMenu,
            Instance, //HINSTANCE hInstance,
            0//LPVOID lpParam
            );

        if (Window)
        {
            // NOTE: Since we specified CS_OWNDC, we can just
            // get one device context and user it forever because we
            // are not sharing it with anyone
            HDC DeviceContext = GetDC(Window);

            //NOTE : graphics test
            int XOffset = 0;
            int YOffset = 0;

            win32_sound_output SoundOutput = {};

            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.BytesPerSample = sizeof(int16) * 2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
            SoundOutput.LatencySampleCount = 3*(SoundOutput.SamplesPerSecond / GameUpdateHz);
            //TODO: actually compute this variable and see what the lowest
            //reasonable value is.
            //NOTE(vargas): divide by 3 didn't really worked for me =/
            SoundOutput.SafetyBytes = (SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample / GameUpdateHz);
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearSoundBuffer(SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            GlobalRunning = true;

#if 0
            //NOTE: Tests the play/write cursor update frequency
            //my machine, it was 480 samples
            while (GlobalRunning)
            {
                DWORD PlayCursor;
                DWORD WriteCursor;
                GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
                sprintf_s(Msg, "PC:%u WC:%u\n", PlayCursor, WriteCursor);
                OutputDebugStringA(Msg);
            }
#endif

            int16* Samples = (int16*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize,
                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL == 1
            LPVOID BaseAddress = (LPVOID)TERABYTES(2);
#else
            LPVOID BaseAddress = 0;
#endif
 

            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = MEGABYTES(64);
            GameMemory.TransientStorageSize = GIGABYTES(1);
            GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
            GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

            uint64 TotalSize = GameMemory.PermanentStorageSize + 
                GameMemory.TransientStorageSize;
            GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize,
                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            GameMemory.TransientStorage = ((uint8*)GameMemory.PermanentStorage) + 
                GameMemory.PermanentStorageSize;

            if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
            {
                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                LARGE_INTEGER LastCounter = Win32GetWallClock();;
                LARGE_INTEGER FlipWallClock = LastCounter;

                int DebugTimeMarkersIndex = 0;
                win32_debug_time_marker DebugTimeMarkers[GameUpdateHz / 2] = {};

                //TODO: Handle startup specially
                DWORD AudioLatencyBytes = 0;
                real32 AudioLatencySeconds = 0.0f;
                bool32 SoundIsValid = false;

                int64 LastCycleCount = __rdtsc();

                const char *SourceDLLName = "handmade.dll";
                win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
                uint64 LoadCounter = 0;

                while (GlobalRunning)
                {
                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceDLLName);
                    
                    if (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                    {
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
                        LoadCounter = 0;
                    }

                    game_controller_input* OldKeyboardController = GetController(OldInput,0);
                    game_controller_input* NewKeyboardController = GetController(NewInput,0);
                    //TODO: We cant zero, if we do, things go south!
                    *NewKeyboardController = {};
                    NewKeyboardController->IsConnected = true;
                    for (int ButtonIndex = 0;
                        ButtonIndex < ARRAY_COUNT(NewKeyboardController->Buttons);
                        ++ButtonIndex)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }

                    Win32ProcessPendingMessages(NewKeyboardController);

                    if (!GlobalPause)
                    {

                        //TODO: Need to not poll disconnected controllers to avoid xinput frame rate 
                        // hit on older libraries
                        //TODO: should we poll this more fdrequently?
                        DWORD MaxControllerCount = XUSER_MAX_COUNT;
                        if (MaxControllerCount > (ARRAY_COUNT(NewInput->Controllers) - 1))
                        {
                            MaxControllerCount = (ARRAY_COUNT(NewInput->Controllers) - 1);
                        }
                        for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
                        {
                            DWORD OurControllerIndex = ControllerIndex + 1;
                            game_controller_input* OldController = GetController(OldInput, OurControllerIndex);
                            game_controller_input* NewController = GetController(NewInput, OurControllerIndex);
                            XINPUT_STATE ControllerState;
                            if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                            {
                                NewController->IsConnected = true;
                                //NOTE: controller is plugged in
                                //TODO: see if controlerState.dwPacketNumber increments too rapidly
                                XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                                NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX,
                                    XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY,
                                    XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                                if (NewController->StickAverageX != 0.0f ||
                                    NewController->StickAverageY != 0.0f)
                                {
                                    NewController->IsAnalog = true;
                                }

                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                                {
                                    NewController->StickAverageY = 1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                                {
                                    NewController->StickAverageY = -1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                                {
                                    NewController->StickAverageX = -1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                                {
                                    NewController->StickAverageX = 1.0f;
                                    NewController->IsAnalog = false;
                                }

                                real32 Threshold = 0.5f;
                                Win32ProcessXInputDigitalButton(
                                    (NewController->StickAverageX < -Threshold) ? 1 : 0,
                                    &OldController->MoveLeft, 1, &NewController->MoveLeft);
                                Win32ProcessXInputDigitalButton(
                                    (NewController->StickAverageX > Threshold) ? 1 : 0,
                                    &OldController->MoveRight, 1, &NewController->MoveRight);
                                Win32ProcessXInputDigitalButton(
                                    (NewController->StickAverageY < -Threshold) ? 1 : 0,
                                    &OldController->MoveDown, 1, &NewController->MoveDown);
                                Win32ProcessXInputDigitalButton(
                                    (NewController->StickAverageY > Threshold) ? 1 : 0,
                                    &OldController->MoveUp, 1, &NewController->MoveUp);


                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionDown,
                                    XINPUT_GAMEPAD_A, &NewController->ActionDown);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionRight,
                                    XINPUT_GAMEPAD_B, &NewController->ActionRight);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionLeft,
                                    XINPUT_GAMEPAD_X, &NewController->ActionLeft);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionUp,
                                    XINPUT_GAMEPAD_Y, &NewController->ActionUp);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder,
                                    XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder,
                                    XINPUT_GAMEPAD_RIGHT_SHOULDER,&NewController->RightShoulder);

                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Start,
                                    XINPUT_GAMEPAD_START, &NewController->Start);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Back,
                                    XINPUT_GAMEPAD_BACK, &NewController->Back);

                            }
                            else
                            {
                                //NOTE: the controller is not available
                                NewController->IsConnected = false;
                            }
                        }


                        game_offscreen_buffer Buffer = {};
                        Buffer.Memory = GlobalBackbuffer.Memory;
                        Buffer.Width = GlobalBackbuffer.Width;
                        Buffer.Height = GlobalBackbuffer.Height;
                        Buffer.Pitch = GlobalBackbuffer.Pitch;
                        Game.UpdateAndRender(&GameMemory, Buffer, *NewInput);

                        LARGE_INTEGER AudioWallClock = Win32GetWallClock();
                        real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

                        DWORD PlayCursor;
                        DWORD WriteCursor;
                        if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                        {
                            /* NOTE:
                            How sound output computaion works.

                            We define a safety value that is the number of samples
                            we think out game update loop may vary by (let's say
                            up to 2ms).

                            When we wake up to write audio, we will look and see what the play
                            cursor position is and we will forecast ahead where we whink the
                            play cursor will be on the next frame boundary.

                            We will then look to see if the write cursor is before that by at
                            least our safety value. If it is, the target fill position is that
                            frame boundary plus one frame. This gives us perfect audio sync in the
                            case of a card that has low enough latency.

                            If the write cursor is _after_ that safety margin, then we assume
                            we can never sync the audio perfectly, so we  will write one frame's
                            worth of audio plus the safety margin's worth of guard samples.
                            */
                            if (!SoundIsValid)
                            {
                                SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
                                SoundIsValid = true;
                            }
                            DWORD ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) %
                                SoundOutput.SecondaryBufferSize);

                            DWORD ExpectedSoundBytesPerFrame =
                                (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz;

                            real32 SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
                            DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip / TargetSecondsPerFrame)  *
                                (real32)ExpectedSoundBytesPerFrame);

                            DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesPerFrame;

                            DWORD SafeWriteCursor = WriteCursor;
                            if (SafeWriteCursor < PlayCursor)
                            {
                                SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            ASSERT(SafeWriteCursor >= PlayCursor);
                            SafeWriteCursor += SoundOutput.SafetyBytes;

                            bool32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

                            DWORD TargetCursor = 0;
                            if (AudioCardIsLowLatency)
                            {
                                TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
                            }
                            else
                            {
                                TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame +
                                    SoundOutput.SafetyBytes);
                            }
                            TargetCursor %= SoundOutput.SecondaryBufferSize;

                            DWORD BytesToWrite = 0;
                            if (ByteToLock > TargetCursor)
                            {
                                BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
                                BytesToWrite += TargetCursor;
                            }
                            else
                            {
                                BytesToWrite = TargetCursor - ByteToLock;
                            }
                            game_sound_output_buffer SoundBuffer = {};
                            SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                            SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                            SoundBuffer.Samples = Samples;
                            Game.GetSoundSamples(&GameMemory, SoundBuffer);
#if HANDMADE_INTERNAL
                            win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkersIndex];
                            Marker->OutputPlayCursor = PlayCursor;
                            Marker->OutputWriteCursor = WriteCursor;
                            Marker->OutputLocation = ByteToLock;
                            Marker->OutputByteCount = BytesToWrite;
                            Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

                            DWORD UnwrappedWriteCursor = WriteCursor;
                            if (UnwrappedWriteCursor < PlayCursor)
                            {
                                UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            AudioLatencyBytes = (UnwrappedWriteCursor - PlayCursor);
                            AudioLatencySeconds =
                                (((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample) /
                                (real32)SoundOutput.SamplesPerSecond);
                            // Samplespersecond already account for the two channels

                            sprintf_s(Msg, "BTL:%u TC:%u BTW:%u PC:%u WC:%u DELTA:%u (%fs)\n",
                                ByteToLock, TargetCursor, BytesToWrite, PlayCursor,
                                WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
                            OutputDebugStringA(Msg);
#endif
                            Win32FillSoundBuffer(SoundOutput, ByteToLock, BytesToWrite, SoundBuffer);
                        }
                        else
                        {
                            SoundIsValid = false;
                        }

                        LARGE_INTEGER WorkCounter = Win32GetWallClock();
                        real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                        real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                        int whiles = 0;
                        DWORD SleepMS = 0;
                        real32 Slept = 0.f;
                        if (SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            LARGE_INTEGER Before = Win32GetWallClock();
                            if (SleepIsGranular)
                            {
                                SleepMS = (DWORD)(1000.0f *(TargetSecondsPerFrame - SecondsElapsedForFrame));
                                if (SleepMS > 0)
                                {
                                    Sleep(SleepMS);
                                }
                            }
                            LARGE_INTEGER After = Win32GetWallClock();
                            Slept = Win32GetSecondsElapsed(Before, After);

                            real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                            if (TestSecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                //TODO: Log Missed sleep here!
                            }

                            while (SecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                                ++whiles;
                            }
                        }
                        else
                        {
                            //TODO: MIssed frame rate
                            //TODO: Logging
                        }

                        LARGE_INTEGER EndCounter = Win32GetWallClock();
                        real32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
                        LastCounter = EndCounter;

                        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
#if HANDMADE_INTERNAL
                        Win32DebugSyncDisplay(&GlobalBackbuffer, ARRAY_COUNT(DebugTimeMarkers),
                            DebugTimeMarkers, DebugTimeMarkersIndex - 1, &SoundOutput, TargetSecondsPerFrame);
#endif
                        Win32DisplayBufferInWindow(GlobalBackbuffer, DeviceContext,
                            Dimension.Width, Dimension.Height);

                        FlipWallClock = Win32GetWallClock();

#if HANDMADE_INTERNAL
                        {
                            DWORD PlayCursor;
                            DWORD WriteCursor;
                            if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                            {
                                ASSERT(DebugTimeMarkersIndex < ARRAY_COUNT(DebugTimeMarkers));
                                win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkersIndex];
                                Marker->FlipPlayCursor = PlayCursor;
                                Marker->FlipWriteCursor = WriteCursor;
                            }
                        }
#endif

                        real32 FPS = (1000.0f / MSPerFrame);


                        game_input *Temp = NewInput;
                        NewInput = OldInput;
                        OldInput = Temp;

                        int64 EndCycleCount = __rdtsc();
                        int64 CyclesElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;

#if 0
                        sprintf_s(Msg, "%.02fms/f, %.02ff/s, (%.02fws/f) whiles=%d sleepms=%d, Slept=%.02fms\n",
                            MSPerFrame, FPS, WorkSecondsElapsed*1000.0f, whiles, SleepMS, Slept*1000.0f);
                        OutputDebugStringA(Msg);
#endif

#if HANDMADE_INTERNAL
                        ++DebugTimeMarkersIndex;
                        if (DebugTimeMarkersIndex == ARRAY_COUNT(DebugTimeMarkers))
                        {
                            DebugTimeMarkersIndex = 0;
                        }
#endif
                    }

                }
            }
            else
            {
                //TODO: LOGGING
            }


        }
        else
        {
            //TODO: Logging
        }
    }
    else
    {
        //TODO: logging
    }

    return 0;
}
