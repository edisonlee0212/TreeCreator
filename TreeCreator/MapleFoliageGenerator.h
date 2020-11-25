#pragma once
#include "FoliageGeneratorBase.h"
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct MapleFoliageInfo : ComponentBase
	{
		float InhibitorLimit;
		float IlluminationLimit;
		int LeafAmount = 10;
		float GenerationRadius = 0.4f;
		glm::vec3 LeafSize = glm::vec3(0.14f, 1.0f, 0.14f);
	};
	
    class MapleFoliageGenerator :
        public FoliageGeneratorBase
    {
		MapleFoliageInfo _DefaultFoliageInfo;
		EntityArchetype _Archetype;
		std::shared_ptr<Texture2D> _LeafSurfaceTex;
		std::shared_ptr<Material> _LeafMaterial;
    public:
    	
		MapleFoliageGenerator();
		void Generate() override;
		void OnGui() override;
    };
}
