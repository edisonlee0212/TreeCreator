#pragma once
#include "FoliageGeneratorBase.h"
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct OakFoliageInfo : ComponentBase
	{
		glm::vec2 LeafSize = glm::vec2(0.06f);
		float LeafIlluminationLimit = 0;
		float LeafInhibitorFactor = 0;
		bool IsBothSide = true;
		int SideLeafAmount = 3;
		float StartBendingAngle = 20;
		float BendingAngleIncrement = 20;
		float LeafPhotoTropism = 999.0f;
		float LeafGravitropism = 1.0f;
		float LeafDistance = 0.2;
	};

	class OakFoliageGenerator :
		public FoliageGeneratorBase
	{
		OakFoliageInfo _DefaultFoliageInfo;
		EntityArchetype _Archetype;
		std::shared_ptr<Texture2D> _LeafSurfaceTex;
		std::shared_ptr<Material> _LeafMaterial;
		void GenerateLeaves(Entity& internode, glm::mat4& treeTransform, std::vector<glm::mat4>& leafTransforms, bool isLeft);
	public:
		OakFoliageGenerator();
		void Generate() override;
		void OnGui() override;
	};

}