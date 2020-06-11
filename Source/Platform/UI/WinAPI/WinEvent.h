#pragma once

#include "Omnia/UI/Event.h"

namespace Omnia {

// Events: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644958%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
class WinEventListener: public EventListener {
public:
	// Default
	WinEventListener();
	~WinEventListener();

	void Callback(void *event = nullptr) override;
	void Update() override;

private:
	intptr_t Register(void *event);
};

}
