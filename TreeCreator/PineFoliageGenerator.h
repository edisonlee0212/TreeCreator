#pragma once
#include "FoliageGeneratorBase.h"
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct PineFoliageInfo : ComponentBase
	{
		float LengthLimit = 1.0f;
		glm::vec3 LeafSize = glm::vec3(0.015f, 1.0f, 0.2f);
		int SideLeafAmount = 5;
		float BendAngleMean = -50;
		float BendAngleVariance = 5;
	};
	class PineFoliageGenerator : public FoliageGeneratorBase
	{
		PineFoliageInfo _DefaultFoliageInfo;
		EntityArchetype _Archetype;
		static std::shared_ptr<Texture2D> _LeafSurfaceTex;
		std::shared_ptr<Material> _LeafMaterial;
	public:
		PineFoliageGenerator();
		void Generate() override;
		void OnGui() override;
	};
}
