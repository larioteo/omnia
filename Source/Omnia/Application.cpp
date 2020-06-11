#include "Application.h"

#include "Omnia/Omnia.pch"
#include "Omnia/Log.h"
#include "Omnia/Utility/Timer.h"
#include "Omnia/Debug/Instrumentor.h"

#include <glad/glad.h>

namespace Omnia {

Application *Application::AppInstance = nullptr;

Application::Application():
	Running{ true },
	Paused{ false } {
	AppInstance = this;

	// Register Components
	Listener = EventListener::Create();
	Window = Window::Create(WindowProperties("Ultra"s));

	// Register Components
	Context = gfx::CreateContext(Window.get(), gfx::ContextProperties());
	gfx::SetContext(Context);

	// Load GL Library
	if (!gladLoadGL()) {
		applog << "Failed to load OpenGL." << std::endl;
		return;
	}
	glViewport(0, 0, Window->GetProperties().Size.Width, Window->GetProperties().Size.Height);

	// Information
	printf("OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);

}

Application::~Application() {}

void Application::Run() {
	APP_PROFILE_BEGIN_SESSION("Playground", "AppProfile.json");
	// Initialization
	Create();
	auto oDispatcher = Window->EventCallback.Subscribe([&](void *event) { Listener->Callback(event); });

	// Subscribe to all events (internal)
	auto oAutoControllerEvent = Listener->ControllerEvent.Subscribe([&](ControllerEventData data) { this->AutoControllerEvent(data); });
	auto oAutoKeyboardEvent = Listener->KeyboardEvent.Subscribe([&](KeyboardEventData data) { this->AutoKeyboardEvent(data); });
	auto oAutoMouseEvent = Listener->MouseEvent.Subscribe([&](MouseEventData data) { this->AutoMouseEvent(data); });
	auto oAutoTouchEvent = Listener->TouchEvent.Subscribe([&](TouchEventData data) { this->AutoTouchEvent(data); });
	auto oAutoWindowEvent = Listener->WindowEvent.Subscribe([&](WindowEventData data) { this->AutoWindowEvent(data); });

	// Subscribe to all Events
	auto oControllerEvent = Listener->ControllerEvent.Subscribe([&](ControllerEventData data) { this->ControllerEvent(data); });
	auto oKeyboardEvent = Listener->KeyboardEvent.Subscribe([&](KeyboardEventData data) { this->KeyboardEvent(data); });
	auto oMouseEvent = Listener->MouseEvent.Subscribe([&](MouseEventData data) { this->MouseEvent(data); });
	auto oTouchEvent = Listener->TouchEvent.Subscribe([&](TouchEventData data) { this->TouchEvent(data); });
	auto oWindowEvent = Listener->WindowEvent.Subscribe([&](WindowEventData data) { this->WindowEvent(data); });

	// Main Logic
	while (Running) {
		glClearColor(1, 0, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		if (Paused) continue;
		for (Layer *layer : Layers) layer->Update();
		Listener->Update();

		
		Update();
		gfx::SwapBuffers(Context);
	}

	// Termination
	Destroy();
	//PrintMemoryUsage();
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
void Application::AutoControllerEvent(ControllerEventData data) {
	for (auto layer : Layers) { layer->ControllerEvent(data); }
}

void Application::AutoKeyboardEvent(KeyboardEventData data) {
	for (auto layer : Layers) { layer->KeyboardEvent(data); }
	//applog << data.Source << ": [" << 
	//	"Action:"	<< data.Action	<< "\t| " <<
	//	"Key:"		<< data.Key		<< "\t| " <<
	//	"State:"	<< data.State	<< "\t| "
	//	<< "]"		<< std::endl;
}

void Application::AutoMouseEvent(MouseEventData data) {
	for (auto layer : Layers) { layer->MouseEvent(data); }
	//applog << data.Source << ": [" << 
	//	"Action:"		<< data.Action		<< " | " <<
	//	"Button/State:"	<< data.Button		<<"/" << data.State << " | " <<
	//	"Modifiers:"	<< data.Modifier	<< " | " <<
	//	"X/Y:"			<< data.X			<< "/" << data.Y << " | " <<
	//	"Last X/Y:"		<< data.LastX		<< "/" << data.LastY << " | " <<
	//	"Delta X/Y:"	<< data.DeltaX		<< "/" << data.DeltaY
	//	<< "]"		<< std::endl;
}

void Application::AutoTouchEvent(TouchEventData data) {
	for (auto layer : Layers) { layer->TouchEvent(data); }
}

void Application::AutoWindowEvent(WindowEventData data) {
	for (auto layer : Layers) { layer->WindowEvent(data); }

	//applog << data.Source << ": [" << 
	//	"Action:"	<< data.Action	<< "\t| " <<
	//	"Raw:"		<< "w:" << data.Width << ", h:" << data.Height << ", X/Y:" << data.X << "/" << data.Y
	//	<< "]"		<< std::endl;
	switch (data.Action) {
		case WindowAction::Destroy: {
			Running = false;
			break;
		}

		case WindowAction::Resize: {
			// ToDo: Needs a redraw to show contents...
			glViewport(0, 0, data.Width, data.Height);

			gfx::SwapBuffers(Context);
			break;
		}

		default: {
			break;
		}
	}
}

}
