#pragma once
#include "TreeUtilitiesAPI.h"
#include "Curve.h"
namespace TreeUtilities {
	class TREEUTILITIES_API BezierCurve :
		public Curve
	{
	public:
		BezierCurve(glm::vec3 cp0, glm::vec3 cp1, glm::vec3 cp2, glm::vec3 cp3);
		glm::vec3 GetPoint(float t) const override;
		glm::vec3 GetAxis(float t) const override;
		glm::vec3 GetStartAxis() const;
		glm::vec3 GetEndAxis() const;
		glm::vec3 CP0, CP1, CP2, CP3;
	};
}
