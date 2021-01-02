#pragma once
#include "UniEngine.h"
#include "TreeManager.h"
#include "FoliageGeneratorBase.h"
#include "TreeVolume.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace TreeUtilities {
	enum PlantSimulationSystemConfigFlags {
		PlantSimulationSystem_None = 0,
	};


	class PlantSimulationSystem :
		public SystemBase
	{
		friend class DataCollectionSystem;
		friend class TreeReconstructionSystem;
		friend class MaskProcessor;
#pragma region Stuff
		bool _AutoGenerateMesh = true;
		bool _AutoGenerateLeaves = true;
		float _GrowthTimer;
		float _DirectionPruningLimitAngle = 60;
		bool _DisplayConvexHull = false;
		bool _EnableDirectionPruning = false;
		bool _DisplayFullParam = true;
		char _CurrentWorkingDir[256] = {};
		char _TempImportFilePath[256] = {};
		char _TempExportFilePath[256] = {};
		int _NewPushIteration = 0;
		static float _MeshGenerationResolution;
		static float _MeshGenerationSubdivision;
		int _NewTreeAmount = 1;
		int _CurrentFocusedNewTreeIndex = 0;
		std::vector<TreeParameters> _NewTreeParameters;
		std::vector<glm::vec3> _NewTreePositions;
		std::shared_ptr<Texture2D> _DefaultTreeSurfaceSurfTex1;
		std::shared_ptr<Texture2D> _DefaultTreeSurfaceNormTex1;
		int _ControlLevel = 0;
		std::shared_ptr<Texture2D> _DefaultTreeSurfaceSurfTex2;
		std::shared_ptr<Texture2D> _DefaultTreeSurfaceSpecTex2;
		std::shared_ptr<Texture2D> _DefaultTreeSurfaceNormTex2;
		std::shared_ptr<Material> _DefaultConvexHullSurfaceMaterial;
		InternodeSystem* _InternodeSystem = nullptr;
		unsigned int _ConfigFlags = 0;
		bool _Growing = false;
		EntityQuery _TreeQuery;
		EntityQuery _InternodeQuery;
#pragma endregion
#pragma region Internal Functions
		float GetApicalControl(std::unique_ptr<TreeData>& treeInfo, InternodeInfo& internodeInfo, TreeParameters& treeParameters, TreeAge& treeAge, int level) const;
		inline void OnGui();
		void UpdateDistanceToBranchEnd(Entity& internode, TreeParameters& treeParameters, int treeAge);
		void UpdateDistanceToBranchStart(Entity& internode);
		void UpdateLocalTransform(Entity& internode, TreeParameters& treeParameters, glm::mat4& parentLTW, glm::mat4& treeLTW, bool calculateForce);
		void UpdateInternodeResource(Entity& internode, TreeParameters& treeParameters, TreeAge& treeAge, glm::mat4& treeTransform, bool isLeft);
		
		bool GrowShoots(Entity& internode, std::unique_ptr<TreeVolume>& treeVolume, std::unique_ptr<TreeData>& treeData, TreeAge& treeAge, TreeParameters& treeParameters, TreeIndex& treeIndex, glm::mat4& treeTransform, bool enableSpaceColonization, int controlLevel = 0);
		bool GrowShootsSpaceColonization(Entity& rootInternode, std::unique_ptr<TreeVolume>& treeVolume, std::unique_ptr<TreeData>& treeData, TreeAge& treeAge, TreeParameters& treeParameters, TreeIndex& treeIndex, glm::mat4& treeTransform);
		void EvaluatePruning(Entity& internode, TreeParameters& treeParameters, TreeAge& treeAge, TreeInfo& treeInfo);
		bool EvaluateRemoval(Entity& internode, TreeParameters& treeParameters, bool& anyRemoved);
		void EvaluateDirectionPruning(Entity& internode, glm::vec3 escapeDirection, float limitAngle);
		void ApplyLocalTransform(Entity& treeEntity) const;
		void CalculateCrownShyness(float detectionDistance = 5.0f);
		inline void PruneInternode(Entity& internode, int pruneReason) const;
		static inline void TreeParameterExportHelper(std::ofstream& ofs, TreeParameters& treeParameters);
		void BuildHullForTree(Entity& tree);
		void ResumeGrowth();
		void PauseGrowth();
#pragma endregion
	public:
		void OnCreate() override;
		void OnDestroy() override;
		//Function will be called each frame.
		void Update() override;
		//Function will be called every time step.
		void FixedUpdate() override;
#pragma region Helpers
		static void SetAllInternodeActivated(Entity tree, bool value);
		static void ApplyTropism(glm::vec3 targetDir, float tropism, glm::vec3& front, glm::vec3& up);
		void GenerateLeaves(const Entity& tree) const;
		void GenerateLeavesForAllTrees(std::vector<Entity>& trees) const;
		void RefreshTrees();
		void ExportSettingsAsXml(const std::string& path);
		void ImportSettings(const std::string& path);
		TreeParameters LoadParameters(const std::string& path);
		static void LoadDefaultTreeParameters(int preset, TreeParameters& tps);
		void TryGrowAllTrees(std::vector<Entity>& trees);
		bool GrowTree(Entity& treeEntity, bool mainBranch = false);
		void CalculatePhysics(Entity tree, bool calculateForce);

		Entity CreateTree(std::shared_ptr<Material> treeSurfaceMaterial, TreeParameters parameters, glm::vec3 position, bool enabled = true);
		Entity CreateTree(TreeParameters parameters, glm::vec3 position, bool enabled = true);
		void CreateDefaultTree();
#pragma endregion
	};
}
