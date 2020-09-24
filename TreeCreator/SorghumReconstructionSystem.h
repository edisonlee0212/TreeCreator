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
		float StartingPoint;
		std::vector<LeafSegment> Segments;
		std::vector<BezierCurve> Curves;
		std::vector<Vertex> Vertices;
		std::vector<unsigned> Indices;
		void Import(std::ifstream& stream);
		void FakeStart();
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
			if (point < 0.1f)
			{
				return 180.0f;
			}
			if (point < 0.2f)
			{
				return 180.0f - (point - 0.1f) * 900.0f;
			}
			if (point < 0.4f)
			{
				return 90.0f - (point - 0.2f) * 400.0f;
			}
			return 10.0f;
		}
		
		float EvaluateWidth(float point)
		{
			if (point < 0.01f)
			{
				return 0.01f;
			}
			if (point < 0.05f)
			{
				return point;
			}
			if (point < 0.1f)
			{
				return 0.05f;
			}
			if (point < 0.2f)
			{
				return 0.05f + (point - 0.1f) * 0.95f;
			}
			if (point < 0.4f)
			{
				return 0.1f + (point - 0.2f) * 0.5f;
			}
			if(point < 0.8f)
			{
				return 0.2f;
			}
			return 1.0f - point * 0.9f;
			
			if (point < 0.05f)
			{
				return point;
			}
			if(point < 0.3f)
			{
				return 0.15f;
			}
			if (point < 0.8f)
			{
				return 0.2f;
			}
			return 1.0f - point * 0.9f;
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