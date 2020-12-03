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
		float _MaxRadius = 0.0f;
	public:
		glm::vec4 DisplayColor = glm::vec4(1.0f);
		float DisplayScale = 0.2f;
		int TierAmount = 10;
		int SliceAmount = 9;
		std::vector<std::vector<CakeSlice>> CakeTiers;
		void CalculateVolume() override;
		bool InVolume(glm::vec3 position) const override;
		void OnGui() override;
	};
}
