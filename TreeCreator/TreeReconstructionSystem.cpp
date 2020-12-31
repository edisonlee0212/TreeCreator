#include "TreeReconstructionSystem.h"

#include "CakeTower.h"

using namespace TreeUtilities;
void TreeUtilities::TreeReconstructionSystem::OnGui()
{
	
}

void TreeReconstructionSystem::ExportAllData()
{
	ExportCakeTower(_StorePath + _Name + "/" + "CakeTowers" + (_EnableSpaceColonization ? "SC" : "IPM"));
	_CakeTowersOutputList.clear();
}

void TreeReconstructionSystem::SetPlantSimulationSystem(PlantSimulationSystem* value)
{
	_PlantSimulationSystem = value;
}

void TreeReconstructionSystem::SetDataCollectionSystem(DataCollectionSystem* value)
{
	_DataCollectionSystem = value;
}

void TreeReconstructionSystem::Init()
{
	_Name = "Apple";
	_StorePath = "./tree_recon/";
	_TargetCakeTowerPath = "./tree_recon/AppleTarget.ct";
	_MaskPath = "./tree_recon/AppleMask.png";
	_TreeParametersPath = "./apple";
	
	_AgeForMainBranches = 4;
	_TargetInternodeSize = 1601;

	_StartIndex = 1;
	_EndIndex = 50;
	_MaxAge = 30;

	_EnableSpaceColonization = false;
	
	_TargetTreeParameter = _PlantSimulationSystem->LoadParameters(_TreeParametersPath);
	_TargetMask = ResourceManager::LoadTexture(_MaskPath);
	_TargetCakeTower = std::make_unique<CakeTower>();
	_TargetCakeTower->Load(_TargetCakeTowerPath);
	_CakeTowersOutputList.emplace_back(1, _Name, _TargetCakeTower);
	_NeedExport = true;
	Enable();
}

void TreeReconstructionSystem::ExportCakeTower(const std::string path)
{
	std::ofstream ofs;
	ofs.open((path + ".csv").c_str(), std::ofstream::out | std::ofstream::trunc);
	if (ofs.is_open())
	{
		for (auto& instance : _CakeTowersOutputList) {
			auto cakeTower = instance.data;
			std::string output = "";
			output += std::to_string(instance.Index) + ",";
			output += instance.Name;
			for (auto& tier : cakeTower)
			{
				for (auto& slice : tier)
				{
					output += "," + std::to_string(slice.MaxDistance);
				}
			}
			output += "\n";
			ofs.write(output.c_str(), output.size());
			ofs.flush();
		}
		ofs.close();
		Debug::Log("Tree group saved: " + path + ".csv");
	}
	else
	{
		Debug::Error("Can't open file!");
	}
}

void TreeUtilities::TreeReconstructionSystem::OnCreate()
{
	
}

void TreeUtilities::TreeReconstructionSystem::Update()
{
	if(_PlantSimulationSystem == nullptr) return;
	_PlantSimulationSystem->_AutoGenerateMesh = false;
	_PlantSimulationSystem->_AutoGenerateLeaves = false;
	std::string path;
	switch (_Status)
	{
	case TreeReconstructionSystemStatus::Idle:
		if (_NeedExport) {
			if (_StartIndex <= _EndIndex)
			{
				_TargetTreeParameter.Seed = _StartIndex;
				_TargetTreeParameter.Age = _AgeForMainBranches;
				_CurrentTree = _PlantSimulationSystem->CreateTree(_TargetTreeParameter, glm::vec3(0.0f));
				_Status = TreeReconstructionSystemStatus::MainBranches;
			}
			else
			{
				ExportAllData();
				_NeedExport = false;
			}
		}
		break;
	case TreeReconstructionSystemStatus::MainBranches:
		if (!_PlantSimulationSystem->_Growing)
		{
			_Status = TreeReconstructionSystemStatus::NormalGrowth;
			auto& cakeTower = _CurrentTree.GetPrivateComponent<CakeTower>();
			cakeTower->Load(_TargetCakeTowerPath);
			cakeTower->GenerateMesh();
			cakeTower->ClearAttractionPoints();
			cakeTower->GenerateAttractionPoints(2000);
			cakeTower->EnableSpaceColonization = _EnableSpaceColonization;
		}
		break;
	case TreeReconstructionSystemStatus::NormalGrowth:
		if (!_PlantSimulationSystem->_Growing)
		{
			
			_Internodes.resize(0);
			const auto treeIndex = _CurrentTree.GetComponentData<TreeIndex>();
			TreeManager::GetInternodeQuery().ToEntityArray(treeIndex, _Internodes);
			if(_Internodes.size() > _TargetInternodeSize)
			{
				TreeManager::GenerateSimpleMeshForTree(_CurrentTree, PlantSimulationSystem::_MeshGenerationResolution, PlantSimulationSystem::_MeshGenerationSubdivision);
				_PlantSimulationSystem->GenerateLeaves(_CurrentTree);
				_Status = TreeReconstructionSystemStatus::Render;
			}else
			{
				TreeAge age = _CurrentTree.GetComponentData<TreeAge>();
				if(age.ToGrowIteration != 0 || age.Value > _MaxAge)
				{
					TreeManager::GenerateSimpleMeshForTree(_CurrentTree, PlantSimulationSystem::_MeshGenerationResolution, PlantSimulationSystem::_MeshGenerationSubdivision);
					_PlantSimulationSystem->GenerateLeaves(_CurrentTree);
					_Status = TreeReconstructionSystemStatus::Render;
				}else
				{
					_PlantSimulationSystem->_Growing = true;
					age.ToGrowIteration++;
					_CurrentTree.SetComponentData(age);
				}
			}
		}
		break;
	case TreeReconstructionSystemStatus::Render:
		_Status = TreeReconstructionSystemStatus::CollectData;
		break;
	case TreeReconstructionSystemStatus::CollectData:
		path = _StorePath + _Name + "/" + (_EnableSpaceColonization ? "SC" : "IPM") + "/" +
			std::string(5 - std::to_string(_StartIndex).length(), '0') + std::to_string(_StartIndex)
			+ ".jpg";
		_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
			path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);
		_CurrentTree.GetPrivateComponent<CakeTower>()->CalculateVolume(_TargetCakeTower->MaxHeight);
		_CakeTowersOutputList.emplace_back(_StartIndex, _Name, _CurrentTree.GetPrivateComponent<CakeTower>());

		_Status = TreeReconstructionSystemStatus::CleanUp;
		break;
	case TreeReconstructionSystemStatus::CleanUp:
		TreeManager::DeleteAllTrees();
		_StartIndex++;
		
		_Status = TreeReconstructionSystemStatus::Idle;
		break;
	}
}
