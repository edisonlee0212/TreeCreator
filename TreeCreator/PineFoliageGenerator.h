#pragma once
#include "FoliageGeneratorBase.h"
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct PineFoliageInfo : ComponentBase
	{
		float InhibitorLimit = 0.001f;
		float IlluminationLimit = 0.001f;
		glm::vec3 LeafSize = glm::vec3(0.1f, 1.0f, 0.15f);
		int SideLeafAmount = 5;
		float BendAngleMean = 60;
		float BendAngleVariance = 10;
	};
	class PineFoliageGenerator : public FoliageGeneratorBase
	{
		PineFoliageInfo _DefaultFoliageInfo;
		EntityArchetype _Archetype;
		std::shared_ptr<Texture2D> _LeafSurfaceTex;
		std::shared_ptr<Material> _LeafMaterial;
	public:
		PineFoliageGenerator();
		void Generate(Entity tree) override;
		void OnParamGui() override;
	};
}
