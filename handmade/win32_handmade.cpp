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

internal void Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
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

        uint32 VKCode = (uint32)WParam;
        bool32 WasDown = (LParam & (1 << 30)) != 0;
        bool32 IsDown = (LParam & (1 << 31)) == 0;

        if (WasDown != IsDown)
        {
            if (VKCode == 'W')
            {
            }
            else if (VKCode == 'A')
            {
            }
            else if (VKCode == 'S')
            {
            }
            else if (VKCode == 'D')
            {
            }
            else if (VKCode == 'Q')
            {
            }
            else if (VKCode == 'E')
            {
            }
            else if (VKCode == VK_UP)
            {
            }
            else if (VKCode == VK_LEFT)
            {
            }
            else if (VKCode == VK_DOWN)
            {
            }
            else if (VKCode == VK_RIGHT)
            {
            }
            else if (VKCode == VK_ESCAPE)
            {
            }
            else if (VKCode == VK_SPACE)
            {
                OutputDebugStringA("ESCAPE - ");
                OutputDebugStringA(WasDown ? "Was DOWN - " : "Was UP   - ");
                OutputDebugStringA(IsDown ? "Is  DOWN\n" : "Is UP\n");
            }
        }

        bool32 AltKeyWasDown = (LParam & (1 << 29)) != 0;
        if ((VKCode == VK_F4) && AltKeyWasDown)
        {
            GlobalRunning = false;
        }

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

internal void Win32ProcessPendingMessages(game_controller_input& KeyboardController)
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
                }
                else if (VKCode == 'A')
                {
                }
                else if (VKCode == 'S')
                {
                }
                else if (VKCode == 'D')
                {
                }
                else if (VKCode == 'Q')
                {
                    Win32ProcessKeyboardMessage(&KeyboardController.LeftShoulder, IsDown);
                }
                else if (VKCode == 'E')
                {
                    Win32ProcessKeyboardMessage(&KeyboardController.RightShoulder, IsDown);
                }
                else if (VKCode == VK_UP)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController.Up, IsDown);
                }
                else if (VKCode == VK_LEFT)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController.Left, IsDown);
                }
                else if (VKCode == VK_DOWN)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController.Down, IsDown);
                }
                else if (VKCode == VK_RIGHT)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController.Right, IsDown);
                }
                else if (VKCode == VK_ESCAPE)
                {
                    GlobalRunning = false;
                }
                else if (VKCode == VK_SPACE)
                {
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


int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE /*PrevInstance*/,
    LPSTR /*CommandLine*/, int /*ShowCode*/)
{
    LARGE_INTEGER PerfCounterFrequencyResult;
    QueryPerformanceFrequency(&PerfCounterFrequencyResult);
    int64 PerfCounterFrequency = PerfCounterFrequencyResult.QuadPart;

    Win32LoadXInput();
    WNDCLASSA WindowClass = {}; //zero initialize

    Win32ResizeDIBSection(GlobalBackbuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; //always redraw when resize
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

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
                game_input *OldInput = &Input[0];

                LARGE_INTEGER LastCounter;
                QueryPerformanceCounter(&LastCounter);
                int64 LastCycleCount = __rdtsc();
                while (GlobalRunning)
                {

                    game_controller_input& KeyboardController = NewInput->Controllers[0];
                    //TODO: We cant zero, if we do, things go south!
                    game_controller_input ZeroController = {};
                    KeyboardController = ZeroController;

                    Win32ProcessPendingMessages(KeyboardController);

                    //TODO: should we poll this more fdrequently?
                    int MaxControllerCount = XUSER_MAX_COUNT;
                    if (MaxControllerCount > ARRAY_COUNT(NewInput->Controllers))
                    {
                        MaxControllerCount = ARRAY_COUNT(NewInput->Controllers);
                    }
                    for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
                    {
                        game_controller_input& OldController = OldInput->Controllers[ControllerIndex];
                        game_controller_input& NewController = NewInput->Controllers[ControllerIndex];
                        XINPUT_STATE ControllerState;
                        if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                        {
                            //NOTE: controller is plugged in
                            //TODO: see if controlerState.dwPacketNumber increments too rapidly
                            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                            //TODO: DPad
                            bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                            bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                            bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                            bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

                            NewController.IsAnalog = true;
                            real32 X = (real32)Pad->sThumbLX /
                                ((Pad->sThumbLX < 0) ? 32768.0f : 32767.0f);
                            NewController.StartX = OldController.EndX;
                            NewController.MinX = NewController.MaxX = NewController.EndX = X;

                            real32 Y = (real32)Pad->sThumbLY /
                                ((Pad->sThumbLY < 0) ? 32768.0f : 32767.0f);
                            NewController.StartY = OldController.EndY;
                            NewController.MinY = NewController.MaxY = NewController.EndY = Y;

                            //TODO: handle deadzone

                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController.Down,
                                XINPUT_GAMEPAD_A,
                                &NewController.Down);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController.Right,
                                XINPUT_GAMEPAD_B,
                                &NewController.Right);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController.Left,
                                XINPUT_GAMEPAD_X,
                                &NewController.Left);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController.Up,
                                XINPUT_GAMEPAD_Y,
                                &NewController.Up);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController.LeftShoulder,
                                XINPUT_GAMEPAD_LEFT_SHOULDER,
                                &NewController.LeftShoulder);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController.RightShoulder,
                                XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                &NewController.RightShoulder);

                            //bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                            //bool32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);

                        }
                        else
                        {
                            //NOTE: the controller is not available
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

                    //Note: Direct sound output test
                    if (SoundIsValid)
                    {
                        Win32FillSoundBuffer(SoundOutput, ByteToLock, BytesToWrite, SoundBuffer);
                    }

                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    Win32DisplayBufferInWindow(GlobalBackbuffer, DeviceContext,
                        Dimension.Width, Dimension.Height);

                    int64 endCycleCount = __rdtsc();
                    LARGE_INTEGER EndCounter;
                    QueryPerformanceCounter(&EndCounter);

                    int64 CyclesElapsed = endCycleCount - LastCycleCount;
                    int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                    int32 MSPerFrame = (int32)(((1000 * CounterElapsed) / PerfCounterFrequency));
                    int32 FPS = (int32)(PerfCounterFrequency / CounterElapsed);
                    int32 MCPF = (int32)(CyclesElapsed / (1000 * 1000));


                    char msg[256];
                    sprintf_s(msg, 256, "%dms/f, %df/s, %dMc/f\n", MSPerFrame, FPS, MCPF);
                    OutputDebugStringA(msg);

                    LastCounter = EndCounter;
                    LastCycleCount = endCycleCount;

                    game_input *Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;

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
