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

#include <stdint.h>

#define internal static
#define local_persist static;
#define global_variable static;

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

//TODO: Implement sine ourselves
#include <math.h>



struct win32_offscreen_buffer
{
	//NOTE: Pixels are always 32bit wide
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

struct win32_sound_output
{
	int SamplesPerSecond;
	int ToneHz;
	uint16 ToneVolume;
	uint32 RunningSampleIndex;
	int SquareWaveCounter;
	int WavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
	real32 tSine;
	int LatencySampleCount;
};


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
		XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary,"XInputGetState");
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
		direct_sound_create *DirectSoundCreate_ = (direct_sound_create*) GetProcAddress(DSoundLibrary, "DirectSoundCreate");
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

internal void Win32FullSoundBuffer(win32_sound_output &Output, DWORD ByteToLock, DWORD BytesToWrite)
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
		//TODO: asset that region sizes are valid

		//TODO: Collapse these two loops
		int16* SampleOut = (int16*)Region1;
		DWORD Region1SampleCount = Region1Size / Output.BytesPerSample;

		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
		{
			real32 SineValue = sinf(Output.tSine);
			int16 SampleValue = (int16)(SineValue*Output.ToneVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = SampleValue;

			Output.tSine += 2.0f * Pi32 * (1.0f/ (real32)Output.WavePeriod);
			++Output.RunningSampleIndex;
		}

		SampleOut = (int16*)Region2;
		DWORD Region2SampleCount = Region2Size / Output.BytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
		{
			real32 SineValue = sinf(Output.tSine);
			int16 SampleValue = (int16)(SineValue*Output.ToneVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = SampleValue;

			Output.tSine += 2.0f * Pi32 * (1.0f / (real32)Output.WavePeriod);
			++Output.RunningSampleIndex;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);

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

LRESULT CALLBACK MainWindowCallback(HWND Window,
	UINT   Message,
	WPARAM WParam,
	LPARAM LParam)
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

			bool32 AltKeyWasDown = (LParam & (1<<29)) != 0;
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


int CALLBACK WinMain(HINSTANCE Instance,
	HINSTANCE /*PrevInstance*/,
	LPSTR /*CommandLine*/,
	int /*ShowCode*/)
{
	LARGE_INTEGER PerfCounterFrequencyResult;
	QueryPerformanceFrequency(&PerfCounterFrequencyResult);
	int PerfCounterFrequency = PerfCounterFrequencyResult.QuadPart;

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

			//NOTE: Sound test
	
			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.ToneHz = 256;
			SoundOutput.ToneVolume = 1000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.SquareWaveCounter = 0;
			SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.tSine = 0.0f;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;

			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32FullSoundBuffer(SoundOutput, 0, SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			GlobalRunning = true;

			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);
			int64 LastCycleCount = __rdtsc();
			while (GlobalRunning)
			{

				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}
					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}

				//TODO: should we poll this more fdrequently?
				for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
				{
					XINPUT_STATE ControllerState;
					if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						//NOTE: controller is plugged in
						//TODO: see if controlerState.dwPacketNumber increments too rapidly
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
						bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool32 LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool32 RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool32 AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool32 BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool32 XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool32 YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

						//This is the LEFT thumbstick
						int16 StickX = Pad->sThumbLX;
						int16 StickY = Pad->sThumbLY;

						if (abs(StickX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
						{
							XOffset += StickX >> 12;
						}
						if (abs(StickY) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
						{
							YOffset -= StickY >> 12;
						}
						//char str[256];
						//sprintf(str, "X= %8d, Y= %8d (%d,%d)\n", (StickX>>12), (StickY>>12), XOffset, YOffset);
						//OutputDebugStringA(str);

						SoundOutput.ToneHz = 512 + (450.0*((real32)StickY / (real32)SHRT_MAX));
						SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
					}
					else
					{
						//NOTE: the controller is not available
					}
				}

				game_offscreen_buffer Buffer = {};
				Buffer.Memory = GlobalBackbuffer.Memory;
				Buffer.Width = GlobalBackbuffer.Width;
				Buffer.Height = GlobalBackbuffer.Height;
				Buffer.Pitch = GlobalBackbuffer.Pitch;
				GameUpdateAndRender(Buffer, XOffset, YOffset);

				//Note: Direct sound output test
				DWORD PlayCursor;
				DWORD WriteCursor;
				if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					DWORD ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) %
						SoundOutput.SecondaryBufferSize;
					DWORD TargetCursor = (PlayCursor +
										  (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) %
										  SoundOutput.SecondaryBufferSize;
					DWORD BytesToWrite;

					if (ByteToLock > TargetCursor)
					{
						BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - ByteToLock;
					}
					Win32FullSoundBuffer(SoundOutput, ByteToLock, BytesToWrite);
				}

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(GlobalBackbuffer, DeviceContext, 
					Dimension.Width, Dimension.Height);

				int64 endCycleCount = __rdtsc();
				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);

				int64 CyclesElapsed = endCycleCount - LastCycleCount;
				int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				int32 MSPerFrame = (int32)(((1000*CounterElapsed) / PerfCounterFrequency));
				int32 FPS = PerfCounterFrequency / CounterElapsed;
				int32 MCPF = (int32)(CyclesElapsed / (1000 * 1000));


				//char Buffer[256];
				//sprintf(Buffer, "%dms/f, %df/s, %dMc/f\n", MSPerFrame, FPS, MCPF);
				//OutputDebugStringA(Buffer);

				LastCounter = EndCounter;
				LastCycleCount = endCycleCount;
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
