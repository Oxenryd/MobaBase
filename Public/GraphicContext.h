#ifndef GRAPHICCONTEXT_H
#define GRAPHICCONTEXT_H

#include <vector>
#include <thread>

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
	virtual void notifyViewResized(void* ctx, uint16_t width, uint16_t height) = 0;
	void setPendingExit() { m_pendingExit = true; }
	const bool& isPendingExit() const { return m_pendingExit; }

	void issueRender(void* renderContext) {

	}

protected:
	bool m_pendingExit = false;
	std::thread m_renderThread;

	void _renderThreadMethod() {

	}
};

#endif