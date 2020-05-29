#pragma once
#include "Core.h"

#ifdef APP_PLATFORM_WINDOWS

extern Omnia::Application *Omnia::CreateApplication();

int main(int argc, char **argv) {
	auto app = Omnia::CreateApplication();
	app->Run();
	delete app;

	return 0;
}

#endif
