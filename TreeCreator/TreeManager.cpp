#include "TreeManager.h"
#include "LightEstimator.h"
#include <gtx/matrix_decompose.hpp>
#include <utility>
using namespace TreeUtilities;

LightEstimator* TreeUtilities::TreeManager::_LightEstimator;

TreeSystem* TreeUtilities::TreeManager::_TreeSystem;
InternodeSystem* TreeUtilities::TreeManager::_InternodeSystem;

EntityArchetype TreeUtilities::TreeManager::_InternodeArchetype;
EntityArchetype TreeUtilities::TreeManager::_TreeArchetype;

EntityQuery TreeUtilities::TreeManager::_TreeQuery;
EntityQuery TreeUtilities::TreeManager::_InternodeQuery;

TreeIndex TreeUtilities::TreeManager::_TreeIndex;
InternodeIndex TreeUtilities::TreeManager::_InternodeIndex;

bool TreeUtilities::TreeManager::_Ready;
#pragma region Helpers
std::size_t InternodeData::GetHashCode()
{
	return (size_t)this;
}

void InternodeData::OnGui()
{
	ImGui::Text(("Leaf count: " + std::to_string(LeafLocalTransforms.size())).c_str());
}

std::size_t TreeData::GetHashCode()
{
	return (size_t)this;
}

void TreeData::OnGui()
{
	
}

