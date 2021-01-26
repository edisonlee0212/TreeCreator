#include "RealTreeReconstructionSystem.h"
#include "AcaciaFoliageGenerator.h"
#include "MapleFoliageGenerator.h"
#include "PineFoliageGenerator.h"
#include "WillowFoliageGenerator.h"
#include "AppleFoliageGenerator.h"
#include "OakFoliageGenerator.h"
#include "BirchFoliageGenerator.h"
#include "MaskProcessor.h"
#include "Ray.h"
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
				_TrainingDoc = rapidcsv::Document("./tree_real/045_rbv_photographs_predicted.csv", rapidcsv::LabelParams(-1, 1));

				_ReconIndex = 7;
				_SpecieIndex = 2;
				_ReconAmount = 12;
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
		_TargetCakeTower->MaxHeight = 16.5;
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
		_TargetInternodeSize = 8087;
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
	_ReconMainBranchInternodeLimit = 1000;
	if (!_UseMask)
	{
		_AgeForMainBranches = 0;
		_ReconMainBranchInternodeLimit = 0;
	}
	
	auto& seq = _DataCollectionSystem->_ImageCaptureSequences[_SpecieIndex];
	_DataCollectionSystem->SetCameraPose(seq.first.CameraPos, seq.first.CameraEulerDegreeRot);
	auto treeParameter = seq.second;

#pragma region Slim
	treeParameter.ThicknessControlFactor = 0.69;
	//treeParameter.LowBranchPruningFactor = 0.7;
	//treeParameter.Age = 8;
#pragma endregion

	
	/*
	treeParameter.ThicknessControlFactor = 0.69;
	treeParameter.ApicalDominanceBase = 4;
	treeParameter.GravityBendingStrength = 0.8;
	//treeParameter.GravitropismLevelFactor = -0.08;
	treeParameter.VarianceApicalAngle = 0.0f;
	*/
	if(_UseMask)treeParameter.Age = 200;
	_MaskPath = _StorePath + "mask/" + std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_tree.bmp";
	_SkeletonPath = _StorePath + "skeleton_recon/" + std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_tree.bmp";
	_TargetMask = ResourceManager::LoadTexture(false, _MaskPath);
	_TargetSkeleton = ResourceManager::LoadTexture(false, _SkeletonPath);

	_CurrentTree = _PlantSimulationSystem->CreateTree(treeParameter, glm::vec3(0.0f));
	auto& maskProcessor = _CurrentTree.GetPrivateComponent<MaskProcessor>();
	maskProcessor->_Skeleton = _TargetSkeleton;
	maskProcessor->_Mask = _TargetMask;
	maskProcessor->AttractionDistance = 1.0f;
	maskProcessor->RemovalDistance = 0.7f;
	maskProcessor->PlaceAttractionPoints();
	//Disable();
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
	PushInternode(rootInternode, cameraLTW, treeLTW);

	_Internodes.resize(0);
	TreeManager::GetInternodeQuery().ToEntityArray(treeIndex, _Internodes);
	_MainBranchInternodeSize = _Internodes.size();
}

