#pragma once
#include "FoliageGeneratorBase.h"
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct AppleFoliageInfo : ComponentBase
	{
		int OrderLimit = 6;
		int LeafAmount = 30;
		float GenerationRadius = 0.25f;
		float YCompress = 1.0f;
		glm::vec3 LeafSize = glm::vec3(0.05f, 1.0f, 0.1f);
	};
	
    class AppleFoliageGenerator :
        public FoliageGeneratorBase
    {
		AppleFoliageInfo _DefaultFoliageInfo;
		EntityArchetype _Archetype;
		std::shared_ptr<Texture2D> _LeafSurfaceTex;
		std::shared_ptr<Material> _LeafMaterial;
	public:
		AppleFoliageGenerator();
		void Generate() override;
		void OnGui() override;
    };

}