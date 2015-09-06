/*
    TODO:  THIS IS NOT A FINAL PLATFORM LAYER

    - Make the right calls so Windows doesn't think we're still loading 

    - Saved game locations
    - Getting a handle to our own executable file
    - Asset loading path
    - Threading (launch a thread)
    - Raw Input (support for multiple keyboards)
    - ClipCursor() (for multimonitor support)
    - QueryCancelAutoplay
    - WM_ACTIVATEAPP (for when we are not the active application)
    - Blt speed improvements (BitBlt)
    - Hardware acceleration (OpenDL or Direct3D or BOTH??)
    - GetKeyboardLayout (for French keyboards, internation WASD support)
    - ChangeDisplaySettings optino if we detect slow fullscreen blit?

    Just a partial list of stuff.
    */

//TODO: Implement sine ourselves

#include "handmade_platform.h"
 
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
global_variable bool32 DEBUGGlobalShowCursor;
global_variable WINDOWPLACEMENT GlobalWindoPosition = { sizeof(GlobalWindoPosition) };

internal void ToggleFullscreen(HWND Window)
{
    //NOTE: Following Raymon Chen prescription for fullscreen toggleing, see:
    // http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
        if (GetWindowPlacement(Window, &GlobalWindoPosition) &&
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
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
        SetWindowPlacement(Window, &GlobalWindoPosition);
        SetWindowPos(Window, 0, 0, 0, 0, 0, 
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

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

internal int StringLength(char* String)
{
    int Count = 0;
    while (*String++)
    {
        ++Count;
    }
    return Count;
}

internal void Win32GetEXEFileName(win32_state *State)
{
    DWORD SizeOfFilename = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
    State->OnePastLastEXEFileNameSlash = State->EXEFileName;
    for (char *Scan = State->EXEFileName; *Scan; ++Scan)
    {
        if (*Scan == '\\')
        {
            State->OnePastLastEXEFileNameSlash = Scan + 1;
        }
    }

}

internal void Win32BuildEXEPathFileName(win32_state *State, char* FileName,
    int DestCount, char* Dest)
{
    CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName, State->EXEFileName,
        StringLength(FileName), FileName,
        DestCount, Dest);
}



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
                    DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
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
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesExA(Filename, GetFileExInfoStandard, &Data))
    {
        Result = Data.ftLastWriteTime;
    }
    return Result;
}

internal win32_game_code Win32LoadGameCode(char* SourceDLLName, char* TempDLLName, char* LockFileName)
{
    win32_game_code Result = {};

    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    if (!GetFileAttributesExA(LockFileName, GetFileExInfoStandard, &Ignored))
    {
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
    }
    if (!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
        Result.GetSoundSamples = 0;
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
    GameCode->UpdateAndRender = 0;
    GameCode->GetSoundSamples = 0;

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
    NewState->HalfTransitionCount = (OldState->EndedDown == NewState->EndedDown) ? 0 : 1;
}

internal real32  Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
    real32 Result = 0;
    if (Value < -DeadZoneThreshold)
    {
        Result = (real32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
    }
    else if (Value > DeadZoneThreshold)
    {
        Result = (real32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
    }
    return Result;
}

internal void Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
    if (NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++(NewState->HalfTransitionCount);
    }
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
    Buffer.BitmapMemorySize = (Buffer.Width * Buffer.Height) * BytesPerPixel;
    Buffer.Memory = VirtualAlloc(0, Buffer.BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    Buffer.Pitch = Width * BytesPerPixel;

    //TODO: probably celar this to black
}

internal void Win32DisplayBufferInWindow(const win32_offscreen_buffer& Buffer,
    HDC DeviceContext, int WindowWidth, int WindowHeigth)
{
    //TODO: Centering / black bars
    if ((WindowWidth >= Buffer.Width*2) &
        (WindowHeigth >= Buffer.Height * 2))
    {
        StretchDIBits(DeviceContext,
            0, 0, 2 * Buffer.Width, 2 * Buffer.Height,//X, Y, DstWidth, DstHeight,
            0, 0, Buffer.Width, Buffer.Height,//X, Y, SrcWidth, SrcHeight,
            Buffer.Memory,
            &Buffer.Info,
            DIB_RGB_COLORS,
            SRCCOPY);
    }
    else
    {
        int OffsetX = 10;
        int OffsetY = 10;
        PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
        PatBlt(DeviceContext, 0, OffsetY + Buffer.Height, WindowWidth, WindowHeigth, BLACKNESS);
        PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeigth, BLACKNESS);
        PatBlt(DeviceContext, OffsetX + Buffer.Width, 0, WindowWidth, WindowHeigth, BLACKNESS);

        //NOTE: for prototyping purpoeses, we're going to always blit
        // 1-to-1 pixels to make sure we don't introduce artifacts with
        // stretching while we are learning to code the renderer.
        StretchDIBits(DeviceContext,
            OffsetX, OffsetY, Buffer.Width, Buffer.Height,//X, Y, Width, Height,
            0, 0, Buffer.Width, Buffer.Height,//X, Y, Width, Height,
            Buffer.Memory,
            &Buffer.Info,
            DIB_RGB_COLORS,
            SRCCOPY);
    }
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

    case WM_SETCURSOR:
    {
        if (DEBUGGlobalShowCursor)
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        }
        else
        {
            SetCursor(0);
        }
    } break;

    case WM_ACTIVATEAPP:
    {
#if 0
        if (WParam == TRUE)
        {
            SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 255, LWA_ALPHA);
        }
        else
        {
            SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 128, LWA_ALPHA);
        }
