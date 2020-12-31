#pragma once
#include "UniEngine.h"
#include "TreeManager.h"
#include "FoliageGeneratorBase.h"
#include "TreeVolume.h"
using namespace UniEngine;
namespace TreeUtilities {

	struct CakeTowerInfo : ComponentBase
	{
		
	};
	
	struct CakeSlice
	{
		float MaxDistance;
	};
	
	class CakeTower :
		public TreeVolume
	{
		glm::ivec2 SelectSlice(glm::vec3 position) const;
		EntityArchetype _CakeTowerArchetype;
		
		std::vector<std::shared_ptr<Mesh>> _BoundMeshes;
		bool _MeshGenerated = false;
		std::shared_ptr<Material> _CakeTowerMaterial;
		
	public:
		float MaxHeight = 0.0f;
		float MaxRadius = 0.0f;
		void GenerateMesh();
		void FormEntity();
		CakeTower();
		std::string Save();
		void Load(const std::string& path);
		glm::vec4 DisplayColor = glm::vec4(0.5f);
		float DisplayScale = 0.2f;
		int SliceAmount = 9;
		int SectorAmount = 12;
		std::vector<std::vector<CakeSlice>> CakeTiers;
		void CalculateVolume() override;
		void CalculateVolume(float maxHeight);
		bool InVolume(glm::vec3 position) const override;
		void OnGui() override;
		void GenerateAttractionPoints();
		void GenerateAttractionPoints(int amount) override;
	};
}
