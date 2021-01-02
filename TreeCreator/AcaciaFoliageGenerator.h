#pragma once
#include "FoliageGeneratorBase.h"
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct AcaciaFoliageInfo : ComponentBase
	{
		int OrderLimit = 11;
		int LeafAmount = 30;
		float GenerationRadius = 1.0f;
		float YCompress = 0.2f;
		glm::vec3 LeafSize = glm::vec3(0.02f, 1.0f, 0.2f);
	};
	class AcaciaFoliageGenerator : public FoliageGeneratorBase
	{
		AcaciaFoliageInfo _DefaultFoliageInfo;
		EntityArchetype _Archetype;
		static std::shared_ptr<Texture2D> _LeafSurfaceTex;
		std::shared_ptr<Material> _LeafMaterial;
	public:
		AcaciaFoliageGenerator();
		
		void Generate() override;
		void OnGui() override;
	};
}

