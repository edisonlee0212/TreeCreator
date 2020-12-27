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
		friend class PlantSimulationSystem;
	protected:
		int _AttractionPointsCount = 2000;
		float _RemoveDistance = 1.0f;
		float _AttractDistance = 2.0f;
		bool _EnableSpaceColonization;
		glm::vec3 _Center;
		float _SphereRadius;
		glm::vec4 _DisplayColor = glm::vec4(0.0f, 0.0f, 1.0f, 0.5f);
		bool _Display = true;
		bool _PruneBuds = false;
		TreeVolumeType _Type = TreeVolumeType::Default;
		void GenerateAttractionPoints(glm::vec3 min, glm::vec3 max, int amount) const;
	public:
		void ClearAttractionPoints() const;
		virtual void GenerateAttractionPoints(int amount);
		void SetPruneBuds(bool value);
		bool PruneBuds() const;
		virtual void CalculateVolume();
		virtual bool InVolume(glm::vec3 position) const;
		void OnGui() override;
	};
}