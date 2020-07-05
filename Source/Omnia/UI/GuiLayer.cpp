#include "Omnia/Omnia.pch"
#include "GuiLayer.h"

#include "Omnia/Core/Application.h"
#include "Omnia/Log.h"

#include "Omnia/UI/GuiBuilder.h"
#include "Platform/GFX/OpenGL/GLContext.cpp"

namespace Omnia {

static bool ShowDemoWindow = false;

GuiLayer::GuiLayer(): Layer("GuiLayer") {}
GuiLayer::~GuiLayer() {}

void GuiLayer::Attach() {
	// Decide GL+GLSL versions
	Application &app = Application::Get();
	const char *glsl_version = "#version 450";
	//ImGui_ImplWin32_EnableDpiAwareness();

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	io.ConfigDockingWithShift = false;

	// ToDo: Works only as an memory leak, the question is why (otherwise ImGui uses old pointer where the data is gone) ...
	string *dataTarget = new string("./Data/"s + Application::GetWindow().GetProperties().Title + ".ini"s);
	string *logTarget = new string("./Log/"s + Application::GetWindow().GetProperties().Title + ".log"s);
	io.IniFilename = dataTarget->c_str();
	io.LogFilename = logTarget->c_str();
	//io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	//io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	io.DisplaySize = ImVec2((float)app.GetWindow().GetContexttSize().Width, (float)app.GetWindow().GetContexttSize().Height);

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplWin32_Init(app.GetWindow().GetNativeWindow(), app.GetContext().GetNativeContext());
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Fonts
	io.Fonts->AddFontFromFileTTF("./Assets/Fonts/Roboto/Roboto-Medium.ttf", 14.0f);
	io.Fonts->AddFontDefault();
}

void GuiLayer::Detach() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void GuiLayer::GuiUpdate() {
	if (ShowDemoWindow) ImGui::ShowDemoWindow(&ShowDemoWindow);
}

void GuiLayer::Update(Timestamp deltaTime) {
	ImGuiIO &io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	return;
}


void GuiLayer::Prepare() {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void GuiLayer::Finish() {
	Application &app = Application::Get();
	ImGuiIO& io = ImGui::GetIO();
	
	// Rendering
	ImGui::Render();
	io.DisplaySize = ImVec2((float)app.GetWindow().GetContexttSize().Width, (float)app.GetWindow().GetContexttSize().Height);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		//app.Get().GetContext().Attach();
	}
}


void GuiLayer::Event(void *event) {}
void GuiLayer::ControllerEvent(ControllerEventData data) {}

void GuiLayer::KeyboardEvent(KeyboardEventData data) {
	if (ImGui::GetCurrentContext() == NULL) return;

	ImGuiIO &io = ImGui::GetIO();

	io.KeyAlt = data.Modifier.Alt;
	io.KeyCtrl = data.Modifier.Control;
	io.KeyShift = data.Modifier.Shift;
	io.KeySuper = data.Modifier.Super;

	//case KeyCode::F1: {
	//	//Menu::show_demo_window = !Menu::show_demo_window;
	//	break;
	//}
	switch (data.Action) {
		case KeyboardAction::Input: {
			io.AddInputCharacterUTF16((uint32_t)data.Key);
			break;
		}
		case KeyboardAction::Default: {
			switch (data.State) {
				case KeyState::Press: {
					io.KeysDown[(uint32_t)data.Key] = true;

					if (data.Key == KeyCode::F1) ShowDemoWindow = true;

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
			io.MousePos = ImVec2(static_cast<float>(data.X), static_cast<float>(data.Y));
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
					if (data.Button == MouseButton::Right)	io.MouseDown[1] = false;
					if (data.Button == MouseButton::Middle) io.MouseDown[2] = false;
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

void GuiLayer::TouchEvent(TouchEventData data) {}
void GuiLayer::WindowEvent(WindowEventData data) {}


// Helpers
namespace UI {

}

}

