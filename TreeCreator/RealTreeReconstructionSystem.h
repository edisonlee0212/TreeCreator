#pragma once
#include "UniEngine.h"
#include "rapidcsv/rapidcsv.h"
#include "DataCollectionSystem.h"
#include "TreeReconstructionSystem.h"
using namespace UniEngine;

namespace  TreeUtilities
{
	class DataCollectionSystem;

	enum class RealTreeReconstructionSystemStatus
	{
		Idle,
		Test,
		CreateTree,
		MainBranches,
		NormalGrowth,
		Render,
		CollectData,
		CaptureCakeTower,
		CleanUp
	};
	class RealTreeReconstructionSystem :
		public SystemBase
	{
		std::string _MaskPath = "";
		std::string _SkeletonPath = "";
		std::shared_ptr<Texture2D> _TargetMask;
		std::shared_ptr<Texture2D> _TargetSkeleton;
		DataCollectionSystem* _DataCollectionSystem = nullptr;
		PlantSimulationSystem* _PlantSimulationSystem = nullptr;
		TreeReconstructionSystem* _TreeReconstructionSystem = nullptr;
		RealTreeReconstructionSystemStatus _Status = RealTreeReconstructionSystemStatus::Idle;
		rapidcsv::Document _TrainingDoc;
		int _MaxAge = 90;
		int _ReconIndex;
		int _ReconAmount;
		Entity _CurrentTree;
		bool _UseMask = false;
		size_t _SpecieIndex = 0;
		bool _NeedExport = false;
		int _AgeForMainBranches;
		std::string _Name;
		std::string _StorePath;
		int _TargetInternodeSize;
		int _ControlLevel;
		bool _EnableSpaceColonization;
		std::unique_ptr<CakeTower> _TargetCakeTower;
		int _ReconMainBranchInternodeLimit = 1000;
		int _MainBranchInternodeSize;
		std::vector<Entity> _Internodes;
		bool _Growing = false;
		void OnGui();
		void CreateTree();
		void TryGrowTree();
	public:
		void PushInternode(Entity internode, const GlobalTransform& cameraTransform, const GlobalTransform& treeLTW);
		void AttachDataCollectionSystem(DataCollectionSystem* dc, PlantSimulationSystem* ps, TreeReconstructionSystem* trs);
		void OnCreate() override;
		void Update() override;
	};
}

