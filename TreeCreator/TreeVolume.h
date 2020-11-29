#pragma once
#include "UniEngine.h"
#include "TreeManager.h"
#include "FoliageGeneratorBase.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace TreeUtilities
{
	class TreeVolume : public PrivateComponentBase
	{
		Bound _Bound;
		glm::vec4 _DisplayColor = glm::vec4(0.0f, 0.0f, 1.0f, 0.5f);
		bool _Display = true;
		bool _PruneBuds = false;
		TreeVolumeType _Type = TreeVolumeType::Default;
	public:
		void SetPruneBuds(bool value);
		bool PruneBuds() const;

		bool InVolume(glm::vec3 position) const;

		void OnGui() override;
	};
	
}