#include "TreeReconstructionSystem.h"
#include "Ray.h"
#include "CakeTower.h"
#include "MaskProcessor.h"
#include "glm/glm/gtx/intersect.hpp"
using namespace TreeUtilities;
void TreeUtilities::TreeReconstructionSystem::OnGui()
{

}

void TreeReconstructionSystem::ExportAllData()
{
	ExportCakeTower(_StorePath + _Name + "/" + "CakeTowers" + (_EnableSpaceColonization ? "SC" + std::to_string(_ControlLevel) : "IPM"));
	_CakeTowersOutputList.clear();
}

void TreeReconstructionSystem::TryGrowTree()
{
	if (_CurrentTree.IsNull() || !_CurrentTree.IsValid() || _CurrentTree.IsDeleted()) return;
	if (!_Growing) return;
	_Growing = _PlantSimulationSystem->GrowTree(_CurrentTree, true);

	if (_Growing) {
		_PlantSimulationSystem->RefreshTrees();
		return;
	}//TODO: Check growing condition;
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
	PushInternode(rootInternode, cameraLTW, treeLTW, 0);
	_PlantSimulationSystem->ApplyLocalTransform(_CurrentTree);
	_Internodes.resize(0);
	const auto treeIndex = _CurrentTree.GetComponentData<TreeIndex>();
	TreeManager::GetInternodeQuery().ToEntityArray(treeIndex, _Internodes);
	_MainBranchInternodeSize = _Internodes.size();
}

