
#include <Windows.h>


LRESULT CALLBACK MainWindowCallback(HWND   Window,
	UINT   Message,
	WPARAM WParam,
	LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message)
	{
	case WM_SIZE:
		OutputDebugStringA("WM_SIZE\n");
		break;

	case WM_DESTROY:
		OutputDebugStringA("WM_DESTROY\n");
		break;

	//case WM_CLOSE:
	//	OutputDebugStringA("WM_CLOSE\n");
	//	break;

	case WM_ACTIVATEAPP:
		OutputDebugStringA("WM_ACTIVATEAAPP\n");
		break;

	case WM_PAINT:
	{
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint(Window, &Paint);
		int X = Paint.rcPaint.left;
		int Y = Paint.rcPaint.top;
		int Width= Paint.rcPaint.right - Paint.rcPaint.left;
		int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
		static DWORD Operation = WHITENESS;
		PatBlt(DeviceContext, X, Y, Width, Height, Operation);
		if (Operation == WHITENESS)
		{
			Operation = BLACKNESS;
		}
		else
		{
			Operation = WHITENESS;
		}
		EndPaint(Window, &Paint);

	} break;

	default:
//		OutputDebugStringA("default\n");
		Result = DefWindowProc(Window, Message, WParam, LParam);
		break;
	}

	return Result;
}


int CALLBACK WinMain(HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR CommandLine,
	int ShowCode)
{
	WNDCLASS WindowClass = {};

	//TODO: check if those flags still matter
	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = Instance;
	////WindowClass.hIcon;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&WindowClass))
	{
		HWND WindowHandle = CreateWindowEx(
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

		if (WindowHandle)
		{
			for (;;)
			{
				MSG Message;
				BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
				if (MessageResult > 0)
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
				else
				{
					break; //out of the infinite for(;;)
				}
			}	
		}
		else
		{
			//TODO
		}
	}
	else
	{
		//TODO
	}

	return 0;
}