#endif
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

internal void Win32GetInputFileLocation(win32_state* State, bool32 InputStream, int SlotIndex, int DestCount, char* Dest)
{
    char Temp[64];
    wsprintf(Temp, "loop_edit_%d_%s.hmi", SlotIndex, (InputStream ? "input" : "state"));
    Win32BuildEXEPathFileName(State, Temp, DestCount, Dest);
}

internal win32_replay_buffer* Win32GetReplaybuffer(win32_state *State, unsigned int Index)
{
    ASSERT(Index > 0);
    ASSERT(Index < ARRAY_COUNT(State->ReplayBuffers));
    win32_replay_buffer * Result = &State->ReplayBuffers[Index];
    return Result;
}

internal void Win32BeginRecordingInput(win32_state *State, int InputRecordingIndex)
{
    win32_replay_buffer *ReplayBuffer = Win32GetReplaybuffer(State, InputRecordingIndex);
    if (ReplayBuffer->MemoryBlock)
    {
        State->InputRecordingIndex = InputRecordingIndex;

        char FileName[WIN32_STATE_FILE_NAME_COUNT];
        Win32GetInputFileLocation(State, true, InputRecordingIndex,
            sizeof(FileName), FileName);

        State->RecordingHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);;

#if 0
        LARGE_INTEGER FilePosition;
        FilePosition.QuadPart = State->TotalSize;
        SetFilePointerEx(State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
#endif

        CopyMemory(ReplayBuffer->MemoryBlock, State->GameMemoryBlock, State->TotalSize);
    }
}

internal void Win32EndRecordingInput(win32_state *State)
{
    CloseHandle(State->RecordingHandle);
    State->RecordingHandle = 0;
    State->InputRecordingIndex = 0;
}


internal void Win32BeginInputPlayBack(win32_state *State, int InputPlayingIndex)
{
    win32_replay_buffer *ReplayBuffer = Win32GetReplaybuffer(State, InputPlayingIndex);
    if (ReplayBuffer->MemoryBlock)
    {
        State->InputPlayingIndex = InputPlayingIndex;
        State->PlaybackHandle = ReplayBuffer->FileHandle;

        char FileName[WIN32_STATE_FILE_NAME_COUNT];
        Win32GetInputFileLocation(State, true, InputPlayingIndex, 
            sizeof(FileName), FileName);

        State->PlaybackHandle = CreateFileA(FileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);

#if 0
        LARGE_INTEGER FilePosition;
        FilePosition.QuadPart = State->TotalSize;
        SetFilePointerEx(State->PlaybackHandle, FilePosition, 0, FILE_BEGIN);
#endif

        CopyMemory(State->GameMemoryBlock, ReplayBuffer->MemoryBlock, State->TotalSize);
    }
}

internal void Win32EndInputPlayBack(win32_state *State)
{
    CloseHandle(State->PlaybackHandle);
    State->PlaybackHandle = 0;
    State->InputPlayingIndex = 0;
}

internal void Win32RecordInput(win32_state *State, game_input *NewInput)
{
    DWORD BytesWritten;
    WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}

internal void Win32PlayBackInput(win32_state *State, game_input *NewInput)
{
    DWORD BytesRead;
    if (ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0))
    {
        if (BytesRead == 0)
        {
            //NOTE: we're hit the end of the stream!
            int PlayingIndex = State->InputPlayingIndex;
            Win32EndInputPlayBack(State);
            Win32BeginInputPlayBack(State, PlayingIndex);
            ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
        }
    }
}

