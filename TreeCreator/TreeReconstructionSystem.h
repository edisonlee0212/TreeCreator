#pragma once
#include "DataCollectionSystem.h"
#include "UniEngine.h"
#include "TreeManager.h"
#include "PlantSimulationSystem.h"
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
		CollectData,
		CleanUp,
		Reconstruction
	};
	class TreeReconstructionSystem :
		public SystemBase
	{
		int _Amount = 10;
		int _ReconAmount = 10;

		int _ReconIndex = 0;
		int _ReconSeed = 0;
		int _ReconCounter = 0;
		int _ReconMainBranchInternodeLimit = 1000;
		
		bool _EnableMaskTrimmer = false;
		std::string _StorePath = "./tree_recon/";
		DataCollectionSystem* _DataCollectionSystem = nullptr;
		PlantSimulationSystem* _PlantSimulationSystem = nullptr;
		bool _UseCakeTower = true;
		TreeReconstructionSystemStatus _Status = TreeReconstructionSystemStatus::Idle;
		std::unique_ptr<CakeTower> _TargetCakeTower;

		std::unique_ptr<KDop> _TargetKDop;
		
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
		
		int _MaxAge = 30;
		bool _NeedExport = false;
		bool _Growing = false;
		void OnGui();
		std::vector<CakeTowerOutput> _CakeTowersOutputList;
		void ExportAllData();
		void TryGrowTree();
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

