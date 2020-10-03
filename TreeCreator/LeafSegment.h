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
		float LeafHalfWidth;
		float Theta;
		float Radius;
		float HeightFactor;
		LeafSegment(glm::vec3 position, glm::vec3 up, glm::vec3 front, float leafHalfWidth, float theta, float heightFactor = 0.0f)
		{
			Position = position;
			Up = up;
			Front = front;
			LeafHalfWidth = leafHalfWidth;
			Theta = theta;
			HeightFactor = heightFactor;
			Radius = theta < 90.0f ? LeafHalfWidth / glm::sin(glm::radians(Theta)) : LeafHalfWidth;
		}

		glm::vec3 GetPoint(float angle)
		{
			if(Theta < 90.0f)
			{
				auto distanceToCenter = Radius * glm::cos(glm::radians(glm::abs(angle)));
				auto actualHeight = (Radius - distanceToCenter) * HeightFactor;
				auto center = Position + (Radius - LeafHalfWidth) * Up;
				auto direction = glm::rotate(Up, glm::radians(angle), Front);
				return center - Radius * direction - actualHeight * Up;
			}
			auto direction = glm::rotate(Up, glm::radians(angle), Front);
			return Position - Radius * direction;
		}
	};
}