void TreeUtilities::TreeManager::SimpleMeshGenerator(Entity& internode, std::vector<Vertex>& vertices, std::vector<unsigned>& indices, glm::vec3 normal, float resolution, int parentStep)
{
	InternodeInfo info = EntityManager::GetComponentData<InternodeInfo>(internode);
	glm::vec3 newNormalDir = normal;
	//glm::vec3 dir = info.DesiredGlobalRotation * glm::vec3(0.0f, 0.0f, -1.0f);
	//newNormalDir = glm::cross(glm::cross(dir, newNormalDir), dir);
	
	auto list = EntityManager::GetSharedComponent<InternodeData>(internode);
	auto step = list->step;
	//For stitching
	int pStep = parentStep > 0 ? parentStep : step;

	list->NormalDir = newNormalDir;
	float angleStep = 360.0f / (float)(pStep);
	int vertexIndex = vertices.size();
	Vertex archetype;
	float textureXstep = 1.0f / pStep * 4.0f;
	for (int i = 0; i < pStep; i++) {
		archetype.Position = list->Rings.at(0).GetPoint(newNormalDir, angleStep * i, true);
		float x = i < (pStep / 2) ? i * textureXstep : (pStep - i) * textureXstep;
		archetype.TexCoords0 = glm::vec2(x, 0.0f);
		vertices.push_back(archetype);
	}
	//TODO: stitch here.
	std::vector<float> angles;
	angles.resize(step);
	std::vector<float> pAngles;
	pAngles.resize(pStep);

	for (int i = 0; i < pStep; i++) {
		pAngles[i] = angleStep * i;
	}
	angleStep = 360.0f / (float)(step);
	for (int i = 0; i < step; i++) {
		angles[i] = angleStep * i;
	}

	std::vector<unsigned> pTarget;
	std::vector<unsigned> target;
	pTarget.resize(pStep);
	target.resize(step);
	for (int i = 0; i < pStep; i++) {
		//First we allocate nearest vertices for parent.
		float minAngleDiff = 360.0f;
		for (int j = 0; j < step; j++) {
			float diff = glm::abs(pAngles[i] - angles[j]);
			if (diff < minAngleDiff) {
				minAngleDiff = diff;
				pTarget[i] = j;
			}
		}
	}
	for (int i = 0; i < step; i++) {
		//Second we allocate nearest vertices for child
		float minAngleDiff = 360.0f;
		for (int j = 0; j < pStep; j++) {
			float diff = glm::abs(angles[i] - pAngles[j]);
			if (diff < minAngleDiff) {
				minAngleDiff = diff;
				target[i] = j;
			}
		}
	}
	for (int i = 0; i < pStep; i++) {
		if (pTarget[i] == pTarget[i == pStep - 1 ? 0 : i + 1]) {
			indices.push_back(vertexIndex + i);
			indices.push_back(vertexIndex + (i == pStep - 1 ? 0 : i + 1));
			indices.push_back(vertexIndex + pStep + pTarget[i]);
		}
		else {
			indices.push_back(vertexIndex + i);
			indices.push_back(vertexIndex + (i == pStep - 1 ? 0 : i + 1));
			indices.push_back(vertexIndex + pStep + pTarget[i]);

			indices.push_back(vertexIndex + pStep + pTarget[i == pStep - 1 ? 0 : i + 1]);
			indices.push_back(vertexIndex + pStep + pTarget[i]);
			indices.push_back(vertexIndex + (i == pStep - 1 ? 0 : i + 1));
		}
	}

	vertexIndex += pStep;
	textureXstep = 1.0f / step * 4.0f;
	int ringSize = list->Rings.size();
	for (int ringIndex = 0; ringIndex < ringSize; ringIndex++) {
		for (int i = 0; i < step; i++) {
			archetype.Position = list->Rings.at(ringIndex).GetPoint(newNormalDir, angleStep * i, false);
			float x = i < (step / 2) ? i * textureXstep : (step - i) * textureXstep;
			float y = ringIndex % 2 == 0 ? 1.0f : 0.0f;
			archetype.TexCoords0 = glm::vec2(x, y);
			vertices.push_back(archetype);
		}
		if (ringIndex != 0) {
			for (int i = 0; i < step - 1; i++) {
				//Down triangle
				indices.push_back(vertexIndex + (ringIndex - 1) * step + i);
				indices.push_back(vertexIndex + (ringIndex - 1) * step + i + 1);
				indices.push_back(vertexIndex + (ringIndex) * step + i);
				//Up triangle
				indices.push_back(vertexIndex + (ringIndex) * step + i + 1);
				indices.push_back(vertexIndex + (ringIndex) * step + i);
				indices.push_back(vertexIndex + (ringIndex - 1) * step + i + 1);
			}
			//Down triangle
			indices.push_back(vertexIndex + (ringIndex - 1) * step + step - 1);
			indices.push_back(vertexIndex + (ringIndex - 1) * step);
			indices.push_back(vertexIndex + (ringIndex) * step + step - 1);
			//Up triangle
			indices.push_back(vertexIndex + (ringIndex) * step);
			indices.push_back(vertexIndex + (ringIndex) * step + step - 1);
			indices.push_back(vertexIndex + (ringIndex - 1) * step);
		}
	}
	
	EntityManager::ForEachChild(internode, [&vertices, &indices, &newNormalDir, resolution, step](Entity child)
		{
			SimpleMeshGenerator(child, vertices, indices, newNormalDir, resolution, step);
		}
	);
}

