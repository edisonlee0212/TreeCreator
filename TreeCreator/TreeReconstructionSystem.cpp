#include "TreeReconstructionSystem.h"
#include "Ray.h"
#include "CakeTower.h"
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
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/volume");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/obj");
							std::filesystem::create_directory("tree_recon/" + seq.first.Name + "/seed_" + std::to_string(i) + "/graph");
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

					_TrainingDoc = rapidcsv::Document("./tree_data/predicted_rbv_species_8_8.csv", rapidcsv::LabelParams(-1, 1));
					std::vector<std::string> firstColumn = _TrainingDoc.GetColumn<std::string>(0);
					Debug::Log("Count: " + std::to_string(firstColumn.size()));
					_TrainingAmount = 1400;// firstColumn.size();
					Init();
					_NeedExport = true;
					_Status = TreeReconstructionSystemStatus::Idle;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			if (ImGui::BeginPopupModal("Reconstruction wizard", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::DragInt("Reconstruction Amount##Reconstruction", &_ReconAmount, 1, 1, 50);
				ImGui::Checkbox("Use volume for main branches", &_UseCakeTower);
				if (ImGui::Button("Start Reconstruction"))
				{
					_FromTraining = false;
					_ReconCounter = -1;
					_ReconIndex = 0;
					_ReconSeed = 0;
					Init();
					_NeedExport = true;
					_Status = TreeReconstructionSystemStatus::Idle;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
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
		branchletRenderer->SetEnabled(enabled);
	}
	if (foliageEntity.HasPrivateComponent<Particles>())
	{
		auto& leavesRenderer = foliageEntity.GetPrivateComponent<Particles>();
		leavesRenderer->SetEnabled(enabled);
	}
}

void TreeReconstructionSystem::PushInternode(Entity internode, const GlobalTransform& cameraTransform, const GlobalTransform& treeLTW)
{
	auto info = internode.GetComponentData<InternodeInfo>();
	if (info.DistanceToRoot > 9.0)
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
				if (_UseCakeTower) {
					auto slice = _TargetCakeTower->SelectSlice(test);
					//slice.x += 1;
					if (slice.x >= _TargetCakeTower->LayerAmount) slice.x = _TargetCakeTower->LayerAmount - 1;
					glm::vec3 weight = glm::vec3(0);
					const float sliceAngle = 360.0f / _TargetCakeTower->SectorAmount;
					for (int i = -1; i <= 1; i++)
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
						glm::vec3 position = glm::normalize(glm::vec3(x, 0.0f, z)) * glm::pow(_TargetCakeTower->CakeTiers[slice.x][y].MaxDistance, 3.0f);
						weight += position;
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
			globalTransform.Value = treeLTW.Value * internodeInfo.GlobalTransform * glm::scale(glm::vec3(_TargetTreeParameter.InternodeSize));
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
	if (_FromTraining)
	{
		_TargetCakeTower = std::make_unique<CakeTower>();
		std::vector<float> rbv = _TrainingDoc.GetRow<float>(_ReconIndex);
		_Name = _TrainingDoc.GetRowName(_ReconIndex);
		_StorePath = "./tree_data/";
		_MaskPath = _StorePath + "mask_val/" + std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_" + _Name + ".png";
		_SkeletonPath = _StorePath + "skeleton_recon/" + std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_" + _Name + ".png";;

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
		//_TargetKDop = std::make_unique<KDop>();
		int index = 0;
		if (_Name._Equal("acacia")) {
			index = 0;
			_TargetCakeTower->MaxHeight = 15;
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 2260;
			_ReconMainBranchInternodeLimit = 300;
		}
		else if (_Name._Equal("apple")) {
			index = 1;
			_TargetCakeTower->MaxHeight = 12.5;
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 1601;
			_ReconMainBranchInternodeLimit = 3000;
		}
		else if (_Name._Equal("willow")) {
			index = 2;
			_TargetCakeTower->MaxHeight = 19;
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 2615;
			_ReconMainBranchInternodeLimit = 300;
		}
		else if (_Name._Equal("maple")) {
			index = 3;
			_TargetCakeTower->MaxHeight = 32;
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 7995;
			_ReconMainBranchInternodeLimit = 0;
		}
		else if (_Name._Equal("birch")) {
			index = 4;
			_TargetCakeTower->MaxHeight = 31;
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 6120;// 5120;
			_ReconMainBranchInternodeLimit = 0;
		}
		else if (_Name._Equal("oak")) {
			index = 5;
			_TargetCakeTower->MaxHeight = 18.5;
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 4487;
			_ReconMainBranchInternodeLimit = 300;
		}
		else if (_Name._Equal("pine")) {
			index = 6;
			_TargetCakeTower->MaxHeight = 15;
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 8538;
			_ReconMainBranchInternodeLimit = 0;
		}
		auto& seq = _DataCollectionSystem->_ImageCaptureSequences[index];
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
			_ReconMainBranchInternodeLimit = 1000;
			break;
		case 1:
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 1601;
			_ReconMainBranchInternodeLimit = 1000;
			break;
		case 2:
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 2615;
			_ReconMainBranchInternodeLimit = 0;
			break;
		case 3:
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 7995;
			_ReconMainBranchInternodeLimit = 0;
			break;
		case 4:
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 5120;
			_ReconMainBranchInternodeLimit = 0;
			break;
		case 5:
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 4487;
			_ReconMainBranchInternodeLimit = 1000;
			break;
		case 6:
			_AgeForMainBranches = 4;
			_TargetInternodeSize = 8538;
			_ReconMainBranchInternodeLimit = 0;
			break;
		}
		_MaxAge = 30;



		_DataCollectionSystem->SetCameraPose(seq.first.CameraPos, seq.first.CameraEulerDegreeRot);

		_TargetTreeParameter = seq.second;
		_TargetMask = ResourceManager::LoadTexture(false, _MaskPath);
		_TargetSkeleton = ResourceManager::LoadTexture(false, _SkeletonPath);


		_TargetCakeTower = std::make_unique<CakeTower>();

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
					parameters.Seed = _ReconCounter + 1;
					parameters.Age = 200;
					_PlantSimulationSystem->_ControlLevel = 2;
					_CurrentTree = _PlantSimulationSystem->CreateTree(parameters, glm::vec3(0.0f));

					_CurrentTree.GetPrivateComponent<MaskProcessor>()->_Skeleton = _TargetSkeleton;
					_CurrentTree.GetPrivateComponent<MaskProcessor>()->_Mask = _TargetMask;
					_CurrentTree.GetPrivateComponent<MaskProcessor>()->AttractionDistance = 1.0f;
					_CurrentTree.GetPrivateComponent<MaskProcessor>()->PlaceAttractionPoints();
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
					_ReconSeed++;
					Init();
				}
				if (_ReconSeed < _GenerateAmount) {
					auto parameters = _TargetTreeParameter;
					parameters.Seed = _ReconCounter;
					parameters.Age = 200;
					_PlantSimulationSystem->_ControlLevel = 2;
					_CurrentTree = _PlantSimulationSystem->CreateTree(parameters, glm::vec3(0.0f));

					_CurrentTree.GetPrivateComponent<MaskProcessor>()->_Skeleton = _TargetSkeleton;
					_CurrentTree.GetPrivateComponent<MaskProcessor>()->_Mask = _TargetMask;
					_CurrentTree.GetPrivateComponent<MaskProcessor>()->AttractionDistance = 1.0f;
					_CurrentTree.GetPrivateComponent<MaskProcessor>()->PlaceAttractionPoints();
					_Status = TreeReconstructionSystemStatus::MainBranches;
					_Growing = true;
				}
				else
				{
					bool quit = false;
					if(_Prefix._Equal("_2_2"))
					{
						_Prefix = "_4_4";
					}else if (_Prefix._Equal("_4_4"))
					{
						_Prefix = "_6_6";
					}
					else if (_Prefix._Equal("_6_6"))
					{
						_Prefix = "_8_8";
					}
					else if (_Prefix._Equal("_8_8"))
					{
						_Prefix = "_10_10";
					}
					else if (_Prefix._Equal("_10_10"))
					{
						_Prefix = "_12_12";
					}else
					{
						quit = true;
					}
					if(quit) _NeedExport = false;
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
	case TreeReconstructionSystemStatus::MainBranches:
		TryGrowTree();
		if (!_Growing)
		{
			_Status = TreeReconstructionSystemStatus::NormalGrowth;
			if (_UseCakeTower) {
				auto& cakeTower = _CurrentTree.GetPrivateComponent<CakeTower>();
				cakeTower->SetEnabled(true);
				cakeTower->CakeTiers = _TargetCakeTower->CakeTiers;

				cakeTower->LayerAmount = _TargetCakeTower->LayerAmount;
				cakeTower->SectorAmount = _TargetCakeTower->SectorAmount;
				cakeTower->MaxHeight = _TargetCakeTower->MaxHeight;
				cakeTower->MaxRadius = _TargetCakeTower->MaxRadius;

				cakeTower->GenerateMesh();
				cakeTower->ClearAttractionPoints();
				cakeTower->GenerateAttractionPoints(2000);
				cakeTower->EnableSpaceColonization = true;
			}
			_PlantSimulationSystem->ResumeGrowth();
			TreeAge age = _CurrentTree.GetComponentData<TreeAge>();
			age.Value = _AgeForMainBranches;
			age.ToGrowIteration = _TargetTreeParameter.Age - _AgeForMainBranches;
			_CurrentTree.SetComponentData(age);
		}
		break;
	case TreeReconstructionSystemStatus::NormalGrowth:
	{
		_PlantSimulationSystem->_AutoGenerateMesh = _Status == TreeReconstructionSystemStatus::Reconstruction || _Status == TreeReconstructionSystemStatus::Idle;
		_PlantSimulationSystem->_AutoGenerateLeaves = _Status == TreeReconstructionSystemStatus::Reconstruction || _Status == TreeReconstructionSystemStatus::Idle;
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
		}

	}
	break;
	case TreeReconstructionSystemStatus::Render:
		SetEnableFoliage(false);
		_Status = TreeReconstructionSystemStatus::RenderBranch;
		break;
	case TreeReconstructionSystemStatus::RenderBranch:
		path = _StorePath + (_FromTraining ? "image_recon/" : "/image/") +
			(_FromTraining ? std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_" + _Name + "_" : "")
			+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter)
			+ _Prefix + "_branch.jpg";
		_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
			path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);
		SetEnableFoliage(true);
		_Status = TreeReconstructionSystemStatus::CollectData;
		break;
	case TreeReconstructionSystemStatus::CollectData:
		path = _StorePath + (_FromTraining ? "image_recon/" : "/image/") +
			(_FromTraining ? std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_" + _Name + "_" : "")
			+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter)
			+ _Prefix + ".jpg";
		_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
			path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);
		_CurrentTree.GetPrivateComponent<CakeTower>()->CalculateVolume(_TargetCakeTower->MaxHeight);
		_CakeTowersOutputList.emplace_back(_ReconIndex, _Name, _CurrentTree.GetPrivateComponent<CakeTower>());

		{
			path = _StorePath + (_FromTraining ? "volume_recon/" : "/volume/") +
				(_FromTraining ? std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_" + _Name + "_" : "")
				+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter)
				+ _Prefix + ".ct";
			const std::string data = _CurrentTree.GetPrivateComponent<CakeTower>()->Save();
			std::ofstream ofs;
			ofs.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
			ofs.write(data.c_str(), data.length());
			ofs.flush();
			ofs.close();
		}

		TreeManager::ExportTreeAsModel(_CurrentTree, _StorePath + (_FromTraining ? "obj_recon/" : "/obj/") +
			(_FromTraining ? std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_" + _Name + "_" : "")
			+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter) + _Prefix
			, true);

		TreeManager::SerializeTreeGraph(_StorePath + (_FromTraining ? "graph_recon/" : "/graph/") +
			(_FromTraining ? std::string(5 - std::to_string(_ReconIndex).length(), '0') + std::to_string(_ReconIndex) + "_" + _Name + "_" : "")
			+ std::string(5 - std::to_string(_ReconCounter).length(), '0') + std::to_string(_ReconCounter) + _Prefix
			, _CurrentTree);

		_Status = TreeReconstructionSystemStatus::CleanUp;
		break;
	case TreeReconstructionSystemStatus::CleanUp:
		TreeManager::DeleteAllTrees();
		_Status = TreeReconstructionSystemStatus::Idle;
		break;
	}
}
