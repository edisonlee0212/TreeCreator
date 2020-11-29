#pragma once

#include "UniEngine.h"
#include "TreeManager.h"
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
		bool IsLeaf;
		LeafSegment(glm::vec3 position, glm::vec3 up, glm::vec3 front, float leafHalfWidth, float theta, bool isLeaf,
			float leftFlatness = 0.0f,
			float rightFlatness = 0.0f,
			float leftFlatnessFactor = 1.0f,
			float rightFlatnessFactor = 1.0f)
		{
			IsLeaf = isLeaf;
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
				auto midRibMaxHeight = LeafHalfWidth / 8.0f;
				auto midRibMaxAngle = Theta / 5.0f;
				
				auto midRibHeight = 0.0f;
				if(glm::abs(angle) < midRibMaxAngle)
				{
					midRibHeight = midRibMaxHeight * glm::cos(glm::radians(90.0f * glm::abs(angle) / midRibMaxAngle));
				}
				
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
				return center - Radius * direction - actualHeight * compressFactor * Up/* + midRibHeight * Up*/;
			}
			auto direction = glm::rotate(Up, glm::radians(angle), Front);
			return Position - Radius * direction;
		}
	};
}