void TreeUtilities::TreeManager::Init()
{
	_InternodeArchetype = EntityManager::CreateEntityArchetype(
		"Internode",
		LocalToWorld(), Connection(),
		Illumination(), Gravity(),
		InternodeIndex(), InternodeInfo(), TreeIndex()
	);
	_TreeArchetype = EntityManager::CreateEntityArchetype(
		"Tree",
		Translation(), Rotation(), Scale(), LocalToWorld(),
		TreeIndex(), TreeInfo(), TreeAge(),
		TreeParameters(),
		RewardEstimation()
		);

	_InternodeQuery = EntityManager::CreateEntityQuery();
	EntityManager::SetEntityQueryAllFilters(_InternodeQuery, InternodeInfo());
	_TreeQuery = EntityManager::CreateEntityQuery();
	EntityManager::SetEntityQueryAllFilters(_TreeQuery, TreeInfo());

	_TreeSystem = Application::GetWorld()->CreateSystem<TreeSystem>(SystemGroup::SimulationSystemGroup);
	_InternodeSystem = Application::GetWorld()->CreateSystem<InternodeSystem>(SystemGroup::SimulationSystemGroup);



	_TreeIndex.Value = 0;
	_InternodeIndex.Value = 0;

	_LightEstimator = new LightEstimator();

	EntityEditorSystem::AddComponentInspector<TreeAge>(
		[](ComponentBase* data)
		{
			ImGui::Text(("Current age: " + std::to_string(*(int*)data)).c_str());
			ImGui::InputInt("Iterations left", (int*)((char*)data + sizeof(int)));
		}
	);
	
	EntityEditorSystem::AddComponentInspector<TreeIndex>(
		[](ComponentBase* data)
		{
			ImGui::Text(("Value: " + std::to_string(*(int*)data)).c_str());
		}
	);

	EntityEditorSystem::AddComponentInspector<TreeParameters>(
		[](ComponentBase* data)
		{
			auto tps = static_cast<TreeParameters*>(data);
			ImGui::InputInt("Seed", &tps->Seed);
			ImGui::InputInt("Lateral Bud Number", &tps->LateralBudPerNode);
			ImGui::InputFloat("Apical Angle Var", &tps->VarianceApicalAngle);
			ImGui::InputFloat2("Branching Angle M/Var", &tps->BranchingAngleMean);
			ImGui::InputFloat2("Roll Angle M/Var", &tps->RollAngleMean);
			ImGui::InputFloat2("Extinction Prob A/L", &tps->ApicalBudKillProbability);
			ImGui::InputFloat3("AD Dis/Age", &tps->ApicalControlDistanceFactor);
			ImGui::InputFloat("Growth Rate", &tps->GrowthRate);
			ImGui::InputFloat2("Node Len Base/Age", &tps->InternodeLengthBase);
			ImGui::InputFloat4("AC Base/Age/Lvl/Dist", &tps->ApicalControlBase);
			ImGui::InputInt("Max Bud Age", &tps->MaxBudAge);
			ImGui::InputFloat("Phototropism", &tps->Phototropism);
			ImGui::InputFloat2("Gravitropism Base/Age", &tps->GravitropismBase);
			ImGui::InputFloat("PruningFactor", &tps->PruningFactor);
			ImGui::InputFloat("LowBranchPruningFactor", &tps->LowBranchPruningFactor);
			ImGui::InputFloat("GravityBendingStrength", &tps->GravityBendingStrength);
			ImGui::InputFloat2("Lighting Factor A/L", &tps->ApicalBudLightingFactor);
			ImGui::InputFloat2("Gravity Base/BPCo", &tps->SaggingFactor);
			ImGui::InputFloat2("Thickness End/Fac", &tps->EndNodeThickness);
		}
	);

	EntityEditorSystem::AddComponentInspector<InternodeInfo>(
		[](ComponentBase* data)
		{
			auto internodeInfo = static_cast<InternodeInfo*>(data);
			ImGui::Text(("Level: " + std::to_string(internodeInfo->Level)).c_str());
		}
	);

	EntityEditorSystem::AddComponentInspector<InternodeInfo>(
		[](ComponentBase* data)
		{
			auto internodeInfo = static_cast<InternodeInfo*>(data);
			ImGui::Text(("Level: " + std::to_string(internodeInfo->Level)).c_str());
			ImGui::Text(("DistanceToParent: " + std::to_string(internodeInfo->DistanceToParent)).c_str());
			ImGui::Text(("DistanceToBranchEnd: " + std::to_string(internodeInfo->DistanceToBranchEnd)).c_str());
			ImGui::Text(("TotalDistanceToBranchEnd: " + std::to_string(internodeInfo->TotalDistanceToBranchEnd)).c_str());
			ImGui::Text(("DistanceToBranchStart: " + std::to_string(internodeInfo->DistanceToBranchStart)).c_str());
			ImGui::Text(("AccumulatedLength: " + std::to_string(internodeInfo->AccumulatedLength)).c_str());
			ImGui::Text(("AccumulatedLight: " + std::to_string(internodeInfo->AccumulatedLight)).c_str());
			ImGui::Text(("AccumulatedActivatedBudsAmount: " + std::to_string(internodeInfo->AccumulatedActivatedBudsAmount)).c_str());
			ImGui::Text(("AccumulatedGravity: " + std::to_string(internodeInfo->AccumulatedGravity)).c_str());
			ImGui::Text(("MaxChildLevel: " + std::to_string(internodeInfo->MaxChildLevel)).c_str());
			ImGui::Text(("MaxActivatedChildLevel: " + std::to_string(internodeInfo->MaxActivatedChildLevel)).c_str());
			ImGui::Text(("Inhibitor: " + std::to_string(internodeInfo->Inhibitor)).c_str());
			ImGui::Text(("ParentInhibitorFactor: " + std::to_string(internodeInfo->ParentInhibitorFactor)).c_str());
			ImGui::Text(("ActivatedBudsAmount: " + std::to_string(internodeInfo->ActivatedBudsAmount)).c_str());
			ImGui::Text(("BranchEndInternodeAmount: " + std::to_string(internodeInfo->BranchEndInternodeAmount)).c_str());
			ImGui::Text(("Pruned: " + std::to_string(internodeInfo->Pruned)).c_str());
			ImGui::Text(("Pruned reason: " + std::to_string(internodeInfo->PruneReason)).c_str());
			ImGui::Text(("IsApical: " + std::to_string(internodeInfo->IsApical)).c_str());
			ImGui::Text(("ApicalBudExist: " + std::to_string(internodeInfo->ApicalBudExist)).c_str());
			ImGui::Text(("IsActivatedEndNode: " + std::to_string(internodeInfo->IsActivatedEndNode)).c_str());
			ImGui::Text(("Length: " + std::to_string(internodeInfo->Length)).c_str());
			ImGui::Text(("Thickness: " + std::to_string(internodeInfo->Thickness)).c_str());
			ImGui::Text(("Deformation: " + std::to_string(internodeInfo->Deformation)).c_str());
			ImGui::Text(("Straightness: " + std::to_string(internodeInfo->Straightness)).c_str());
			ImGui::Text(("Slope: " + std::to_string(internodeInfo->Slope)).c_str());
			ImGui::Text(("SiblingAngle: " + std::to_string(internodeInfo->SiblingAngle)).c_str());
			ImGui::Text(("ParentAngle: " + std::to_string(internodeInfo->ParentAngle)).c_str());
			ImGui::Text(("ParentThickness: " + std::to_string(internodeInfo->ParentThickness)).c_str());
			ImGui::Text(("IsMainChild: " + std::to_string(internodeInfo->IsMainChild)).c_str());
		}
	);

	EntityEditorSystem::AddComponentInspector<Illumination>(
		[](ComponentBase* data)
		{
			auto illumination = static_cast<Illumination*>(data);
			ImGui::Text(("Value: " + std::to_string(illumination->Value)).c_str());
		}
	);

	EntityEditorSystem::AddComponentInspector<TreeInfo>(
		[](ComponentBase* data)
		{
			auto info = static_cast<TreeInfo*>(data);
			ImGui::Text(("Current seed " + std::to_string(info->CurrentSeed)).c_str());
			ImGui::Text(("Height " + std::to_string(info->Height)).c_str());
			ImGui::Text(("Max branching depth " + std::to_string(info->MaxBranchingDepth)).c_str());
			ImGui::Text(("Lateral buds count " + std::to_string(info->LateralBudsCount)).c_str());
		}
	);
	
	
	_Ready = true;
}

