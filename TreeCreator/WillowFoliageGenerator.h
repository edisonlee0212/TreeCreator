#pragma once
#include "FoliageGeneratorBase.h"
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct WillowFoliageInfo : ComponentBase
	{
		float InhibitorLimit = 0.001f;
		float DownDistanceMean = 3.0f;
		float DownDistanceVariance = 1.0f;
		float LowLimit = 1.0f;
		float PushDistance = 1.0f;
		float Thickness = 0.01f;
		float BendAngle = 25.0f;
		int SubdivisionAmount = 8;
		int LeafAmount = 25;
		glm::vec3 LeafSize = glm::vec3(0.01f, 1.0f, 0.05f);
	};
	
	class WillowFoliageGenerator : public FoliageGeneratorBase
	{
		WillowFoliageInfo _DefaultFoliageInfo;
		EntityArchetype _Archetype;
		std::shared_ptr<Texture2D> _LeafSurfaceTex;
		std::shared_ptr<Texture2D> _BranchletSurfaceTex;
		std::shared_ptr<Material> _LeafMaterial;
		std::shared_ptr<Material> _BranchletMaterial;

		void SimpleMeshGenerator(Branchlet& branchlet, std::vector<Vertex>& vertices, std::vector<unsigned>& indices);
	public:
		WillowFoliageGenerator();
		void Generate(Entity tree) override;
		void OnParamGui() override;
	};
}
