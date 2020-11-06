#pragma once
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct TreeParameters;

	class FoliageGeneratorBase
	{
	public:
		virtual void Generate(Entity treeEntity) = 0;
		virtual void OnGui() = 0;
	};

	class DefaultFoliageGenerator : public FoliageGeneratorBase
	{
		void GenerateLeaves(Entity& internode, TreeParameters& treeParameters, glm::mat4& treeTransform, std::vector<glm::mat4>& leafTransforms, bool isLeft);
	public:
		
		void Generate(Entity tree) override;
		void OnGui() override;
	};

	class AcaciaFoliageGenerator : public FoliageGeneratorBase
	{
	public:
		void Generate(Entity tree) override;
		void OnGui() override;
	};

	class WillowFoliageGenerator : public FoliageGeneratorBase
	{
	public:
		void Generate(Entity tree) override;
		void OnGui() override;
	};
}