#pragma once

#include "UniEngine.h"
#include "TreeUtilities.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace SorghumReconstruction {
	struct LeafSegment
	{
		glm::vec3 Position;
		glm::quat Rotation;
		float HalfWidth;
		float Theta;

		LeafSegment(glm::vec3 position, glm::quat rotation, float halfWidth, float theta)
		{
			Position = position;
			Rotation = rotation;
			HalfWidth = halfWidth;
			Theta = theta;
		}

		glm::vec3 GetPoint(float angle) const
		{
			const auto down = glm::normalize(Rotation * glm::vec3(0.0f, -1.0f, 0.0f));
			const auto front = glm::normalize(Rotation * glm::vec3(0.0f, 0.0f, -1.0f));
			if(Theta < 90.0f)
			{
				const auto distanceToCenter = HalfWidth / glm::tan(glm::radians(Theta));
				const auto center = Position - distanceToCenter * down;
				const auto direction = glm::rotate(down, glm::radians(angle), front);
				return center + HalfWidth / glm::sin(glm::radians(Theta)) * direction;
			}
			const auto direction = glm::rotate(down, glm::radians(angle), front);
			return Position + HalfWidth * direction;
		}
	};
}

