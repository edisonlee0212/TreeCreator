#include "pch.h"
#include "BezierCurve.h"
using namespace TreeUtilities;
TreeUtilities::BezierCurve::BezierCurve(glm::vec3 cp0, glm::vec3 cp1, glm::vec3 cp2, glm::vec3 cp3)
	: Curve(),
	CP0(cp0),
	CP1(cp1),
	CP2(cp2),
	CP3(cp3)
{
}

glm::vec3 TreeUtilities::BezierCurve::GetPoint(float t)
{
	float b0, b1, b2, b3;
	b0 = (1.0f - t) * (1.0f - t) * (1.0f - t);
	b1 = 3.0f * t * (1.0f - t) * (1.0f - t);
	b2 = 3.0f * t * t * (1.0f - t);
	b3 = t * t * t;
	return glm::vec3(
		(CP0.x * b0 + CP1.x * b1 + CP2.x * b2 + CP3.x * b3),
		(CP0.y * b0 + CP1.y * b1 + CP2.y * b2 + CP3.y * b3),
		(CP0.z * b0 + CP1.z * b1 + CP2.z * b2 + CP3.z * b3));
}

glm::vec3 TreeUtilities::BezierCurve::GetStartAxis()
{
	return glm::normalize(CP1 - CP0);
}

glm::vec3 TreeUtilities::BezierCurve::GetEndAxis()
{
	return glm::normalize(CP3 - CP2);
}

glm::vec3 TreeUtilities::BezierCurve::Evaluate(float u)
{
	u = glm::clamp(u, 0.f, 1.f);

	return CP0 * (1.0f - u) * (1.0f - u) * (1.0f - u)
		+ CP1 * 3.0f * u * (1.0f - u) * (1.0f - u)
		+ CP2 * 3.0f * u * u * (1.0f - u)
		+ CP3 * u * u * u;
}
