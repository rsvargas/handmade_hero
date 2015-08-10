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
#include <math.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#include "handmade.cpp"

#include <Windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <stdio.h>
#include <malloc.h>

#include "win32_handmade.h"

//TODO: global for now
global_variable bool32 GlobalRunning;
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

internal debug_read_file_result DEBUGPlatformReadEntireFile(char * Filename)
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
        }
        CloseHandle(FileHandle);
    }
    return Result;
}

internal void DEBUGPlatformFreeFileMemory(void* Memory)
{
    VirtualFree(Memory, 0, MEM_RELEASE);
}

internal bool32 DEBUGPlatformWriteEntireFile(char *Filename,
    uint32 MemorySize, void* Memory)
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




template<typename T>
T abs(T v)
{
    return (v < 0) ? -v : v;
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
            BufferDescriptor.dwFlags = 0;
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

inline real32 Win32etSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result = ((real32)End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCountFrequency;
    return Result;
}


int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE /*PrevInstance*/,
    LPSTR /*CommandLine*/, int /*ShowCode*/)
{
    LARGE_INTEGER PerfCounterFrequencyResult;
    QueryPerformanceFrequency(&PerfCounterFrequencyResult);
    GlobalPerfCountFrequency = PerfCounterFrequencyResult.QuadPart;

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

    int MonitorRefreshHz = 60;
    int GameUpdateHz = MonitorRefreshHz / 2;
    real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

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
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearSoundBuffer(SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            GlobalRunning = true;

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

                int64 LastCycleCount = __rdtsc();
                while (GlobalRunning)
                {

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

                    DWORD ByteToLock = 0;
                    DWORD TargetCursor = 0;
                    DWORD BytesToWrite = 0;
                    DWORD PlayCursor = 0;
                    DWORD WriteCursor = 0;
                    bool32 SoundIsValid = false;
                    //TODO: tighten up sound logic so that we know where  we should be
                    // writing to and can anticipate the time spent in the game update.
                    if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                    {
                        ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) %
                            SoundOutput.SecondaryBufferSize);
                        TargetCursor = ((PlayCursor +
                            (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) %
                            SoundOutput.SecondaryBufferSize);

                        if (ByteToLock > TargetCursor)
                        {
                            BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
                            BytesToWrite += TargetCursor;
                        }
                        else
                        {
                            BytesToWrite = TargetCursor - ByteToLock;
                        }

                        SoundIsValid = true;
                    }

                    //TODO: sound is wrong now because we didn't updated with the new frame loop
                    game_sound_output_buffer SoundBuffer = {};
                    SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                    SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                    SoundBuffer.Samples = Samples;

                    game_offscreen_buffer Buffer = {};
                    Buffer.Memory = GlobalBackbuffer.Memory;
                    Buffer.Width = GlobalBackbuffer.Width;
                    Buffer.Height = GlobalBackbuffer.Height;
                    Buffer.Pitch = GlobalBackbuffer.Pitch;
                    GameUpdateAndRender(&GameMemory, Buffer, *NewInput, SoundBuffer);

                    if (SoundIsValid)
                    {
                        Win32FillSoundBuffer(SoundOutput, ByteToLock, BytesToWrite, SoundBuffer);
                    }
                    
                    LARGE_INTEGER WorkCounter = Win32GetWallClock();
                    real32 WorkSecondsElapsed = Win32etSecondsElapsed(LastCounter, WorkCounter);

                    real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                    int whiles = 0;
                    if (SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {
                        if (SleepIsGranular)
                        {
                            DWORD SleepMS = (DWORD)(1000.0f *(TargetSecondsPerFrame - SecondsElapsedForFrame));
                            if (SleepMS > 0)
                            {
                                Sleep(SleepMS);
                            }
                        }

                        //NOTE(vargas): this does not really works in my computer
                        //real32 TestSecondsElapsedForFrame = Win32etSecondsElapsed(LastCounter, Win32GetWallClock());
                        //ASSERT(TestSecondsElapsedForFrame < TargetSecondsPerFrame);

                        while (SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            SecondsElapsedForFrame = Win32etSecondsElapsed(LastCounter, Win32GetWallClock());
                            ++whiles;
                        }
                    }
                    else
                    {
                        //TODO: MIssed frame rate
                        //TODO: Logging
                    }

                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    Win32DisplayBufferInWindow(GlobalBackbuffer, DeviceContext,
                        Dimension.Width, Dimension.Height);


                    real32 MSPerFrame = (((1000.0f * SecondsElapsedForFrame)));
                    real32 FPS = (1.0f / SecondsElapsedForFrame);

                    game_state *Stt = (game_state *)GameMemory.PermanentStorage;

                    char msg[256];
                    sprintf_s(msg, "%.02fms/f, %.02f/s, %.02fHz whiles=%d\n", MSPerFrame, FPS, Stt->ToneHz, whiles);
                    OutputDebugStringA(msg);

                    game_input *Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;

                    LARGE_INTEGER EndCounter = Win32GetWallClock();
                    LastCounter = EndCounter;

                    int64 EndCycleCount = __rdtsc();
                    int64 CyclesElapsed = EndCycleCount - LastCycleCount;
                    LastCycleCount = EndCycleCount;


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
