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

	static Scope<Window> Create(const WindowProperties &properties);
	virtual void Update() = 0;

	// Subjects
	Subject<void *> EventCallback;

	// Accessors
	virtual void *GetNativeHandle() const = 0;
	virtual WindowProperties GetProperties() const = 0;
	virtual WindowSize GetContexttSize() const = 0;
	virtual WindowPosition GetDisplayPosition() const = 0;
	virtual WindowSize GetDisplaySize() const = 0;
	virtual WindowSize GetScreentSize() const = 0;
	virtual string GetTitle() const = 0;

	// Modifiers
	virtual void SetProperties(const WindowProperties &properties) = 0;
	virtual void SetCursorPosition(long x, long y) = 0;
	virtual void SetDisplayPosition(const int x, const int y) = 0;
	virtual void SetDisplaySize(const unsigned int width, const unsigned int height) = 0;
	virtual void SetProgress(float progress) = 0;
	virtual void SetTitle(const std::string &title) = 0;
};

}
