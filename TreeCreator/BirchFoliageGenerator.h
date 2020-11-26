#pragma once
#include "FoliageGeneratorBase.h"
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct BirchFoliageInfo : ComponentBase
	{
		float DistanceLimit = 0.5f;
		int LeafAmount = 10;
		float GenerationRadius = 0.5f;
		glm::vec3 LeafSize = glm::vec3(0.12f, 1.0f, 0.12f);
	};

	class BirchFoliageGenerator :
		public FoliageGeneratorBase
	{
		BirchFoliageInfo _DefaultFoliageInfo;
		EntityArchetype _Archetype;
		std::shared_ptr<Texture2D> _LeafSurfaceTex;
		std::shared_ptr<Material> _LeafMaterial;
	public:
		BirchFoliageGenerator();
		void Generate() override;
		void OnGui() override;
	};

}