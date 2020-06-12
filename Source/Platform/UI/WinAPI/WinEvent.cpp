#include <windowsx.h>
#include <ShObjIdl.h>
#include <dwmapi.h>

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#ifndef HID_USAGE_PAGE_GENERIC
	#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
	#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

#include "Omnia/Log.h"

#include "WinEvent.h"
#include "WinWindow.h"

namespace Omnia {

// Properties
HWND pWindow;
RAWINPUTDEVICE RawInputDevice[1];


// Default
WinEventListener::WinEventListener() {}
WinEventListener::~WinEventListener() {}


// Events
bool WinEventListener::Callback(void *event) {
	intptr_t result = Register(event);
	if (!result) { return true; }
	return false;
}

void WinEventListener::Update() {
	MSG message = {};
	while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}

intptr_t WinEventListener::Register(void *event) {
	static MSG msg; // = *reinterpret_cast<MSG *>(event);
	try {
		msg = *(MSG *)(event);
	} catch (...) {
		return (LRESULT)0;
	}
	UINT uMsg = msg.message;
	LRESULT result = 0;

	static bool Initialized = false;
	if (!Initialized) {
		Initialized = true;
		RawInputDevice[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
		RawInputDevice[0].usUsage = HID_USAGE_GENERIC_MOUSE;
		RawInputDevice[0].dwFlags = RIDEV_INPUTSINK;
		RawInputDevice[0].hwndTarget = msg.hwnd;
		RegisterRawInputDevices(RawInputDevice, 1, sizeof(RawInputDevice[0]));
	}

	// Do the magic
	// Sources:
	// - Raw Input:			https://docs.microsoft.com/en-us/windows/win32/inputdev/raw-input-notifications
	// - Keyboard Input:	
	// - Mouse Input:		https://docs.microsoft.com/en-us/windows/win32/inputdev/mouse-input-notifications
	// Pre-Flight-Test: Check if there are any observers in the goups, so the user gets only what he needs.
	switch (uMsg) {
		/**
		 *	Input Events
		*/
		// Raw (must be requested by HighPrecision flag)
		case WM_INPUT: {
			if (MouseEvent.Empty()) break; // || KeyboardEvent.Empty()
			break;
			static int32_t lastX = 0;
			static int32_t lastY = 0;

			MouseEventData data;
			data.Action = MouseAction::Raw;
			data.LastX = lastX;
			data.LastY = lastY;
			UINT dwSize = 40;
			static BYTE lpb[40];

			// Get Data
			uint32_t result = GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
			//if (result != dwSize) OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));
			RAWINPUT *raw = (RAWINPUT *)lpb;

			// Extract raw data
			switch (raw->header.dwType) {
				case RIM_TYPEKEYBOARD: {
					break;
				}
				case RIM_TYPEMOUSE: {
					data.X = raw->data.mouse.lLastX;
					data.Y = raw->data.mouse.lLastY;
					break;
				}
			}
			data.DeltaX = data.X - data.LastX;
			data.DeltaY = data.Y - data.LastY;
			lastX = data.X;
			lastY = data.Y;

			// Finalization
			MouseEvent.Publish(data);
			break;
		}
		
		// Keyboard
		case WM_KEYDOWN:	case WM_SYSKEYDOWN:
		case WM_KEYUP:		case WM_SYSKEYUP: {
			if (KeyboardEvent.Empty()) return result;

			// Perparation
			KeyboardEventData data;
			data.Action = KeyboardAction::Default;

			// Get Key State
			switch (msg.message) {
				case WM_KEYDOWN:	case WM_SYSKEYDOWN: {
					data.State = KeyState::Press;
					break;
				}
				case WM_KEYUP:		case WM_SYSKEYUP: {
					data.State = KeyState::Release;
					break;
				}
				default: {
					data.State = KeyState::Null;
					break;
				}
			}

			// Get Key Code
			data.Key = KeyCode{ msg.wParam };
			
			// Finalization
			KeyboardEvent.Publish(data);
			result = 0;
			break;
		}
		
		// Mouse
		case WM_LBUTTONDBLCLK:		case WM_MBUTTONDBLCLK:		case WM_RBUTTONDBLCLK:		case WM_XBUTTONDBLCLK: {
			if (MouseEvent.Empty()) return result;

			MouseEventData data;
			data.Action = MouseAction::DoubleClick;

			switch (msg.message) {
				case WM_LBUTTONDBLCLK: {
					data.Button = MouseButton::Left;
					break;
				}
				case WM_MBUTTONDBLCLK: {
					data.Button = MouseButton::Middle;
					break;
				}
				case WM_RBUTTONDBLCLK: {
					data.Button = MouseButton::Right;
					break;
				}
				case WM_XBUTTONDBLCLK: {
					short button = HIWORD(msg.wParam);
					data.Button = MouseButton(button & XBUTTON1 ? MouseButton::X1 : MouseButton::X2);
				}
				default: {
					data.Button = MouseButton::Undefined;
					break;
				}
			}
			
			data.State = ButtonState::Press;

			MouseEvent.Publish(data);
			break;
		}
		case WM_LBUTTONDOWN:		case WM_MBUTTONDOWN:		case WM_RBUTTONDOWN:		case WM_XBUTTONDOWN:
		case WM_LBUTTONUP:			case WM_MBUTTONUP:			case WM_RBUTTONUP:			case WM_XBUTTONUP: {
			if (MouseEvent.Empty()) return result;

			MouseEventData data;
			data.Action = MouseAction::Click;

			switch (msg.message) {
				case WM_LBUTTONDOWN:
				case WM_LBUTTONUP: {
					data.Button = MouseButton::Left;
					data.State = (msg.message == WM_LBUTTONDOWN ? ButtonState::Press : ButtonState::Release);
					break;
				}
				case WM_MBUTTONDOWN:
				case WM_MBUTTONUP: {
					data.Button = MouseButton::Middle;
					data.State = (msg.message == WM_MBUTTONDOWN ? ButtonState::Press : ButtonState::Release);
					break;
				}
				case WM_RBUTTONDOWN:
				case WM_RBUTTONUP: {
					data.Button = MouseButton::Right;
					data.State = (msg.message == WM_RBUTTONDOWN ? ButtonState::Press : ButtonState::Release);
					break;
				}
				case WM_XBUTTONDOWN:
				case WM_XBUTTONUP: {
					short button = HIWORD(msg.wParam);
					data.Button = MouseButton(button & XBUTTON1 ? MouseButton::X1 : MouseButton::X2);
					data.State = (msg.message == WM_XBUTTONDOWN ? ButtonState::Press : ButtonState::Release);
				}
				default: {
					data.Button = MouseButton::Undefined;
					data.State = ButtonState::Undefined;
					break;
				}
			}

			MouseEvent.Publish(data);
			break;

			short modifiers = LOWORD(msg.wParam);
			//event = Event(MouseInputData(MouseInput::Left, ButtonState::Pressed, ModifierState(modifiers & MK_CONTROL, modifiers & MK_ALT, modifiers & MK_SHIFT, modifiers & 0)), window);
		}
		case WM_MOUSEMOVE: {
			if (MouseEvent.Empty()) return result;
			static int32_t lastX = 0;
			static int32_t lastY = 0;

			MouseEventData data;
			data.Action = MouseAction::Move;

			data.LastX = lastX;
			data.LastY = lastY;

			data.X = GET_X_LPARAM(msg.lParam);	// static_cast<short>(LOWORD(msg.lParam));
			data.Y = GET_Y_LPARAM(msg.lParam);	// static_cast<short>(HIWORD(msg.lParam));
			
			data.DeltaX = data.X - data.LastX;
			data.DeltaY = data.Y - data.LastY;

			lastX = data.X;
			lastY = data.Y;

			// Get Modifiers
			switch (msg.wParam) {
				case MK_CONTROL: {
					data.Modifier.Control = true;
					break;
				}
				case MK_SHIFT: {
					data.Modifier.Shift = true;
					break;
				}
				case MK_LBUTTON: {
					data.Button = MouseButton::Left;
					break;
				}
				case MK_MBUTTON: {
					data.Button = MouseButton::Middle;
					break;
				}
				case MK_RBUTTON: {
					data.Button = MouseButton::Right;
					break;
				}
				case MK_XBUTTON1: {
					data.Button = MouseButton::X1;
					break;
				}
				case MK_XBUTTON2: {
					data.Button = MouseButton::X2;
					break;
				}
				default: {
					data.Button = MouseButton::Undefined;
					break;
				}
			}

			MouseEvent.Publish(data);
			break;
			// Capture the mouse in case the user wants to drag it outside
			// Get the client area of the window
			//RECT area;
			//GetClientRect(msg.hwnd, &area);

			//data.X = static_cast<unsigned int>(area.left <= x && x <= area.right ? x - area.left : 0xFFFFFFFF);
			//data.Y = static_cast<unsigned int>(area.top <= y && y <= area.bottom ? y - area.top : 0xFFFFFFFF);
			//data.ScreenX = static_cast<unsigned int>(x);
			//data.ScreenY = static_cast<unsigned int>(y);
			//data.DeltaX = static_cast<int>(x - pWindow->prevMouseX);
			//data.DeltaY = static_cast<int>(y - pWindow->prevMouseY);
			//pWindow->prevMouseX = static_cast<unsigned int>(x);
			//pWindow->prevMouseY = static_cast<unsigned int>(y);

			/*
			// Capture the mouse in case the user wants to drag it outside
			if ((msg.wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON | MK_XBUTTON1 | MK_XBUTTON2)) == 0) {
			// Only release the capture if we really have it
			if (GetCapture() == hwnd)
			ReleaseCapture();
			}
			else if (GetCapture() != hwnd)
			{
			// Set the capture to continue receiving mouse events
			SetCapture(hwnd);
			}

			// If the cursor is outside the client area...
			if ((x < area.left) || (x > area.right) || (y < area.top) || (y > area.bottom)) {
			// and it used to be inside, the mouse left it.
			if (m_mouseInside) {
			m_mouseInside = false;

			// No longer care for the mouse leaving the window
			setTracking(false);

			// Generate a MouseLeft event
			Event event;
			event.type = Event::MouseLeft;
			pushEvent(event);
			}
			} else {
			// and vice-versa
			if (!m_mouseInside) {
			m_mouseInside = true;

			// Look for the mouse leaving the window
			setTracking(true);

			// Generate a MouseEntered event
			Event event;
			event.type = Event::MouseEntered;
			pushEvent(event);
			}
			}*/
		}
		case WM_MOUSEWHEEL: {
			if (MouseEvent.Empty()) return result;
			MouseEventData data;
			data.Action = MouseAction::Wheel;

			data.DeltaWheelY =(float) GET_WHEEL_DELTA_WPARAM(msg.wParam) / (float)WHEEL_DELTA;

			MouseEvent.Publish(data);
			break;
		}
		case WM_MOUSEHWHEEL: {
			if (MouseEvent.Empty()) return result;
			MouseEventData data;
			data.Action = MouseAction::Wheel;

			data.DeltaWheelX = (float)GET_WHEEL_DELTA_WPARAM(msg.wParam) / (float)WHEEL_DELTA;

			MouseEvent.Publish(data);
			break;
		}

		// Touch
		case WM_TOUCH: {
			if (TouchEvent.Empty()) return result;
			TouchEventData data;

			TouchEvent.Publish(data);
			break;
		}

		// System
		case WM_CHAR:		case WM_SYSCHAR:		case WM_UNICHAR: {
			// Perparation
			KeyboardEventData data;
			data.Action = KeyboardAction::Input;
			data.State = KeyState::Undefined;

			// Get Key Code
			data.Character = msg.wParam;
			data.Key = KeyCode{ (unsigned int)msg.wParam };

			// Finalization
			KeyboardEvent.Publish(data);
			result = 0;
			break;
		}

		/**
		 *	Window Events (they serve only for notification purposes, every window handles the events on their own)
		*/
		case WM_NULL: {
			break;
		}
		
		// Information
		case WM_DPICHANGED: {
			WindowEventData data;
			data.Action = WindowAction::DpiUpdate;
			// ToDo: Something like DPI data would be nice.
			WindowEvent.Publish(data);
			break;
		}
		case WM_GETMINMAXINFO: {
			// ToDo: Restrict Container to bounds
			break;
		}

		// Creation and Destruction
		case WM_CLOSE: {
			// Currently there is no use for this event.
			break;
		}
		case WM_CREATE: {
			WindowEventData data;
			data.Action = WindowAction::Create;
			WindowEvent.Publish(data);
			break;
		}
		case WM_DESTROY: {
			WindowEventData data;
			data.Action = WindowAction::Destroy;
			WindowEvent.Publish(data);
			break;
		}

		// Drawing
		case WM_PAINT: {
			WindowEventData data;
			data.Action = WindowAction::Draw;
			WindowEvent.Publish(data);
			break;
		}
		case WM_MOVE: {
			WindowEventData data;
			data.Action = WindowAction::Move;

			data.X = (int)(short) LOWORD(msg.lParam);   // horizontal position 
			data.Y = (int)(short) HIWORD(msg.lParam);   // vertical position 

			WindowEvent.Publish(data);
			break;
		}
		case WM_SHOWWINDOW: {
			WindowEventData data;
			data.Action == WindowAction::Show;
			data.Visible = (bool)msg.wParam;
			WindowEvent.Publish(data);
			break;
		}
		case WM_SIZE: {
			WindowEventData data;
			data.Action == WindowAction::Show;
			switch (msg.wParam) {
				case SIZE_MAXIMIZED: {
					data.Action == WindowAction::Maximize;
					break;
				}
				case SIZE_MINIMIZED: {
					data.Action == WindowAction::Minimize;
					break;
				}
				case SIZE_RESTORED: {
					data.Action == WindowAction::Restore;
				}
			}
			data.Width = static_cast<uint32_t>((UINT64)msg.lParam & 0xFFFF);
			data.Height = static_cast<uint32_t>((UINT64)msg.lParam >> 16);
			WindowEvent.Publish(data);
			break;
		}
		case WM_SIZING: {
			WindowEventData data;
			data.Action = WindowAction::Resize;

			PRECT pWindowDimension = (PRECT)msg.lParam;
			data.X = pWindowDimension->left;
			data.Y = pWindowDimension->top;
			data.Width = pWindowDimension->right - pWindowDimension->left;
			data.Height = pWindowDimension->bottom - pWindowDimension->top;

			WindowEvent.Publish(data);
			break;
		}
		
		// State
		case WM_ACTIVATE: {
			WindowEventData data;
			data.Action = WindowAction::Activate;
			
			data.Active = (bool)msg.wParam;;

			WindowEvent.Publish(data);
			break;
		}
		case WM_KILLFOCUS: {
			WindowEventData data;
			data.Action = WindowAction::Focus;

			data.Focus = false;

			WindowEvent.Publish(data);
			break;
		}
		case WM_SETFOCUS: {
			WindowEventData data;
			data.Action = WindowAction::Focus;

			data.Focus = true;

			WindowEvent.Publish(data);
			break;
		}

		// System
		case WM_SYSCOMMAND: {
			switch (msg.wParam) {
				// Devices
				case SC_MONITORPOWER: {
					DeviceEventData data;
					data.Action = DeviceAction::Null;
					DeviceEvent.Publish(data);
					break;
				}

				// Power
				case SC_SCREENSAVE: {
					PowerEventData data;
					data.Action = PowerAction::Null;
					PowerEvent.Publish(data);
				}
			}
			break;
		}
		
		default: {
			break;
		}
	}

	// ToDo: Doesn't make any sense anymore...
	return result;
}

}
