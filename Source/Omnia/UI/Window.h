#pragma once

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"
#include "Omnia/Utility/Message.h"

#include "WindowData.h"

namespace Omnia {

class Window {
public:
	// Default
	Window() {};
	virtual ~Window() = default;
	static Scope<Window> Create(const WindowProperties &properties = WindowProperties());

	// Events
	virtual void Update() = 0;
	Subject<void *> EventCallback;

	// Accessors
	virtual void *GetNativeWindow() const = 0;
	virtual WindowProperties GetProperties() const = 0;
	virtual WindowSize GetContexttSize() const = 0;
	virtual WindowPosition GetDisplayPosition() const = 0;
	virtual WindowSize GetDisplaySize() const = 0;
	virtual WindowSize GetScreentSize() const = 0;
	virtual string GetTitle() const = 0;

	// Modifiers
	virtual void SetProperties(const WindowProperties &properties) = 0;
	virtual void SetCursorPosition(const int32_t x, const int32_t y) = 0;
	virtual void SetDisplayPosition(const int32_t x, const int32_t y) = 0;
	virtual void SetDisplaySize(const uint32_t width, const uint32_t height) = 0;
	virtual void SetProgress(const float progress) = 0;
	virtual void SetTitle(const string_view title) = 0;
};

}