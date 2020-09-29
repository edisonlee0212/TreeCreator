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
        unsigned StartAge;
        glm::vec3 EulerAngles;
    };

    struct Gravity {
        float Value;
    };

    class InternodeData : public SharedComponentBase {
    public:
        std::vector<Bud> Buds;
        std::vector<InternodeRingSegment> Rings;
        std::vector<glm::mat4> LeafLocalTransforms;
        glm::vec3 NormalDir;
        int step;
        std::size_t GetHashCode() override;
        void OnGui() override;
    };
	
    struct InternodeIndex : ComponentBase {
        unsigned Value;
        bool operator ==(const InternodeIndex& other) const {
            return other.Value == Value;
        }
    };
    struct InternodeInfo : ComponentBase {
#pragma region General
        int Level = 0;
        float DistanceToParent = 0;
        //The distance to 
        float DistanceToBranchEnd = 0;
        float TotalDistanceToBranchEnd = 0;
        float DistanceToBranchStart = 0;
        float AccumulatedLength = 0;
        float AccumulatedLight = 0;
        float AccumulatedActivatedBudsAmount = 0;
        float AccumulatedGravity = 0;
        unsigned NumValidChild;
#pragma endregion
#pragma region Growth
        int MaxChildLevel;
        int MaxActivatedChildLevel;
        float Inhibitor = 0;
        float ParentInhibitorFactor = 1;
        int ActivatedBudsAmount = 0;
        unsigned BranchEndInternodeAmount;
        bool Pruned;
        bool IsApical;
        bool ApicalBudExist = false;
        bool IsActivatedEndNode;
#pragma endregion
#pragma region Geometric
        float Length;
        float Thickness;
        float Deformation;
        float Straightness;
        float Slope;
        float SiblingAngle;
        float ParentAngle;
#pragma endregion
#pragma region Transformation
        glm::quat DesiredGlobalRotation;
        glm::quat DesiredLocalRotation;
        glm::mat4 GlobalTransform;
        glm::mat4 LocalTransform;


        
#pragma endregion

#pragma region Mesh generation
        glm::vec3 ParentTranslation;
        glm::quat ParentRotation;
        float ParentThickness;
        glm::quat MainChildRotation;
        glm::quat ParentMainChildRotation;
        bool IsMainChild = false;
#pragma endregion

    };
#pragma endregion
#pragma region Tree
    struct TreeParameters : ComponentBase {
        int Seed;

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
        float Phototropism;
        float GravitropismBase;
        float GravitropismLevelFactor;

        float PruningFactor; 
        float LowBranchPruningFactor;

        float GravityBendingStrength;

        float ApicalBudLightingFactor;
        float LateralBudLightingFactor;
#pragma endregion

#pragma region Sagging
        float SaggingFactor;
        float SaggingForceBackPropagateFixedCoefficient;
#pragma endregion

        float EndNodeThickness;
        float ThicknessControlFactor;
    };

    struct RewardEstimation : ComponentBase {
        float LightEstimationResult = 0.0f;
    };

    struct TreeAge : ComponentBase {
        int Value;
        int ToGrowIteration;
        bool Enable;
    };

    struct TreeInfo : ComponentType
    {
	    
    };
	
    class TreeData : public SharedComponentBase {
    public:
        int CurrentSeed;
        float Height;
        int MaxBranchingDepth;
        int LateralBudsCount;
        bool MeshGenerated;
        bool FoliageGenerated;
        std::vector<float> ApicalDominanceTimeVal;
        std::vector<float> GravitropismLevelVal;
        std::vector<float> ApicalControlTimeVal;
        std::vector<std::vector<float>> ApicalControlTimeLevelVal;
        std::shared_ptr<Mesh> ConvexHull;
        float ResourceToGrow;
        std::size_t GetHashCode() override;
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
        static Entity CreateTree(std::shared_ptr<Material> treeSurfaceMaterial);
        static Entity CreateInternode(TreeIndex treeIndex, Entity parentEntity);

        static void ExportMeshToOBJ(Entity treeEntity, std::string filename);

        static LightEstimator* GetLightEstimator();

        static void CalculateRewards(Entity treeEntity, float snapShotWidth = 100.0f);
    };
}
