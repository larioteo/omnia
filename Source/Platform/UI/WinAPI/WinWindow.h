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
	virtual WindowProperties GetProperties() const override;
	virtual WindowSize GetContexttSize() const override;
	virtual WindowPosition GetDisplayPosition() const override;
	virtual WindowSize GetDisplaySize() const override;
	virtual WindowSize GetScreentSize() const override;
	virtual string GetTitle() const override;

	// Modifiers
	virtual void SetProperties(const WindowProperties &properties) override;
	virtual void SetCursorPosition(const int32_t x, const int32_t y) override;
	virtual void SetDisplayPosition(const int32_t x, const int32_t y) override;
	virtual void SetDisplaySize(const uint32_t width, const uint32_t height) override;
	virtual void SetProgress(const float progress) override;
	virtual void SetTitle(const string_view title) override;
};

}
