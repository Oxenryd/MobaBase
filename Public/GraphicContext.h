#ifndef GRAPHICCONTEXT_H
#define GRAPHICCONTEXT_H

#include <vector>

#include "Shader.hpp"

class GraphicContext
{
public:
	explicit GraphicContext(WindowSurface* const wndSurface) :
		windowSurface{ wndSurface } {}

	virtual ~GraphicContext() {}

	virtual bool compileShader(Shader& shader) = 0;
	virtual bool checkForShaderChanges(Shader& shader) = 0;

	WindowSurface* const windowSurface;
	std::vector<Shader> vertexShaders;
	std::vector<Shader> fragmentShaders;
};

#endif