bool TreeUtilities::TreeManager::IsReady()
{
	return _Ready;
}

EntityQuery TreeUtilities::TreeManager::GetInternodeQuery()
{
	return _InternodeQuery;
}

EntityQuery TreeUtilities::TreeManager::GetTreeQuery()
{
	return _TreeQuery;
}

InternodeSystem* TreeUtilities::TreeManager::GetInternodeSystem()
{
	return _InternodeSystem;
}

TreeSystem* TreeUtilities::TreeManager::GetTreeSystem()
{
	return _TreeSystem;
}



void TreeUtilities::TreeManager::GetAllTrees(std::vector<Entity>& container)
{
	if (!_Ready) {
		Debug::Error("TreeManager: Not initialized!");
		return;
	}
	return _TreeQuery.ToEntityArray(container);
}

void TreeUtilities::TreeManager::CalculateInternodeIllumination()
{
	std::vector<Entity> internodes;
	_InternodeQuery.ToEntityArray(internodes);
	GetLightEstimator()->TakeSnapShot(true);
	EntityManager::ForEach<Illumination, TreeIndex>(_InternodeQuery, [](int i, Entity leafEntity, Illumination* illumination, TreeIndex* index) 
		{
			illumination->LightDir = glm::vec3(0);
			illumination->Value = 0;
		}
	);
	auto snapShots = _LightEstimator->GetSnapShots();
	float maxIllumination = 0;
	for (const auto& shot : *snapShots) {
		size_t resolution = shot->Resolution();
		std::vector<std::shared_future<void>> futures;
		std::mutex writeMutex;
		std::mutex maxIlluminationMutex;
		for (size_t i = 0; i < resolution; i++) {
			futures.push_back(_World->GetThreadPool()->Push([i, shot, resolution, &writeMutex, &maxIlluminationMutex, &maxIllumination](int id)
				{
					float localMaxIllumination = 0;
					for (size_t j = 0; j < resolution; j++) {
						unsigned index = shot->GetEntityIndex(i, j);
						if (index != 0) {
							std::lock_guard<std::mutex> lock(writeMutex);
							Illumination illumination = EntityManager::GetComponentData<Illumination>(index);
							illumination.LightDir += shot->GetDirection() * shot->Weight();
							illumination.Value += shot->Weight();
							if (localMaxIllumination < illumination.Value) localMaxIllumination = illumination.Value;
							EntityManager::SetComponentData(index, illumination);
						}
					}
					std::lock_guard<std::mutex> lock(maxIlluminationMutex);
					if (maxIllumination < localMaxIllumination) maxIllumination = localMaxIllumination;
				}
			).share());
		}
		for (auto i : futures) i.wait();
	}
	_LightEstimator->SetMaxIllumination(maxIllumination);
	EntityManager::ForEach<Illumination, TreeIndex>(_InternodeQuery, [maxIllumination](int i, Entity leafEntity, Illumination* illumination, TreeIndex* index)
		{
			illumination->LightDir = glm::normalize(illumination->LightDir);
			illumination->Value /= maxIllumination;
		}
	);
}


