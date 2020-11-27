#pragma once
#include "FoliageGeneratorBase.h"
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct OakFoliageInfo : ComponentBase
	{
		int OrderLimit = 6;
		int LeafAmount = 20;
		float GenerationRadius = 0.5f;
		glm::vec3 LeafSize = glm::vec3(0.1f, 1.0f, 0.1f);
	};

	class OakFoliageGenerator :
		public FoliageGeneratorBase
	{
		OakFoliageInfo _DefaultFoliageInfo;
		EntityArchetype _Archetype;
		std::shared_ptr<Texture2D> _LeafSurfaceTex;
		std::shared_ptr<Material> _LeafMaterial;
	public:
		OakFoliageGenerator();
		void Generate() override;
		void OnGui() override;
	};

}