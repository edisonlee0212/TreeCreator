#pragma once
#include "FoliageGeneratorBase.h"
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	struct WillowFoliageInfo : ComponentBase
	{
		
	};
	
	class WillowFoliageGenerator : public FoliageGeneratorBase
	{
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
