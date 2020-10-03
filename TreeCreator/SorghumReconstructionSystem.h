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

	struct PlantNode
	{
		glm::vec3 Position;
		float Theta;
		float Width;
		glm::vec3 Axis;
		PlantNode(glm::vec3 position, float angle, float width, glm::vec3 axis)
		{
			Position = position;
			Theta = angle;
			Width = width;
			Axis = axis;
		}
	};
	
	class Spline : public SharedComponentBase {
	public:
		glm::vec3 Left;
		float StartingPoint;

		std::vector<PlantNode> Nodes;
		
		std::vector<LeafSegment> Segments;
		std::vector<BezierCurve> Curves;
		std::vector<Vertex> Vertices;
		std::vector<unsigned> Indices;
		void Import(std::ifstream& stream);
		glm::vec3 EvaluatePointFromCurve(float point)
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

		glm::vec3 EvaluateAxisFromCurve(float point)
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
		EntityArchetype _PlantArchetype;
		EntityArchetype _LeafArchetype;
		EntityQuery _SplineQuery;
		EntityQuery _PlantQuery;
		std::shared_ptr<Material> _StemMaterial;
		std::shared_ptr<Material> _LeafMaterial;
		void DrawGUI();
		void ObjExportHelper(glm::vec3 position, std::shared_ptr<Mesh> mesh, std::ofstream& of, unsigned& startIndex) const;
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