void RealTreeReconstructionSystem::PushInternode(Entity internode, const GlobalTransform& cameraTransform,
	const GlobalTransform& treeLTW)
{
	auto info = internode.GetComponentData<InternodeInfo>();
	if (info.DistanceToRoot > 19.0)
	{
		EntityManager::DeleteEntity(internode);
		return;
	}
	auto parentLTW = info.GlobalTransform;
	if (glm::any(glm::isnan(parentLTW[3])))
	{
		EntityManager::DeleteEntity(internode);
		return;
	}
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
			GlobalTransform globalTransform;
			globalTransform.Value = (treeLTW.Value * parentLTW * internodeInfo.LocalTransform);
			Ray ray;
			ray.Start = cameraTransform.GetPosition();
			ray.Direction = globalTransform.GetPosition() - ray.Start;
			ray.Length = glm::length(ray.Direction);
			ray.Direction = glm::normalize(ray.Direction);
			if (internodeInfo.IsMaxChild)
			{
				//Is max child, no rotation.
				//glm::vec3 closestPointOnRay = glm::closestPointOnLine();
				glm::vec3 front = newGlobalRotation * glm::vec3(0, 0, -1);
				glm::vec3 up = newGlobalRotation * glm::vec3(0, 1, 0);
				up = glm::normalize(glm::cross(glm::cross(front, up), front));
				newGlobalRotation = glm::quatLookAt(front, up);
			}
			else if (internodeInfo.DistanceToRoot < 10)
			{
				//1. Calculate the plane in 3d space.
				glm::vec3 orig = internodeInfo.ParentTranslation;

				glm::vec3 front = newGlobalRotation * glm::vec3(0, 0, -1);
				auto test = orig + glm::vec3(front.x, 0, front.z) * 100.0f;
				if (true) {
					auto slice = _TargetCakeTower->SelectSlice(test);
					//slice.x += 1;
					if (slice.x >= _TargetCakeTower->LayerAmount) slice.x = _TargetCakeTower->LayerAmount - 1;
					glm::vec3 weight = glm::vec3(0);
					const float sliceAngle = 360.0f / _TargetCakeTower->SectorAmount;
					int range = 1;
					float max = -1;
					for (int i = -range; i <= range; i++)
					{
						auto y = slice.y + i;
						if (y < 0) y += _TargetCakeTower->SectorAmount;
						if (y >= _TargetCakeTower->SectorAmount) y -= _TargetCakeTower->SectorAmount;

						if (_TargetCakeTower->CakeTiers[slice.x][y].MaxDistance == 0) continue;

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
						float distance = _TargetCakeTower->CakeTiers[slice.x][y].MaxDistance;
						if (distance > max)
						{
							weight = glm::normalize(glm::vec3(x, 0.0f, z));
							max = distance;
						}
					}
					front = glm::normalize(weight);
				}
				else
				{
					front = glm::rotate(glm::vec3(front.x, 0, front.z), glm::radians(glm::linearRand(-10.0f, 10.0f)), glm::vec3(0, 1, 0));
				}
				glm::vec3 normal = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
				float stored = ray.Length;
				auto d = glm::dot(ray.Direction, normal);
				ray.Length = glm::dot(orig - ray.Start, normal) / d;
				if (glm::abs(stored - ray.Length) > 0.5f)
				{
					if (stored > ray.Length) ray.Length = stored - 0.5;
					else ray.Length = stored + 0.5;
				}
				glm::vec3 target = ray.GetEnd();
				front = target - internodeInfo.ParentTranslation;
				glm::vec3 up = newGlobalRotation * glm::vec3(0, 1, 0);
				internodeInfo.DistanceToParent = glm::length(front);
				front = glm::normalize(front);
				up = glm::normalize(glm::cross(glm::cross(front, up), front));
				newGlobalRotation = glm::quatLookAt(front, up);
			}
			internodeInfo.MainChildRotation = newGlobalRotation;
			actualLocalRotation = glm::inverse(rotation) * glm::inverse(internodeInfo.ParentRotation) * newGlobalRotation;
			internodeInfo.DesiredLocalRotation = actualLocalRotation;
			internodeInfo.LocalTransform = glm::translate(glm::mat4(1.0f), actualLocalRotation * glm::vec3(0, 0, -1)
				* internodeInfo.DistanceToParent) * glm::mat4_cast(actualLocalRotation)
				* glm::scale(glm::vec3(1.0f));
			internodeInfo.GlobalTransform = parentLTW * internodeInfo.LocalTransform;
			internodeInfo.StartAge = 0;
			EntityManager::SetComponentData(child, internodeInfo);
			globalTransform.Value = treeLTW.Value * internodeInfo.GlobalTransform * glm::scale(glm::vec3(0.3f));
			EntityManager::SetComponentData(child, globalTransform);
		}
	);
	EntityManager::ForEachChild(internode, [&cameraTransform, this, &treeLTW](Entity child)
		{
			PushInternode(child, cameraTransform, treeLTW);
		}
	);
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
			_Status = RealTreeReconstructionSystemStatus::Test;
			CreateTree();
		}
		else OnGui();
		break;
	case RealTreeReconstructionSystemStatus::Test:
		_Status = RealTreeReconstructionSystemStatus::CreateTree;
		break;
	case RealTreeReconstructionSystemStatus::CreateTree:
		
		if (_UseMask) {
			_Growing = true;
			_Status = RealTreeReconstructionSystemStatus::MainBranches;
		}
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
		//Disable();
		break;
	case RealTreeReconstructionSystemStatus::MainBranches:
		TryGrowTree();
		
		if (!_Growing)
		{
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
		if(true)
		{
			Disable();
		}
		break;
	case RealTreeReconstructionSystemStatus::CollectData:
		{
			std::string path = _StorePath + "image_recon/" +
				std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) +
				"_" + _Name + ".png";
			_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);

		}

		if (true/*(_ReconIndex == 0 && _SpecieIndex == 0) ||
			(_ReconIndex == 1 && _SpecieIndex == 5) ||
			(_ReconIndex == 2 && _SpecieIndex == 4) ||
			(_ReconIndex == 3 && _SpecieIndex == 1)*/
			) {
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
		
		if (true/*(_ReconIndex == 0 && _SpecieIndex == 0) ||
			(_ReconIndex == 1 && _SpecieIndex == 5) ||
			(_ReconIndex == 2 && _SpecieIndex == 4) ||
			(_ReconIndex == 3 && _SpecieIndex == 1)*/
		){
		std::string path = _StorePath + "rbv_recon/" +
			std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + 
			std::string(5 - std::to_string(_SpecieIndex).length(), '0') + std::to_string(_SpecieIndex) +
			"_tree.png";
		_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
			path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);
	}
	TreeManager::DeleteAllTrees();
	_Status = RealTreeReconstructionSystemStatus::Idle;
	break;
	}
}
