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

	struct Spline : ComponentBase {
		float StartingPoint;
		std::vector<BezierCurve>* Curves;
		std::vector<Vertex>* Vertices;
		std::vector<unsigned>* Indices;
		void Import(std::ifstream& stream);
	};


	class SorghumReconstructionSystem :
		public SystemBase
	{
		EntityArchetype _TruckArchetype;
		EntityArchetype _LeafArchetype;
		EntityQuery _SplineQuery;

		Material* _TruckMaterial;
		Material* _LeafMaterial;

		void DeleteTruck(Entity& truckEntity);

		Entity CreateTruck();
		Entity CreateLeafForTruck(Entity& truckEntity);
	public:

		Entity CreatePlant(std::string path, float resolution);

		void OnCreate();
		void OnDestroy();
		void Update();
		void FixedUpdate();
	};

}