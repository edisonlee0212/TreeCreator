#include "TreeReconstructionSystem.h"

#include "CakeTower.h"

using namespace TreeUtilities;
void TreeUtilities::TreeReconstructionSystem::OnGui()
{
	
}

void TreeReconstructionSystem::ExportAllData()
{
	ExportCakeTower(_StorePath + _Name + "/" + "CakeTowers" + (_EnableSpaceColonization ? "SC" + std::to_string(_ControlLevel) : "IPM"));
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
	int cameraTransformIndex;
	switch (_Type) {
	case TreeType::Acacia:
		_Name = "Acacia";
		_StorePath = "./tree_recon/";
		_TargetCakeTowerPath = "./tree_recon/Acacia/AcaciaTarget.ct";
		_MaskPath = "./tree_recon/Acacia/AcaciaMask.png";
		_TreeParametersPath = "./acacia";
		cameraTransformIndex = 0;
		_ControlLevel = 0;
		_AgeForMainBranches = 4;
		_TargetInternodeSize = 2260;
		break;
	case TreeType::Apple:
		_Name = "Apple";
		_StorePath = "./tree_recon/";
		_TargetCakeTowerPath = "./tree_recon/Apple/AppleTarget.ct";
		_MaskPath = "./tree_recon/Apple/AppleMask.png";
		_TreeParametersPath = "./apple";
		cameraTransformIndex = 1;
		_ControlLevel = 0;
		_AgeForMainBranches = 4;
		_TargetInternodeSize = 1601;
		break;
	case TreeType::Willow:
		_Name = "Willow";
		_StorePath = "./tree_recon/";
		_TargetCakeTowerPath = "./tree_recon/Willow/WillowTarget.ct";
		_MaskPath = "./tree_recon/Willow/WillowMask.png";
		_TreeParametersPath = "./willow";
		cameraTransformIndex = 2;
		_ControlLevel = 0;
		_AgeForMainBranches = 4;
		_TargetInternodeSize = 2615;
		break;
	case TreeType::Maple:
		_Name = "Maple";
		_StorePath = "./tree_recon/";
		_TargetCakeTowerPath = "./tree_recon/Maple/MapleTarget.ct";
		_MaskPath = "./tree_recon/Maple/MapleMask.png";
		_TreeParametersPath = "./maple";
		cameraTransformIndex = 3;
		_ControlLevel = 0;
		_AgeForMainBranches = 4;
		_TargetInternodeSize = 7995;
		break;
	case TreeType::Birch:
		_Name = "Birch";
		_StorePath = "./tree_recon/";
		_TargetCakeTowerPath = "./tree_recon/Birch/BirchTarget.ct";
		_MaskPath = "./tree_recon/Maple/BirchMask.png";
		_TreeParametersPath = "./birch";
		cameraTransformIndex = 4;
		_ControlLevel = 0;
		_AgeForMainBranches = 4;
		_TargetInternodeSize = 5120;
		break;
	case TreeType::Oak:
		_Name = "Oak";
		_StorePath = "./tree_recon/";
		_TargetCakeTowerPath = "./tree_recon/Oak/OakTarget.ct";
		_MaskPath = "./tree_recon/Oak/OakMask.png";
		_TreeParametersPath = "./oak";
		cameraTransformIndex = 5;
		_ControlLevel = 0;
		_AgeForMainBranches = 4;
		_TargetInternodeSize = 4487;
		break;
	case TreeType::Pine:
		_Name = "Pine";
		_StorePath = "./tree_recon/";
		_TargetCakeTowerPath = "./tree_recon/Pine/PineTarget.ct";
		_MaskPath = "./tree_recon/Pine/PineMask.png";
		_TreeParametersPath = "./pine";
		cameraTransformIndex = 6;
		_ControlLevel = 0;
		_AgeForMainBranches = 4;
		_TargetInternodeSize = 8538;
		break;
	}
	
	_StartIndex = 1;
	_EndIndex = 50;
	_MaxAge = 30;

	_EnableSpaceColonization = false;
	
	auto& sequence = _DataCollectionSystem->_ImageCaptureSequences[cameraTransformIndex].first;
	_DataCollectionSystem->SetCameraPose(sequence.CameraPos, sequence.CameraEulerDegreeRot);
	
	_TargetTreeParameter = _PlantSimulationSystem->LoadParameters(_TreeParametersPath);
	_TargetMask = ResourceManager::LoadTexture(_MaskPath);
	_TargetCakeTower = std::make_unique<CakeTower>();
	_TargetCakeTower->Load(_TargetCakeTowerPath);
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
				_PlantSimulationSystem->_ControlLevel = _ControlLevel;
				_CurrentTree = _PlantSimulationSystem->CreateTree(_TargetTreeParameter, glm::vec3(0.0f));
				_PlantSimulationSystem->ResumeGrowth();
				_Status = TreeReconstructionSystemStatus::MainBranches;
			}
			else
			{
				ExportAllData();
				if (!_EnableSpaceColonization) {
					_StartIndex = 1;
					_EnableSpaceColonization = true;
				}
				else if (_ControlLevel < 2)
				{
					_ControlLevel++;
					_StartIndex = 1;
				}
				else {
					switch (_Type) {
					case TreeType::Acacia:
						_Type = TreeType::Apple;
						Init();
						break;
					case TreeType::Apple:
						_Type = TreeType::Willow;
						Init();
						break;
					case TreeType::Willow:
						_Type = TreeType::Maple;
						Init();
						break;
					case TreeType::Maple:
						_NeedExport = false;
						break;
						_Type = TreeType::Birch;
						Init();
						break;
					case TreeType::Birch:
						_Type = TreeType::Oak;
						Init();
						break;
					case TreeType::Oak:
						_Type = TreeType::Pine;
						Init();
						break;
					case TreeType::Pine:
						_NeedExport = false;
						break;
					}
				}
				
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
		path = _StorePath + _Name + "/" + (_EnableSpaceColonization ? "SC" + std::to_string(_ControlLevel) : "IPM") + "/" +
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
