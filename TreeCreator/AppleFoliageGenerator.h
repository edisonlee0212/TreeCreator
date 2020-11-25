#pragma once
#include "FoliageGeneratorBase.h"
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct AppleFoliageInfo : ComponentBase
	{
		glm::vec2 LeafSize = glm::vec2(0.1f);
		float LeafIlluminationLimit = 0;
		float LeafInhibitorFactor = 0;
		bool IsBothSide = true;
		int SideLeafAmount = 1;
		float StartBendingAngle = 45;
		float BendingAngleIncrement = 0;
		float LeafPhotoTropism = 999.0f;
		float LeafGravitropism = 1.0f;
		float LeafDistance = 0;
	};
	
    class AppleFoliageGenerator :
        public FoliageGeneratorBase
    {
		AppleFoliageInfo _DefaultFoliageInfo;
		EntityArchetype _Archetype;
		std::shared_ptr<Texture2D> _LeafSurfaceTex;
		std::shared_ptr<Material> _LeafMaterial;
		void GenerateLeaves(Entity& internode, glm::mat4& treeTransform, std::vector<glm::mat4>& leafTransforms, bool isLeft);
	public:
		AppleFoliageGenerator();
		void Generate(Entity tree) override;
		void OnParamGui() override;
    };

}