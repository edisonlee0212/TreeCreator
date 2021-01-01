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
		CleanUp
	};
	class TreeReconstructionSystem :
		public SystemBase
	{
		TreeType _Type = TreeType::Maple;
		bool _EnableSpaceColonization = false;
		bool _EnableMaskTrimmer = false;
		std::string _StorePath = "./tree_recon/";
		DataCollectionSystem* _DataCollectionSystem = nullptr;
		PlantSimulationSystem* _PlantSimulationSystem = nullptr;
		TreeReconstructionSystemStatus _Status = TreeReconstructionSystemStatus::Idle;
		std::unique_ptr<CakeTower> _TargetCakeTower;
		Entity _CurrentTree;
		std::vector<Entity> _Internodes;
		std::string _MaskPath = "./tree_recon/AppleMask.png";
		std::shared_ptr<Texture2D> _TargetMask;
		std::string _TargetCakeTowerPath = "./tree_recon/AppleTarget.ct";
		int _AgeForMainBranches = 4;
		int _ControlLevel = 0;
		int _TargetInternodeSize = 1601;
		TreeParameters _TargetTreeParameter;
		std::string _TreeParametersPath;
		int _StartIndex = 1;
		std::string _Name = "Apple";
		int _EndIndex = 5;
		int _MaxAge = 30;
		bool _NeedExport = false;
		void OnGui();
		std::vector<CakeTowerOutput> _CakeTowersOutputList;
		void ExportAllData();
	public:
		void SetPlantSimulationSystem(PlantSimulationSystem* value);
		void SetDataCollectionSystem(DataCollectionSystem* value);
		void Init();
		void ExportCakeTower(const std::string path);
		void OnCreate() override;
		void Update() override;
	};
}

