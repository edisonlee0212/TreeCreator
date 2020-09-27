#pragma once

#include "UniEngine.h"
#include "TreeUtilities.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace SorghumReconstruction {
	struct LeafSegment
	{
		glm::vec3 Position;
		glm::vec3 Front;
		glm::vec3 Up;
		glm::quat Rotation;
		float HalfWidth;
		float Theta;
		float Radius;
		LeafSegment(glm::vec3 position, glm::vec3 up, glm::vec3 front, float halfWidth, float theta)
		{
			Position = position;
			Up = up;
			Front = front;
			HalfWidth = halfWidth;
			Theta = theta;
			Radius = theta < 90.0f ? HalfWidth / glm::sin(glm::radians(Theta)) : HalfWidth;
		}

		glm::vec3 GetPoint(float angle)
		{
			if(Theta < 90.0f)
			{
				auto distanceToCenter = HalfWidth / glm::tan(glm::radians(Theta));
				auto center = Position + (Radius - HalfWidth) * Up;
				auto direction = glm::rotate(Up, glm::radians(angle), Front);
				return center - Radius * direction;
			}
			auto direction = glm::rotate(Up, glm::radians(angle), Front);
			return Position - Radius * direction;
		}
	};
}

