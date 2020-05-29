#pragma once

#include "Core.h"

namespace Omnia {

class APP_API Application {
public:
	Application();
	virtual ~Application();

	void Run();
};

/// @brief To be defined in the client.
/// @return 
Application *CreateApplication();

}