void TreeReconstructionSystem::PushInternode(Entity internode, const GlobalTransform& cameraTransform, const GlobalTransform& treeLTW, int branchingLevel)
{
	if (branchingLevel > 2) {
		EntityManager::DeleteEntity(internode);
		return;
	}
	auto parentLTW = internode.GetComponentData<InternodeInfo>().GlobalTransform;
	EntityManager::ForEachChild(internode, [&cameraTransform, this, &treeLTW, &parentLTW](Entity child)
		{
			auto internodeInfo = child.GetComponentData<InternodeInfo>();
			glm::vec3 scale;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::quat rotation;
			glm::vec3 translation;
			glm::decompose(parentLTW, scale, internodeInfo.ParentRotation, internodeInfo.ParentTranslation, skew, perspective);
			glm::quat actualLocalRotation;

			glm::decompose(treeLTW.Value, scale, rotation, translation, skew, perspective);
			glm::quat newGlobalRotation = rotation * internodeInfo.ParentRotation * internodeInfo.DesiredLocalRotation;
			auto globalTransform = child.GetComponentData<GlobalTransform>();
			Ray ray;
			ray.Start = cameraTransform.GetPosition();
			ray.Direction = globalTransform.GetPosition() - ray.Start;
			ray.Length = glm::length(ray.Direction);
			ray.Direction = glm::normalize(ray.Direction);
			if (internodeInfo.IsMaxChild)
			{
				//Is max child, no rotation.
				//glm::vec3 closestPointOnRay = glm::closestPointOnLine();
				//glm::vec3 front = newGlobalRotation * glm::vec3(0, 0, -1);
				//glm::vec3 up = newGlobalRotation * glm::vec3(0, 1, 0);
			}
			else
			{
				//1. Calculate the plane in 3d space.
				glm::vec3 orig = internodeInfo.ParentTranslation;

				glm::vec3 front = newGlobalRotation * glm::vec3(0, 0, -1);

				auto test = orig + (glm::vec3(front.x, 0, front.z) * 100.0f);
				auto slice = _TargetCakeTower->SelectSlice(test);
				slice.x += 1;
				if (slice.x >= _TargetCakeTower->SliceAmount) slice.x = _TargetCakeTower->SliceAmount - 1;
				Debug::Log(std::to_string(slice.x) + "-" + std::to_string(slice.y));
				glm::vec3 weight = glm::vec3(0);
				const float sliceAngle = 360.0f / _TargetCakeTower->SectorAmount;
				for (int i = -2; i <= 2; i++)
				{
					auto y = slice.y + i;
					if (_TargetCakeTower->CakeTiers[slice.x][y].MaxDistance == 0) continue;
					if (y < 0) y += _TargetCakeTower->SectorAmount;
					if (y >= _TargetCakeTower->SectorAmount) y -= _TargetCakeTower->SectorAmount;

					float currentAngle = sliceAngle * (static_cast<float>(y) + 0.5f);
					if (currentAngle >= 360) currentAngle = 0;
					float x = glm::abs(glm::tan(glm::radians(currentAngle)));
					float z = 1.0f;
					if (currentAngle >= 0 && currentAngle <= 90)
					{
						z *= -1;
						x *= -1;
					}
					else if (currentAngle > 90 && currentAngle <= 180)
					{
						x *= -1;
					}
					else if (currentAngle > 270 && currentAngle <= 360)
					{
						z *= -1;
					}
					glm::vec3 position = glm::normalize(glm::vec3(x, 0.0f, z)) * _TargetCakeTower->CakeTiers[slice.x][y].MaxDistance;
					weight += position;
				}
				front = glm::normalize(weight);
				glm::vec3 normal = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
				Debug::Log("Target branch direction: " + std::to_string(front.x) + "," + std::to_string(front.y) + "," + std::to_string(front.z));
				Debug::Log("Plane normal: " + std::to_string(normal.x) + "," + std::to_string(normal.y) + "," + std::to_string(normal.z));
				auto d = glm::dot(ray.Direction, normal);
				Debug::Log(std::to_string(ray.Length));
				ray.Length = glm::dot(orig - ray.Start, normal) / d;
				//glm::intersectRayPlane(ray.Start, -ray.Direction, orig, normal, ray.Length);
				Debug::Log(std::to_string(ray.Length));
				glm::vec3 target = ray.GetEnd();
				Debug::Log("Ray Start: " + std::to_string(ray.Start.x) + "," + std::to_string(ray.Start.y) + "," + std::to_string(ray.Start.z));
				Debug::Log("Ray Dir: " + std::to_string(ray.Direction.x) + "," + std::to_string(ray.Direction.y) + "," + std::to_string(ray.Direction.z));
				Debug::Log("Target point: " + std::to_string(target.x) + "," + std::to_string(target.y) + "," + std::to_string(target.z));
				Debug::Log("Parent Position: " + std::to_string(internodeInfo.ParentTranslation.x) + "," + std::to_string(internodeInfo.ParentTranslation.y) + "," + std::to_string(internodeInfo.ParentTranslation.z));
				front = target - internodeInfo.ParentTranslation;
				glm::vec3 up = newGlobalRotation * glm::vec3(0, 1, 0);
				internodeInfo.DistanceToParent = glm::length(front);
				front = glm::normalize(front);
				up = glm::normalize(glm::cross(glm::cross(front, up), front));
				Debug::Log("Branch direction: " + std::to_string(front.x) + "," + std::to_string(front.y) + "," + std::to_string(front.z));
				newGlobalRotation = glm::quatLookAt(front, up);
			}

			actualLocalRotation = glm::inverse(rotation) * glm::inverse(internodeInfo.ParentRotation) * newGlobalRotation;
			internodeInfo.DesiredLocalRotation = actualLocalRotation;
			internodeInfo.LocalTransform = glm::translate(glm::mat4(1.0f), actualLocalRotation * glm::vec3(0, 0, -1)
				* internodeInfo.DistanceToParent) * glm::mat4_cast(actualLocalRotation)
				* glm::scale(glm::vec3(1.0f));
			if (!internodeInfo.IsMaxChild) Debug::Log("Original Position: " + std::to_string(internodeInfo.GlobalTransform[3].x) + "," + std::to_string(internodeInfo.GlobalTransform[3].y) + "," + std::to_string(internodeInfo.GlobalTransform[3].z));
			internodeInfo.GlobalTransform = parentLTW * internodeInfo.LocalTransform;
			if (!internodeInfo.IsMaxChild) Debug::Log("Result Position: " + std::to_string(internodeInfo.GlobalTransform[3].x) + "," + std::to_string(internodeInfo.GlobalTransform[3].y) + "," + std::to_string(internodeInfo.GlobalTransform[3].z));
			internodeInfo.StartAge = 0;
			EntityManager::SetComponentData(child, internodeInfo);
			globalTransform.Value = treeLTW.Value * internodeInfo.GlobalTransform * glm::scale(glm::vec3(_TargetTreeParameter.InternodeSize));
			EntityManager::SetComponentData(child, globalTransform);
		}
	);
	EntityManager::ForEachChild(internode, [&cameraTransform, this, &treeLTW, branchingLevel](Entity child)
		{
			auto internodeInfo = child.GetComponentData<InternodeInfo>();
			if (internodeInfo.IsMaxChild)
			{
				PushInternode(child, cameraTransform, treeLTW, branchingLevel);
			}else
			{
				PushInternode(child, cameraTransform, treeLTW, branchingLevel + 1);
			}
		});
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
		_SkeletonPath = "./tree_recon/Acacia/Skeleton.png";
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
		_SkeletonPath = "./tree_recon/Apple/Skeleton.png";
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
		_SkeletonPath = "./tree_recon/Willow/Skeleton.png";
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
		_SkeletonPath = "./tree_recon/Maple/Skeleton.png";
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
		_SkeletonPath = "./tree_recon/Birch/Skeleton.png";
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
		_SkeletonPath = "./tree_recon/Oak/Skeleton.png";
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
		_SkeletonPath = "./tree_recon/Pine/Skeleton.png";
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
	_TargetMask = ResourceManager::LoadTexture(false, _MaskPath);
	_TargetSkeleton = ResourceManager::LoadTexture(false, _SkeletonPath);
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
	if (_PlantSimulationSystem == nullptr) return;
	_PlantSimulationSystem->_AutoGenerateMesh = false;
	_PlantSimulationSystem->_AutoGenerateLeaves = false;
	std::string path;
	switch (_Status)
	{
	case TreeReconstructionSystemStatus::Idle:
		if (_NeedExport) {
			if (_StartIndex <= _EndIndex)
			{
				auto parameters = _TargetTreeParameter;
				parameters.Seed = _StartIndex;
				parameters.Age = 999;// _TargetTreeParameter.Age - _AgeForMainBranches;
				_PlantSimulationSystem->_ControlLevel = _ControlLevel;
				_CurrentTree = _PlantSimulationSystem->CreateTree(parameters, glm::vec3(0.0f));
				
				_CurrentTree.GetPrivateComponent<MaskProcessor>()->_Skeleton = _TargetSkeleton;
				_CurrentTree.GetPrivateComponent<MaskProcessor>()->_Mask = _TargetMask;
				_CurrentTree.GetPrivateComponent<MaskProcessor>()->PlaceAttractionPoints();
				_Status = TreeReconstructionSystemStatus::MainBranches;
				_Growing = true;
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
		TryGrowTree();
		if (!_Growing)
		{
			_Status = TreeReconstructionSystemStatus::NormalGrowth;
			auto& cakeTower = _CurrentTree.GetPrivateComponent<CakeTower>();
			cakeTower->Load(_TargetCakeTowerPath);
			cakeTower->GenerateMesh();
			cakeTower->ClearAttractionPoints();
			cakeTower->GenerateAttractionPoints(2000);
			cakeTower->EnableSpaceColonization = _EnableSpaceColonization;
			_PlantSimulationSystem->ResumeGrowth();
			TreeAge age = _CurrentTree.GetComponentData<TreeAge>();
			age.Value = _AgeForMainBranches;
			age.ToGrowIteration = _TargetTreeParameter.Age - _AgeForMainBranches;
			_CurrentTree.SetComponentData(age);
		}
		break;
	case TreeReconstructionSystemStatus::NormalGrowth:
		{
			bool stop = false;
			_Internodes.resize(0);
			const auto treeIndex = _CurrentTree.GetComponentData<TreeIndex>();
			TreeManager::GetInternodeQuery().ToEntityArray(treeIndex, _Internodes);
			TreeAge age = _CurrentTree.GetComponentData<TreeAge>();
			if (_Internodes.size() > _TargetInternodeSize + _MainBranchInternodeSize)
			{
				stop = true;
			}
			if (!stop && !_PlantSimulationSystem->_Growing) {
				if (age.ToGrowIteration != 0 || age.Value > _MaxAge)
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
				_PlantSimulationSystem->PauseGrowth();
				TreeManager::GenerateSimpleMeshForTree(_CurrentTree, PlantSimulationSystem::_MeshGenerationResolution, PlantSimulationSystem::_MeshGenerationSubdivision);
				_PlantSimulationSystem->GenerateLeaves(_CurrentTree);
				_Status = TreeReconstructionSystemStatus::Render;
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
		//TreeManager::DeleteAllTrees();
		_StartIndex++;
		Disable();
		_Status = TreeReconstructionSystemStatus::Idle;
		break;
	}
}
