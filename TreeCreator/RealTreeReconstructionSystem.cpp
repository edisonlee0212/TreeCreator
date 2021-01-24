#include "RealTreeReconstructionSystem.h"
#include "AcaciaFoliageGenerator.h"
#include "MapleFoliageGenerator.h"
#include "PineFoliageGenerator.h"
#include "WillowFoliageGenerator.h"
#include "AppleFoliageGenerator.h"
#include "OakFoliageGenerator.h"
#include "BirchFoliageGenerator.h"
#include "MaskProcessor.h"
void TreeUtilities::RealTreeReconstructionSystem::OnGui()
{
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Real Tree Reconstruction")) {
			if (ImGui::Button("Reconstruct"))
			{
				std::filesystem::remove_all("tree_real/image_recon");
				std::filesystem::remove_all("tree_real/rbv_recon");
				std::filesystem::create_directory("tree_real/image_recon");
				std::filesystem::create_directory("tree_real/rbv_recon");
				_TrainingDoc = rapidcsv::Document("./tree_real/023_rbv_photographs_predicted.csv", rapidcsv::LabelParams(-1, 1));

				_ReconIndex = 0;
				_SpecieIndex = -1;
				_ReconAmount = 23;
				_NeedExport = true;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void RealTreeReconstructionSystem::CreateTree()
{
	_EnableSpaceColonization = true;
	_ControlLevel = 0;


	_TargetCakeTower = std::make_unique<CakeTower>();
	std::vector<float> rbv = _TrainingDoc.GetRow<float>(_ReconIndex);

	_TargetCakeTower->CakeTiers.resize(8);
	for (int i = 0; i < 8; i++)
	{
		_TargetCakeTower->CakeTiers[i].resize(8);
		for (int j = 0; j < 8; j++)
		{
			_TargetCakeTower->CakeTiers[i][j].MaxDistance = rbv[i * 8 + j] * 30;
		}
	}
	_TargetCakeTower->LayerAmount = 8;
	_TargetCakeTower->SectorAmount = 8;

	_TargetCakeTower->MaxRadius = 30;
	_TargetCakeTower->GenerateMesh();

	
	_StorePath = "./tree_real/";
	switch (_SpecieIndex) {
	case 0:
		_Name = "acacia";
		_AgeForMainBranches = 4;
		_TargetCakeTower->MaxHeight = 15;
		_TargetInternodeSize = 2260;
		_ReconMainBranchInternodeLimit = 1000;
		break;
	case 1:
		_Name = "apple";
		_AgeForMainBranches = 4;
		_TargetCakeTower->MaxHeight = 12.5;
		_TargetInternodeSize = 1601;
		_ReconMainBranchInternodeLimit = 1000;
		break;
	case 2:
		_Name = "willow";
		_AgeForMainBranches = 0;
		_TargetCakeTower->MaxHeight = 19;
		_TargetInternodeSize = 2615;
		_ReconMainBranchInternodeLimit = 0;
		break;
	case 3:
		_Name = "maple";
		_ControlLevel = 1;
		_TargetCakeTower->MaxHeight = 32;
		_AgeForMainBranches = 1;
		_TargetInternodeSize = 7995;
		_ReconMainBranchInternodeLimit = 0;
		break;
	case 4:
		_Name = "birch";
		_TargetCakeTower->MaxHeight = 31;
		_AgeForMainBranches = 1;
		_TargetInternodeSize = 5120;
		_ReconMainBranchInternodeLimit = 0;
		break;
	case 5:
		_Name = "oak";
		_TargetCakeTower->MaxHeight = 18.5;
		_AgeForMainBranches = 5;
		_TargetInternodeSize = 4487;
		_ReconMainBranchInternodeLimit = 1000;
		break;
	case 6:
		_Name = "pine";
		_TargetCakeTower->MaxHeight = 15;
		_EnableSpaceColonization = false;
		_ControlLevel = 0;
		_AgeForMainBranches = 0;
		_TargetInternodeSize = 8538;
		_ReconMainBranchInternodeLimit = 0;
		break;
	}
	
	if (!_UseMask)
	{
		_AgeForMainBranches = 0;
		_ReconMainBranchInternodeLimit = 0;
	}
	
	auto& seq = _DataCollectionSystem->_ImageCaptureSequences[_SpecieIndex];
	_DataCollectionSystem->SetCameraPose(seq.first.CameraPos, seq.first.CameraEulerDegreeRot);
	auto treeParameter = seq.second;
	_MaskPath = _StorePath + "mask_real_small/" + std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_tree.png";
	_SkeletonPath = _StorePath + "skeleton_recon/" + std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_tree.png";
	_TargetMask = ResourceManager::LoadTexture(false, _MaskPath);
	_TargetSkeleton = ResourceManager::LoadTexture(false, _SkeletonPath);

	_CurrentTree = _PlantSimulationSystem->CreateTree(treeParameter, glm::vec3(0.0f));
	auto& maskProcessor = _CurrentTree.GetPrivateComponent<MaskProcessor>();
	maskProcessor->_Skeleton = _TargetSkeleton;
	maskProcessor->_Mask = _TargetMask;
	maskProcessor->AttractionDistance = 1.0f;
	maskProcessor->PlaceAttractionPoints();
}

void RealTreeReconstructionSystem::TryGrowTree()
{
	if (_CurrentTree.IsNull() || !_CurrentTree.IsValid() || _CurrentTree.IsDeleted()) return;
	_PlantSimulationSystem->RefreshTrees();
	_Internodes.resize(0);
	const auto treeIndex = _CurrentTree.GetComponentData<TreeIndex>();
	TreeManager::GetInternodeQuery().ToEntityArray(treeIndex, _Internodes);
	TreeAge age = _CurrentTree.GetComponentData<TreeAge>();
	if (_Internodes.size() > _ReconMainBranchInternodeLimit)
	{
		_Growing = false;
	}
	if (_Growing) {
		_Growing = _PlantSimulationSystem->GrowTree(_CurrentTree, true);
	}
	if (_Growing) {
		return;
	}

	GlobalTransform treeLTW = _CurrentTree.GetComponentData<GlobalTransform>();
	GlobalTransform cameraLTW = _DataCollectionSystem->_SemanticMaskCameraEntity.GetComponentData<GlobalTransform>();
	Entity rootInternode = Entity();
	EntityManager::ForEachChild(_CurrentTree, [&](Entity child)
		{
			if (child.HasComponentData<InternodeInfo>()) rootInternode = child;
		}
	);
	if (rootInternode.IsNull()) return;
	auto id = glm::translate(glm::vec3(0.0f)) * glm::mat4_cast(glm::quat(glm::vec3(0.0f))) * glm::scale(glm::vec3(1.0f));
	//_PlantSimulationSystem->UpdateLocalTransform(rootInternode, _TargetTreeParameter, id, treeLTW.Value, false);
	//_PlantSimulationSystem->ApplyLocalTransform(_CurrentTree);
	_TreeReconstructionSystem->PushInternode(rootInternode, cameraLTW, treeLTW);

	_Internodes.resize(0);
	TreeManager::GetInternodeQuery().ToEntityArray(treeIndex, _Internodes);
	_MainBranchInternodeSize = _Internodes.size();
}


void TreeUtilities::RealTreeReconstructionSystem::AttachDataCollectionSystem(DataCollectionSystem* dc, PlantSimulationSystem* ps, TreeReconstructionSystem* trs)
{
	_DataCollectionSystem = dc;
	_PlantSimulationSystem = ps;
	_TreeReconstructionSystem = trs;
}

void TreeUtilities::RealTreeReconstructionSystem::OnCreate()
{
	Enable();
}

void TreeUtilities::RealTreeReconstructionSystem::Update()
{
	if (_DataCollectionSystem == nullptr) return;
	switch (_Status)
	{
	case RealTreeReconstructionSystemStatus::Idle:
		if (_NeedExport)
		{
			_SpecieIndex++;
			if (_ReconIndex == _ReconAmount - 1 && _SpecieIndex == _DataCollectionSystem->_ImageCaptureSequences.size())
			{
				_NeedExport = false;
				_SpecieIndex = 0;
				_ReconIndex = 0;
				break;
			}
			
			if (_SpecieIndex == _DataCollectionSystem->_ImageCaptureSequences.size())
			{
				_SpecieIndex = 0;
				_ReconIndex++;
			}
			_Status = RealTreeReconstructionSystemStatus::CreateTree;
		}
		else OnGui();
		break;
	case RealTreeReconstructionSystemStatus::CreateTree:
		CreateTree();
		if (_UseMask) _Status = RealTreeReconstructionSystemStatus::MainBranches;
		else {
			auto& cakeTower = _CurrentTree.GetPrivateComponent<CakeTower>();
			cakeTower->SetEnabled(true);
			cakeTower->CakeTiers = _TargetCakeTower->CakeTiers;

			cakeTower->LayerAmount = _TargetCakeTower->LayerAmount;
			cakeTower->SectorAmount = _TargetCakeTower->SectorAmount;
			cakeTower->MaxHeight = _TargetCakeTower->MaxHeight;
			cakeTower->MaxRadius = _TargetCakeTower->MaxRadius;

			cakeTower->GenerateMesh();
			cakeTower->ClearAttractionPoints();
			cakeTower->GenerateAttractionPoints(6000);
			cakeTower->EnableSpaceColonization = _EnableSpaceColonization;
			_PlantSimulationSystem->ResumeGrowth();
			TreeAge age = _CurrentTree.GetComponentData<TreeAge>();
			age.Value = _AgeForMainBranches;
			age.ToGrowIteration = 100 - _AgeForMainBranches;
			//_CurrentTree.SetComponentData(age);
			_Status = RealTreeReconstructionSystemStatus::NormalGrowth;
		}
		break;
	case RealTreeReconstructionSystemStatus::MainBranches:
		break;
	case RealTreeReconstructionSystemStatus::NormalGrowth: {
		_PlantSimulationSystem->_AutoGenerateMesh = true;
		_PlantSimulationSystem->_AutoGenerateLeaves = true;
		bool stop = false;
		_Internodes.resize(0);
		const auto treeIndex = _CurrentTree.GetComponentData<TreeIndex>();
		TreeManager::GetInternodeQuery().ToEntityArray(treeIndex, _Internodes);
		TreeAge age = _CurrentTree.GetComponentData<TreeAge>();
		if (_UseMask) _MainBranchInternodeSize = 0;
		if (_Internodes.size() > _TargetInternodeSize + _MainBranchInternodeSize)
		{
			stop = true;
		}
		if (!stop && !_PlantSimulationSystem->_Growing) {
			if (age.ToGrowIteration == 0 || age.Value > _MaxAge)
			{
				stop = true;
			}
			else
			{
				_PlantSimulationSystem->_Growing = true;
				age.ToGrowIteration++;
				_CurrentTree.SetComponentData(age);
			}
		}
		if (stop)
		{
			/*
			if (_Internodes.size() < 10)
			{
				_Add++;
				if (_Add < 100)
				{
					TreeManager::DeleteAllTrees();
					auto parameters = _TargetTreeParameter;

					parameters.Seed = _ReconCounter + 1 + _Add;
					parameters.Age = 200;
					_PlantSimulationSystem->_ControlLevel = 2;
					_CurrentTree = _PlantSimulationSystem->CreateTree(parameters, glm::vec3(0.0f));

					_CurrentTree.GetPrivateComponent<MaskProcessor>()->_Skeleton = _TargetSkeleton;
					_CurrentTree.GetPrivateComponent<MaskProcessor>()->_Mask = _TargetMask;
					_CurrentTree.GetPrivateComponent<MaskProcessor>()->AttractionDistance = 1.0f;
					_CurrentTree.GetPrivateComponent<MaskProcessor>()->PlaceAttractionPoints();
					_Status = TreeReconstructionSystemStatus::MainBranches;
					_Growing = true;
					break;
				}
			}*/
			_PlantSimulationSystem->PauseGrowth();
			TreeManager::GenerateSimpleMeshForTree(_CurrentTree, PlantSimulationSystem::_MeshGenerationResolution, PlantSimulationSystem::_MeshGenerationSubdivision);
			_PlantSimulationSystem->GenerateLeaves(_CurrentTree);
			_Status = RealTreeReconstructionSystemStatus::Render;
			auto& [fst, snd] = _DataCollectionSystem->_ImageCaptureSequences[_SpecieIndex];
			Transform cameraTransform;
			cameraTransform.SetPosition(glm::vec3(0, fst.CameraPos.z * 1.5f, 0));
			cameraTransform.SetEulerRotation(glm::radians(glm::vec3(-90, 0, 0)));
			_DataCollectionSystem->_SemanticMaskCameraEntity.SetComponentData(cameraTransform);
		}
	}
		break;
	case RealTreeReconstructionSystemStatus::Render:
		_Status = RealTreeReconstructionSystemStatus::CollectData;
		break;
	case RealTreeReconstructionSystemStatus::CollectData:
		{
			std::string path = _StorePath + "image_recon/" +
				std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) +
				"_" + _Name + ".png";
			_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);

		}

		if (_SpecieIndex == 0) {
#pragma region CakeTower
			Entity foliageEntity;
			EntityManager::ForEachChild(_CurrentTree, [&foliageEntity](Entity child)
				{
					if (child.HasComponentData<WillowFoliageInfo>())
					{
						foliageEntity = child;
					}
					else if (child.HasComponentData<AppleFoliageInfo>())
					{
						foliageEntity = child;
					}
					else if (child.HasComponentData<AcaciaFoliageInfo>())
					{
						foliageEntity = child;
					}
					else if (child.HasComponentData<BirchFoliageInfo>())
					{
						foliageEntity = child;
					}
					else if (child.HasComponentData<OakFoliageInfo>())
					{
						foliageEntity = child;
					}
					else if (child.HasComponentData<MapleFoliageInfo>())
					{
						foliageEntity = child;
					}
					else if (child.HasComponentData<DefaultFoliageInfo>())
					{
						foliageEntity = child;
					}
					else if (child.HasComponentData<PineFoliageInfo>())
					{
						foliageEntity = child;
					}
				}
			);
			if (foliageEntity.HasPrivateComponent<MeshRenderer>())
			{
				auto& branchletRenderer = foliageEntity.GetPrivateComponent<MeshRenderer>();
				branchletRenderer->SetEnabled(false);
			}
			if (foliageEntity.HasPrivateComponent<Particles>())
			{
				auto& leavesRenderer = foliageEntity.GetPrivateComponent<Particles>();
				leavesRenderer->SetEnabled(false);
			}
			auto seq = _DataCollectionSystem->_ImageCaptureSequences[_SpecieIndex];
			Transform cameraTransform;

			cameraTransform.SetPosition(glm::vec3(seq.first.CameraPos.x, _CurrentTree.GetPrivateComponent<CakeTower>()->MaxHeight / 2.0f, seq.first.CameraPos.z * 1.5f));
			cameraTransform.SetEulerRotation(glm::radians(glm::vec3(0, 0, 0)));
			_DataCollectionSystem->_ImageCameraEntity.SetComponentData(cameraTransform);

			_CurrentTree.GetPrivateComponent<CakeTower>()->FormEntity();
			_CurrentTree.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
			_Status = RealTreeReconstructionSystemStatus::CaptureCakeTower;
#pragma endregion
		}else
		{
			_Status = RealTreeReconstructionSystemStatus::CleanUp;
		}
		break;
	case RealTreeReconstructionSystemStatus::CaptureCakeTower:
		_Status = RealTreeReconstructionSystemStatus::CleanUp;
		break;
	case RealTreeReconstructionSystemStatus::CleanUp:
		if (_SpecieIndex == 0) {
		std::string path = _StorePath + "rbv_recon/" +
			std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_tree.png";
		_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
			path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);
	}
	TreeManager::DeleteAllTrees();
	_Status = RealTreeReconstructionSystemStatus::Idle;
	break;
	}
}
