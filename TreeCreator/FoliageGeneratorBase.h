#pragma once
#include "InternodeRingSegment.h"
#include "UniEngine.h"
using namespace UniEngine;
namespace TreeUtilities {
	struct TreeParameters;
	struct Branchlet
	{
		std::vector<InternodeRingSegment> Rings;
		std::vector<glm::mat4> LeafLocalTransforms;
		glm::vec3 Normal;
	};
	class FoliageGeneratorBase : public PrivateComponentBase
	{
	public:
		virtual void Generate() = 0;
	};

	struct DefaultFoliageInfo : ComponentBase
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

	class DefaultFoliageGenerator : public FoliageGeneratorBase
	{
		DefaultFoliageInfo _DefaultFoliageInfo;
		EntityArchetype _Archetype;
		static std::shared_ptr<Texture2D> _LeafSurfaceTex;
		std::shared_ptr<Material> _LeafMaterial;
		void GenerateLeaves(Entity& internode, glm::mat4& treeTransform, std::vector<glm::mat4>& leafTransforms, bool isLeft);
	public:
		DefaultFoliageGenerator();
		void Generate() override;
		void OnGui() override;
	};
}