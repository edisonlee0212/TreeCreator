#pragma once
#include "UniEngine.h"
#include "TreeSystem.h"
#include "BranchSystem.h"
#include "InternodeSystem.h"
#include "LightEstimator.h"

#include "InternodeRingSegment.h"
#include "BezierCurve.h"
using namespace UniEngine;
namespace TreeUtilities {
#pragma region Common
	struct TreeIndex : ComponentBase {
		unsigned Value;
		bool operator ==(const TreeIndex& other) const {
			return other.Value == Value;
		}
	};
#pragma endregion

#pragma region Internode


	struct Connection : ComponentBase {
		glm::mat4 Value;
		bool operator ==(const Connection& other) const {
			return other.Value == Value;
		}
	};

	struct Illumination : ComponentBase {
		float Value;
		glm::vec3 LightDir;
	};


	struct Bud {
		bool IsActive;
		bool IsApical;
		glm::vec3 EulerAngles;
	};

	class InternodeData : public PrivateComponentBase {
	public:
		std::mutex InternodeLock;
		std::vector<glm::vec3> Points;
		std::vector<Bud> Buds;
		std::vector<InternodeRingSegment> Rings;
		glm::vec3 NormalDir;
		int step;
		void OnGui() override;
	};

	struct InternodeIndex : ComponentBase {
		unsigned Value;
		bool operator ==(const InternodeIndex& other) const {
			return other.Value == Value;
		}
	};

	struct AttractionPointInfo : ComponentBase
	{
		
	};
	
	struct InternodeInfo : ComponentBase {
		bool Activated = true;
#pragma region General
		int StartAge;
		int Order = 0;
		int Level = 0;
		float DistanceToParent = 0;
		glm::vec3 BranchEndPosition;
		glm::vec3 BranchStartPosition;
		//The distance to
		float DistanceToBranchEnd = 0;
		float LongestDistanceToEnd = 0;
		float TotalDistanceToEnd = 0;
		float DistanceToBranchStart = 0;
		float DistanceToRoot = 0;
		float AccumulatedLength = 0;
		float AccumulatedLight = 0;
		float AccumulatedActivatedBudsAmount = 0;
#pragma endregion
#pragma region Growth
		int MaxChildOrder = 0;
		int MaxChildLevel = 0;
		float Inhibitor = 0;
		float ParentInhibitorFactor = 1;
		int ActivatedBudsAmount = 0;
		unsigned BranchEndInternodeAmount = 0;
		bool Pruned = false;
		int PruneReason = 0;
		bool IsMaxChild = false;
		bool ApicalBudExist = false;
		bool IsEndNode = false;
#pragma endregion
#pragma region Geometric
		glm::vec3 ChildBranchesMeanPosition = glm::vec3(0.0f);
		float MeanWeight;
		float Sagging = 0.0f;
		float Length;
		float Thickness;
		float ParentThickness;
		float Deformation;
		float Straightness;
		float Slope;
		float SiblingAngle;
		float ParentAngle;
#pragma endregion
#pragma region Transformation
		glm::quat DesiredLocalRotation;
		glm::mat4 GlobalTransform;
		glm::mat4 LocalTransform;
#pragma endregion

#pragma region Mesh generation
		glm::vec3 ParentTranslation;
		glm::quat ParentRotation;
		glm::quat MainChildRotation;
		glm::quat ParentMainChildRotation;
		bool IsMainChild = false;
#pragma endregion

		float CrownShyness = 1.0f;
#pragma region Space Colonization
		glm::vec3 DirectionVector;
#pragma endregion

	};
#pragma endregion
#pragma region Tree
	enum class TreeVolumeType
	{
		Default,
		Sphere
	};
	

	struct TreeParameters : ComponentBase {
		int Seed;
		int Age = 0;

#pragma region Geometric
		int LateralBudPerNode;

		float VarianceApicalAngle;

		float BranchingAngleMean;
		float BranchingAngleVariance;

