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

void RealTreeReconstructionSystem::ToNormalGrowth()
{
	auto& cakeTower = _CurrentTree.GetPrivateComponent<RBV>();
	cakeTower->SetEnabled(true);
	cakeTower->CakeTiers = _TargetCakeTower->CakeTiers;
	cakeTower->SetPruneBuds(_TargetCakeTower->PruneBuds());
	cakeTower->LayerAmount = _TargetCakeTower->LayerAmount;
	cakeTower->SectorAmount = _TargetCakeTower->SectorAmount;
	cakeTower->MaxHeight = _TargetCakeTower->MaxHeight;
	cakeTower->MaxRadius = _TargetCakeTower->MaxRadius;
	
	cakeTower->GenerateMesh();
	cakeTower->ClearAttractionPoints();
	cakeTower->GenerateAttractionPoints(12000);
	cakeTower->EnableSpaceColonization = _EnableSpaceColonization;
	_PlantSimulationSystem->ResumeGrowth();
	TreeAge age = _CurrentTree.GetComponentData<TreeAge>();
	age.Value = _AgeForMainBranches;
	age.ToGrowIteration = _TargetTreeParameter.Age - _AgeForMainBranches;
	_CurrentTree.SetComponentData(age);
	_Status = RealTreeReconstructionSystemStatus::NormalGrowth;
}

