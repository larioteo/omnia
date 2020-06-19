#pragma once

#include "Omnia/Layer.h"
#include <imgui/imgui.h>

namespace Omnia {

class GuiLayer: public Layer {
	Reference<EventListener> listener;
	uint32_t Width;
	uint32_t Height;

public:
	GuiLayer();
	virtual ~GuiLayer();

	virtual void Attach() override;
	virtual void Detach() override;
	virtual void Event(void *event) override;
	virtual void Update(Timestep deltaTime) override;
	virtual void GuiRender() override;

	void Begin();
	void End();

	virtual void ControllerEvent(ControllerEventData data) override;
	virtual void KeyboardEvent(KeyboardEventData data) override;
	virtual void MouseEvent(MouseEventData data) override;
	virtual void TouchEvent(TouchEventData data) override;
	virtual void WindowEvent(WindowEventData data) override;
};

}
