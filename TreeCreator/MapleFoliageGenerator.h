#pragma once
#include "FoliageGeneratorBase.h"
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct MapleFoliageInfo : ComponentBase
	{
		float InhibitorLimit;
		float IlluminationLimit;
		float BendAngleMean = 25.0f;
		float BendAngleVariance = 5.0f;
		int LeafAmount = 25;
		glm::vec3 LeafSize = glm::vec3(0.01f, 1.0f, 0.05f);
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
		void Generate(Entity tree) override;
		void OnParamGui() override;
    };
}
