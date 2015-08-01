
#include <Windows.h>
#include <stdint.h>

#define internal static
#define local_persist static;
#define global_variable static;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

//TODO: global for now
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;

win32_window_dimension Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}


internal void RenderWeirdGradiend(const win32_offscreen_buffer& Buffer, int XOffset, int YOffset)
{
	//TODO: Lets see what the optimizer does

	uint8* Row = (uint8*)Buffer.Memory;
	for (int Y = 0; Y < Buffer.Height; ++Y)
	{
		uint32* Pixel = (uint32*)Row;
		for (int X = 0; X < Buffer.Width; ++X)
		{
			uint8 Green = (Y + YOffset);
			uint8 Blue = (X + XOffset);
			/*
			Memory:   BB GG RR xx
			Register: xx RR GG BB
			Pixel (32-bits)
			*/
			*Pixel++ = (Green << 8) | Blue;
		}
		Row += Buffer.Pitch;
	}
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
	Buffer.BytesPerPixel = 4;

	Buffer.Info.bmiHeader.biSize = sizeof(Buffer.Info.bmiHeader);
	Buffer.Info.bmiHeader.biWidth = Buffer.Width;
	Buffer.Info.bmiHeader.biHeight = -Buffer.Height; //negative is top-down, positive is bottom-up
	Buffer.Info.bmiHeader.biPlanes = 1;
	Buffer.Info.bmiHeader.biBitCount = 32;
	Buffer.Info.bmiHeader.biCompression = BI_RGB;

	//Note: casey thanks Chris Hecker!!
	int BitmapMemorySize = (Buffer.Width * Buffer.Height) * Buffer.BytesPerPixel;
	Buffer.Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	Buffer.Pitch = Width * Buffer.BytesPerPixel;

	//TODO: probably celar this to black
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, 
	int WindowWidth, int WindowHeigth,
	const win32_offscreen_buffer& Buffer,
	int X, int Y, int Width, int Height)
{
	//TODO: aspect ratio coeection
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
			Running = false;
		} break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAAPP\n");
		} break;

		case WM_DESTROY:
		{
			//TODO: handle this with a error - recreate window?
			Running = false;
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width= Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer,
				X, Y, Width, Height);
			EndPaint(Window, &Paint);

		} break;

		default:
		{
			//		OutputDebugStringA("default\n");
			Result = DefWindowProc(Window, Message, WParam, LParam);
		} break;
	}

	return Result;
}


int CALLBACK WinMain(HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR CommandLine,
	int ShowCode)
{
	WNDCLASSA WindowClass = {}; //zero initialize

	Win32ResizeDIBSection(GlobalBackbuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW; //always redraw when resize
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
			Running = true;
			int XOffset = 0;
			int YOffset = 0;
			while (Running)
			{
				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						Running = false;
					}
					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}
				RenderWeirdGradiend(GlobalBackbuffer, XOffset, YOffset);
				HDC DeviceContext = GetDC(Window);
				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer,
					0, 0, Dimension.Width, Dimension.Height);
				ReleaseDC(Window, DeviceContext);

				//sprintf(str, "Redraw (%d)\n", counter++);
				//OutputDebugStringA(str);

				++XOffset;
				YOffset += 2;
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
