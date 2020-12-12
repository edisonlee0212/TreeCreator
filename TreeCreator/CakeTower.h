#pragma once
#include "UniEngine.h"
#include "TreeManager.h"
#include "FoliageGeneratorBase.h"
#include "TreeVolume.h"
using namespace UniEngine;
namespace TreeUtilities {
	struct CakeSlice
	{
		float MaxDistance;
	};
	
	class CakeTower :
		public TreeVolume
	{
		glm::ivec2 SelectSlice(glm::vec3 position) const;
		float _MaxHeight = 0.0f;
		//float _MaxRadius = 0.0f;
		std::shared_ptr<Mesh> _BoundMesh;
		bool _MeshGenerated = false;
		void GenerateMesh();
	public:
		std::string Serialize();
		void Deserialize(const std::string& path);
		glm::vec4 DisplayColor = glm::vec4(0.5f);
		float DisplayScale = 0.2f;
		int TierAmount = 1;
		int SliceAmount = 3;
		std::vector<std::vector<CakeSlice>> CakeTiers;
		void CalculateVolume() override;
		bool InVolume(glm::vec3 position) const override;
		void OnGui() override;
	};
}
