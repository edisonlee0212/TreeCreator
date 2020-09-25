#pragma once
#include "UniEngine.h"
#include "TreeUtilities.h"
#include "LeafSegment.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace SorghumReconstruction {
	struct SorghumInfo : ComponentBase {
	};

	struct LeafInfo : ComponentBase {
	};

	class Spline : public SharedComponentBase {
	public:
		glm::vec3 Left;
		bool IsMin;
		float StartingPoint;
		glm::vec3 EndPoint;
		float BreakingHeight;
		glm::vec3 BreakingPoint;
		float TopHeight;
		float StemWidth;
		std::vector<LeafSegment> Segments;
		std::vector<BezierCurve> Curves;
		std::vector<Vertex> Vertices;
		std::vector<unsigned> Indices;
		void Import(std::ifstream& stream);
		void BuildStem(std::shared_ptr<Spline>& truckSpline);
		glm::vec3 EvaluatePoint(float point)
		{
			const float splineU = glm::clamp(point, 0.0f, 1.0f) * float(Curves.size());

			// Decompose the global u coordinate on the spline
			float integerPart;
			const float fractionalPart = modff(splineU, &integerPart);

			auto curveIndex = int(integerPart);
			auto curveU = fractionalPart;

			// If evaluating the very last point on the spline
			if (curveIndex == Curves.size() && curveU <= 0.0f)
			{
				// Flip to the end of the last patch
				curveIndex--;
				curveU = 1.0f;
			}
			return Curves.at(curveIndex).GetPoint(curveU);
		}

		glm::vec3 EvaluateAxis(float point)
		{
			const float splineU = glm::clamp(point, 0.0f, 1.0f) * float(Curves.size());

			// Decompose the global u coordinate on the spline
			float integerPart;
			const float fractionalPart = modff(splineU, &integerPart);

			auto curveIndex = int(integerPart);
			auto curveU = fractionalPart;

			// If evaluating the very last point on the spline
			if (curveIndex == Curves.size() && curveU <= 0.0f)
			{
				// Flip to the end of the last patch
				curveIndex--;
				curveU = 1.0f;
			}
			return Curves.at(curveIndex).GetAxis(curveU);
		}

		float EvaluateTheta(float point)
		{
			glm::vec3 position = EvaluatePoint(point);
			float height = position.z;
			float distance = glm::distance(position, BreakingPoint);
			float xyDistance = glm::distance(glm::vec2(position.x, position.y), glm::vec2(BreakingPoint.x, BreakingPoint.y));
			if (height < BreakingHeight && xyDistance < 1.0f)
			{
				return 180;
			}
			float percent = distance / glm::distance(BreakingPoint, EndPoint);
			
			if (percent < 0.3f)
			{
				return 180.0f - percent * 300.0f;
			}
			if (percent < 0.5f)
			{
				return 90.0f - (percent - 0.3f) * 350.0f;
			}
			return 20.0f;
		}
		
		float EvaluateWidth(float point)
		{
			glm::vec3 position = EvaluatePoint(point);
			float height = position.z;
			float distance = glm::distance(position, BreakingPoint);
			float xyDistance = glm::distance(glm::vec2(position.x, position.y), glm::vec2(BreakingPoint.x, BreakingPoint.y));
			if (height < BreakingHeight && xyDistance < 1.0f)
			{
				float ret = StemWidth;
				if(!IsMin)
				{
					if(distance > 0.5f)
					{
						ret -= (distance - 0.5f) * 0.2f;
					}
				}
				return ret;
			}

			float percent = distance / glm::distance(BreakingPoint, EndPoint);
			
			if(percent < 0.2f)
			{
				return StemWidth + (IsMin ? 0.01f : 0.0f);
			}
			if (percent < 0.4f)
			{
				return StemWidth + (percent - 0.2f) * 0.5f + (IsMin ? 0.01f : 0.0f);
			}
			if (percent < 0.7f)
			{
				return StemWidth + 0.1f + (IsMin ? 0.01f : 0.0f);
			}
			return StemWidth + 0.1f - (percent - 0.7) * 0.4f + (IsMin ? 0.01f : 0.0f);
		}
		size_t GetHashCode() override;
	};

	inline size_t Spline::GetHashCode()
	{
		return (size_t)this;
	}

	
	class SorghumReconstructionSystem :
		public SystemBase
	{
		EntityArchetype _PlantArchetype;
		EntityArchetype _LeafArchetype;
		EntityQuery _SplineQuery;
		EntityQuery _PlantQuery;
		std::shared_ptr<Material> _PlantMaterial;
		std::shared_ptr<Material> _LeafMaterial;
		void DrawGUI();
		
	public:
		Entity CreatePlant() const;
		Entity CreateLeafForPlant(Entity& plantEntity) const;
		void GenerateMeshForAllPlants();
		Entity ImportPlant(std::string path, float resolution, std::string name) const;
		void ExportPlant(Entity plant, std::string path) const;
		void OnCreate() override;
		void OnDestroy() override;
		void Update() override;
		void FixedUpdate() override;
	};

}