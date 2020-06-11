#pragma once

#include "Omnia/Layer.h"

namespace Omnia {

class GuiLayer: public Layer {
	Reference<EventListener> listener;

public:
	GuiLayer();
	~GuiLayer();

	void Attach();
	void Detach();
	void Event(void *event);
	void Update();
	void GuiRender();

	void ControllerEvent(ControllerEventData data) override;
	void KeyboardEvent(KeyboardEventData data) override;
	void MouseEvent(MouseEventData data) override;
	void TouchEvent(TouchEventData data) override;
	void WindowEvent(WindowEventData data) override;
};

}
