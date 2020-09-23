#pragma once
#include "TreeUtilitiesAPI.h"
namespace TreeUtilities {
	class TREEUTILITIES_API Curve
	{
	public:
		virtual glm::vec3 GetPoint(float t) const = 0;
		virtual glm::vec3 GetAxis(float t) const = 0;
		void GetUniformCurve(size_t pointAmount, std::vector<glm::vec3>& points);
	private:
	};
}
