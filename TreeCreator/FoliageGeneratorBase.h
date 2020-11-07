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
	class FoliageGeneratorBase
	{
	public:
		virtual void Generate(Entity treeEntity) = 0;
		virtual void OnParamGui() = 0;
	};

	class DefaultFoliageGenerator : public FoliageGeneratorBase
	{
		void GenerateLeaves(Entity& internode, TreeParameters& treeParameters, glm::mat4& treeTransform, std::vector<glm::mat4>& leafTransforms, bool isLeft);
	public:
		
		void Generate(Entity tree) override;
		void OnParamGui() override;
	};

	class AcaciaFoliageGenerator : public FoliageGeneratorBase
	{
	public:
		void Generate(Entity tree) override;
		void OnParamGui() override;
	};

	
}