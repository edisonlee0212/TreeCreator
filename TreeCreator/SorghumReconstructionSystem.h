#pragma once
#include "UniEngine.h"
#include "TreeUtilities.h"
#include "LeafSegment.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace SorghumReconstruction {
	struct TruckInfo : ComponentBase {
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
		
		size_t GetHashCode() override;
	};

	inline size_t Spline::GetHashCode()
	{
		return (size_t)this;
	}

	
	class SorghumReconstructionSystem :
		public SystemBase
	{
		EntityArchetype _TruckArchetype;
		EntityArchetype _LeafArchetype;
		EntityQuery _SplineQuery;

		std::shared_ptr<Material> _TruckMaterial;
		std::shared_ptr<Material> _LeafMaterial;

		Entity CreateTruck() const;
		Entity CreateLeafForTruck(Entity& truckEntity) const;

		void DrawGUI();
		
	public:

		Entity CreatePlant(std::string path, float resolution, std::string name) const;
		void ExportPlant(Entity plant, std::string path) const;
		void OnCreate() override;
		void OnDestroy() override;
		void Update() override;
		void FixedUpdate() override;
	};

}