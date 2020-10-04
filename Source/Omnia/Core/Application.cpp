#include "Application.h"

#include "Omnia/Omnia.pch"
#include "Omnia/Log.h"

#include "Omnia/Debug/Instrumentor.h"

namespace Omnia {

Application *Application::AppInstance = nullptr;

Application::Application(const ApplicationProperties &properties):
	Running(true),
	Paused(false) {
	// Preparation
	applog << "Application started..."s << "\n";
	applog << "... on: " << apptime.GetIsoDate() << "\n";
	applog << "... at: " << apptime.GetIsoTime() << "\n";
	AppInstance = this;

	// Initialization
	applog << Log::Caption << "Initialization" << "\n";

	// Load Configuration
	pConfig = CreateReference<Config>();

	// Load Window, Context and Events
	pWindow = Window::Create(WindowProperties(properties.Title, properties.Width, properties.Height));
    pDialog = Dialog::Create();
	AppLogDebug("[Application] ", "Created window '", properties.Title, "' with size '", properties.Width, "x", properties.Height, "'");
	pContext = Context::Create(pWindow->GetNativeWindow());
	pContext->Attach();
	pContext->Load();
	pContext->SetViewport(pWindow->GetContexttSize().Width, pWindow->GetContexttSize().Height);
	pListener = EventListener::Create();

	// Load Core Layer
	CoreLayer = new GuiLayer();
	PushOverlay(CoreLayer);
}

Application::~Application() {
	applog << "\nApplication finished ..."s << "\n";
	applog << "... on: " << apptime.GetIsoDate() << "\n";
	applog << "... at: " << apptime.GetIsoTime() << "\n";
}

void Application::Run() {
	// Preparation
	APP_PROFILE_BEGIN_SESSION("Application", "AppProfile.json");
	Timer timer = {};
	double delay = {};
	size_t frames = {};
	string statistics;
	string title = pWindow->GetTitle();

    

	// Subscribe to all events (internal)
	auto oDispatcher = pWindow->EventCallback.Subscribe([&](bool &result, void *event) { pListener->Callback(result, event); });

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
	applog << Log::Caption << "Main Loop"s << "\n";

	while (Running) {
		// Update events and check if application is paused
		pListener->Update();
		if (Paused) continue;

		// Calcualte 
		Timestamp deltaTime = timer.GetDeltaTime();
		frames++;
		delay += deltaTime;
		this->statistics.msPF = deltaTime.GetMilliseconds();

		if (delay >= 1.0f) {
			this->statistics.fps = frames;
			float msPF = 1000.0f / (float)frames;

			statistics = title + " [FPS:" + std::to_string(frames) + " | msPF:" + std::to_string(msPF) + "]";
			pWindow->SetTitle(statistics);

			frames = 0;
			delay = 0.0f;
		}

		// Update
		pContext->Attach();
        for (Layer *layer : Layers) layer->Update(deltaTime);
        Update(deltaTime);
        if (pWindow->GetState(WindowState::Alive)) {
            CoreLayer->Prepare();
            for (Layer *layer : Layers) layer->GuiUpdate();
            CoreLayer->Finish();
        }
		pContext->SwapBuffers();
		pContext->Detach();
	}

	// Termination
	applog << Log::Caption << "Termination" << "\n";
	for (Layer *layer : Layers) layer->Destroy();
	Destroy();
	APP_PROFILE_END_SESSION();
}


// Workflow
void Application::Create() {}
void Application::Destroy() {}
void Application::Update(Timestamp deltaTime) {}

void Application::Exit() {
	Running = false;
}


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
    AppLog("[Application::AutoDeviceEvent]:");
}
void Application::AutoPowerEvent(PowerEventData &data) {
    AppLog("[Application::AutoPowerEvent]:");
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
			pContext->SetViewport(pWindow->GetContexttSize().Width, pWindow->GetContexttSize().Height);
			break;
		}

		default: {
			break;
		}
	}
}

void Application::AutoContextEvent(ContextEventData &data) {
    AppLog("[Application::AutoContextEvent]:");
}

}
