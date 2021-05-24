#pragma once
#include "DataCollectionSystem.h"
#include "UniEngine.h"
#include "TreeManager.h"
#include "PlantSimulationSystem.h"
#include "rapidcsv/rapidcsv.h"
using namespace UniEngine;
namespace TreeUtilities
{
	enum class TreeType
	{
		Acacia,
		Apple,
		Willow,
		Maple,
		Birch,
		Oak,
		Pine
	};
	enum class TreeReconstructionSystemStatus
	{
		Idle,
		CreateTree,
		MainBranches,
		NormalGrowth,
		Render,
		RenderBranch,
		CollectData,
		CaptureCakeTower,
		CleanUp,
		Reconstruction
	};
	class TreeReconstructionSystem :
		public SystemBase
	{
		int _GenerateAmount = 1;
		int _ReconAmount = 1;
		int _Add = 0;
		int _ControlLevel = 0;
		int _EnableSpaceColonization = true;
		int _ReconIndex = 0;	//0
		int _ReconStartSeed = 0;
		int _ReconSeed = 0;		//0	
		int _ReconCounter = 0;	//0	1
		int _ReconMainBranchInternodeLimit = 1000;
		int _LearningIndex = 0;
		rapidcsv::Document _TrainingDoc;
		bool _FromTraining = false;
		int _TrainingAmount;
		bool _EnableMaskTrimmer = false;
		std::string _StorePath = "./tree_recon/";
		DataCollectionSystem* _DataCollectionSystem = nullptr;
		PlantSimulationSystem* _PlantSimulationSystem = nullptr;
		bool _UseCakeTower = true;
		TreeReconstructionSystemStatus _Status = TreeReconstructionSystemStatus::Idle;
		TreeReconstructionSystemStatus _PreviousStatus = TreeReconstructionSystemStatus::Idle;
		std::unique_ptr<RBV> _TargetCakeTower;

		bool _Paused = false;
		Entity _CurrentTree;
		std::vector<Entity> _Internodes;
		std::string _MaskPath = "";
		std::string _SkeletonPath = "";
		
		std::string _TargetCakeTowerPath = "";
		std::string _TargetKdopPath = "";
		std::shared_ptr<Texture2D> _TargetMask;
		std::shared_ptr<Texture2D> _TargetSkeleton;
		int _MainBranchInternodeSize;
		int _AgeForMainBranches = 4;
		int _TargetInternodeSize = 1601;

		TreeParameters _TargetTreeParameter;
		int _Capture = 0;
		std::string _Name = "Apple";
		std::string _Prefix = "_1_1";
		int _MaxAge = 30;
		bool _NeedExport = false;
		bool _UseMask = false;
		bool _Growing = false;
		void OnGui();
		std::vector<CakeTowerOutput> _CakeTowersOutputList;
		void Switch();
	public:
		void ExportAllData();
		void TryGrowTree();
		void SetEnableFoliage(bool enabled) const;
		Entity FindFoliageEntity() const;
		void PushInternode(Entity internode, const GlobalTransform& cameraTransform, const GlobalTransform& treeLTW);
		void SetPlantSimulationSystem(PlantSimulationSystem* value);
		void SetDataCollectionSystem(DataCollectionSystem* value);
		void Init();
		void ExportCakeTower(const std::string& path);
		void OnCreate() override;
		void Update() override;
		void LateUpdate() override;
	};
}

