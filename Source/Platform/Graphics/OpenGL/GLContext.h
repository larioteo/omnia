#pragma once

#include "Omnia/Graphics/Context.h"

namespace Omnia { namespace Gfx {

struct ContextData;
struct ContextProperties;

class GLContext: public Context {
	ContextData *Data;
	ContextProperties *Properties;
	void *pWindow;

public:
	GLContext(void *window);

	virtual void Load() override;
	virtual void SwapBuffers() override;
};

}}
