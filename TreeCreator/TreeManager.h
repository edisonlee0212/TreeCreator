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
		std::vector<Bud> Buds;
		std::vector<InternodeRingSegment> Rings;
		std::vector<glm::mat4> LeafLocalTransforms;
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

		float CrownShyness;
	};
#pragma endregion
#pragma region Tree
	enum class TreeVolumeType
	{
		Default,
		Cube,
		Sphere
	};
	class TreeVolume : public PrivateComponentBase
	{
		Bound _Bound;
		glm::vec4 _DisplayColor = glm::vec4(0.0f, 0.0f, 1.0f, 0.5f);
		bool _Display = true;
		bool _PruneBuds = false;
		TreeVolumeType _Type = TreeVolumeType::Default;
	public:
		void SetPruneBuds(bool value) { _PruneBuds = value; }
		bool PruneBuds() const { return _PruneBuds; }
		bool InVolume(glm::vec3 position) const
		{
			switch (_Type)
			{
			case TreeVolumeType::Cube:
				return _Bound.InBound(position);
			case TreeVolumeType::Sphere:
				return glm::distance(position, _Bound.Center) <= _Bound.Radius;
			}
			return true;
		}
		void OnGui() override
		{
			ImGui::Checkbox("Prune Buds", &_PruneBuds);
			static const char* TVTypes[]{ "Default", "Cube", "Sphere" };
			ImGui::Combo("Display mode", (int*)(void*)&_Type, TVTypes, IM_ARRAYSIZE(TVTypes));
			switch (_Type)
			{
			case TreeVolumeType::Cube:
				ImGui::Text("Type: Cube");
				ImGui::Checkbox("Display bounds", &_Display);
				if (_Display)
				{
					ImGui::ColorEdit4("Color: ", (float*)(void*)&_DisplayColor);
					RenderManager::DrawGizmoCube(_DisplayColor, glm::translate(_Bound.Center) * glm::scale(_Bound.Size), 1);
				}
				ImGui::DragFloat3("Center: ", (float*)(void*)&_Bound.Center);
				ImGui::DragFloat3("Size: ", (float*)(void*)&_Bound.Size);
				break;
			case TreeVolumeType::Sphere:
				ImGui::Text("Type: Sphere");
				ImGui::Checkbox("Display bounds", &_Display);
				if (_Display)
				{
					ImGui::ColorEdit4("Color: ", (float*)(void*)&_DisplayColor);
					RenderManager::DrawGizmoPoint(_DisplayColor, glm::translate(_Bound.Center) * glm::scale(glm::vec3(_Bound.Radius)), 1);
				}
				ImGui::DragFloat3("Center: ", (float*)(void*)&_Bound.Center);
				ImGui::DragFloat("Radius: ", (float*)(void*)&_Bound.Radius);
				break;
			default:
				ImGui::Text("Type: Default");
				break;
			}
		}
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
		glm::vec2 LeafSize = glm::vec2(0.1f);
		float LeafIlluminationLimit = 0;
		float LeafInhibitorFactor = 0;
		bool IsBothSide = true;
		int SideLeafAmount = 1;
		float StartBendingAngle = 45;
		float BendingAngleIncrement = 0;
		float LeafPhotoTropism = 999.0f;
		float LeafGravitropism = 1.0f;
		float LeafDistance = 0;
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
		public ManagerBase
	{
		static LightEstimator* _LightEstimator;

		static TreeSystem* _TreeSystem;
		static InternodeSystem* _InternodeSystem;

		static EntityArchetype _InternodeArchetype;
		static EntityArchetype _TreeArchetype;

		static EntityQuery _TreeQuery;
		static EntityQuery _InternodeQuery;

		static TreeIndex _TreeIndex;
		static InternodeIndex _InternodeIndex;

		static bool _Ready;

		static void SimpleMeshGenerator(Entity& internode, std::vector<Vertex>& vertices, std::vector<unsigned>& indices, glm::vec3 normal, float resolution, int parentStep = -1);
	public:
		static void Init();
		static bool IsReady();

		static EntityQuery GetInternodeQuery();
		static EntityQuery GetTreeQuery();

		static InternodeSystem* GetInternodeSystem();
		static TreeSystem* GetTreeSystem();

		static void GetAllTrees(std::vector<Entity>& container);

		static void CalculateInternodeIllumination();

		static std::shared_ptr<Mesh> GetMeshForTree(Entity treeEntity);
		static void GenerateSimpleMeshForTree(Entity treeEntity, float resolution, float subdivision = 1.0f);
		static void DeleteAllTrees();
		static Entity CreateTree(std::shared_ptr<Material> treeSurfaceMaterial, std::shared_ptr<Material> treeLeafMaterial, std::shared_ptr<Mesh> treeLeafMesh);
		static Entity CreateInternode(TreeIndex treeIndex, Entity parentEntity);

		static void ExportMeshToOBJ(Entity treeEntity, std::string filename);

		static LightEstimator* GetLightEstimator();
	};
}