		float RollAngleMean;
		float RollAngleVariance;
#pragma endregion
#pragma region Bud fate
		float ApicalBudKillProbability;
		float LateralBudKillProbability;

		float ApicalDominanceBase;
		float ApicalDominanceDistanceFactor;
		float ApicalDominanceAgeFactor;

		float GrowthRate;

		float InternodeLengthBase;
		float InternodeLengthAgeFactor;


		float ApicalControlBase;
		float ApicalControlAgeFactor;
		float ApicalControlLevelFactor;
		float ApicalControlDistanceFactor;

		int MaxBudAge;
#pragma endregion
#pragma region Environmental
		float InternodeSize = 0.3f;

		float Phototropism;
		float GravitropismBase;
		float GravitropismLevelFactor;

		float PruningFactor;
		float LowBranchPruningFactor;

		float ThicknessRemovalFactor;

		float GravityBendingStrength;
		float GravityBendingAngleFactor;

		float ApicalBudLightingFactor;
		float LateralBudLightingFactor;
#pragma endregion

		float EndNodeThickness;
		float ThicknessControlFactor;

#pragma region CrownShyness
		float CrownShynessBase;
		float CrownShynessFactor = 1.0f;
#pragma endregion

#pragma region Organs
		int FoliageType = 0;
#pragma endregion

	};

	struct TreeAge : ComponentBase {

		int Value;
		int ToGrowIteration;
		bool Enable;
	};

	struct TreeInfo : ComponentBase
	{
		int CurrentSeed;
		float Height;
		int MaxBranchingDepth;
		int LateralBudsCount;
	};

	class TreeData : public PrivateComponentBase {
	public:
		bool MeshGenerated;
		bool FoliageGenerated;
		float ActiveLength;
		std::vector<float> ApicalControlTimeVal;
		std::vector<std::vector<float>> ApicalControlTimeLevelVal;
		std::shared_ptr<Mesh> ConvexHull;
		void OnGui() override;
	};
#pragma endregion
	class TreeManager :
		public Singleton<TreeManager>
	{
		static LightEstimator* _LightEstimator;

		static TreeSystem* _TreeSystem;
		static InternodeSystem* _InternodeSystem;

		static EntityArchetype _InternodeArchetype;
		static EntityArchetype _TreeArchetype;
		static EntityArchetype _AttractionPointArchetype;
		static EntityQuery _TreeQuery;
		static EntityQuery _InternodeQuery;
		static EntityQuery _AttractionPointQuery;
		static TreeIndex _TreeIndex;
		static InternodeIndex _InternodeIndex;

		static bool _Ready;

		static void SimpleMeshGenerator(Entity& internode, std::vector<Vertex>& vertices, std::vector<unsigned>& indices, glm::vec3 normal, float resolution, int parentStep = -1);
	public:
		static void SerializeTreeGraph(std::string path, Entity tree);
		static std::shared_ptr<Material> SemanticTreeBranchMaterial;
		static std::shared_ptr<Material> SemanticTreeLeafMaterial;
		static void Init();
		static bool IsReady();

		static EntityQuery GetInternodeQuery();
		static EntityQuery GetTreeQuery();
		static EntityQuery GetAttractionPointQuery();
		static InternodeSystem* GetInternodeSystem();
		static TreeSystem* GetTreeSystem();

		static void GetAllTrees(std::vector<Entity>& container);

		static void CalculateInternodeIllumination();

		static void GenerateSimpleMeshForTree(Entity treeEntity, float resolution, float subdivision = 1.0f);
		static void DeleteAllTrees();
		static Entity CreateTree(std::shared_ptr<Material> treeSurfaceMaterial, TreeParameters& treeParameters);
		static Entity CreateInternode(TreeIndex treeIndex, Entity parentEntity);
		static Entity CreateAttractionPoint(const TreeIndex& treeIndex, const glm::vec3& position, const Entity& tree);

		static void ExportTreeAsModel(Entity treeEntity, std::string filename, bool includeFoliage = false);
		
		static LightEstimator* GetLightEstimator();
	};
}
