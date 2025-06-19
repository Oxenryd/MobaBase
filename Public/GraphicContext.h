#ifndef GRAPHICCONTEXT_H
#define GRAPHICCONTEXT_H

#include <vector>

#include "Shader.hpp"

class GraphicContext
{
public:
	explicit GraphicContext(WindowSurface* const wndSurface) :
		windowSurface{ wndSurface } {}

	virtual ~GraphicContext() {

	}

	WindowSurface* const windowSurface;

	virtual void draw(void* rendCtx) = 0;
};

#endif