#pragma once

#include "Omnia/UI/Window.h"

namespace Omnia {

class WinWindow: public Window {
	// Properties
	WindowProperties Properties;
	
public:
	// Default
	WinWindow(const WindowProperties &properties);
	virtual ~WinWindow();

	// Events
	virtual void Update() override;
	intptr_t Message(void *event);

	// Accessors
	virtual void *GetNativeWindow() const override;
	virtual const WindowProperties GetProperties() const override;
	virtual const WindowSize GetContexttSize() const override;
	virtual const WindowPosition GetDisplayPosition() const override;
	virtual const WindowSize GetDisplaySize() const override;
	virtual const WindowSize GetScreentSize() const override;
	virtual const string GetTitle() const override;

	// Modifiers
	virtual void SetProperties(const WindowProperties &properties) override;
	virtual void SetCursorPosition(const int32_t x, const int32_t y) override;
	virtual void SetDisplayPosition(const int32_t x, const int32_t y) override;
	virtual void SetDisplaySize(const uint32_t width, const uint32_t height) override;
	virtual void SetProgress(const float progress) override;
	virtual void SetTitle(const string_view title) override;
};

}
