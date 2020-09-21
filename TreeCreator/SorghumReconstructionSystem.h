#pragma once
#include "UniEngine.h"
#include "TreeUtilities.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace SorghumRecon {
	struct TruckInfo : ComponentBase {
	};

	struct LeafInfo : ComponentBase {
	};

	class Spline : public SharedComponentBase {
	public:
		float StartingPoint;
		std::vector<BezierCurve>* Curves;
		std::vector<Vertex>* Vertices;
		std::vector<unsigned>* Indices;
		void Import(std::ifstream& stream);

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
	public:

		Entity CreatePlant(std::string path, float resolution) const;

		void OnCreate() override;
		void OnDestroy() override;
		void Update() override;
		void FixedUpdate() override;
	};

}