#include "Application.h"

#include "Omnia/Omnia.pch"
#include "Omnia/Log.h"
#include "UI/GuiLayer.h"
#include "Omnia/Utility/Timer.h"

#include "Omnia/Debug/Instrumentor.h"
#include "Omnia/Debug/Memory.h"

#include <glad/glad.h>

namespace Omnia {

Application *Application::AppInstance = nullptr;

Application::Application():
	Running{ true },
	Paused{ false } {
	AppInstance = this;

	// Register Components
	Listener = EventListener::Create();
	Window = Window::Create(WindowProperties("Ultra"s, 1024, 768));

	// Register Components
	Context = Gfx::CreateContext(Window.get(), Gfx::ContextProperties());
	Gfx::SetContext(Context);

	// Load GL Library
	if (!gladLoadGL()) {
		applog << "Failed to load OpenGL." << std::endl;
		return;
	}
	glViewport(0, 0, Window->GetProperties().Size.Width, Window->GetProperties().Size.Height);

	// Information
	printf("OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);
	CoreLayer = new GuiLayer();

	PushOverlay(CoreLayer);
}

Application::~Application() {}

void Application::Run() {
	APP_PROFILE_BEGIN_SESSION("Playground", "AppProfile.json");
	// Initialization
	auto oDispatcher = Window->EventCallback.Subscribe([&](void *event) { Listener->Callback(event); });

	// Subscribe to all events (internal)
	auto oAutoDeviceEvent = Listener->DeviceEvent.Subscribe([&](DeviceEventData data) { this->AutoDeviceEvent(data); });
	auto oAutoPowerEvent = Listener->PowerEvent.Subscribe([&](PowerEventData data) { this->AutoPowerEvent(data); });

	auto oAutoControllerEvent = Listener->ControllerEvent.Subscribe([&](ControllerEventData data) { this->AutoControllerEvent(data); });
	auto oAutoKeyboardEvent = Listener->KeyboardEvent.Subscribe([&](KeyboardEventData data) { this->AutoKeyboardEvent(data); });
	auto oAutoMouseEvent = Listener->MouseEvent.Subscribe([&](MouseEventData data) { this->AutoMouseEvent(data); });
	auto oAutoTouchEvent = Listener->TouchEvent.Subscribe([&](TouchEventData data) { this->AutoTouchEvent(data); });
	auto oAutoWindowEvent = Listener->WindowEvent.Subscribe([&](WindowEventData data) { this->AutoWindowEvent(data); });

	auto oAutoContextEvent = Listener->ContextEvent.Subscribe([&](ContextEventData data) { this->AutoContextEvent(data); });

	// Subscribe to all Events
	auto oControllerEvent = Listener->ControllerEvent.Subscribe([&](ControllerEventData data) { this->ControllerEvent(data); });
	auto oKeyboardEvent = Listener->KeyboardEvent.Subscribe([&](KeyboardEventData data) { this->KeyboardEvent(data); });
	auto oMouseEvent = Listener->MouseEvent.Subscribe([&](MouseEventData data) { this->MouseEvent(data); });
	auto oTouchEvent = Listener->TouchEvent.Subscribe([&](TouchEventData data) { this->TouchEvent(data); });
	auto oWindowEvent = Listener->WindowEvent.Subscribe([&](WindowEventData data) { this->WindowEvent(data); });

	// Main Logic
	Create();
	//PrintMemoryUsage();
	while (Running) {
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		if (Paused) continue;
		Listener->Update();
		Update();
		
		for (Layer *layer : Layers) layer->Update();


		Gfx::SwapBuffers(Context);
	}

	// Termination
	Destroy();
	APP_PROFILE_END_SESSION();
}

// Workflow
void Application::Create() {}
void Application::Destroy() {}
void Application::Update() {}

// Event System
void Application::ControllerEvent(ControllerEventData data) {}
void Application::KeyboardEvent(KeyboardEventData data) {}
void Application::MouseEvent(MouseEventData data) {}
void Application::TouchEvent(TouchEventData data) {}
void Application::WindowEvent(WindowEventData data) {}

// Layer System
void Application::PushLayer(Layer *layer) {
	Layers.PushLayer(layer);
}

void Application::PushOverlay(Layer *overlay) {
	Layers.PushOverlay(overlay);
}


// Event System (internal)
void Application::AutoDeviceEvent(DeviceEventData data) {
	applog << "Device Event:" << std::endl;
}

void Application::AutoPowerEvent(PowerEventData data) {
	applog << "Power Event:" << std::endl;
}


void Application::AutoControllerEvent(ControllerEventData data) {
	for (auto layer : Layers) { layer->ControllerEvent(data); }
}

void Application::AutoKeyboardEvent(KeyboardEventData data) {
	for (auto layer : Layers) { layer->KeyboardEvent(data); }

	// Left for debugging purposes
	//if (data.Action == KeyboardAction::Input) {
	//	applog << data.Source << ": [" <<
	//		"Action:"		<< data.Action		<< " | "	<<
	//		"State:"		<< data.State		<< " | "	<<
	//		"Key:'"			<< data.Character	<< "' | "	<<
	//		"Modifiers:"	<< data.Modifier	<< " | "	<<
	//		"] \n";
	//} else {
	//	applog << data.Source << ": [" <<
	//		"Action:"		<< data.Action		<< " | "	<<
	//		"State:"		<< data.State		<< " | "	<<
	//		"Key:"			<< data.Key			<< " | "	<<
	//		"Modifiers:"	<< data.Modifier	<< " | "	<<
	//		"-] \n";
	//}
}

void Application::AutoMouseEvent(MouseEventData data) {
	for (auto layer : Layers) { layer->MouseEvent(data); }

	// Left for debugging purposes
	//applog << data.Source << ": [" << 
	//	"Action:"		<< data.Action		<< " | " <<
	//	"State:"		<< data.State		<< " | " <<
	//	"Button:"		<< data.Button		<< " | " <<
	//	"X/Y:"			<< data.X			<< "/"	<< data.Y			<< " | " <<
	//	"DeltaX/Y:"		<< data.DeltaX		<< "/"	<< data.DeltaY		<< " | " <<
	//	"LastX/Y:"		<< data.LastX		<< "/"	<< data.LastY		<< " | " <<
	//	"DeltaW X/Y:"	<< data.DeltaWheelX	<< "/"	<< data.DeltaWheelY <<
	//	"Modifiers:"	<< data.Modifier	<< " | " <<
	//	"-] \n";
}

void Application::AutoTouchEvent(TouchEventData data) {
	for (auto layer : Layers) { layer->TouchEvent(data); }
}

void Application::AutoWindowEvent(WindowEventData data) {
	for (auto layer : Layers) { layer->WindowEvent(data); }

	// Left for debugging purposes
	applog << data.Source << ": [" << 
		"Action:"		<< data.Action		<< " | " <<
		"A/F:"			<< data.Active		<< "/"	<< data.Focus		<< " | " <<
		"W/H:"			<< data.Width		<< "/"	<< data.Height		<< " | " <<
		"X/Y:"			<< data.X			<< "/"	<< data.Y			<< " | " <<
		"DeltaX/Y:"		<< data.DeltaX		<< "/"	<< data.DeltaY		<< " | " <<
		"LastX/Y:"		<< data.LastX		<< "/"	<< data.LastY		<< " | " <<
		"-] \n";

	switch (data.Action) {
		case WindowAction::Destroy: {
			Running = false;
			break;
		}

		case WindowAction::Resize: {
			// ToDo: Needs a redraw to show contents...
			glViewport(0, 0, data.Width, data.Height);
			//gfx::SwapBuffers(Context);
			break;
		}

		default: {
			break;
		}
	}
}


void Application::AutoContextEvent(ContextEventData data) {
	applog << "Context Event:" << std::endl;
}

}
