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

    struct Gravity : ComponentBase{
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
        float AccumulatedGravity = 0;
#pragma endregion
#pragma region Growth
        int MaxChildOrder;
        int MaxChildLevel;
        float Inhibitor = 0;
        float ParentInhibitorFactor = 1;
        int ActivatedBudsAmount = 0;
        unsigned BranchEndInternodeAmount;
        bool Pruned;
        int PruneReason;
        bool IsMaxChild;
        bool ApicalBudExist = false;
        bool IsActivatedEndNode;
#pragma endregion
#pragma region Geometric
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
    	
        float ThicknessRemovalFactor;
    	
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
    	
#pragma region CrownShyness
        float CrownShynessBase;
        float CrownShynessFactor = 1.0f;
#pragma endregion

#pragma region Organs
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

    struct RewardEstimation : ComponentBase {
        float LightEstimationResult = 0.0f;
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
	
    class TreeData : public SharedComponentBase {
    public:
        bool MeshGenerated;
        bool FoliageGenerated;
        float ActiveLength;
        std::vector<float> ApicalControlTimeVal;
        std::vector<std::vector<float>> ApicalControlTimeLevelVal;
        std::shared_ptr<Mesh> ConvexHull;
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
        static Entity CreateTree(std::shared_ptr<Material> treeSurfaceMaterial, std::shared_ptr<Material> treeLeafMaterial, std::shared_ptr<Mesh> treeLeafMesh);
        static Entity CreateInternode(TreeIndex treeIndex, Entity parentEntity);

        static void ExportMeshToOBJ(Entity treeEntity, std::string filename);

        static LightEstimator* GetLightEstimator();

        static void CalculateRewards(Entity treeEntity, float snapShotWidth = 100.0f);
    };
}
