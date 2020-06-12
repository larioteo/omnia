#pragma once

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"
#include "Omnia/Utility/Message.h"

#include "EventData.h"

namespace Omnia {

class EventListener {
public:
	// Default
	EventListener() {};
	virtual ~EventListener() = default;
	static Scope<EventListener> Create();

	// Events
	virtual bool Callback(void *event = nullptr) = 0;
	virtual void Update() = 0;

	// Subjects
	Subject<DeviceEventData> DeviceEvent;
	Subject<PowerEventData> PowerEvent;

	Subject<ControllerEventData> ControllerEvent;
	Subject<KeyboardEventData> KeyboardEvent;
	Subject<MouseEventData> MouseEvent;
	Subject<TouchEventData> TouchEvent;
	Subject<WindowEventData> WindowEvent;

	Subject<ContextEventData> ContextEvent;
};

}
