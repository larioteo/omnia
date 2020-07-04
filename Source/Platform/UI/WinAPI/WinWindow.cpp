#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#undef APIENTRY
#include <DwmApi.h>
#include <ShObjIdl.h>
#include <Windows.h>
#include <WindowsX.h>

#include "Omnia/Log.h"
#include "WinWindow.h"


namespace Omnia {

// Internal Callbacks
static LRESULT CALLBACK MessageCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Internal Properties
HINSTANCE hApplication;
HWND hWindow;
HICON AppIcon;
ITaskbarList3 *TaskbarList;
HBRUSH ClearColor = (HBRUSH)GetStockObject(NULL_BRUSH);

// Internal Styles
enum class ClassStyle: uint32_t {
	Application	= CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
	Global		= CS_GLOBALCLASS | CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
};

enum class Styles: uint32_t {
	// NEHE SetWindowLongPtr(window, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	Default = WS_OVERLAPPEDWINDOW,		// Contains: WS_SYSMENU | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE
	Aero = WS_POPUP | WS_THICKFRAME,
	Borderless = WS_POPUP | WS_VISIBLE,
	Full = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_VISIBLE
};

enum class StylesX: uint32_t {
	// WindowStyleEx &= (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)
	// NEHE SetWindowLongPtr(window, GWL_EXSTYLE, WS_EX_APPWINDOW);
	DefaultX = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
	FullX = WS_EX_APPWINDOW
};

struct PlatformWindowStyle {
	uint32_t ClassStyle;
	uint32_t WindowStyle;
	uint32_t WindowStyleEx;
};


// Default
WinWindow::WinWindow(const WindowProperties &properties):
	Properties{ properties } {
	// Properties
	PlatformWindowStyle windowStyle = {};

	// Get Application Information
	hApplication = GetModuleHandle(NULL);
	LPSTR lpCmdLine = GetCommandLine();
	STARTUPINFOA StartupInfo;
	GetStartupInfoA(&StartupInfo);

	// Settings
	windowStyle.ClassStyle = (DWORD)ClassStyle::Application;
	//SetThreadExecutionState(ES_DISPLAY_REQUIRED && ES_SYSTEM_REQUIRED);

	// Load Ressources
	//AppIcon = (HICON)LoadIconFile(Properties.Icon);

	// Register Window Class
	WNDCLASSEX classProperties = {
		.cbSize	= sizeof(WNDCLASSEX),			// Structure Size (in bytes)
		.style = windowStyle.ClassStyle,		// Seperate device context for window and redraw on move
		.lpfnWndProc = MessageCallback,			// Message Callback (WndProc)
		.cbClsExtra = 0,						// Extra class data
		.cbWndExtra = WS_EX_TOPMOST,			// Extra window data
		.hInstance = hApplication,				// Application Intance
		.hIcon = AppIcon,						// Load Icon (Default: LoadIcon(NULL, IDI_APPLICATION);)
		.hCursor = LoadCursor(NULL, IDC_ARROW),	// Load Cursor (Default: IDC_ARROW)
		.hbrBackground = ClearColor,			// Background (Not required for GL -> NULL)
		.lpszMenuName = NULL,					// We Don't Want A Menu
		.lpszClassName = Properties.ID.c_str(),	// Class Name should be unique per window
		.hIconSm = AppIcon,						// Load Icon Symbol (Default: LoadIcon(NULL, IDI_WINLOGO);)
	};
	if (!RegisterClassEx(&classProperties)) {
		AppLogCritical("[Window]: ", "Failed to register the window class!");
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
					Properties.Style = WindowStyle::Default;
				} else {
					AppLogCritical("[Window]: ", "Switching to fullscreen mode failed!");
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
		AppLogCritical("[Window]: ", "Failed to create the window!");
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
	static const DWM_BLURBEHIND blur {0, TRUE, NULL, TRUE};
	static const MARGINS shadow[2] {{0, 0, 0, 0}, {1, 1, 1, 1}};
	DwmEnableBlurBehindWindow(hWindow, &blur);
	DwmExtendFrameIntoClientArea(hWindow, &shadow[0]);

	// Taskbar Settings
	RegisterWindowMessage("TaskbarButtonCreated");
	HRESULT hrf = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (LPVOID *)&TaskbarList);

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
LRESULT CALLBACK MessageCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// Properties
	WinWindow *pCurrentWindow = nullptr;

	// Get/Set the current window class pointer as userdata in WinAPI window
	switch (uMsg) {
		case WM_NCCREATE: {
			pCurrentWindow = static_cast<WinWindow*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
			SetLastError(0);
			if (!SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCurrentWindow))) {
				if (GetLastError() != 0) { return FALSE; }
			}
			break;
		}

