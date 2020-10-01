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
		float _DirectionPruningLimitAngle = 70;
		bool _DisplayConvexHull = false;
		bool _EnableDirectionPruning = false;
		bool _GravityChanged = false;
		bool _DisplayFullParam = true;
		char _CurrentWorkingDir[256] = {};
		char _TempImportFilePath[256] = {};
		char _TempExportFilePath[256] = {};
		int _NewPushIteration = 0;
		float _MeshGenerationResolution = 0.01f;
		float _MeshGenerationSubdivision = 1.0f;
		int _NewTreeAmount = 1;
		int _CurrentFocusedNewTreeIndex = 0;
		std::vector<TreeParameters> _NewTreeParameters;
		std::vector<glm::vec3> _NewTreePositions;
		std::shared_ptr<Material> _DefaultTreeSurfaceMaterial1;
		std::shared_ptr<Material> _DefaultTreeSurfaceMaterial2;

		std::shared_ptr<Material> _DefaultTreeLeafMaterial1;
		std::shared_ptr<Material> _DefaultTreeLeafMaterial2;

		std::shared_ptr<Mesh> _DefaultTreeLeafMesh;
#pragma endregion

		unsigned int _ConfigFlags = 0;
		float _Gravity = 0;
		bool _Growing = false;
		EntityQuery _TreeQuery;
		EntityQuery _InternodeQuery;

		float GetApicalControl(std::shared_ptr<TreeData>& treeInfo, InternodeInfo& internodeInfo, TreeParameters& treeParameters, TreeAge& treeAge, int level) const;
		inline void DrawGui();
		void UpdateDistanceToBranchEnd(Entity& internode);
		void UpdateDistanceToBranchStart(Entity& internode);
		void UpdateLocalTransform(Entity& internode, TreeParameters& treeParameters, glm::mat4& parentLTW, glm::quat& treeRotation, std::vector<glm::mat4>& leafTransforms);
		void UpdateInternodeResource(Entity& internode, TreeParameters& treeParameters, TreeAge& treeAge);
		bool GrowShoots(Entity& internode, std::shared_ptr<TreeData>& treeInfo, TreeAge& treeAge, TreeParameters& treeParameters, TreeIndex& treeIndex);
		static void DeactivateBud(InternodeInfo& internodeInfo, Bud& bud);
		void EvaluatePruning(Entity& internode, TreeParameters& treeParameters, TreeAge& treeAge, TreeInfo& treeInfo);
		void EvaluateRemoval(Entity& internode, TreeParameters& treeParameters);
		void EvaluateDirectionPruning(Entity& internode, glm::vec3 escapeDirection, float limitAngle);
		void ApplyLocalTransform(Entity& treeEntity) const;
		void CalculateDirectGravityForce(Entity& treeEntity, float gravity) const;
		void BackPropagateForce(Entity& internode, float fixedPropagationCoefficient);
		void CalculateCrownShyness();
		inline void PruneInternode(Entity& internode, InternodeInfo* internodeInfo) const;
		static inline void TreeParameterImportHelper(std::ifstream& ifs, TreeParameters& treeParameters);
		static inline void TreeParameterExportHelper(std::ofstream& ofs, TreeParameters& treeParameters);
		void BuildConvexHullForTree(Entity& tree);
	public:
		static TreeParameters ImportTreeParameters(const std::string& path);
		static void ExportTreeParameters(const std::string& path, TreeParameters& treeParameters);
		void ExportSettings(const std::string& path);
		void ImportSettings(const std::string& path);
		static void LoadDefaultTreeParameters(int preset, TreeParameters& tps);
		void TryGrowAllTrees(std::vector<Entity>& trees);
		bool GrowTree(Entity& treeEntity);
		void CalculatePhysics(std::vector<Entity>& trees);
		void OnCreate() override;
		void OnDestroy() override;
		void Update() override;
		void FixedUpdate() override;
		Entity CreateTree(std::shared_ptr<Material> treeSurfaceMaterial, std::shared_ptr<Material> treeLeafMaterial, std::shared_ptr<Mesh> treeLeafMesh, TreeParameters parameters, glm::vec3 position, bool enabled = true);
		void CreateDefaultTree();
	};
}
