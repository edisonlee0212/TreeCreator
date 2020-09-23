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

glm::vec3 BezierCurve::GetPoint(float t) const
{
	t = glm::clamp(t, 0.f, 1.f);
	return CP0 * (1.0f - t) * (1.0f - t) * (1.0f - t)
		+ CP1 * 3.0f * t * (1.0f - t) * (1.0f - t)
		+ CP2 * 3.0f * t * t * (1.0f - t)
		+ CP3 * t * t * t;
}

glm::vec3 BezierCurve::GetAxis(float t) const
{
	t = glm::clamp(t, 0.f, 1.f);
	float mt = 1.0f - t;
	return (CP1 - CP0) * 3.0f * mt * mt + 6.0f * t * mt * (CP2 - CP1) + 3.0f * t * t * (CP3 - CP2);
}

glm::vec3 TreeUtilities::BezierCurve::GetStartAxis() const
{
	return glm::normalize(CP1 - CP0);
}

glm::vec3 TreeUtilities::BezierCurve::GetEndAxis() const
{
	return glm::normalize(CP3 - CP2);
}

