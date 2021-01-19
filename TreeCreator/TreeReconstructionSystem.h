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
		MainBranches,
		NormalGrowth,
		Render,
		RenderBranch,
		CollectData,
		CleanUp,
		Reconstruction
	};
	class TreeReconstructionSystem :
		public SystemBase
	{
		int _GenerateAmount = 10;
		int _ReconAmount = 3;
		int _Add = 0;
		int _ReconIndex = 0;
		int _ReconSeed = 0;
		int _ReconCounter = 0;
		int _ReconMainBranchInternodeLimit = 1000;

		rapidcsv::Document _TrainingDoc;
		bool _FromTraining = false;
		int _TrainingAmount;
		bool _EnableMaskTrimmer = false;
		std::string _StorePath = "./tree_recon/";
		DataCollectionSystem* _DataCollectionSystem = nullptr;
		PlantSimulationSystem* _PlantSimulationSystem = nullptr;
		bool _UseCakeTower = true;
		TreeReconstructionSystemStatus _Status = TreeReconstructionSystemStatus::Idle;
		std::unique_ptr<CakeTower> _TargetCakeTower;

		
		Entity _CurrentTree;
		std::vector<Entity> _Internodes;
		std::string _MaskPath = "";
		std::string _SkeletonPath = "";
		std::shared_ptr<Texture2D> _TargetMask;
		std::string _TargetCakeTowerPath = "";
		std::string _TargetKdopPath = "";
		std::shared_ptr<Texture2D> _TargetSkeleton;
		int _MainBranchInternodeSize;
		int _AgeForMainBranches = 4;
		int _TargetInternodeSize = 1601;
		TreeParameters _TargetTreeParameter;

		std::string _Name = "Apple";
		std::string _Prefix = "_2_2";
		int _MaxAge = 30;
		bool _NeedExport = false;
		bool _Growing = false;
		void OnGui();
		std::vector<CakeTowerOutput> _CakeTowersOutputList;
		void ExportAllData();
		void TryGrowTree();
		void SetEnableFoliage(bool enabled) const;
		
		void PushInternode(Entity internode, const GlobalTransform& cameraTransform, const GlobalTransform& treeLTW);
	public:
		void SetPlantSimulationSystem(PlantSimulationSystem* value);
		void SetDataCollectionSystem(DataCollectionSystem* value);
		void Init();
		void ExportCakeTower(const std::string& path);
		void OnCreate() override;
		void Update() override;
	};
}

