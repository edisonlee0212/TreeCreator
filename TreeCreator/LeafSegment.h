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
		float LeftFlatness;
		float LeftFlatnessFactor;
		float RightFlatness;
		float RightFlatnessFactor;
		LeafSegment(glm::vec3 position, glm::vec3 up, glm::vec3 front, float leafHalfWidth, float theta, 
			float leftFlatness = 0.0f,
			float rightFlatness = 0.0f,
			float leftFlatnessFactor = 1.0f,
			float rightFlatnessFactor = 1.0f)
		{
			Position = position;
			Up = up;
			Front = front;
			LeafHalfWidth = leafHalfWidth;
			Theta = theta;
			LeftFlatness = leftFlatness;
			RightFlatness = rightFlatness;
			LeftFlatnessFactor = leftFlatnessFactor;
			RightFlatnessFactor = rightFlatnessFactor;
			Radius = theta < 90.0f ? LeafHalfWidth / glm::sin(glm::radians(Theta)) : LeafHalfWidth;
		}

		glm::vec3 GetPoint(float angle)
		{
			if(Theta < 90.0f)
			{
				auto distanceToCenter = Radius * glm::cos(glm::radians(glm::abs(angle)));
				auto actualHeight = (Radius - distanceToCenter) * (angle < 0 ? LeftFlatness : RightFlatness);
				auto maxHeight = Radius * (1.0f - glm::cos(glm::radians(glm::abs(Theta))));
				auto center = Position + (Radius - LeafHalfWidth) * Up;
				auto direction = glm::rotate(Up, glm::radians(angle), Front);
				float compressFactor = glm::pow(actualHeight / maxHeight, angle < 0 ? LeftFlatnessFactor : RightFlatnessFactor);
				if(glm::isnan(compressFactor))
				{
					compressFactor = 0.0f;
				}
				return center - Radius * direction - actualHeight * compressFactor * Up;
			}
			auto direction = glm::rotate(Up, glm::radians(angle), Front);
			return Position - Radius * direction;
		}
	};
}