Entity TreeUtilities::TreeManager::CreateTree(std::shared_ptr<Material> treeSurfaceMaterial, std::shared_ptr<Material> treeLeafMaterial, std::shared_ptr<Mesh> treeLeafMesh)
{
	const auto entity = EntityManager::CreateEntity(_TreeArchetype);
	auto instancedMeshMaterialComponent = std::make_shared<InstancedMeshMaterialComponent>();
	instancedMeshMaterialComponent->Matrices.clear();
	instancedMeshMaterialComponent->Material = std::move(treeLeafMaterial);
	instancedMeshMaterialComponent->Mesh = std::move(treeLeafMesh);
	instancedMeshMaterialComponent->BackCulling = false;
	EntityManager::SetSharedComponent(entity, std::move(instancedMeshMaterialComponent));
	EntityManager::SetSharedComponent(entity, std::make_shared<TreeData>());
	EntityManager::SetComponentData(entity, _TreeIndex);
	auto mmc = std::make_shared<MeshMaterialComponent>();
	mmc->Material = std::move(treeSurfaceMaterial);
	EntityManager::SetSharedComponent(entity, std::move(mmc));
	_TreeIndex.Value++;
	return entity;
}



void TreeManager::DeleteAllTrees()
{
	std::vector<Entity> trees;
	_TreeQuery.ToEntityArray(trees);
	for(auto& tree : trees)
	{
		EntityManager::DeleteEntity(tree);
	}
}

