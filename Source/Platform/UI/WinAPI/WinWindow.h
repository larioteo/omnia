#pragma once

#include "Omnia/UI/Window.h"

namespace Omnia {

class WinWindow: public Window {
	// Properties
	WindowProperties Properties;
	struct WindowData {
		unsigned prevMouseX;
		unsigned prevMouseY;
	} ___Data;

public:
	// Default
	WinWindow(const WindowProperties &properties);
	~WinWindow();

	void Update() override;
	intptr_t Message(void *event);

	// Accessors
	virtual void *GetNativeHandle() const override;
	WindowProperties GetProperties() const override;
	WindowSize GetContexttSize() const override;
	WindowPosition GetDisplayPosition() const override;
	WindowSize GetDisplaySize() const override;
	WindowSize GetScreentSize() const override;
	string GetTitle() const override;

	// Modifiers
	void SetProperties(const WindowProperties &properties) override;
	void SetCursorPosition(long x, long y) override;
	void SetDisplayPosition(const int x, const int y) override;
	void SetDisplaySize(const unsigned int width, const unsigned int height) override;
	void SetProgress(float progress) override;
	void SetTitle(const std::string &title) override;
};

static thread_local inline WinWindow *pCurrentWindow = nullptr;
static thread_local inline std::unordered_map<void *, WinWindow *> pWindowList = {};

}