internal void Win32ProcessPendingMessages(win32_state *State, game_controller_input* KeyboardController)
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
            //NOTE: since we are comparing WasDown to IsDown
            // we must use == 0 and != 0 to convert there bit tests 
            // to actual 0 or 1 values.
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
                    Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                }
                else if (VKCode == VK_SPACE)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                }
#if HANDMADE_INTERNAL
                else if (VKCode == 'P')
                {
                    if (IsDown)
                    {
                        GlobalPause = !GlobalPause;
                    }
                }
                else if (VKCode == 'L')
                {
                    if (IsDown)
                    {
                        if (State->InputPlayingIndex == 0)
                        {
                            if (State->InputRecordingIndex == 0)
                            {
                                Win32BeginRecordingInput(State, 1);
                            }
                            else
                            {
                                Win32EndRecordingInput(State);
                                Win32BeginInputPlayBack(State, 1);
                            }
                        }
                        else
                        {
                            Win32EndInputPlayBack(State);
                        }

                    }
                }
#endif
                if (IsDown)
                {
                    bool32 AltKeyWasDown = (Message.lParam & (1 << 29)) != 0;
                    if ((VKCode == VK_F4) && AltKeyWasDown)
                    {
                        GlobalRunning = false;
                    }
                    if ((VKCode == VK_RETURN) && AltKeyWasDown)
                    {
                        if (Message.hwnd)
                        {
                            ToggleFullscreen(Message.hwnd);
                        }
                    }
                }
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
    real32 Result = (real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCountFrequency;
    return Result;
}

#if 0
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
#endif

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE /*PrevInstance*/,
    LPSTR /*CommandLine*/, int /*ShowCode*/)
{
    win32_state Win32State = {};
    Win32GetEXEFileName(&Win32State);

    LARGE_INTEGER PerfCounterFrequencyResult;
    QueryPerformanceFrequency(&PerfCounterFrequencyResult);
    GlobalPerfCountFrequency = PerfCounterFrequencyResult.QuadPart;

    char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&Win32State, "handmade.dll",
        sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

    char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&Win32State, "handmade_temp.dll",
        sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);

    char GameCodeLockFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&Win32State, "lock.tmp",
        sizeof(GameCodeLockFullPath), GameCodeLockFullPath);

    char Msg[256];
    TIMECAPS caps;
    timeGetDevCaps(&caps, sizeof(caps));
    sprintf_s(Msg, "CAPS: min=%d, max=%d\n", caps.wPeriodMin, caps.wPeriodMax);
    OutputDebugStringA(Msg);

    UINT DesiredSchedulerMS = 1;
    bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    Win32LoadXInput();
    
#if HANDMADE_INTERNAL
    DEBUGGlobalShowCursor = true;