Entity TreeUtilities::TreeManager::CreateInternode(TreeIndex treeIndex, Entity parentEntity)
{
	auto entity = EntityManager::CreateEntity(_InternodeArchetype);
	auto internodeData = std::make_shared<InternodeData>();
	EntityManager::SetComponentData(entity, treeIndex);
	EntityManager::SetParent(entity, parentEntity);
	EntityManager::SetComponentData(entity, _InternodeIndex);
	EntityManager::SetSharedComponent(entity, internodeData);
	_InternodeIndex.Value++;
	InternodeInfo internodeInfo;
	internodeInfo.IsActivatedEndNode = false;
	internodeInfo.MaxActivatedChildLevel = 0;
	EntityManager::SetComponentData(entity, internodeInfo);
	return entity;
}

void TreeUtilities::TreeManager::ExportMeshToOBJ(Entity treeEntity, std::string filename)
{
	//TreeData info = EntityManager::GetComponentData<TreeData>(treeEntity);
	auto mesh = GetMeshForTree(treeEntity);
	auto vertices = mesh->GetVerticesUnsafe();
	auto indices = mesh->GetIndicesUnsafe();

	if (vertices->size() == 0) {
		Debug::Log("Mesh not generated!");
		return;
	}

	std::ofstream of;
	of.open((filename + ".obj").c_str(), std::ofstream::out | std::ofstream::trunc);
	if (of.is_open())
	{
		std::string data = "";
#pragma region Data collection
		data += "# List of geometric vertices, with (x, y, z).\n";
		for (const auto& vertex : *vertices) {
			data += "v " + std::to_string(vertex.Position.x)
				+ " " + std::to_string(vertex.Position.y)
				+ " " + std::to_string(vertex.Position.z)
				+ "\n";
		}
		data += "# List of indices for faces vertices, with (x, y, z).\n";
		for (int i = 0; i < indices->size() / 3; i++) {
			data += "f " + std::to_string(indices->at(i * 3) + 1)
				+ " " + std::to_string(indices->at(i * 3 + 1) + 1)
				+ " " + std::to_string(indices->at(i * 3 + 2) + 1)
				+ "\n";
		}
#pragma endregion
		of.write(data.c_str(), data.size());
		of.flush();
		of.close();
		Debug::Log("Model saved as " + filename + ".obj");
	}
	else
	{
		Debug::Error("Can't open file!");
	}
}

LightEstimator* TreeUtilities::TreeManager::GetLightEstimator()
{
	return _LightEstimator;
}

void TreeUtilities::TreeManager::CalculateRewards(Entity treeEntity, float snapShotWidth)
{
	RewardEstimation estimation = EntityManager::GetComponentData<RewardEstimation>(treeEntity);
	estimation.LightEstimationResult = _LightEstimator->CalculateScore();

	EntityManager::SetComponentData(treeEntity, estimation);
}

std::shared_ptr<Mesh> TreeUtilities::TreeManager::GetMeshForTree(Entity treeEntity)
{
	if (!_Ready) {
		Debug::Error("TreeManager: Not initialized!");
		return nullptr;
	}
	return EntityManager::GetSharedComponent<MeshMaterialComponent>(treeEntity)->Mesh;
}
#pragma endregion


