#include "WinWindow.h"

#include "Omnia/Log.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <DwmApi.h>
#include <ShObjIdl.h>
#include <Windows.h>
#include <WindowsX.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

namespace Omnia {

// Internal Properties
HINSTANCE hApplication;
HWND hWindow;
HICON AppIcon;
ITaskbarList3 *TaskbarList;

// Internal Message Callback
static intptr_t __stdcall MessageCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Internal Styles
enum class Styles {
	// NEHE SetWindowLongPtr(window, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	Default = WS_OVERLAPPEDWINDOW,		// Contains: WS_SYSMENU | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE
	Aero = WS_POPUP | WS_THICKFRAME,
	Borderless = WS_POPUP | WS_VISIBLE,
	Full = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_VISIBLE
};
enum class StylesX {
	// WindowStyleEx &= (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)
	// NEHE SetWindowLongPtr(window, GWL_EXSTYLE, WS_EX_APPWINDOW);
	DefaultX = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
	FullX = WS_EX_APPWINDOW
};
struct PlatformWindowStyle {
	uint32_t WindowStyle;
	uint32_t WindowStyleEx;
};


// Default
WinWindow::WinWindow(const WindowProperties &properties):
	Properties{ properties } {
	// Properties
	PlatformWindowStyle windowStyle;
	HBRUSH ClearColor = CreateSolidBrush(RGB(0, 0, 0));

	// Get Application Information
	hApplication = GetModuleHandle(NULL);
	LPSTR lpCmdLine = GetCommandLineA();
	STARTUPINFOA StartupInfo;
	GetStartupInfoA(&StartupInfo);

	// Settings
	//SetThreadExecutionState(ES_DISPLAY_REQUIRED && ES_SYSTEM_REQUIRED);

	// Load Ressources
	//AppIcon = (HICON)LoadIconFile(Properties.Icon);

	// Register Window Class
	WNDCLASSEX classProperties = {
		.cbSize	= sizeof(WNDCLASSEX),					// Structure Size (in bytes)
		.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,	// Seperate device context for window and redraw on move
		.lpfnWndProc = MessageCallback,					// Message Callback (WndProc)
		.cbClsExtra = 0,								// Extra class data
		.cbWndExtra = WS_EX_NOPARENTNOTIFY,				// Extra window data
		.hInstance = hApplication,						// Application Intance
		.hIcon = AppIcon,								// Load Icon (Default: LoadIcon(NULL, IDI_APPLICATION);)
		.hCursor = LoadCursor(NULL, IDC_ARROW),			// Load Cursor (Default: IDC_ARROW)
		.hbrBackground = ClearColor,					// Background (Not required for GL -> NULL)
		.lpszMenuName = NULL,							// We Don't Want A Menu
		.lpszClassName = Properties.ID.c_str(),			// Class Name should be unique per window
		.hIconSm = AppIcon,								// Load Icon Symbol (Default: LoadIcon(NULL, IDI_WINLOGO);)
	};
	if (!RegisterClassEx(&classProperties)) {
		applog << Log::Error << __FUNCTION__ << ": Failed to register the window class." << std::endl;
		return;
	}

	// Switch to FullScreen Mode if requested
	unsigned int screenWidth = GetSystemMetricsForDpi(SM_CXSCREEN, GetDpiForSystem());
	unsigned int screenHeight = GetSystemMetricsForDpi(SM_CYSCREEN, GetDpiForSystem());
	if (Properties.Style == WindowStyle::FullScreen) {
		// Device Mode
		//memset(&screenProperties, 0, sizeof(screenProperties));	// Makes sure memory's cleared
		DEVMODE screenProperties = {
			.dmSize = sizeof(DEVMODE),				// Structure Size
			.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT,
			.dmBitsPerPel = 32,						// Selected Bits Per Pixel
			.dmPelsWidth = screenWidth,		// Selected Screen Width
			.dmPelsHeight = screenHeight,	// Selected Screen Height
		};
		
		// Try to set selected fullscreen mode
		// Note: CDS_FULLSCREEN gets rid of Taskbar.
		if ((Properties.Size.Width != screenWidth) && (Properties.Size.Height != screenHeight)) {
			// If the switching fails, offer the user an option to switch to windowed mode
			if (ChangeDisplaySettings(&screenProperties, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
				if (MessageBox(NULL, "The requested FullScreen Mode isn't supported by\n<our graphics card. Switch to windowed mode Instead?", __FUNCTION__, MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
					Properties.Style == WindowStyle::Default;
				} else {
					applog << Log::Error << __FUNCTION__ << ": Switching to fullscreen mode failed!" << std::endl;
					return;
				}
			}
		}
	}

	// Configure Window
	ShowCursor(!Properties.State.Cursor);
	RECT WindowDimension;
	switch (Properties.Style) {
		case WindowStyle::Default: {
			windowStyle.WindowStyle = (DWORD)Styles::Default;
			windowStyle.WindowStyleEx = (DWORD)StylesX::DefaultX;
			break;
		}
		case WindowStyle::Borderless: {
			windowStyle.WindowStyle = (DWORD)Styles::Borderless;
			windowStyle.WindowStyleEx = (DWORD)StylesX::DefaultX;
			break;
		}
		case WindowStyle::FullScreen: {
			windowStyle.WindowStyle = (DWORD)Styles::Full;
			windowStyle.WindowStyleEx = (DWORD)StylesX::FullX;
			break;
		}
	}
	WindowDimension.left = Properties.Position.X;
	WindowDimension.top = Properties.Position.Y;
	WindowDimension.right = (Properties.Style == WindowStyle::FullScreen) ? (long)screenWidth : (long)Properties.Size.Width;
	WindowDimension.bottom = (Properties.Style == WindowStyle::FullScreen) ? (long)screenHeight : (long)Properties.Size.Height;
	AdjustWindowRectEx(&WindowDimension, windowStyle.WindowStyle, FALSE, windowStyle.WindowStyleEx);

	// Create Window
	hWindow = CreateWindowEx(
		windowStyle.WindowStyleEx,	// Window Style (extended)
		Properties.ID.c_str(),		// Window ClassName
		Properties.Title.c_str(),	// Window Title
		windowStyle.WindowStyle,	// Window Style

		// Window Position and Size (if not centered, system should decide where to position the window)
		CW_USEDEFAULT, CW_USEDEFAULT, WindowDimension.right - WindowDimension.left, WindowDimension.bottom - WindowDimension.top,

		NULL,						// Parent Window
		NULL,						// Menu
		hApplication,				// Instance Handle
		this						// Application Data
	);
	if (!hWindow) {
		applog << Log::Error << __FUNCTION__ << ": Failed to create the window." << std::endl;
		//Destroy();
		return;
	}

	// Center Window
	if (!(Properties.Style == WindowStyle::FullScreen) && Properties.Position.Centered) {
		unsigned int x = (GetSystemMetrics(SM_CXSCREEN) - WindowDimension.right) / 2;
		unsigned int y = (GetSystemMetrics(SM_CYSCREEN) - WindowDimension.bottom) / 2;
		SetWindowPos(hWindow, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}

	// Show window
	if (Properties.State.Visible) {
		ShowWindow(hWindow, SW_SHOW);
		SetForegroundWindow(hWindow);
		SetFocus(hWindow);
	}

	// DWM Settings
	static const DWM_BLURBEHIND blur {{0}, {TRUE}, {NULL}, {TRUE}};
	static const MARGINS shadow[2] {{0, 0, 0, 0}, {1, 1, 1, 1}};
	DwmEnableBlurBehindWindow(hWindow, &blur);
	DwmExtendFrameIntoClientArea(hWindow, &shadow[0]);

	// Taskbar Settings
	RegisterWindowMessage("TaskbarButtonCreated");
	HRESULT hrf = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (LPVOID *)&TaskbarList);
	//SetProgress(0.f);

	// Flash on Taskbar
	FlashWindow(hWindow, true);
}

WinWindow::~WinWindow() {
	// Switch back to Standard mode if in FullScreen mode
	if (Properties.Style == WindowStyle::FullScreen) {
		ChangeDisplaySettings(NULL, NULL);
		ShowCursor(true);
	}
	
	// Ensure that the Window gets destroyed and release the handle to it
	if (hWindow) {
		if (DestroyWindow(hWindow)) {
			hWindow = nullptr;
		} else {
			applog << Log::Error << "Could not release handle to window.\n";
		}
	}

	// Ensure that the Window Class gets released
	if (hApplication) {
		if (UnregisterClass(Properties.ID.c_str(), hApplication)) {
			hApplication = nullptr;
		} else {
			applog << Log::Error << "Could not unregister window class.\n";
		}
	}
}


// Events
intptr_t __stdcall MessageCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// Properties
	WinWindow *pCurrentWindow = nullptr;

	// Get/Set the current window class pointer as userdata in WinAPI window
	if (uMsg == WM_NCCREATE) {
		pCurrentWindow = static_cast<WinWindow*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
		SetLastError(0);
		if (!SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCurrentWindow))) {
			if (GetLastError() != 0) { return FALSE; }
		}
	} else {
		pCurrentWindow = reinterpret_cast<WinWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}

