#pragma once
#include "UniEngine.h"
#include "TreeManager.h"
#include "FoliageGeneratorBase.h"
#include "TreeVolume.h"
using namespace UniEngine;
namespace TreeUtilities {
	class KDop : public TreeVolume
	{
	public:
		float DirectionalDistance[26] = { 1.0f };
		void CalculateVolume() override;
		bool InVolume(glm::vec3 position) const override;
		void OnGui() override;
	};
}