void TreeUtilities::TreeManager::GenerateSimpleMeshForTree(Entity treeEntity, float resolution, float subdivision)
{
	if (resolution <= 0.0f) {
		Debug::Error("TreeManager: Resolution must be larger than 0!");
		return;
	}
	if (!_Ready) {
		Debug::Error("TreeManager: Not initialized!");
		return;
	}

	std::mutex creationM;
	
	//Prepare ring mesh.
	EntityManager::ForEach<InternodeInfo>(_InternodeQuery, [&creationM, resolution, subdivision](int i, Entity internode, InternodeInfo* info) 
		{
			if (EntityManager::HasComponentData<TreeInfo>(EntityManager::GetParent(internode))) return;
			auto list = EntityManager::GetSharedComponent<InternodeData>(internode);
		
			list->Rings.clear();
			glm::quat parentRotation = info->ParentRotation;
			glm::vec3 parentTranslation = info->ParentTranslation;
			float parentThickness = info->ParentThickness;

			glm::vec3 scale;
			glm::quat rotation;
			glm::vec3 translation;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose(info->GlobalTransform, scale, rotation, translation, skew, perspective);
			
			glm::vec3 parentDir = parentRotation * glm::vec3(0, 0, -1);
			glm::vec3 dir = rotation * glm::vec3(0, 0, -1);
			glm::vec3 mainChildDir = info->MainChildRotation * glm::vec3(0, 0, -1);
			glm::vec3 parentMainChildDir = info->ParentMainChildRotation * glm::vec3(0, 0, -1);
			glm::vec3 fromDir = (parentDir + parentMainChildDir) / 2.0f;
			dir = (dir + mainChildDir) / 2.0f;
#pragma region Subdivision internode here.
			auto distance = glm::distance(parentTranslation, translation);

			int step = (parentThickness / resolution);
			if (step < 4) step = 4;
			if (step % 2 != 0) step++;
			list->step = step;
			int amount = (int)(0.5f + distance / ((info->Thickness + parentThickness) / 2.0f) * subdivision);
			if (amount % 2 != 0) amount++;
			BezierCurve curve = BezierCurve(parentTranslation, parentTranslation + distance / 3.0f * fromDir, translation - distance / 3.0f * dir, translation);
			float posStep = 1.0f / (float)amount;
			glm::vec3 dirStep = (dir - fromDir) / (float)amount;
			float radiusStep = (info->Thickness - parentThickness) / (float)amount;

			//list->NormalDir = fromDir == dir ? glm::cross(dir, glm::vec3(0, 0, 1)) : glm::cross(fromDir, dir);

			for (int i = 1; i < amount; i++) {
				list->Rings.emplace_back(
					curve.GetPoint(posStep * (i - 1)), curve.GetPoint(posStep * i),
					fromDir + (float)(i - 1) * dirStep, fromDir + (float)i * dirStep,
					parentThickness + (float)(i - 1) * radiusStep, parentThickness + (float)i * radiusStep);
			}
			if (amount > 1)list->Rings.emplace_back(curve.GetPoint(1.0f - posStep), translation, dir - dirStep, dir, info->Thickness - radiusStep, info->Thickness);
			else list->Rings.emplace_back(parentTranslation, translation, fromDir, dir, parentThickness, info->Thickness);
#pragma endregion
		}

	);

	auto mmc = EntityManager::GetSharedComponent<MeshMaterialComponent>(treeEntity);
	auto treeData = EntityManager::GetSharedComponent<TreeData>(treeEntity);
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;

	auto internodeEntity = EntityManager::GetChildren(treeEntity).at(0);
	
	if(EntityManager::GetChildrenAmount(internodeEntity) != 0){
		SimpleMeshGenerator(EntityManager::GetChildren(internodeEntity).at(0), vertices, indices, glm::vec3(1, 0, 0), resolution);
		mmc->Mesh = std::make_shared<Mesh>();
		mmc->Mesh->SetVertices(17, vertices, indices, true);
		treeData->MeshGenerated = true;
	}
}