		default: {
			pCurrentWindow = reinterpret_cast<WinWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

			// Capture Mouse while button is down
			if (uMsg == WM_LBUTTONDOWN) SetCapture(hWnd);
			if (uMsg == WM_LBUTTONUP) ReleaseCapture();
			break;
		}
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
	MSG &msg = *(reinterpret_cast<MSG *>(event));
	//HWND &hWnd = msg.hwnd;
	//UINT &uMsg = msg.message;
	//WPARAM &wParam = msg.wParam;
	//LPARAM &lParam = msg.lParam;

	// Publish events to an external event system
	if (!EventCallback.Empty()) { EventCallback.Publish(event); }

	/**
	* @brief	Window Messages
	* @source	https://docs.microsoft.com/en-us/windows/win32/winmsg/window-messages
	*			https://docs.microsoft.com/en-us/windows/win32/winmsg/window-notifications
	*			https://docs.microsoft.com/en-us/windows/win32/inputdev/keyboard-input-notifications
	*			https://docs.microsoft.com/en-us/windows/win32/gdi/painting-and-drawing-messages
	*/
	// Process important window related events internally
	switch (msg.message) {
		// Pre-Check: Does this message belong to the current window?
		case WM_NULL: {
			// Note: Could be usefull in the future.
			break;
		}
		
		// Preparation
		case WM_NCCREATE: {
			// Note: Could be usefull in the future.
			break;
		}
		case WM_NCDESTROY : {
			// Note: Could be usefull in the future.
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

		 // Information
		case WM_DPICHANGED: {
			// Note: Could be usefull in the future.
			break;
		}
		case WM_GETMINMAXINFO: {
			if (Properties.Style == WindowStyle::FullScreen) {
				break;
			} else {
				MINMAXINFO *bounds = reinterpret_cast<MINMAXINFO *>(msg.lParam);
				if (this->Properties.MaxSize.Width > 0 && this->Properties.MaxSize.Height > 0) {
					bounds->ptMaxTrackSize.x = this->Properties.MaxSize.Width;
					bounds->ptMaxTrackSize.y = this->Properties.MaxSize.Height;
				}
				if (this->Properties.MinSize.Width > 0 && this->Properties.MinSize.Height > 0) {
					bounds->ptMinTrackSize.x = this->Properties.MinSize.Width;
					bounds->ptMinTrackSize.y = this->Properties.MinSize.Height;
				}
				return 0;
			}
		}

		// Drawing
		case WM_PAINT: {
			Properties.State.Decorated = true;
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(msg.hwnd, &ps);
			FillRect(hdc, &ps.rcPaint, ClearColor);
			EndPaint(msg.hwnd, &ps);
			return 0;
		}
		case WM_MOVE: {
			// Note: Could be usefull in the future.
			break;
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
			RedrawWindow(msg.hwnd, NULL, NULL, RDW_UPDATENOW | RDW_NOERASE);
			break;
		}

		// State
		case WM_ACTIVATE: {
			Properties.State.Active = (bool)msg.wParam;
			return 0;
		}
		case WM_CAPTURECHANGED: {
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
			// Disable ALT application menu
			if ((msg.wParam & 0xfff0) == SC_KEYMENU) return 0;

			switch (msg.wParam) {
				// FulLScree-Mode: Prevent ScreenSaver or Monitor PowerSaver
				case SC_SCREENSAVE: case SC_MONITORPOWER: {
					if (Properties.Style == WindowStyle::FullScreen) {
						return 0;
					} else {
						break;
					}
				}

				default: {
					break;
				}
			}
		}
		
		// Default: Currently nothing of interest
		default: {
			break;
		}
	}
	return DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
}

void WinWindow::Update() {
	MSG msg = {};
	while (PeekMessage(&msg, hWindow, 0, 0, PM_NOREMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}


// Accessors
void *WinWindow::GetNativeWindow() const {
	return (void *)hWindow;
}

const WindowProperties WinWindow::GetProperties() const {
	return Properties;
}

const WindowSize WinWindow::GetContexttSize() const {
	RECT area;
	GetClientRect(hWindow, &area);

	return WindowSize(area.right - area.left, area.bottom - area.top);
}

const WindowPosition WinWindow::GetDisplayPosition() const {
	//WINDOWPLACEMENT position;
	//GetWindowPlacement(hWindow, &position);
	//WindowPosition(position.ptMinPosition.x, position.ptMinPosition.y);
	return Properties.Position;
}

const WindowSize WinWindow::GetDisplaySize() const {
	//RECT area;
	//GetWindowRect(hWindow, &area);
	//WindowSize(area.right - area.left, area.bottom - area.top);
	return Properties.Size;
}

const WindowSize WinWindow::GetScreentSize() const {
	int32_t width = GetSystemMetrics(SM_CXSCREEN);
	int32_t height = GetSystemMetrics(SM_CYSCREEN);

	return WindowSize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

const string WinWindow::GetTitle() const {
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
