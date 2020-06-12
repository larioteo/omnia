#include "Omnia/Omnia.pch"
#include "GuiLayer.h"

#include "Omnia/Application.h"
#include "Omnia/Log.h"

#include "Platform/Graphics/OpenGL/GLGuiRenderer.h"
#include "Platform/UI/WinApi/WinGuiApi.h"

namespace Omnia {

GuiLayer::GuiLayer():
	Layer("GuiLayer") {
}

GuiLayer::~GuiLayer() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}


void GuiLayer::Attach() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsClassic();

	const char *glsl_version = "#version 410";
	Application &app = Application::Get();

	ImGuiIO &io = ImGui::GetIO();
	io.IniFilename = "Designer.ini";
	io.LogFilename = "Designer.log";
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	io.DisplaySize = ImVec2(app.GetWindow().GetDisplaySize().Width, app.GetWindow().GetDisplaySize().Height);

	if (ImGui_ImplOpenGL3_Init(glsl_version)) {
		applog << Log::Success << "OpenGLInit: Success\n";
	}
	
	if (ImGui_ImplWin32_Init((HWND)app.GetWindow().GetNativeWindow())) {
		applog << Log::Success << "Win32Init: Success\n";
	}

}

void GuiLayer::Detach() {
}

void GuiLayer::Event(void *event) {
}

void GuiLayer::Update() {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//GetDeltaTime...
	//ImGuiIO &= = ImGui::GetIO();
	//io.DeltaTime = Time > 0.0f ? (time - Time) : (1.0f / 60.0f);

	static bool show = true;
	ImGui::ShowDemoWindow(&show);

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GuiLayer::GuiRender() {

}



void GuiLayer::ControllerEvent(ControllerEventData data) {
}

void GuiLayer::KeyboardEvent(KeyboardEventData data) {
	if (ImGui::GetCurrentContext() == NULL) return;
	ImGuiIO &io = ImGui::GetIO();

	io.KeyAlt = data.Modifier.Alt;
	io.KeyCtrl = data.Modifier.Control;
	io.KeyShift = data.Modifier.Shift;
	io.KeySuper = data.Modifier.Super;

	switch (data.Action) {
		case KeyboardAction::Input: {
			io.AddInputCharacterUTF16((uint32_t)data.Key);
			break;
		}
		case KeyboardAction::Default: {
			switch (data.State) {
				case KeyState::Press: {
					io.KeysDown[(uint32_t)data.Key] = true;
					break;
				}
				case KeyState::Release: {
					io.KeysDown[(uint32_t)data.Key] = false;
					break;
				}
				default: {
					break;
				}
			}
		}
		default: {
			break;
		}
	}
}

void GuiLayer::MouseEvent(MouseEventData data) {
	if (ImGui::GetCurrentContext() == NULL) return;
	ImGuiIO &io = ImGui::GetIO();

	io.KeyAlt = data.Modifier.Alt;
	io.KeyCtrl = data.Modifier.Control;
	io.KeyShift = data.Modifier.Shift;
	io.KeySuper = data.Modifier.Super;

	switch (data.Action) {
		case MouseAction::Move:	{
			io.MousePos = ImVec2(data.X, data.Y);
		}

		case MouseAction::Wheel: {
			io.MouseWheel += data.DeltaWheelY;
			io.MouseWheelH += data.DeltaWheelX;
			break;
		}

		default: {
			switch (data.State) {
				case ButtonState::Press: {
					if (data.Button == MouseButton::Left)	io.MouseDown[0] = true;
					if (data.Button == MouseButton::Right)	io.MouseDown[1] = true;
					if (data.Button == MouseButton::Middle)	io.MouseDown[2] = true;
					if (data.Button == MouseButton::X1)		io.MouseDown[3] = true;
					if (data.Button == MouseButton::X2)		io.MouseDown[4] = true;
					break;
				}

				case ButtonState::Release: {
					if (data.Button == MouseButton::Left)	io.MouseDown[0] = false;
					if (data.Button == MouseButton::Middle) io.MouseDown[1] = false;
					if (data.Button == MouseButton::Right)	io.MouseDown[2] = false;
					if (data.Button == MouseButton::X1)		io.MouseDown[3] = false;
					if (data.Button == MouseButton::X2)		io.MouseDown[4] = false;
					break;
				}

				default: {
					break;
				}
			}
		}
	}
}

void GuiLayer::TouchEvent(TouchEventData data) {

}

void GuiLayer::WindowEvent(WindowEventData data) {

}

}
