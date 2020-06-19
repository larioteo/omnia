#pragma once

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"

namespace Omnia {

/**
* @brief	Collection of platform independet window related types
*/

struct WindowPosition {
	int32_t X;	// Display position from the left
	int32_t Y;	// Display position form the top

	uint8_t Monitor; // Monitor identifier

	bool Centered;

	// Window centered on main monitor
	WindowPosition():
		X {-1},
		Y {-1},
		Monitor {0},
		Centered {true} {
	};

	// Window centered on specified monitor
	WindowPosition(uint8_t monitor):
		X {-1},
		Y {-1},
		Monitor {monitor},
		Centered {true} {
	};

	// Window with specified position on main monitor as default
	WindowPosition(int32_t x, int32_t y, uint8_t monitor = 0):
		X {x},
		Y {y},
		Monitor {monitor},
		Centered {false} {
	};
};

struct WindowSettings {
	static constexpr uint32_t BorderWidth = 12;
	static constexpr uint32_t TitleBarWidth = 5;

	static constexpr uint32_t MaximumHeight = 0;
	static constexpr uint32_t MaximumWidth = 0;
	static constexpr uint32_t MinimumHeight = 0;
	static constexpr uint32_t MimimumWidth = 0;
};

struct WindowSize {
	uint32_t Width;		// Display width
	uint32_t Height;	// Display height

	// Window with default resolution 640x480
	WindowSize():
		Width {640u},
		Height {480u} {
	};

	// Window with spepcified resolution
	WindowSize(uint32_t width, uint32_t height):
		Width {width},
		Height {height} {
	};
};

struct WindowState {
	bool Active = false;
	bool Alive = false;
	bool Cursor = false;
	bool Decorated = false;
	bool Focused = false;
	bool Maximized = false;
	bool Minimized = false;
	bool Visible = true;
};

enum class WindowStyle: uint8_t {
	Default		= 0x00,
	Borderless	= 0x01,
	FullScreen	= 0xFF
};

/**
* @brief	Collection of platform independet window related properties
*/

struct WindowProperties {
	string ID;
	string Title;
	string Icon;

	WindowPosition Position;
	WindowSettings Settings;
	WindowSize Size;
	WindowState State;
	WindowStyle Style;

	WindowSize MaxSize;
	WindowSize MinSize;

	// Window with predefined values (can be used only once for now...)
	WindowProperties():
		ID {"Window[App]"},
		Title {"App"},
		Icon {"Data/App.ico"},
		Position {},
		Settings {},
		Size {},
		State {},
		Style {WindowStyle::Default},
		MaxSize {0, 0},
		MinSize {} {
	};

	// Window with recommended values
	WindowProperties(string &title, unsigned int width = 640u, unsigned int height = 480u, WindowStyle style = WindowStyle::Default):
		ID {"Window[" + title + "]"},
		Title {title},
		Icon {"Data/" + title + ".ico"},
		Position {},
		Settings {},
		State {},
		Size {width, height},
		Style {style},
		MaxSize {0, 0},
		MinSize {width / 2, height / 2} {
	};
};

}
