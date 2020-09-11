#pragma once
#include "UniEngine.h"
#include "TreeManager.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace TreeUtilities {
    enum PlantSimulationSystemConfigFlags {
        PlantSimulationSystem_None = 0,
    };

    
    class PlantSimulationSystem :
        public SystemBase
    {
#pragma region GUI Related
        bool _GuiMenuEnabled = true;
        bool _DisplayFullParam = true;
        char _TempFilePath[256];
        TreeParameters _NewTreeParameters;
        glm::vec3 _NewTreePosition = glm::vec3(0);
        TreeColor _NewTreeColor;
        Material* _DefaultTreeSurfaceMaterial1;
        Material* _DefaultTreeSurfaceMaterial2;
#pragma endregion
         
        int _GrowIterationCount;

        unsigned int _ConfigFlags = 0;
        float _Gravity;
        bool _Growing = false;
        EntityQuery _LeafQuery;
        EntityQuery _TreeQuery;
        EntityQuery _BranchNodeQuery;
        
        std::queue<Entity> _MeshGenerationQueue;

        float GetApicalControl(TreeInfo& treeInfo, BranchNodeInfo& branchNodeInfo, TreeParameters& treeParameters, TreeAge& treeAge, int level);
        void DrawGUI();
        void UpdateBranchNodeLength(Entity& branchNode);
        void UpdateBranchNodeActivatedLevel(Entity& branchNode);
        void UpdateLocalTransform(Entity& branchNode, TreeParameters& treeParameters, glm::mat4& parentLTW, glm::quat& treeRotation);
        void UpdateBranchNodeResource(Entity& branchNode, TreeParameters& treeParameters, TreeAge& treeAge);
        void UpdateBranchNodeResourceAllocation(Entity& branchNode); 
        bool GrowShoots(Entity& branchNode, TreeInfo& treeInfo, TreeAge& treeAge, TreeParameters& treeParameters, TreeIndex& treeIndex);
        void DeactivateBud(BranchNodeInfo& branchNodeInfo, Bud& bud);
        void PruneBranchNode(Entity& branchNode, TreeInfo& treeInfo);
        void EvaluatePruning(Entity& branchNode, TreeParameters& treeParameters, TreeAge& treeAge, TreeInfo& treeInfo);
        void ApplyLocalTransform(Entity& treeEntity);
        void CalculateDirectGravityForce(Entity& treeEntity, float gravity);
        void BackPropagateForce(Entity& branchNode, float fixedPropagationCoefficient);

    public:
        void LoadDefaultTreeParameters(int preset);
        void TryGrowAllTrees(std::vector<Entity>& trees);
        bool GrowTree(Entity& treeEntity);
        void CalculatePhysics(std::vector<Entity>& trees);
        void OnCreate();
        void OnDestroy();
        void Update();
        void FixedUpdate();
        Entity CreateTree(Material* treeSurfaceMaterial, TreeParameters parameters, TreeColor color, glm::vec3 position, bool enabled = false);
    };
}
