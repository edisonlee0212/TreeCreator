#pragma once
#include "FoliageGeneratorBase.h"
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct WillowFoliageInfo : ComponentBase
	{
		float InhibitorLimit = 0.001f;
		float DownDistanceMean = 2.0f;
		float DownDistanceVariance = 1.0f;
		float LowLimit = 1.0f;
		float PushDistance = 0.75f;
		float Thickness = 0.01f;
		float BendAngleMean = 25.0f;
		float BendAngleVariance = 5.0f;
		int SubdivisionAmount = 3;
		int LeafAmount = 15;
		glm::vec3 LeafSize = glm::vec3(0.05f, 1.0f, 0.1f);
	};
	
	class WillowFoliageGenerator : public FoliageGeneratorBase
	{
		WillowFoliageInfo _DefaultFoliageInfo;
		EntityArchetype _Archetype;
		static std::shared_ptr<Texture2D> _LeafSurfaceTex;
		static std::shared_ptr<Texture2D> _BranchletSurfaceTex;
		std::shared_ptr<Material> _LeafMaterial;
		std::shared_ptr<Material> _BranchletMaterial;

		void SimpleMeshGenerator(Branchlet& branchlet, std::vector<Vertex>& vertices, std::vector<unsigned>& indices);
	public:
		WillowFoliageGenerator();
		void Generate() override;
		void OnGui() override;
	};
}