void TreeUtilities::RealTreeReconstructionSystem::OnGui()
{
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Real Tree Reconstruction")) {
			if(ImGui::Button("RBV rendering"))
			{
				_OnlyRenderRBV = true;
				_NeedExport = true;
				_ReconIndex = -1;
				_ReconAmount = 1;
				_Status = RealTreeReconstructionSystemStatus::Idle;
				_StartTime = Application::EngineTime();
				_TrainingDoc = rapidcsv::Document("./tree_real/005_rbv_predicted_fig7.csv", rapidcsv::LabelParams(-1, 1));
				std::filesystem::remove_all("tree_real/rbv_recon");
				std::filesystem::create_directory("tree_real/rbv_recon");
			}
			
			ImGui::DragInt("Start Index", &_GeneralIndex, 1, 0, 2800);
			if (ImGui::Button("Reconstruct"))
			{
				if (_GeneralIndex == 0) {
					std::filesystem::remove_all("tree_real/image_recon");
					std::filesystem::remove_all("tree_real/rbv_recon");
					std::filesystem::remove_all("tree_real/graph_recon");
					std::filesystem::remove_all("tree_real/obj_recon");

					std::filesystem::create_directory("tree_real/image_recon");
					std::filesystem::create_directory("tree_real/rbv_recon");
					std::filesystem::create_directory("tree_real/graph_recon");
					std::filesystem::create_directory("tree_real/obj_recon");
				}
				_TrainingDoc = rapidcsv::Document("./tree_real/rbv_species_8_8_val.csv", rapidcsv::LabelParams(-1, 1));
				//_TrainingDoc = rapidcsv::Document("./tree_real/074_rbv_predicted_val_segmentation.csv", rapidcsv::LabelParams(-1, 1));
				_StartTime = Application::EngineTime();
				_ReconIndex = -1;//0 to _ReconAmount
				_ReconAmount = 3; //How many recon per instance
				_NeedExport = true;
				_OnlyRenderRBV = false;
				_Status = RealTreeReconstructionSystemStatus::Idle;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void TreeUtilities::RealTreeReconstructionSystem::Update()
{
	if (_DataCollectionSystem == nullptr) return;
	switch (_Status)
	{
	case RealTreeReconstructionSystemStatus::Idle:
		if (_NeedExport)
		{
			if (_GeneralIndex < _GeneralTotal) {
				if (_ReconIndex < _ReconAmount - 1)
				{
					_ReconIndex++;
				}
				else
				{
					_GeneralIndex++;
					_ReconIndex = 0;
					_Add = 0;
					if (_GeneralIndex == _GeneralTotal)
					{
						_NeedExport = false;
						Debug::Log("Reconstruction finished in " + std::to_string(Application::EngineTime() - _StartTime) + " s.");
						break;
					}
				}
				_Status = RealTreeReconstructionSystemStatus::Test;
				CreateTree();
			}
		}
		else OnGui();
		break;
	case RealTreeReconstructionSystemStatus::Test:
		if(_OnlyRenderRBV)
		{
			auto& cakeTower = _CurrentTree.GetPrivateComponent<RBV>();
			cakeTower->SetEnabled(true);
			cakeTower->CakeTiers = _TargetCakeTower->CakeTiers;
			cakeTower->SetPruneBuds(_TargetCakeTower->PruneBuds());
			cakeTower->LayerAmount = _TargetCakeTower->LayerAmount;
			cakeTower->SectorAmount = _TargetCakeTower->SectorAmount;
			cakeTower->MaxHeight = _TargetCakeTower->MaxHeight;
			cakeTower->MaxRadius = _TargetCakeTower->MaxRadius;

			cakeTower->GenerateMesh();
			_Status = RealTreeReconstructionSystemStatus::CollectData;
		}
		else _Status = RealTreeReconstructionSystemStatus::CreateTree;
		break;
	case RealTreeReconstructionSystemStatus::CreateTree:
		if (_UseMask) {
			_Growing = true;
			_Status = RealTreeReconstructionSystemStatus::MainBranches;
		}
		else {
			ToNormalGrowth();
		}
		break;
	case RealTreeReconstructionSystemStatus::MainBranches:
		TryGrowTree();
		if (!_Growing)
		{
			ToNormalGrowth();
		}
		break;
	case RealTreeReconstructionSystemStatus::NormalGrowth:
	{
		_PlantSimulationSystem->_AutoGenerateMesh = false;
		_PlantSimulationSystem->_AutoGenerateLeaves = false;
		bool stop = false;
		_Internodes.resize(0);
		const auto treeIndex = _CurrentTree.GetComponentData<TreeIndex>();
		TreeManager::GetInternodeQuery().ToEntityArray(treeIndex, _Internodes);
		TreeAge age = _CurrentTree.GetComponentData<TreeAge>();
		if (_UseMask) _MainBranchInternodeSize = 0;
		if (_Internodes.size() > _TargetInternodeSize + _MainBranchInternodeSize)
		{
			stop = true;
			Debug::Log("Size reached max");
		}
		if (age.Value > _MaxAge)
		{
			stop = true;
			Debug::Log("Age reached max");
		}
		if (!stop && !_PlantSimulationSystem->_Growing) {
			if (age.ToGrowIteration == 0 && age.Value > _MaxAge)
			{
				stop = true;
				Debug::Log("Age reached max and still growing");
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

					parameters.Seed = _SpecieIndex + _ReconAmount + 1 + _Add;
					parameters.Age = 200;
					_PlantSimulationSystem->_ControlLevel = 2;
					_CurrentTree = _PlantSimulationSystem->CreateTree(parameters, glm::vec3(0.0f));

					_CurrentTree.GetPrivateComponent<MaskProcessor>()->_Skeleton = _TargetSkeleton;
					//_CurrentTree.GetPrivateComponent<MaskProcessor>()->_Mask = _TargetMask;
					_CurrentTree.GetPrivateComponent<MaskProcessor>()->AttractionDistance = 1.0f;
					_CurrentTree.GetPrivateComponent<MaskProcessor>()->PlaceAttractionPoints();
					_Status = RealTreeReconstructionSystemStatus::MainBranches;
					_Growing = true;
					break;
				}
			}
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
	if(!_OnlyRenderRBV){
		std::string path = _StorePath + "image_recon/" +
			std::string(5 - std::to_string(_GeneralIndex).length(), '0') + std::to_string(_GeneralIndex) +
			"_" + _Name + "_" + std::to_string(_ReconIndex) + ".png";
		_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
			path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);
		path = _StorePath + "graph_recon/" +
			std::string(5 - std::to_string(_GeneralIndex).length(), '0') + std::to_string(_GeneralIndex) +
			"_" + _Name + "_" + std::to_string(_ReconIndex);
		TreeManager::SerializeTreeGraph(path, _CurrentTree);

		path = _StorePath + "obj_recon/" +
			std::string(5 - std::to_string(_GeneralIndex).length(), '0') + std::to_string(_GeneralIndex) +
			"_" + _Name + "_" + std::to_string(_ReconIndex);
		TreeManager::ExportTreeAsModel(_CurrentTree, path, true);

		_TargetCakeTower->ExportAsObj(path);
	}else
	{
		std::string path = _StorePath + "obj_recon/" +
			std::string(5 - std::to_string(_GeneralIndex).length(), '0') + std::to_string(_GeneralIndex) +
			"_" + _Name + "_" + std::to_string(_ReconIndex);
		_TargetCakeTower->GenerateMesh();
		_TargetCakeTower->ExportAsObj(path);
	}
	if (true) {
		
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
		_DataCollectionSystem->SetCameraPose(seq.first.CameraPos, seq.first.CameraEulerDegreeRot, false);
		_CurrentTree.GetPrivateComponent<RBV>()->FormEntity();
		_CurrentTree.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
		_Status = RealTreeReconstructionSystemStatus::CaptureCakeTower;
#pragma endregion
	}
	else
	{
		_Status = RealTreeReconstructionSystemStatus::CleanUp;
	}
	break;
	case RealTreeReconstructionSystemStatus::CaptureCakeTower:
		_Status = RealTreeReconstructionSystemStatus::CleanUp;
		break;
	case RealTreeReconstructionSystemStatus::CleanUp:
		if (true) {
			std::string path = _StorePath + "rbv_recon/" +
				std::string(5 - std::to_string(_GeneralIndex).length(), '0') + std::to_string(_GeneralIndex) +
				+"_" + _Name + "_" + std::to_string(_ReconIndex) + ".png";
			_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				path, _DataCollectionSystem->_TargetResolution, _DataCollectionSystem->_TargetResolution);
		}
		TreeManager::DeleteAllTrees();
		_Status = RealTreeReconstructionSystemStatus::Idle;
		break;
	}
}

void RealTreeReconstructionSystem::CreateTree()
{
	_EnableSpaceColonization = true;


	_TargetCakeTower = std::make_unique<RBV>();
	std::vector<float> rbv = _TrainingDoc.GetRow<float>(_GeneralIndex);

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
	_UseMask = true;
	_SpecieIndex = _GeneralIndex % static_cast<int>(_DataCollectionSystem->_ImageCaptureSequences.size());
	_StorePath = "./tree_real/";
	_TargetCakeTower->SetPruneBuds(true);
	
	auto& seq = _DataCollectionSystem->_ImageCaptureSequences[_SpecieIndex];
	_DataCollectionSystem->SetCameraPose(seq.first.CameraPos, seq.first.CameraEulerDegreeRot);
	_TargetTreeParameter = seq.second;

	
	switch (_SpecieIndex) {
	case 0:
		_Name = "acacia";
		_AgeForMainBranches = 3;
		_TargetCakeTower->MaxHeight = 15;
		_TargetInternodeSize = 2360;
		_ReconMainBranchInternodeLimit = 100;

		_TargetTreeParameter.EndNodeThickness = 0.007f;
		break;
	case 1:
		_Name = "apple";
		_TargetCakeTower->SetPruneBuds(false);
		_AgeForMainBranches = 4;
		_TargetCakeTower->MaxHeight = 12.5;
		_TargetInternodeSize = 1601;
		_ReconMainBranchInternodeLimit = 50;

		_TargetTreeParameter.EndNodeThickness = 0.012f;
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
		_UseMask = false;
		_EnableSpaceColonization = false;
		_TargetCakeTower->MaxHeight = 31;
		_TargetInternodeSize = 6000;
		_ReconMainBranchInternodeLimit = 0;
		
		_TargetTreeParameter.InternodeLengthBase = glm::gaussRand(0.92f, 0.1f);
		_TargetTreeParameter.EndNodeThickness = glm::gaussRand(0.009f, 0.001f);
		break;
	case 4:
		_Name = "birch";
		_UseMask = false;
		_EnableSpaceColonization = false;
		_TargetCakeTower->MaxHeight = 31;
		_TargetInternodeSize = 5120;
		_ReconMainBranchInternodeLimit = 0;

		_TargetTreeParameter.InternodeLengthBase = glm::gaussRand(0.92f, 0.1f);
		_TargetTreeParameter.EndNodeThickness = glm::gaussRand(0.009f, 0.001f);
		break;
	case 5:
		_Name = "oak";
		_TargetCakeTower->SetPruneBuds(false);
		_TargetCakeTower->MaxHeight = 18;
		_AgeForMainBranches = 5;
		_TargetInternodeSize = 4600;
		_ReconMainBranchInternodeLimit = 25;
		break;
	case 6:
		_UseMask = false;
		_Name = "pine";
		_TargetCakeTower->MaxHeight = 15;
		_EnableSpaceColonization = false;
		_AgeForMainBranches = 0;
		_TargetInternodeSize = 8538;
		_ReconMainBranchInternodeLimit = 0;

		_TargetTreeParameter.InternodeLengthBase = glm::gaussRand(0.4f, 0.07f);
		break;
	}
	if (!_UseMask)
	{
		_AgeForMainBranches = 0;
		_ReconMainBranchInternodeLimit = 0;
	}
	else {
		_TargetTreeParameter.Age = 200;
	}
	if(_OnlyRenderRBV)
	{
		_SpecieIndex = 4;
		_TargetCakeTower->MaxHeight = 15;
	}
	_TargetTreeParameter.Seed = _SpecieIndex + _ReconIndex;
	if (_ReconIndex == 0) {
		_MaskPath = _StorePath + "mask/" + std::string(5 - std::to_string(_GeneralIndex).length(), '0') + std::to_string(_GeneralIndex) + "_" + _Name + "_0.png";
		_SkeletonPath = _StorePath + "skeleton/" + std::string(5 - std::to_string(_GeneralIndex).length(), '0') + std::to_string(_GeneralIndex) + "_" + _Name + "_0.png";
		//_TargetMask = ResourceManager::LoadTexture(false, _MaskPath);
		_TargetSkeleton = ResourceManager::LoadTexture(false, _SkeletonPath);
	}
	_CurrentTree = _PlantSimulationSystem->CreateTree(_TargetTreeParameter, glm::vec3(0.0f));
	auto& maskProcessor = _CurrentTree.GetPrivateComponent<MaskProcessor>();
	maskProcessor->_Skeleton = _TargetSkeleton;
	maskProcessor->_Mask = _TargetMask;
	maskProcessor->AttractionDistance = 0.1f;
	maskProcessor->RemovalDistance = 0.05f;
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
	if (age.Value > 200) _Growing = false;
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
	if (info.DistanceToRoot > 6.0)
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
			globalTransform.m_value = treeLTW.m_value * internodeInfo.GlobalTransform * glm::scale(glm::vec3(0.3f));
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

