#include "Application.h"

#include "Omnia/Omnia.pch"
#include "Omnia/Log.h"

#include "Omnia/Debug/Instrumentor.h"

namespace Omnia {

Application *Application::AppInstance = nullptr;

Application::Application(const string &title):
	Running(true),
	Paused(false) {
	AppInstance = this;

	// Load Window and Events
	pWindow = Window::Create(WindowProperties(title, 1024, 768));
	Context = Gfx::CreateContext(pWindow.get(), Gfx::ContextProperties());
	pListener = EventListener::Create();

	// Load GFX Context
	Gfx::SetContext(Context);
	Gfx::LoadGL();
	Gfx::SetViewport( pWindow->GetProperties().Size.Width, pWindow->GetProperties().Size.Height);

	// Load Core Layer
	CoreLayer = new GuiLayer();
	PushOverlay(CoreLayer);
}

Application::~Application() {}

void Application::Run() {
	// Preparation
	APP_PROFILE_BEGIN_SESSION("Application", "AppProfile.json");
	Timer timer = {};
	double delay = {};
	size_t frames = {};
	string statistics;

	// Subscribe to all events (internal)
	auto oDispatcher = pWindow->EventCallback.Subscribe([&](void *event) { pListener->Callback(event); });

	auto oAutoDeviceEvent = pListener->DeviceEvent.Subscribe([&](DeviceEventData data) { this->AutoDeviceEvent(data); });
	auto oAutoPowerEvent = pListener->PowerEvent.Subscribe([&](PowerEventData data) { this->AutoPowerEvent(data); });

	auto oAutoControllerEvent = pListener->ControllerEvent.Subscribe([&](ControllerEventData data) { this->AutoControllerEvent(data); });
	auto oAutoKeyboardEvent = pListener->KeyboardEvent.Subscribe([&](KeyboardEventData data) { this->AutoKeyboardEvent(data); });
	auto oAutoMouseEvent = pListener->MouseEvent.Subscribe([&](MouseEventData data) { this->AutoMouseEvent(data); });
	auto oAutoTouchEvent = pListener->TouchEvent.Subscribe([&](TouchEventData data) { this->AutoTouchEvent(data); });
	auto oAutoWindowEvent = pListener->WindowEvent.Subscribe([&](WindowEventData data) { this->AutoWindowEvent(data); });

	auto oAutoContextEvent = pListener->ContextEvent.Subscribe([&](ContextEventData data) { this->AutoContextEvent(data); });

	// Subscribe to all Events (external)
	auto oControllerEvent = pListener->ControllerEvent.Subscribe([&](ControllerEventData data) { this->ControllerEvent(data); });
	auto oKeyboardEvent = pListener->KeyboardEvent.Subscribe([&](KeyboardEventData data) { this->KeyboardEvent(data); });
	auto oMouseEvent = pListener->MouseEvent.Subscribe([&](MouseEventData data) { this->MouseEvent(data); });
	auto oTouchEvent = pListener->TouchEvent.Subscribe([&](TouchEventData data) { this->TouchEvent(data); });
	auto oWindowEvent = pListener->WindowEvent.Subscribe([&](WindowEventData data) { this->WindowEvent(data); });

	// Main Logic
	Create();
	for (Layer *layer : Layers) layer->Create();
	while (Running) {
		// Update events and check if application is paused
		pListener->Update();
		if (Paused) continue;

		// Calcualte Statistics
		Timestamp deltaTime = timer.GetDeltaTime();
		delay += deltaTime;
		frames++;
		if (delay >= 1.0f) {
			float msPF = 1000.0f / (float)frames;

			statistics = "Ultra"s + " [FPS:" + std::to_string(frames) + " | msPF:" + std::to_string(msPF) + "]";
			pWindow->SetTitle(statistics);

			frames = 0;
			delay = 0.0f;
		}

		// Update
		Update(deltaTime);
		for (Layer *layer : Layers) layer->Update(deltaTime);
		if (pWindow->GetProperties().State.Alive) {
			CoreLayer->Prepare();
			for (Layer *layer : Layers) layer->GuiUpdate();
			CoreLayer->Finish();
		}
		Gfx::SwapBuffers(Context);
	}

	// Termination
	for (Layer *layer : Layers) layer->Destroy();
	Destroy();
	APP_PROFILE_END_SESSION();
}


// Workflow
void Application::Create() {}
void Application::Destroy() {}
void Application::Update(Timestamp deltaTime) {}


// Event System
void Application::ControllerEvent(ControllerEventData &data) {}
void Application::KeyboardEvent(KeyboardEventData &data) {}
void Application::MouseEvent(MouseEventData &data) {}
void Application::TouchEvent(TouchEventData &data) {}
void Application::WindowEvent(WindowEventData &data) {}


// Layer System
void Application::PushLayer(Layer *layer) {
	Layers.PushLayer(layer);
	layer->Attach();
}

void Application::PushOverlay(Layer *overlay) {
	Layers.PushOverlay(overlay);
	overlay->Attach();
}


// Event System (internal)
void Application::AutoDeviceEvent(DeviceEventData &data) {
	applog << "Device Event:" << std::endl;
}
void Application::AutoPowerEvent(PowerEventData &data) {
	applog << "Power Event:" << std::endl;
}

void Application::AutoControllerEvent(ControllerEventData &data) {
	for (auto layer : Layers) { layer->ControllerEvent(data); }
}
void Application::AutoKeyboardEvent(KeyboardEventData &data) {
	for (auto layer : Layers) { layer->KeyboardEvent(data); }

	//// Left for debugging purposes
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
void Application::AutoMouseEvent(MouseEventData &data) {
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
void Application::AutoTouchEvent(TouchEventData &data) {
	for (auto layer : Layers) { layer->TouchEvent(data); }
}
void Application::AutoWindowEvent(WindowEventData &data) {
	for (auto layer : Layers) { layer->WindowEvent(data); }

	//if (data.Action == WindowAction::Draw) return;
	//if (data.Action == WindowAction::Focus) return;
	//if (data.Action == WindowAction::Activate) return;
	//if (data.Action == WindowAction::Move) return;

	// Left for debugging purposes
	//applog << data.Source << ": [" << 
	//	"Action:"		<< data.Action		<< " | " <<
	//	"A/F/V:"		<< data.Active		<< "/"	<< data.Focus		<< "/"	<< data.Visible		<< " | " <<
	//	"W/H:"			<< data.Width		<< "/"	<< data.Height		<< " | " <<
	//	"X/Y:"			<< data.X			<< "/"	<< data.Y			<< " | " <<
	//	"DeltaX/Y:"		<< data.DeltaX		<< "/"	<< data.DeltaY		<< " | " <<
	//	"LastX/Y:"		<< data.LastX		<< "/"	<< data.LastY		<< " | " <<
	//	"-] \n";

	switch (data.Action) {
		case WindowAction::Destroy: {
			Running = false;
			break;
		}

		case WindowAction::Resize: {
			// ToDo: Needs a redraw to show contents...
			//Gfx::SetViewport( pWindow->GetProperties().Size.Width, pWindow->GetProperties().Size.Height);
			//Gfx::SwapBuffers(Context);
			break;
		}

		default: {
			break;
		}
	}
}

void Application::AutoContextEvent(ContextEventData &data) {
	applog << "Context Event:" << std::endl;
}

}
