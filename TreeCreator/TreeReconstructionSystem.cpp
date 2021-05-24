#include "TreeReconstructionSystem.h"
#include "Ray.h"
#include "RBV.h"
#include "MaskProcessor.h"
#include "glm/glm/gtx/intersect.hpp"
#include "rapidcsv/rapidcsv.h"

#include "AcaciaFoliageGenerator.h"
#include "MapleFoliageGenerator.h"
#include "PineFoliageGenerator.h"
#include "WillowFoliageGenerator.h"
#include "AppleFoliageGenerator.h"
#include "OakFoliageGenerator.h"
#include "BirchFoliageGenerator.h"
using namespace TreeUtilities;
void TreeUtilities::TreeReconstructionSystem::OnGui()
{
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Reconstruction Data Generation")) {
			if (ImGui::Button("Create new data set...")) {
				ImGui::OpenPopup("Data set wizard");
			}

			if (ImGui::Button("Start reconstruction...")) {
				ImGui::OpenPopup("Reconstruction wizard");
			}

			if (ImGui::Button("Start reconstruction with learning data...")) {
				ImGui::OpenPopup("Reconstruction wizard 2");
			}

			const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Data set wizard", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Data set options:");
				ImGui::DragInt("Target Amount##Reconstruction", &_GenerateAmount, 1, 1, 50);
				if (ImGui::Button("Start Generation"))
				{
					std::filesystem::remove_all("tree_recon");
					std::filesystem::create_directory("tree_recon");
					for (const auto& seq : _DataCollectionSystem->_ImageCaptureSequences)
					{
						std::filesystem::create_directory("tree_recon/" + seq.first.Name);
						for (auto i = 0; i < _GenerateAmount; i++)
						{
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i));
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/image");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/image/mask");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/image/no_mask");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/image_top");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/image_top/mask");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/image_top/no_mask");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/tower");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/tower/mask");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/tower/no_mask");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/tower_top");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/tower_top/mask");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/tower_top/no_mask");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/volume");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/volume/mask");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/volume/no_mask");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/obj");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/obj/mask");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/obj/no_mask");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/graph");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/graph/mask");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/graph/no_mask");
						}

					}
					_DataCollectionSystem->_ExportImages = true;
					_Status = TreeReconstructionSystemStatus::Reconstruction;
					_DataCollectionSystem->_Reconstruction = true;
					_DataCollectionSystem->_Index = 0;
					_DataCollectionSystem->_Seed = 0;
					_DataCollectionSystem->_ReconPath = "tree_recon/" + _DataCollectionSystem->_ImageCaptureSequences[_DataCollectionSystem->_Index].first.Name + "/seed_" + std::to_string(_DataCollectionSystem->_Seed) + "/target";
					ImGui::CloseCurrentPopup();
					
				}
				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			if (ImGui::BeginPopupModal("Reconstruction wizard 2", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::DragInt("Reconstruction Amount##Reconstruction", &_ReconAmount, 1, 1, 50);
				ImGui::Checkbox("Use volume for main branches", &_UseCakeTower);
				if (ImGui::Button("Start Reconstruction"))
				{
					_FromTraining = true;
					_ReconCounter = -1;
					_ReconIndex = 0;
					_UseMask = true;
					_TrainingDoc = rapidcsv::Document("./tree_data/036_rbv_multi_predicted.csv", rapidcsv::LabelParams(-1, 1));
					std::vector<std::string> firstColumn = _TrainingDoc.GetColumn<std::string>(0);
					Debug::Log("Count: " + std::to_string(firstColumn.size()));
					_TrainingAmount = 4	;// firstColumn.size();
					Init();
					_NeedExport = true;
					_Status = TreeReconstructionSystemStatus::Idle;
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			if (ImGui::BeginPopupModal("Reconstruction wizard", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::DragInt("Reconstruction Start seed##Reconstruction", &_ReconSeed, 1, 0, 500);
				ImGui::DragInt("Reconstruction End (seed + 1)##Reconstruction", &_GenerateAmount, 1, 0, 500);
				ImGui::DragInt("Target per specie##Reconstruction", &_ReconAmount, 1, 1, 500);
				ImGui::Checkbox("Use RBV", &_UseCakeTower);
				if (ImGui::Button("Start Reconstruction"))
				{
					_Prefix = "_8_8";
					_UseMask = true;
					_FromTraining = false;
					//_ReconCounter = 15;//-1 //Cherry;
					_ReconCounter = -1;
					_ReconIndex = 5;
					_ReconStartSeed = _ReconSeed;
					Init();
					
					_NeedExport = true;
					_Status = TreeReconstructionSystemStatus::Idle;
					ImGui::CloseCurrentPopup();
				}
				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine();
			
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void TreeReconstructionSystem::Switch()
{
	_Paused = true;
	_PreviousStatus = _Status;
	Application::SetPlaying(false);
}

void TreeReconstructionSystem::ExportAllData()
{
	ExportCakeTower(_StorePath + "/CakeTowers");
	_CakeTowersOutputList.clear();
}

void TreeReconstructionSystem::TryGrowTree()
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
	if(age.Value > 200) _Growing = false;
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

void TreeReconstructionSystem::SetEnableFoliage(bool enabled) const
{
	auto foliageEntity = FindFoliageEntity();
	if (foliageEntity.HasPrivateComponent<MeshRenderer>())
	{
		auto& branchletRenderer = foliageEntity.GetPrivateComponent<MeshRenderer>();
		branchletRenderer->SetEnabled(enabled);
	}
	if (foliageEntity.HasPrivateComponent<Particles>())
	{
		auto& leavesRenderer = foliageEntity.GetPrivateComponent<Particles>();
		leavesRenderer->SetEnabled(enabled);
	}
}

Entity TreeReconstructionSystem::FindFoliageEntity() const
{
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
	return foliageEntity;
}

void TreeReconstructionSystem::PushInternode(Entity internode, const GlobalTransform& cameraTransform, const GlobalTransform& treeLTW)
{
	auto info = internode.GetComponentData<InternodeInfo>();
	if (info.DistanceToRoot > 99)
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

			glm::decompose(treeLTW.m_value, scale, rotation, translation, skew, perspective);
			glm::quat newGlobalRotation = rotation * internodeInfo.ParentRotation * internodeInfo.DesiredLocalRotation;
			GlobalTransform globalTransform;
			globalTransform.m_value = (treeLTW.m_value * parentLTW * internodeInfo.LocalTransform);
			Ray ray;
			ray.m_start = cameraTransform.GetPosition();
			ray.m_direction = globalTransform.GetPosition() - ray.m_start;
			ray.m_length = glm::length(ray.m_direction);
			ray.m_direction = glm::normalize(ray.m_direction);
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
				if (_UseCakeTower) {
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
				float stored = ray.m_length;
				auto d = glm::dot(ray.m_direction, normal);
				ray.m_length = glm::dot(orig - ray.m_start, normal) / d;
				if (glm::abs(stored - ray.m_length) > 0.5f)
				{
					if (stored > ray.m_length) ray.m_length = stored - 0.5;
					else ray.m_length = stored + 0.5;
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
			globalTransform.m_value = treeLTW.m_value * internodeInfo.GlobalTransform * glm::scale(glm::vec3(_TargetTreeParameter.InternodeSize));
			EntityManager::SetComponentData(child, globalTransform);
		}
	);
	EntityManager::ForEachChild(internode, [&cameraTransform, this, &treeLTW](Entity child)
		{
			PushInternode(child, cameraTransform, treeLTW);
		}
	);
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
	_ControlLevel = 0;
	_EnableSpaceColonization = true;
	if (_FromTraining)
	{
		_TargetCakeTower = std::make_unique<RBV>();
		std::vector<float> rbv = _TrainingDoc.GetRow<float>(_ReconIndex);
		_Name = _TrainingDoc.GetRowName(_ReconIndex);
		_StorePath = "./tree_data/";
		std::string postFix = "_0";
		/*
		switch (_ReconIndex % 4)
		{
		case 0:
			postFix = "_0";
			break;
		case 1:
			postFix = "_90";
			break;
		case 2:
			postFix = "_180";
			break;
		case 3:
			postFix = "_270";
			break;
		}
		*/
		std::string name = std::string(5 - std::to_string((_ReconIndex - _ReconIndex % 4) / 4).length(), '0') + std::to_string((_ReconIndex - _ReconIndex % 4) / 4) + "_" + _Name + postFix + ".png";
		_MaskPath = _StorePath + "mask_val/" + name;
		_SkeletonPath = _StorePath + "skeleton_recon/" + name;
		_TargetCakeTower->CakeTiers.resize(8);
		for (int i = 0; i < 8; i++)
		{
			_TargetCakeTower->CakeTiers[i].resize(8);
			for (int j = 0; j < 8; j++)
			{
				_TargetCakeTower->CakeTiers[i][j].MaxDistance = rbv[i * 8 + (j + 2 * _ReconIndex % 4) % 8] * 30;
			}
		}
		_TargetCakeTower->LayerAmount = 8;
		_TargetCakeTower->SectorAmount = 8;

		_TargetCakeTower->MaxRadius = 30;
		_TargetCakeTower->GenerateMesh();
		//_TargetKDop = std::make_unique<KDop>();
		if (_Name._Equal("acacia")) {
			_LearningIndex = 0;
			_TargetCakeTower->MaxHeight = 15;
			_AgeForMainBranches = 2;
			_TargetInternodeSize = 2260;
			_ReconMainBranchInternodeLimit = 100;
		}
		else if (_Name._Equal("apple")) {
			_LearningIndex = 1;
			_TargetCakeTower->MaxHeight = 12.5;
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 2001;
			_ReconMainBranchInternodeLimit = 50;
		}
		else if (_Name._Equal("willow")) {
			_LearningIndex = 2;
			_TargetCakeTower->MaxHeight = 19;
			_AgeForMainBranches = 0;
			_TargetInternodeSize = 2615;
			_ReconMainBranchInternodeLimit = 0;
		}
		else if (_Name._Equal("maple")) {
			_LearningIndex = 3;
			_ControlLevel = 1;
			_TargetCakeTower->MaxHeight = 32;
			_AgeForMainBranches = 0;
			_TargetInternodeSize = 7995;
			_ReconMainBranchInternodeLimit = 0;
		}
		else if (_Name._Equal("birch")) {
			_LearningIndex = 4;
			_TargetCakeTower->MaxHeight = 31;
			_AgeForMainBranches = 0;
			_TargetInternodeSize = 5120;// 5120;
			_ReconMainBranchInternodeLimit = 0;
		}
		else if (_Name._Equal("oak")) {
			_LearningIndex = 5;
			_TargetCakeTower->MaxHeight = 18.5;
			_AgeForMainBranches = 1;
			_TargetInternodeSize = 4887;
			_ReconMainBranchInternodeLimit = 25;
		}
		else if (_Name._Equal("pine")) {
			_EnableSpaceColonization = false;
			_ControlLevel = 2;
			_LearningIndex = 6;
			_TargetCakeTower->MaxHeight = 15;
			_AgeForMainBranches = 0;
			_TargetInternodeSize = 8538;
			_ReconMainBranchInternodeLimit = 0;
		}

		//_EnableSpaceColonization = false;
		//_AgeForMainBranches = 0;
		//_ReconMainBranchInternodeLimit = 0;

		auto& seq = _DataCollectionSystem->_ImageCaptureSequences[_LearningIndex];
		_DataCollectionSystem->SetCameraPose(seq.first.CameraPos, seq.first.CameraEulerDegreeRot);
		_TargetTreeParameter = seq.second;
		_TargetMask = ResourceManager::LoadTexture(false, _MaskPath);
		_TargetSkeleton = ResourceManager::LoadTexture(false, _SkeletonPath);
	}
	else {
		auto& seq = _DataCollectionSystem->_ImageCaptureSequences[_ReconIndex];
		_Name = seq.first.Name;
		_StorePath = "./tree_recon/" + _Name + "/seed_" + std::to_string(_ReconSeed);
		_TargetCakeTowerPath = _StorePath + "/target" + _Prefix + ".ct";
		_TargetKdopPath = _StorePath + "/target.csv";
		_MaskPath = _StorePath + "/target.png";
		_SkeletonPath = _StorePath + "/skeleton.png";

		switch (_ReconIndex) {
		case 0:
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 2260;
			_ReconMainBranchInternodeLimit = 200;
			break;
		case 1:
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 1601;
			_ReconMainBranchInternodeLimit = 200;
			break;
		case 2:
			_EnableSpaceColonization = false;
			_AgeForMainBranches = 0;
			_TargetInternodeSize = 2615;
			_ReconMainBranchInternodeLimit = 0;
			break;
		case 3:
			_ControlLevel = 1;
			_AgeForMainBranches = 1;
			_TargetInternodeSize = 7995;
			_ReconMainBranchInternodeLimit = 0;
			break;
		case 4:
			_AgeForMainBranches = 1;
			_TargetInternodeSize = 5120;
			_ReconMainBranchInternodeLimit = 0;
			break;
		case 5:
			_AgeForMainBranches = 5;
			_TargetInternodeSize = 4487;
			_ReconMainBranchInternodeLimit = 200;
			break;
		case 6:
			_EnableSpaceColonization = false;
			_ControlLevel = 0;
			_AgeForMainBranches = 0;
			_TargetInternodeSize = 8538;
			_ReconMainBranchInternodeLimit = 0;
			break;
		}
		_MaxAge = 30;

		if (!_UseMask)
		{
			_AgeForMainBranches = 0;
			_ReconMainBranchInternodeLimit = 0;
		}


		_TargetTreeParameter = seq.second;
		_TargetMask = ResourceManager::LoadTexture(false, _MaskPath);
		_TargetSkeleton = ResourceManager::LoadTexture(false, _SkeletonPath);


		_TargetCakeTower = std::make_unique<RBV>();

		_TargetCakeTower->Load(_TargetCakeTowerPath);
	}

	_NeedExport = true;

}

void TreeReconstructionSystem::ExportCakeTower(const std::string& path)
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
	Enable();
}

void TreeUtilities::TreeReconstructionSystem::Update()
{
	if(_Paused)
	{
		_Paused = false;
		_Status = _PreviousStatus;
	}
	if (_PlantSimulationSystem == nullptr) return;
	std::string path;
	switch (_Status)
	{
	case TreeReconstructionSystemStatus::Reconstruction:
		if (!_DataCollectionSystem->_Reconstruction)
		{
			if (_DataCollectionSystem->_Index < static_cast<int>(_DataCollectionSystem->_ImageCaptureSequences.size()) - 1)
			{
				_DataCollectionSystem->_Index++;
			}
			else
			{
				_DataCollectionSystem->_Index = 0;
				_DataCollectionSystem->_Seed++;
			}
			if (_DataCollectionSystem->_Seed < _GenerateAmount) {
				_DataCollectionSystem->_Reconstruction = true;
				_DataCollectionSystem->_ReconPath = "tree_recon/" + _DataCollectionSystem->_ImageCaptureSequences[_DataCollectionSystem->_Index].first.Name + "/seed_" + std::to_string(_DataCollectionSystem->_Seed) + "/target";
			}
			else
			{
				_Status = TreeReconstructionSystemStatus::Idle;
			}
		}
		break;
	case TreeReconstructionSystemStatus::Idle:

		if (_NeedExport) {
			if (_FromTraining)
			{
				if (_ReconCounter < _ReconAmount - 1)
				{
					_ReconCounter++;
				}
				else if (_ReconIndex < _TrainingAmount - 1)
				{
					_ReconCounter = 0;
					_ReconIndex++;
					Init();
				}
				if (_ReconIndex < _TrainingAmount) {
					_Add = 0;
					auto parameters = _TargetTreeParameter;
					parameters.Seed = (_ReconIndex - _ReconIndex % 28) / 28; //_ReconCounter + 1;
					parameters.Age = 200;
					_PlantSimulationSystem->_ControlLevel = 2;



					_CurrentTree = _PlantSimulationSystem->CreateTree(parameters, glm::vec3(0.0f));
					auto& maskProcessor = _CurrentTree.GetPrivateComponent<MaskProcessor>();
					maskProcessor->_Skeleton = _TargetSkeleton;
					maskProcessor->_Mask = _TargetMask;
					maskProcessor->AttractionDistance = 1.0f;
					maskProcessor->PlaceAttractionPoints();
					_Status = TreeReconstructionSystemStatus::MainBranches;
					_Growing = true;
					if (_ReconCounter == _ReconAmount - 1 && _ReconIndex == _TrainingAmount - 1)
					{
						ExportAllData();
						_NeedExport = false;
					}
				}
			}
			else {
				if (_ReconCounter < _ReconAmount - 1)
				{
					_ReconCounter++;
				}
				else if (_ReconIndex < static_cast<int>(_DataCollectionSystem->_ImageCaptureSequences.size()) - 1)
				{
					ExportAllData();
					_ReconCounter = 0;
					_ReconIndex++;
					Init();
				}
				else
				{
					_ReconCounter = 0;
					_ReconIndex = 0;
					//_ReconIndex = 5;
					_ReconSeed++;
					if (_ReconSeed < _GenerateAmount)Init();
				}
				if (_ReconSeed < _GenerateAmount) {
					_DataCollectionSystem->SetCameraPose(_DataCollectionSystem->_ImageCaptureSequences[_ReconIndex].first.CameraPos, _DataCollectionSystem->_ImageCaptureSequences[_ReconIndex].first.CameraEulerDegreeRot);
					_Status = TreeReconstructionSystemStatus::CreateTree;
				}
				else
				{
					bool mask = false;
					if (_Prefix._Equal("_1_1"))
					{
						_Prefix = "_2_2";
					}
					else if (_Prefix._Equal("_2_2"))
					{
						_Prefix = "_4_4";
					}
					else if (_Prefix._Equal("_4_4"))
					{
						_Prefix = "_8_8";
					}
					else
					{
						mask = true;
					}
					if (mask) {
						if (_UseMask) {
							_Prefix = "_8_8";
							_UseMask = false;
							//_GenerateAmount = 5;
							_FromTraining = false;
							_ReconCounter = -1;
							_ReconIndex = 0;
							_ReconSeed = _ReconStartSeed;
							Init();
							_NeedExport = true;
							_Status = TreeReconstructionSystemStatus::Idle;
						}
						else
						{
							_NeedExport = false;
						}
					}
					else
					{
						_FromTraining = false;
						_ReconCounter = -1;
						_ReconIndex = 0;
						_ReconSeed = 0;
						Init();
					}
				}
			}
		}
		else
		{
			OnGui();
		}
		break;

	case TreeReconstructionSystemStatus::CreateTree:
	{
		auto parameters = _TargetTreeParameter;
		parameters.Seed = _ReconCounter + 999;
		parameters.Age = 200;
		//parameters.FoliageType = 1;
		_PlantSimulationSystem->_ControlLevel = 2;
		_CurrentTree = _PlantSimulationSystem->CreateTree(parameters, glm::vec3(0.0f));
		_CurrentTree.GetPrivateComponent<MaskProcessor>()->_Skeleton = _TargetSkeleton;
		_CurrentTree.GetPrivateComponent<MaskProcessor>()->_Mask = _TargetMask;
		_CurrentTree.GetPrivateComponent<MaskProcessor>()->AttractionDistance = 1.0f;
		_CurrentTree.GetPrivateComponent<MaskProcessor>()->PlaceAttractionPoints();
		_Status = TreeReconstructionSystemStatus::MainBranches;
		_Growing = true;
		Application::SetPlaying(false);
	}
	break;
	case TreeReconstructionSystemStatus::MainBranches:
		TryGrowTree();
		
		if (!_Growing)
		{
			_Status = TreeReconstructionSystemStatus::NormalGrowth;
			if (_UseCakeTower) {
				auto& cakeTower = _CurrentTree.GetPrivateComponent<RBV>();
				cakeTower->SetEnabled(true);
				cakeTower->CakeTiers = _TargetCakeTower->CakeTiers;

				cakeTower->LayerAmount = _TargetCakeTower->LayerAmount;
				cakeTower->SectorAmount = _TargetCakeTower->SectorAmount;
				cakeTower->MaxHeight = _TargetCakeTower->MaxHeight;
				cakeTower->MaxRadius = _TargetCakeTower->MaxRadius;

				cakeTower->GenerateMesh();
				cakeTower->ClearAttractionPoints();
				cakeTower->GenerateAttractionPoints(8000);
				cakeTower->EnableSpaceColonization = _EnableSpaceColonization;
			}
			_PlantSimulationSystem->ResumeGrowth();
			TreeAge age = _CurrentTree.GetComponentData<TreeAge>();
			age.Value = _AgeForMainBranches;
			age.ToGrowIteration = _TargetTreeParameter.Age - _AgeForMainBranches;
			_CurrentTree.SetComponentData(age);
			Disable();
			//break;
			//Application::SetPlaying(false);
		}
		break;
	case TreeReconstructionSystemStatus::NormalGrowth:
	{
		_PlantSimulationSystem->_AutoGenerateMesh = true; //_Status == TreeReconstructionSystemStatus::Reconstruction || _Status == TreeReconstructionSystemStatus::Idle;
			_PlantSimulationSystem->_AutoGenerateLeaves = true;//_Status == TreeReconstructionSystemStatus::Reconstruction || _Status == TreeReconstructionSystemStatus::Idle;
		bool stop = false;
		_Internodes.resize(0);
		const auto treeIndex = _CurrentTree.GetComponentData<TreeIndex>();
		TreeManager::GetInternodeQuery().ToEntityArray(treeIndex, _Internodes);
		TreeAge age = _CurrentTree.GetComponentData<TreeAge>();
		if (_Internodes.size() > _TargetInternodeSize + _MainBranchInternodeSize || age.Value > _MaxAge)
		{
			stop = true;
			Debug::Log("Size");
		}
			
		if (!stop && !_PlantSimulationSystem->_Growing) {
			if (age.ToGrowIteration == 0 && age.Value > _MaxAge)
			{
				stop = true;
				Debug::Log("Age");
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
			}
			_PlantSimulationSystem->PauseGrowth();
			TreeManager::GenerateSimpleMeshForTree(_CurrentTree, PlantSimulationSystem::_MeshGenerationResolution, PlantSimulationSystem::_MeshGenerationSubdivision);
			_PlantSimulationSystem->GenerateLeaves(_CurrentTree);
			_Status = TreeReconstructionSystemStatus::Render;
			//_NeedExport = false;
			auto seq = _DataCollectionSystem->_ImageCaptureSequences[_LearningIndex];
			Transform cameraTransform;
			cameraTransform.SetPosition(glm::vec3(0, seq.first.CameraPos.z * 1.5f, 0));
			cameraTransform.SetEulerRotation(glm::radians(glm::vec3(-90, 0, 0)));
			_DataCollectionSystem->_SemanticMaskCameraEntity.SetComponentData(cameraTransform);
		}

	}
	break;
	case TreeReconstructionSystemStatus::Render:
		SetEnableFoliage(false);
		_Status = TreeReconstructionSystemStatus::RenderBranch;
		break;
	case TreeReconstructionSystemStatus::RenderBranch:
		path = _StorePath + (_FromTraining ? "image_recon/" : "/image/") + (_UseMask ? "mask/" : "no_mask/") +
			(_FromTraining ? std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_" + _Name + "_" : "")
			+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter)
			+ _Prefix + "_branch.jpg";
		_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
			path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);
		SetEnableFoliage(true);
		_Status = TreeReconstructionSystemStatus::CollectData;
		break;
	case TreeReconstructionSystemStatus::CollectData:
	{
		std::string postFix;
		switch (_ReconIndex % 4)
		{
		case 0:
			postFix = "_0";
			break;
		case 1:
			postFix = "_90";
			break;
		case 2:
			postFix = "_180";
			break;
		case 3:
			postFix = "_270";
			break;
		}
		int index = (_ReconIndex - _ReconIndex % 4) / 4;
		path = _StorePath + (_FromTraining ? "image_recon/" : "/image/") + (_FromTraining ? "" : _UseMask ? "mask/" : "no_mask/") +
			(_FromTraining ? std::string(5 - std::to_string(index).length(), '0') + std::to_string(index) + "_" + _Name + postFix + "_" : "")
			+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter)
			+ _Prefix + ".jpg";

		_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
			path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);

		path = _StorePath + (_FromTraining ? "image_recon_top/" : "/image_top/") + (_FromTraining ? "" : _UseMask ? "mask/" : "no_mask/") +
			(_FromTraining ? std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_" + _Name + postFix + "_" : "")
			+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter)
			+ _Prefix + ".jpg";

		_DataCollectionSystem->_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
			path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);

		if (!_FromTraining)_CurrentTree.GetPrivateComponent<RBV>()->CalculateVolume(_TargetCakeTower->MaxHeight);

		_CakeTowersOutputList.emplace_back(_ReconIndex, _Name, _CurrentTree.GetPrivateComponent<RBV>());

		{
			path = _StorePath + (_FromTraining ? "volume_recon/" : "/volume/") + (_FromTraining ? "" : _UseMask ? "mask/" : "no_mask/") +
				(_FromTraining ? std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_" + _Name + postFix + "_" : "")
				+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter)
				+ _Prefix + ".ct";
			const std::string data = _CurrentTree.GetPrivateComponent<RBV>()->Save();
			std::ofstream ofs;
			ofs.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
			ofs.write(data.c_str(), data.length());
			ofs.flush();
			ofs.close();
		}
		
		TreeManager::ExportTreeAsModel(_CurrentTree, _StorePath + (_FromTraining ? "obj_recon/" : "/obj/") + (_FromTraining ? "" : _UseMask ? "mask/" : "no_mask/") +
			(_FromTraining ? std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_" + _Name + postFix + "_" : "")
			+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter) + _Prefix
			, true);

		TreeManager::SerializeTreeGraph(_StorePath + (_FromTraining ? "graph_recon/" : "/graph/") + (_FromTraining ? "" : _UseMask ? "mask/" : "no_mask/") +
			(_FromTraining ? std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_" + _Name + postFix + "_" : "")
			+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter) + _Prefix
			, _CurrentTree);

		

		SetEnableFoliage(false);

		auto seq = _DataCollectionSystem->_ImageCaptureSequences[_FromTraining ? _LearningIndex : _ReconIndex];
		Transform cameraTransform;

		cameraTransform.SetPosition(glm::vec3(seq.first.CameraPos.x, _CurrentTree.GetPrivateComponent<RBV>()->MaxHeight / 2.0f, seq.first.CameraPos.z * 1.5f));
		cameraTransform.SetEulerRotation(glm::radians(glm::vec3(0, 0, 0)));
		_DataCollectionSystem->_ImageCameraEntity.SetComponentData(cameraTransform);

		_CurrentTree.GetPrivateComponent<RBV>()->FormEntity();
		_CurrentTree.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);

		_Status = TreeReconstructionSystemStatus::CaptureCakeTower;

	}
	break;
	case TreeReconstructionSystemStatus::CaptureCakeTower:
		_Status = TreeReconstructionSystemStatus::CleanUp;
		break;
	case TreeReconstructionSystemStatus::CleanUp:
		if (!_FromTraining)
		{
			path = _StorePath + "/tower/" + (_UseMask ? "mask/" : "no_mask/")
				+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter)
				+ _Prefix + ".jpg";

			_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);

			path = _StorePath + "/tower_top/" + (_UseMask ? "mask/" : "no_mask/")
				+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter)
				+ _Prefix + ".jpg";

			_DataCollectionSystem->_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);
		}
		else if (_FromTraining)
		{

			std::string postFix;
			switch (_ReconIndex % 4)
			{
			case 0:
				postFix = "_0";
				break;
			case 1:
				postFix = "_90";
				break;
			case 2:
				postFix = "_180";
				break;
			case 3:
				postFix = "_270";
				break;
			}
			int index = (_ReconIndex - _ReconIndex % 4) / 4;
			path = _StorePath + "rbv_val/" +
				(_FromTraining ? std::string(5 - std::to_string(index).length(), '0') + std::to_string(index) + "_" + _Name + "_" : "")
				+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter)
				+ _Prefix + postFix + ".jpg";

			_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);
			/*
			path = _StorePath + "rbv_val/" +
				(_FromTraining ? std::string(5 - std::to_string(index).length(), '0') + std::to_string(index) + "_" + _Name + "_" : "")
				+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter)
				+ _Prefix + "_top.jpg";

			_DataCollectionSystem->_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);
			*/
			auto& seq = _DataCollectionSystem->_ImageCaptureSequences[_LearningIndex];
			_DataCollectionSystem->SetCameraPose(seq.first.CameraPos, seq.first.CameraEulerDegreeRot);

		}
		//_NeedExport = false;
		TreeManager::DeleteAllTrees();
		//Application::SetPlaying(false);
		_Status = TreeReconstructionSystemStatus::Idle;
		break;
	}
}

void TreeReconstructionSystem::LateUpdate()
{
	return;
	if(_Status == TreeReconstructionSystemStatus::NormalGrowth || _Status == TreeReconstructionSystemStatus::MainBranches || _Status == TreeReconstructionSystemStatus::Render)
	{
		TreeManager::GenerateSimpleMeshForTree(_CurrentTree, PlantSimulationSystem::_MeshGenerationResolution, PlantSimulationSystem::_MeshGenerationSubdivision);
		_PlantSimulationSystem->GenerateLeaves(_CurrentTree);
		EditorManager::GetInstance().m_sceneCamera->StoreToJpg(
			"./video_seq/" + std::to_string(_Capture) + ".jpg", -1, -1);
		_Capture++;
	}
}
