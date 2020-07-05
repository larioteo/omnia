#pragma once

#include "Omnia/Core/Layer.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace Omnia {

class GuiLayer: public Layer {
public:
	GuiLayer();
	virtual ~GuiLayer();

	virtual void Attach() override;
	virtual void Detach() override;
	virtual void GuiUpdate() override;
	virtual void Update(Timestamp deltaTime) override;

	void Prepare();
	void Finish();

	virtual void Event(void *event) override;
	virtual void ControllerEvent(ControllerEventData data) override;
	virtual void KeyboardEvent(KeyboardEventData data) override;
	virtual void MouseEvent(MouseEventData data) override;
	virtual void TouchEvent(TouchEventData data) override;
	virtual void WindowEvent(WindowEventData data) override;
};

// Helpers
namespace UI {

template <typename ... Args>
void Label(const string &format, const Args &...arguments) {
	ImGui::Text(format.c_str(), (..., arguments));
}

}

}