#endif
    WNDCLASSA WindowClass = {}; //zero initialize

    Win32ResizeDIBSection(GlobalBackbuffer, 960, 540);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW; //always redraw when resize
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    //WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(
            0, //WS_EX_TOPMOST | WS_EX_LAYERED,
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
            int MonitorRefreshHz = 60;
            HDC RefreshDC = GetDC(Window);
            int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(Window, RefreshDC);
            if (Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }
            real32 GameUpdateHz = ((real32)MonitorRefreshHz/2.0f);
            real32 TargetSecondsPerFrame = (1.0f / (GameUpdateHz));

            win32_sound_output SoundOutput = {};

            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.BytesPerSample = sizeof(int16) * 2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;

             //NOTE(vargas): divide by 3 didn't really worked for me =/
            SoundOutput.SafetyBytes = (int)((real32)SoundOutput.SamplesPerSecond*
                (real32)SoundOutput.BytesPerSample / GameUpdateHz);
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
            GameMemory.TransientStorageSize = MEGABYTES(100); //My HDD cant write a Gb of data without freezing
            GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
            GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

            Win32State.TotalSize = GameMemory.PermanentStorageSize +
                GameMemory.TransientStorageSize;
            Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize,
                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

            for (int ReplayIndex = 0; ReplayIndex < ARRAY_COUNT(Win32State.ReplayBuffers); ++ReplayIndex)
            {
                win32_replay_buffer *ReplayBuffer = &Win32State.ReplayBuffers[ReplayIndex];
                
                Win32GetInputFileLocation(&Win32State, false, ReplayIndex,
                    sizeof(ReplayBuffer->FileName), ReplayBuffer->FileName);

                ReplayBuffer->FileHandle =
                    CreateFileA(ReplayBuffer->FileName, GENERIC_WRITE | GENERIC_READ, 0, 0, CREATE_ALWAYS, 0, 0);

                LARGE_INTEGER MaxSize;
                MaxSize.QuadPart= Win32State.TotalSize;
                ReplayBuffer->MemoryMap = CreateFileMappingA(ReplayBuffer->FileHandle, 0,
                    PAGE_READWRITE, MaxSize.HighPart, MaxSize.LowPart, 0);

                ReplayBuffer->MemoryBlock  = MapViewOfFile(ReplayBuffer->MemoryMap,
                    FILE_MAP_ALL_ACCESS, 0, 0, Win32State.TotalSize);

                if (ReplayBuffer->MemoryBlock)
                {

                }
                else
                {
                    //TODO: diagnostic
                }
            }


            GameMemory.PermanentStorage = Win32State.GameMemoryBlock;

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
                win32_debug_time_marker DebugTimeMarkers[30] = {};

                //TODO: Handle startup specially
                DWORD AudioLatencyBytes = 0;
                real32 AudioLatencySeconds = 0.0f;
                bool32 SoundIsValid = false;

                int64 LastCycleCount = __rdtsc();

                win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);
                uint64 LoadCounter = 0;

                while (GlobalRunning)
                {
                    NewInput->dtForFrame = TargetSecondsPerFrame;
                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    
                    if (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                    {
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);
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

                    Win32ProcessPendingMessages(&Win32State, NewKeyboardController);

                    if (!GlobalPause)
                    {
                        POINT MouseP;
                        GetCursorPos(&MouseP);
                        ScreenToClient(Window, &MouseP);
                        NewInput->MouseX = MouseP.x;
                        NewInput->MouseY = MouseP.y;
                        NewInput->MouseZ = 0;
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[0],
                            GetKeyState(VK_LBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[1],
                            GetKeyState(VK_MBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[2],
                            GetKeyState(VK_RBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[3],
                            GetKeyState(VK_XBUTTON1) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[4],
                            GetKeyState(VK_XBUTTON2) & (1 << 15));

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
                                NewController->IsAnalog = OldController->IsAnalog;

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
                        thread_context Thread;

                        game_offscreen_buffer Buffer = {};
                        Buffer.Memory = GlobalBackbuffer.Memory;
                        Buffer.Width = GlobalBackbuffer.Width;
                        Buffer.Height = GlobalBackbuffer.Height;
                        Buffer.Pitch = GlobalBackbuffer.Pitch;
                        Buffer.BytesPerPixel = GlobalBackbuffer.BytesPerPixel;
                        Buffer.MemorySize = GlobalBackbuffer.BitmapMemorySize;


                        if (Win32State.InputRecordingIndex)
                        {
                            Win32RecordInput(&Win32State, NewInput);
                        }

                        if (Win32State.InputPlayingIndex)
                        {
                            Win32PlayBackInput(&Win32State, NewInput);
                        }
                        if (Game.UpdateAndRender)
                        {
                            Game.UpdateAndRender(&Thread, &GameMemory, Buffer, *NewInput);
                        }

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
                                (int)(((real32)SoundOutput.SamplesPerSecond * (real32)SoundOutput.BytesPerSample) 
                                / GameUpdateHz);

                            real32 SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
                            DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip / TargetSecondsPerFrame)  *
                                (real32)ExpectedSoundBytesPerFrame);

                            DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;

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
                            if (Game.GetSoundSamples)
                            {
                                Game.GetSoundSamples(&Thread, &GameMemory, SoundBuffer);
                            }
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
#if 0
                            sprintf_s(Msg, "X=%.1f Y=%.1f BTL:%u TC:%u BTW:%u PC:%u WC:%u DELTA:%u (%fs)\n",
                                NewInput->Controllers[1].StickAverageX,
                                NewInput->Controllers[1].StickAverageY,
                                ByteToLock, TargetCursor, BytesToWrite, PlayCursor,
                                WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
                            OutputDebugStringA(Msg);
#endif
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
                        //Win32DebugSyncDisplay(&GlobalBackbuffer, ARRAY_COUNT(DebugTimeMarkers),
                        //    DebugTimeMarkers, DebugTimeMarkersIndex - 1, &SoundOutput, TargetSecondsPerFrame);
#endif
                        HDC DeviceContext = GetDC(Window);
                        Win32DisplayBufferInWindow(GlobalBackbuffer, DeviceContext,
                            Dimension.Width, Dimension.Height);
                        ReleaseDC(Window, DeviceContext);

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



                        game_input *Temp = NewInput;
                        NewInput = OldInput;
                        OldInput = Temp;

#if 0
                        real32 FPS = (1000.0f / MSPerFrame);

                        int64 EndCycleCount = __rdtsc();
                        int64 CyclesElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;

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
