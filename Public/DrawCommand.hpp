#ifndef DRAWCOMMAND_HPP
#define DRAWCOMMAND_HPP

class MaterialInstance;

enum class DrawType : uint8_t
{
	Mesh,
	SkinnedMesh,
	Billboard,
	Sprite,
	UI,
	Custom
};

struct DrawCommand
{
	MaterialInstance* material;
	void* drawContextPtr;
	uint16_t priority;
	DrawType type;
	bool instanceRequest;
};

#endif