	// Dispatch Message or ...
	if (pCurrentWindow) {
		MSG message = {
			.hwnd    = hWnd,
			.message = uMsg,
			.wParam  = wParam,
			.lParam  = lParam,
		};
		return pCurrentWindow->Message(&message);
	}

	// ... return it as unhandeld
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

intptr_t WinWindow::Message(void *event) {
	// Preparation
	MSG &msg = *(MSG*)(event);

	// Publish events to an external event system
	if (!EventCallback.Empty()) { EventCallback.Publish(event); }

	/**
	* @brief	Window Messages
	* @source	https://docs.microsoft.com/en-us/windows/win32/winmsg/window-messages
	*			https://docs.microsoft.com/en-us/windows/win32/winmsg/window-notifications
	*			https://docs.microsoft.com/en-us/windows/win32/inputdev/keyboard-input-notifications
	*			https://docs.microsoft.com/en-us/windows/win32/gdi/painting-and-drawing-messages
	*/
	switch (msg.message) {
		// Default: This message doesn't belong to the current window
		case WM_NULL: {
			break;
		}

		// Preparation
		case WM_NCCALCSIZE: {
			//if (msg.lParam) {
			//	NCCALCSIZE_PARAMS *sz = (NCCALCSIZE_PARAMS *)msg.lParam;
			//	if (msg.wParam) {
			//		sz->rgrc[0].bottom +=
			//			window->BorderWidth;  // rgrc[0] is what makes this work, don't know
			//						  // what others (rgrc[1], rgrc[2]) do, but why
			//						  // not change them all?
			//		sz->rgrc[0].right += window->BorderWidth;
			//		sz->rgrc[1].bottom += window->BorderWidth;
			//		sz->rgrc[1].right += window->BorderWidth;
			//		sz->rgrc[2].bottom += window->BorderWidth;
			//		sz->rgrc[2].right += window->BorderWidth;
			//		return 0;
			//	}
			//}
			break;
		}
		case WM_NCHITTEST: {
			//RECT WindowRect;
			//int x, y;
			//GetWindowRect(msg.hwnd, &WindowRect);
			//x = GET_X_LPARAM(msg.lParam) - WindowRect.left;
			//y = GET_Y_LPARAM(msg.lParam) - WindowRect.top;
			//if (x >= pWindow->BorderWidth && x <= WindowRect.right - WindowRect.left - pWindow->BorderWidth && y >= pWindow->BorderWidth && y <= pWindow->TitleBarWidth)
			//	result = HTCAPTION;
			//else if (x < pWindow->BorderWidth && y < pWindow->BorderWidth)
			//	result = HTTOPLEFT;
			//else if (x > WindowRect.right - WindowRect.left - pWindow->BorderWidth && y < pWindow->BorderWidth)
			//	result = HTTOPRIGHT;
			//else if (x > WindowRect.right - WindowRect.left - pWindow->BorderWidth && y > WindowRect.bottom - WindowRect.top - pWindow->BorderWidth)
			//	result = HTBOTTOMRIGHT;
			//else if (x < pWindow->BorderWidth && y > WindowRect.bottom - WindowRect.top - pWindow->BorderWidth)
			//	result = HTBOTTOMLEFT;
			//else if (x < pWindow->BorderWidth)
			//	result = HTLEFT;
			//else if (y < pWindow->BorderWidth)
			//	result = HTTOP;
			//else if (x > WindowRect.right - WindowRect.left - pWindow->BorderWidth)
			//	result = HTRIGHT;
			//else if (y > WindowRect.bottom - WindowRect.top - pWindow->BorderWidth)
			//	result = HTBOTTOM;
			//else
			//	result = HTCLIENT;
			break;
		}

		// Information
		case WM_DPICHANGED: {
			// ToDo: Update Widnow
			break;
		}
		case WM_GETMINMAXINFO: {
			// ToDo: Restrict WS_POPUP window size
			//MINMAXINFO *pBounds = reinterpret_cast<MINMAXINFO*>(msg.lParam);
			//pBounds->ptMinTrackSize.x = this->Properties->MinMaxX;
			//pBounds->ptMinTrackSize.y = this->Properties->MinMaxY;
			break;
		}

		// Creation and Destruction
		case WM_CLOSE: {
			DestroyWindow(msg.hwnd);
			hWindow = nullptr;
			return 0;
		}
		case WM_CREATE: {
			Properties.State.Alive = true;
			return 0;
		}
		case WM_DESTROY: {
			Properties.State.Alive = false;
			PostQuitMessage(0);
			return 0;
		}

		// Drawing
		case WM_PAINT: {
			Properties.State.Decorated = true;

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(msg.hwnd, &ps);
			FillRect(hdc, &ps.rcPaint, (HBRUSH)CreateSolidBrush(RGB(0, 0, 0)));
			EndPaint(msg.hwnd, &ps);
			return 0;


			//PAINTSTRUCT ps;
			//HDC hdc = BeginPaint(msg.hwnd, &ps);

			/*
			BP_PAINTPARAMS params = {sizeof(params), BPPF_NOCLIP | BPPF_ERASE};
			HDC memdc;
			HPAINTBUFFER hbuffer =
			BeginBufferedPaint(hdc, &rc, BPBF_TOPDOWNDIB, &params, &memdc);

			HBRUSH brush = CreateSolidBrush(RGB(23, 26, 30));
			FillRect(hdc, &rc, brush);
			DeleteObject(brush);

			BufferedPaintSetAlpha(hbuffer, &rc, 255);
			EndBufferedPaint(hbuffer, TRUE);

			*/

			RECT ClientRect;
			RECT rc = ps.rcPaint;
			GetClientRect(msg.hwnd, &ClientRect);

			//RECT BorderRect = { pWindow->BorderWidth, pWindow->BorderWidth, ClientRect.right - pWindow->BorderWidth - pWindow->BorderWidth, ClientRect.bottom - pWindow->BorderWidth - pWindow->BorderWidth };
			//RECT TitleRect = { pWindow->BorderWidth, pWindow->BorderWidth, ClientRect.right - pWindow->BorderWidth - pWindow->BorderWidth, pWindow->TitleBarWidth };

			//HBRUSH BorderBrush = CreateSolidBrush(RGB(23, 26, 30));
			//FillRect(ps.hdc, &ClientRect, BorderBrush);
			//FillRect(ps.hdc, &BorderRect, BorderBrush);
			//FillRect(ps.hdc, &TitleRect, BorderBrush);
			//DeleteObject(BorderBrush);

			EndPaint(msg.hwnd, &ps);

		}
		case WM_MOVE: {
			// Store pointer to associated Window class as userdata in Win32 window
			//CREATESTRUCT *WindowInfo = reinterpret_cast<CREATESTRUCT *>(msg.lParam);
			//void *WindowClass = reinterpret_cast<Window *>(WindowInfo->lpCreateParams);
			//if (WindowClass = pWindow) {
			//	applog << "Same" << std::endl;
			//}
			//pWindow = reinterpret_cast<Window *>(WindowInfo->lpCreateParams);
			//SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
			//pWindow->hWindow = hWnd;
			//MoveWindow(msg.hwnd, WindowInfo->x, WindowInfo->y, WindowInfo->cx, WindowInfo->cy, TRUE);
			//MoveWindow(msg.hwnd, WindowInfo->x, WindowInfo->y, WindowInfo->cx - pWindow->BorderWidth, WindowInfo->cy - pWindow->BorderWidth, TRUE);
			//data.X = WindowInfo->x;
			//data.Y = WindowInfo->y;
			//data.Width = WindowInfo->cx - pWindow->BorderWidth;
			//data.Height = WindowInfo->cy - pWindow->BorderWidth;
			return 1;
		}
		case WM_SHOWWINDOW: {
			Properties.State.Visible = (bool)msg.wParam;
			return 0;
		}
		case WM_SIZE: {
			switch (msg.wParam) {
				case SIZE_MAXIMIZED: {
					Properties.State.Maximized = true;
					break;
				}
				case SIZE_MINIMIZED: {
					Properties.State.Minimized = true;
					break;
				}
				case SIZE_RESTORED: {
					Properties.State.Maximized = false;
					Properties.State.Minimized = false;
				}
			}
			Properties.Size.Width = static_cast<uint32_t>((UINT64)msg.lParam & 0xFFFF);
			Properties.Size.Height = static_cast<uint32_t>((UINT64)msg.lParam >> 16);
			return 0;
		}
		case WM_SIZING: {
			RedrawWindow(msg.hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE | RDW_NOERASE | RDW_INTERNALPAINT);
			return 0;
		}

		// State
		case WM_ACTIVATE: {
			Properties.State.Active = (bool)msg.wParam;
			return 0;
		}
		case WM_KILLFOCUS: {
			Properties.State.Focused = false;
			return 0;
		}
		case WM_SETFOCUS: {
			Properties.State.Focused = true;
			return 0;

			// ToDo: Reset mouse position
			POINT position;
			if (GetCursorPos(&position)) {
				//window->prevMouseX = static_cast<unsigned int>(position.x);
				//window->prevMouseY = static_cast<unsigned int>(position.y);
			}
		}

		// System
		case WM_SYSCOMMAND: {
			switch (msg.wParam) {
				// Prevent ScreenSaver or Monitor PowerSaver
				case SC_SCREENSAVE: case SC_MONITORPOWER: {
					return 0;
				}
			}
			break;
		}

		default: {
			break;
		}
	}
	return DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
}

void WinWindow::Update() {
	MSG message = {};
	while (PeekMessage(&message, hWindow, 0, 0, PM_NOREMOVE)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}


// Accessors
void *WinWindow::GetNativeWindow() const {
	return (void *)hWindow;
}

WindowProperties WinWindow::GetProperties() const {
	return Properties;
}

WindowSize WinWindow::GetContexttSize() const {
	RECT area;
	GetClientRect(hWindow, &area);

	return WindowSize(area.right - area.left, area.bottom - area.top);
}

WindowPosition WinWindow::GetDisplayPosition() const {
	//WINDOWPLACEMENT position;
	//GetWindowPlacement(hWindow, &position);
	//WindowPosition(position.ptMinPosition.x, position.ptMinPosition.y);
	return Properties.Position;
}

WindowSize WinWindow::GetDisplaySize() const {
	//RECT area;
	//GetWindowRect(hWindow, &area);
	//WindowSize(area.right - area.left, area.bottom - area.top);
	return Properties.Size;
}

WindowSize WinWindow::GetScreentSize() const {
	int32_t width = GetSystemMetrics(SM_CXSCREEN);
	int32_t height = GetSystemMetrics(SM_CYSCREEN);

	return WindowSize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

string WinWindow::GetTitle() const {
	//char title[1024];
	//memset(title, 0, sizeof(char) * 1024);
	//GetWindowText(hWindow, title, sizeof(char) * 1024);
	return Properties.Title;
}


// Modifiers
void WinWindow::SetProperties(const WindowProperties &properties) {
	string id = Properties.ID;
	Properties = properties;
	Properties.ID = id;

	SetDisplayPosition(properties.Position.X, properties.Position.Y);
	SetDisplaySize(properties.Size.Width, properties.Size.Height);
	SetTitle(properties.Title);
}

void WinWindow::SetCursorPosition(const int32_t x, const int32_t y) {
	SetCursorPos(x, y);
}

void WinWindow::SetDisplayPosition(const int32_t x, const int32_t y) {
	Properties.Position.X = x;
	Properties.Position.Y = y;

	SetWindowPos(hWindow, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void WinWindow::SetDisplaySize(const uint32_t width, const uint32_t height) {
	WINDOWPLACEMENT position;
	GetWindowPlacement(hWindow, &position);

	RECT WindowDimension;
	WindowDimension.left = position.ptMinPosition.x;
	WindowDimension.top = position.ptMinPosition.y;
	WindowDimension.right = (long)width;
	WindowDimension.bottom = (long)height;

	//AdjustWindowRectEx(&WindowDimension, Properties.Style.WindowStyle, FALSE, Properties.Style.WindowStyleEx);
	//AdjustWindowRectEx(&WindowDimension, Style2.WindowStyle, FALSE, Style2.WindowStyleEx);	//SWP_NOMOVE | SWP_NOZORDER
}

void WinWindow::SetProgress(const float progress) {
	static uint32_t max = 100;
	uint32_t current = (uint32_t)(progress * (float)max);

	TaskbarList->SetProgressValue(hWindow, current, max);
}

void WinWindow::SetTitle(const string_view title) {
	Properties.Title = title;

	SetWindowText(hWindow, title.data());
}

}