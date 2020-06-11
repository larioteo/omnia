#include "WinWindow.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <dwmapi.h>
#include <ShObjIdl.h>
#include <Windows.h>
#include <windowsx.h>

#include "Omnia/Log.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

namespace Omnia {

static intptr_t __stdcall MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void ShowDebug(unsigned int message, bool window = false, bool keyboard = false, bool mouse = false, bool system = false);

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

HINSTANCE hApplication;
HWND hWindow;
HICON AppIcon;
RECT WindowDimension;
ITaskbarList3 *TaskbarList;

struct PlatformWindowStyle {
	uint32_t WindowStyle;
	uint32_t WindowStyleEx;
};


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

	// Load Ressources
	//AppIcon = (HICON)LoadIconFile(Properties.Icon);

	// Register Window Class
	WNDCLASSEX classProperties = {
		.cbSize	= sizeof(WNDCLASSEX),					// Structure Size (in bytes)
		.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,	// Seperate device context for window and redraw on move
		.lpfnWndProc = MessageHandler,					// Message Handler (WndProc)
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
	pCurrentWindow = this;
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
	// Switch back to default mode
	if (Properties.Style == WindowStyle::FullScreen) {
		ChangeDisplaySettings(NULL, NULL);
		ShowCursor(true);
	}
	if (!DestroyWindow(hWindow)) {
		hWindow = nullptr;
	} else {
		applog << Log::Error << "Could not release handle to window.\n";
	}
	if (!UnregisterClass(Properties.ID.c_str(), hApplication)) {
		hApplication = nullptr;
	} else {
		applog << Log::Error << "Could not unregister window class.\n";
	}
}


void WinWindow::Update() {
	MSG message = {};
	while (PeekMessage(&message, hWindow, 0, 0, PM_NOREMOVE)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}

intptr_t __stdcall MessageHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	WinWindow *pWindow = nullptr;
	if (pCurrentWindow != nullptr) {
		pWindowList.emplace(hWnd, pCurrentWindow);
		hWindow = hWnd;
		pWindow = pCurrentWindow;
		pCurrentWindow = nullptr;

	} else {
		auto window = pWindowList.find(hWnd);
		pWindow = window->second;
	}

	MSG message = {
		.hwnd = hWindow,
		.message = uMsg,
		.wParam = wParam,
		.lParam = lParam,
	};
	return pWindow->Message(&message);
}

intptr_t WinWindow::Message(void *event) {
	// Publish events to an external event system
	if (!EventCallback.Empty()) EventCallback.Publish(event);

	MSG &msg = *(MSG*)(event);
	LRESULT result = 0;
	//static uint64_t counter = 0;
	//counter++;
	//std::cout << counter << ":" << msg.message << std::endl;

	/**
	* @brief	Window Messages and Notifications
	* @source	https://docs.microsoft.com/en-us/windows/win32/winmsg/window-messages
	*			https://docs.microsoft.com/en-us/windows/win32/winmsg/window-notifications
	*			https://docs.microsoft.com/en-us/windows/win32/inputdev/keyboard-input-notifications
	*			https://docs.microsoft.com/en-us/windows/win32/gdi/painting-and-drawing-messages
	*/
	switch (msg.message) {
		// Window Messages
		case WM_ERASEBKGND:	{
			break;
		}
		case WM_GETMINMAXINFO: {
			//WindowEventData data;
			//data.Action = WindowAction::Resize;

			//// It is used to restrict WS_POPUP window size
			//MINMAXINFO *pBounds = reinterpret_cast<MINMAXINFO *>(msg.lParam);

			////pBounds->ptMinTrackSize.x = window->InfoMinMaxX;
			////pBounds->ptMinTrackSize.y = window->InfoMinMaxY;

			//WindowEvent.Publish(data);
			break;
		}
		
		// Window Notifications
		case WM_NULL: {
			break;
		}
		
		case WM_CLOSE: {
			DestroyWindow(msg.hwnd);
			return 0;
		}
		case WM_CREATE: {
			//WinWindow *pWindow = nullptr;
			Properties.State.Alive = true;
			// Store pointer to associated Window class as userdata in Win32 window
			//CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(msg.lParam);
			//pWindow = pCreate->lpCreateParams;
			//SetWindowLongPtr(msg.hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
			//pHandle = msg.hwnd;
			//pWindow->hWindow = hWnd;

			//MoveWindow(msg.hwnd, WindowInfo->x, WindowInfo->y, WindowInfo->cx, WindowInfo->cy, TRUE);

			// Store pointer to associated Window class as userdata in Win32 window
			//CREATESTRUCT *WindowInfo = reinterpret_cast<CREATESTRUCT *>(msg.lParam);
			//void *WindowClass = reinterpret_cast<Window *>(WindowInfo->lpCreateParams);
			//if (WindowClass = pWindow) {
			//	applog << "Same" << std::endl;
			//}
			//pWindow = reinterpret_cast<Window *>(WindowInfo->lpCreateParams);
			//SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
			//pWindow->hWindow = hWnd;

			//MoveWindow(msg.hwnd, WindowInfo->x, WindowInfo->y, WindowInfo->cx - pWindow->BorderWidth, WindowInfo->cy - pWindow->BorderWidth, TRUE);
			//data.X = WindowInfo->x;
			//data.Y = WindowInfo->y;
			//data.Width = WindowInfo->cx - pWindow->BorderWidth;
			//data.Height = WindowInfo->cy - pWindow->BorderWidth;


			// Repaint window when borderless to avoid 6px resizable border.
			break;
		}
		case WM_DESTROY: {
			Properties.State.Alive = false;
			PostQuitMessage(0);
			return 0;
		}
		
		case WM_DPICHANGED: {
			break;
		}
		case WM_MOVE: {
			break;
		}
		case WM_SHOWWINDOW: {
			Properties.State.Visible = (bool)msg.wParam;
			break;
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
			//data.Width = static_cast<unsigned>((UINT64)msg.lParam & 0xFFFF);
			//data.Height = static_cast<unsigned>((UINT64)msg.lParam >> 16);
			//Resize(LOWORD(lParam), HIWORD(lParam));
			break;
		}
		case WM_SIZING: {
			RedrawWindow(msg.hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE | RDW_NOERASE | RDW_INTERNALPAINT);
			break;
		}
		
		// Keyboard Input Notifications
		case WM_ACTIVATE: {
			Properties.State.Active = (bool)msg.wParam;
			break;
		}
		case WM_KILLFOCUS: {
			Properties.State.Focused = false;
			break;
		}
		case WM_SETFOCUS: {
			Properties.State.Focused = true;

			// ToDo: Reset mouse position
			POINT position;
			if (GetCursorPos(&position)) {
				//window->prevMouseX = static_cast<unsigned int>(position.x);
				//window->prevMouseY = static_cast<unsigned int>(position.y);
			}

			break;
		}
		
		// Painting and Drawing Messages
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

		// System
		//case WM_NCCALCSIZE: {
		//	//if (msg.lParam) {
		//	//	NCCALCSIZE_PARAMS *sz = (NCCALCSIZE_PARAMS *)msg.lParam;
		//	//	if (msg.wParam) {
		//	//		sz->rgrc[0].bottom +=
		//	//			window->BorderWidth;  // rgrc[0] is what makes this work, don't know
		//	//						  // what others (rgrc[1], rgrc[2]) do, but why
		//	//						  // not change them all?
		//	//		sz->rgrc[0].right += window->BorderWidth;
		//	//		sz->rgrc[1].bottom += window->BorderWidth;
		//	//		sz->rgrc[1].right += window->BorderWidth;
		//	//		sz->rgrc[2].bottom += window->BorderWidth;
		//	//		sz->rgrc[2].right += window->BorderWidth;
		//	//		return 0;
		//	//	}
		//	//}
		//	break;
		//}
		//case WM_NCHITTEST: {
		//	//RECT WindowRect;
		//	//int x, y;
		//	//GetWindowRect(msg.hwnd, &WindowRect);
		//	//x = GET_X_LPARAM(msg.lParam) - WindowRect.left;
		//	//y = GET_Y_LPARAM(msg.lParam) - WindowRect.top;
		//	//if (x >= pWindow->BorderWidth && x <= WindowRect.right - WindowRect.left - pWindow->BorderWidth && y >= pWindow->BorderWidth && y <= pWindow->TitleBarWidth)
		//	//	result = HTCAPTION;
		//	//else if (x < pWindow->BorderWidth && y < pWindow->BorderWidth)
		//	//	result = HTTOPLEFT;
		//	//else if (x > WindowRect.right - WindowRect.left - pWindow->BorderWidth && y < pWindow->BorderWidth)
		//	//	result = HTTOPRIGHT;
		//	//else if (x > WindowRect.right - WindowRect.left - pWindow->BorderWidth && y > WindowRect.bottom - WindowRect.top - pWindow->BorderWidth)
		//	//	result = HTBOTTOMRIGHT;
		//	//else if (x < pWindow->BorderWidth && y > WindowRect.bottom - WindowRect.top - pWindow->BorderWidth)
		//	//	result = HTBOTTOMLEFT;
		//	//else if (x < pWindow->BorderWidth)
		//	//	result = HTLEFT;
		//	//else if (y < pWindow->BorderWidth)
		//	//	result = HTTOP;
		//	//else if (x > WindowRect.right - WindowRect.left - pWindow->BorderWidth)
		//	//	result = HTRIGHT;
		//	//else if (y > WindowRect.bottom - WindowRect.top - pWindow->BorderWidth)
		//	//	result = HTBOTTOM;
		//	//else
		//	//	result = HTCLIENT;
		//	//break;
		//}
		//case WM_SYSCOMMAND: {
		//	// Prevent ScreenSaver or Monitor PowerSaver
		//	//switch (msg.wParam) {
		//	//	case SC_SCREENSAVE:
		//	//	case SC_MONITORPOWER:
		//	//		return 0;
		//	//}
		//	break;
		//}

		// Anything else
		default: {
			return DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
		}
	}
	return result;
}


// Accessors
void *WinWindow::GetNativeHandle() const {
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
	WINDOWPLACEMENT position;
	GetWindowPlacement(hWindow, &position);

	return WindowPosition(position.ptMinPosition.x, position.ptMinPosition.y);
}

WindowSize WinWindow::GetDisplaySize() const {
	RECT area;
	GetWindowRect(hWindow, &area);

	return WindowSize(area.right - area.left, area.bottom - area.top);
}

WindowSize WinWindow::GetScreentSize() const {
	long screenWidth = GetSystemMetrics(SM_CXSCREEN);
	long screenHeight = GetSystemMetrics(SM_CYSCREEN);
	
	return WindowSize(static_cast<unsigned>(screenWidth), static_cast<unsigned>(screenHeight));
}

string WinWindow::GetTitle() const {
	char title[1024];
	memset(title, 0, sizeof(char) * 1024);
	GetWindowTextA(hWindow, title, sizeof(char) * 1024);

	return title;
}


// Modifiers
void WinWindow::SetProperties(const WindowProperties &properties) {
	//Properties = properties;
	WindowDimension.left = Properties.Position.X;
	WindowDimension.top = Properties.Position.Y;
	WindowDimension.right = (long)properties.Size.Width;
	WindowDimension.bottom = (long)properties.Size.Height;

	//AdjustWindowRectEx(&WindowDimension, Properties.Style.WindowStyle, FALSE, Properties.Style.WindowStyleEx);
	//SetWindowPos(hWindow, 0, Properties.Position.X, Properties.Position.Y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

void WinWindow::SetCursorPosition(long x, long y) {
	SetCursorPos(x, y);
}

void WinWindow::SetDisplayPosition(const int x, const int y) {
	Properties.Position.X = x;
	Properties.Position.Y = y;

	SetWindowPos(hWindow, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void WinWindow::SetDisplaySize(const unsigned int width, const unsigned int height) {
	WINDOWPLACEMENT position;
	GetWindowPlacement(hWindow, &position);

	WindowDimension.left = position.ptMinPosition.x;
	WindowDimension.top = position.ptMinPosition.y;
	WindowDimension.right = (long)width;
	WindowDimension.bottom = (long)height;

	//AdjustWindowRectEx(&WindowDimension, Style2.WindowStyle, FALSE, Style2.WindowStyleEx);	//SWP_NOMOVE | SWP_NOZORDER
}

void WinWindow::SetProgress(float progress) {
	unsigned max = 10000;
	unsigned cur = (unsigned)(progress * (float)max);

	TaskbarList->SetProgressValue(hWindow, cur, max);
}

void WinWindow::SetTitle(const std::string &title) {
	Properties.Title = title;

	SetWindowText(hWindow, title.c_str());
}


// Debugging
static void ShowDebug(unsigned int message, bool window, bool keyboard, bool mouse, bool system) {
	// Show Window Events
	if (window) {
		switch (message) {
			// Window Messages
			case MN_GETHMENU:				applog << "MN_GETHMENU\n";					break;
			case WM_GETFONT:				applog << "WM_GETFONT\n";					break;
			case WM_GETTEXT:				applog << "WM_GETTEXT\n";					break;
			case WM_GETTEXTLENGTH:			applog << "WM_GETTEXTLENGTH\n";				break;
			case WM_SETFONT:				applog << "WM_SETFONT\n";					break;
			case WM_SETICON:				applog << "WM_SETICON\n";					break;
			case WM_SETTEXT:				applog << "WM_SETTEXT\n";					break;

				// Window Notifications
			case WM_ACTIVATEAPP:			applog << "WM_ACTIVATEAPP\n";				break;
			case WM_CANCELMODE:				applog << "WM_CANCELMODE\n";				break;
			case WM_CHILDACTIVATE:			applog << "WM_CHILDACTIVATE\n";				break;
			case WM_CLOSE:					applog << "WM_CLOSE\n";						break;
			case WM_COMPACTING:				applog << "WM_CLOSE\n";						break;
			case WM_DESTROY:				applog << "WM_DESTROY\n";					break;
			case WM_DPICHANGED:				applog << "WM_DPICHANGED\n";				break;
			case WM_ENABLE:					applog << "WM_ENABLE\n";					break;
			case WM_ENTERSIZEMOVE:			applog << "WM_ENTERSIZEMOVE\n";				break;
			case WM_EXITSIZEMOVE:			applog << "WM_EXITSIZEMOVE\n";				break;
			case WM_GETICON:				applog << "WM_EXITSIZEMOVE\n";				break;
			case WM_GETMINMAXINFO:			applog << "WM_GETMINMAXINFO\n";				break;
			case WM_INPUTLANGCHANGE:		applog << "WM_INPUTLANGCHANGE\n";			break;
			case WM_INPUTLANGCHANGEREQUEST:	applog << "WM_INPUTLANGCHANGEREQUEST\n";	break;
			case WM_MOVE:					applog << "WM_MOVE\n";						break;
			case WM_MOVING:					applog << "WM_MOVING\n";					break;
			case WM_NCACTIVATE:				applog << "WM_NCACTIVATE\n";				break;
			case WM_NCCALCSIZE:				applog << "WM_NCCALCSIZE\n";				break;
			case WM_NCCREATE:				applog << "WM_NCCREATE\n";					break;
			case WM_NCDESTROY:				applog << "WM_NCDESTROY\n";					break;
			case WM_NULL:					applog << "WM_NULL\n";						break;
			case WM_QUERYDRAGICON:			applog << "WM_QUERYDRAGICON\n";				break;
			case WM_QUERYOPEN:				applog << "WM_QUERYOPEN\n";					break;
			case WM_QUIT:					applog << "WM_QUIT\n";						break;
			case WM_SHOWWINDOW:				applog << "WM_SHOWWINDOW\n";				break;
			case WM_SIZE:					applog << "WM_SIZE\n";						break;
			case WM_SIZING:					applog << "WM_SIZING\n";					break;
			case WM_STYLECHANGED:			applog << "WM_STYLECHANGED\n";				break;
			case WM_STYLECHANGING:			applog << "WM_STYLECHANGING\n";				break;
			case WM_THEMECHANGED:			applog << "WM_THEMECHANGED\n";				break;
			case WM_USERCHANGED:			applog << "WM_USERCHANGED\n";				break;
			case WM_WINDOWPOSCHANGED:		applog << "WM_WINDOWPOSCHANGED\n";			break;
			case WM_WINDOWPOSCHANGING:		applog << "WM_WINDOWPOSCHANGING\n";			break;
		}
	}

	// Show Keyboard Events
	if (keyboard) {
		switch (message) {
			// Keyboard Messages
			case WM_GETHOTKEY:				applog << "WM_GETHOTKEY\n";					break;
			case WM_SETHOTKEY:				applog << "WM_SETHOTKEY\n";					break;

				// Keyboard Input Notifications
			case WM_ACTIVATE:				applog << "WM_ACTIVATE\n";					break;
			case WM_APPCOMMAND:				applog << "WM_APPCOMMAND\n";				break;
			case WM_CHAR:					applog << "WM_CHAR\n";						break;
			case WM_DEADCHAR:				applog << "WM_DEADCHAR\n";					break;
			case WM_HOTKEY:					applog << "WM_HOTKEY\n";					break;
			case WM_KEYDOWN:				applog << "WM_KEYDOWN\n";					break;
			case WM_KEYUP:					applog << "WM_KEYUP\n";						break;
			case WM_KILLFOCUS:				applog << "WM_KILLFOCUS\n";					break;
			case WM_SETFOCUS:				applog << "WM_SETFOCUS\n";					break;
			case WM_SYSDEADCHAR:			applog << "WM_SYSDEADCHAR\n";				break;
			case WM_SYSKEYDOWN:				applog << "WM_SYSKEYDOWN\n";				break;
			case WM_SYSKEYUP:				applog << "WM_SYSKEYUP\n";					break;
			case WM_UNICHAR:				applog << "WM_UNICHAR\n";					break;
		}
	}

	// Show Mouse Events
	if (mouse) {
		switch (message) {
			// Mouse Input Notifications
			case WM_CAPTURECHANGED:			applog << "WM_CAPTURECHANGED\n";				break;
			case WM_LBUTTONDBLCLK:			applog << "WM_LBUTTONDBLCLK\n";					break;
			case WM_LBUTTONDOWN:			applog << "WM_LBUTTONDOWN\n";					break;
			case WM_LBUTTONUP:				applog << "WM_LBUTTONUP\n";						break;
			case WM_MBUTTONDBLCLK:			applog << "WM_MBUTTONDBLCLK\n";					break;
			case WM_MBUTTONDOWN:			applog << "WM_MBUTTONDOWN\n";					break;
			case WM_MBUTTONUP:				applog << "WM_MBUTTONUP\n";						break;
			case WM_MOUSEACTIVATE:			applog << "WM_MOUSEACTIVATE\n";					break;
			case WM_MOUSEHOVER:				applog << "WM_MOUSEHOVER\n";					break;
			case WM_MOUSEHWHEEL:			applog << "WM_MOUSEHWHEEL\n";					break;
			case WM_MOUSELEAVE:				applog << "WM_MOUSELEAVE\n";					break;
			case WM_MOUSEMOVE:				applog << "WM_MOUSEMOVE\n";						break;
			case WM_MOUSEWHEEL:				applog << "WM_MOUSEWHEEL\n";					break;
			case WM_RBUTTONDBLCLK:			applog << "WM_RBUTTONDBLCLK\n";					break;
			case WM_RBUTTONDOWN:			applog << "WM_RBUTTONDOWN\n";					break;
			case WM_RBUTTONUP:				applog << "WM_RBUTTONUP\n";						break;
			case WM_XBUTTONDBLCLK:			applog << "WM_XBUTTONDBLCLK\n";					break;
			case WM_XBUTTONDOWN:			applog << "WM_XBUTTONDOWN\n";					break;
			case WM_XBUTTONUP:				applog << "WM_XBUTTONUP\n";						break;
		}
	}

	if (system) {
		switch (message) {
			// Painting and Drawing Messages
			case WM_DISPLAYCHANGE:			applog << "WM_DISPLAYCHANGE\n";				break;
			case WM_ERASEBKGND:				applog << "WM_ERASEBKGND\n";				break;
			case WM_NCPAINT:				applog << "WM_NCPAINT\n";					break;
			case WM_PAINT:					applog << "WM_PAINT\n";						break;
			case WM_PRINT:					applog << "WM_PRINT\n";						break;
			case WM_SETREDRAW:				applog << "WM_SETREDRAW\n";					break;
			case WM_SYNCPAINT:				applog << "WM_SYNCPAINT\n";					break;
		}
	}
}

}
