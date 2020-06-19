#pragma once

#include "Omnia/Core.h"
#include "Omnia/UI/Event.h"
#include "Omnia/Utility/Timer.h"

namespace Omnia {

class Layer {
protected:
	string DebugName;

public:
	Layer(const string &name = "Layer"):
		DebugName{ name } {
	}
	virtual ~Layer() = default;

	virtual void Attach() {}
	virtual void Detach() {}
	virtual void Event(void *event) {}

	virtual void Create() {}
	virtual void Update(Timestep deltaTime) {}
	virtual void Destroy() {}
	virtual void GuiRender() {}

	inline const string &GetName() const { return DebugName; }

	virtual void ControllerEvent(ControllerEventData data) {}
	virtual void KeyboardEvent(KeyboardEventData data) {}
	virtual void MouseEvent(MouseEventData data) {}
	virtual void TouchEvent(TouchEventData data) {}
	virtual void WindowEvent(WindowEventData data) {}
};

}
