#include "pch.h"
#include "TreeManager.h"
#include "LightEstimator.h"
#include <gtx/matrix_decompose.hpp>
using namespace TreeUtilities;

LightEstimator* TreeUtilities::TreeManager::_LightEstimator;

TreeSystem* TreeUtilities::TreeManager::_TreeSystem;
BranchNodeSystem* TreeUtilities::TreeManager::_BranchNodeSystem;

EntityArchetype TreeUtilities::TreeManager::_BranchNodeArchetype;
EntityArchetype TreeUtilities::TreeManager::_TreeArchetype;

EntityQuery TreeUtilities::TreeManager::_TreeQuery;
EntityQuery TreeUtilities::TreeManager::_BranchNodeQuery;

TreeIndex TreeUtilities::TreeManager::_TreeIndex;
BranchNodeIndex TreeUtilities::TreeManager::_BranchNodeIndex;

bool TreeUtilities::TreeManager::_Ready;
#pragma region Helpers
void TreeUtilities::TreeManager::SimpleMeshGenerator(Entity& branchNode, std::vector<Vertex>& vertices, std::vector<unsigned>& indices, glm::vec3 normal, float resolution, int parentStep)
{
	BranchNodeInfo info = EntityManager::GetComponentData<BranchNodeInfo>(branchNode);
	glm::vec3 newNormalDir = normal;
	//glm::vec3 dir = info.DesiredGlobalRotation * glm::vec3(0.0f, 0.0f, -1.0f);
	//newNormalDir = glm::cross(glm::cross(dir, newNormalDir), dir);
	
	auto list = EntityManager::GetComponentData<RingMeshList>(branchNode);
	auto rings = list.Rings;
	auto step = list.step;
	//For stitching
	int pStep = parentStep > 0 ? parentStep : step;

	list.NormalDir = newNormalDir;
	EntityManager::SetComponentData(branchNode, list);
	float angleStep = 360.0f / (float)(pStep);
	int vertexIndex = vertices.size();
	Vertex archetype;
	float textureXstep = 1.0f / pStep * 4.0f;
	for (int i = 0; i < pStep; i++) {
		archetype.Position = rings->at(0).GetPoint(newNormalDir, angleStep * i, true);
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
	int ringSize = rings->size();
	for (int ringIndex = 0; ringIndex < ringSize; ringIndex++) {
		for (int i = 0; i < step; i++) {
			archetype.Position = rings->at(ringIndex).GetPoint(newNormalDir, angleStep * i, false);
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
	
	EntityManager::ForEachChild(branchNode, [&vertices, &indices, &newNormalDir, resolution, step](Entity child)
		{
			SimpleMeshGenerator(child, vertices, indices, newNormalDir, resolution, step);
		}
	);
}

void TreeUtilities::TreeManager::BranchNodeCleaner(Entity branchEntity)
{
	BudList ob = EntityManager::GetComponentData<BudList>(branchEntity);
	delete ob.Buds;
	RingMeshList rml = EntityManager::GetComponentData<RingMeshList>(branchEntity);
	delete rml.Rings;

	EntityManager::ForEachChild(branchEntity, [](Entity child) {
		BranchNodeCleaner(child);
		});
}
void TreeUtilities::TreeManager::Init()
{
	_BranchNodeArchetype = EntityManager::CreateEntityArchetype(
		"BranchNode",
		LocalToWorld(), Connection(),
		Illumination(), Gravity(), RingMeshList(),
		BranchNodeIndex(), BranchNodeInfo(), TreeIndex(), BudList()
	);
	_TreeArchetype = EntityManager::CreateEntityArchetype(
		"Tree",
		Translation(), Rotation(), Scale(), LocalToWorld(),
		TreeIndex(), TreeInfo(), TreeColor(), TreeAge(),
		TreeParameters(),
		RewardEstimation()
		);

	_BranchNodeQuery = EntityManager::CreateEntityQuery();
	EntityManager::SetEntityQueryAllFilters(_BranchNodeQuery, BranchNodeInfo());
	_TreeQuery = EntityManager::CreateEntityQuery();
	EntityManager::SetEntityQueryAllFilters(_TreeQuery, TreeInfo());

	_TreeSystem = Application::GetWorld()->CreateSystem<TreeSystem>(SystemGroup::SimulationSystemGroup);
	_BranchNodeSystem = Application::GetWorld()->CreateSystem<BranchNodeSystem>(SystemGroup::SimulationSystemGroup);



	_TreeIndex.Value = 0;
	_BranchNodeIndex.Value = 0;

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
			ImGui::InputFloat2("Node Len Base/Age", &tps->BranchNodeLengthBase);
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

	EntityEditorSystem::AddComponentInspector<TreeInfo>(
		[](ComponentBase* data)
		{
			auto info = static_cast<TreeInfo*>(data);
			ImGui::Text(("Height: " + std::to_string(info->Height)).c_str());
			ImGui::Text(("Active Length: " + std::to_string(info->ActiveLength)).c_str());
			ImGui::Text(("Max Branching Depth: " + std::to_string(info->MaxBranchingDepth)).c_str());
			ImGui::Text(("Lateral Buds Count: " + std::to_string(info->LateralBudsCount)).c_str());
			ImGui::Text(("Mesh Generated: " + std::to_string(info->MeshGenerated)).c_str());
			ImGui::Text(("Foliage Generated: " + std::to_string(info->FoliageGenerated)).c_str());
		}
	);
	
	_Ready = true;
}

bool TreeUtilities::TreeManager::IsReady()
{
	return _Ready;
}

EntityQuery TreeUtilities::TreeManager::GetBranchNodeQuery()
{
	return _BranchNodeQuery;
}

EntityQuery TreeUtilities::TreeManager::GetTreeQuery()
{
	return _TreeQuery;
}

BranchNodeSystem* TreeUtilities::TreeManager::GetBranchNodeSystem()
{
	return _BranchNodeSystem;
}

TreeSystem* TreeUtilities::TreeManager::GetTreeSystem()
{
	return _TreeSystem;
}



void TreeUtilities::TreeManager::GetAllTrees(std::vector<Entity>* container)
{
	if (!_Ready) {
		Debug::Error("TreeManager: Not initialized!");
		return;
	}
	return _TreeQuery.ToEntityArray(container);
}

void TreeUtilities::TreeManager::CalculateBranchNodeIllumination()
{
	std::vector<Entity> branchNodes;
	_BranchNodeQuery.ToEntityArray(&branchNodes);
	TreeManager::GetLightEstimator()->TakeSnapShot(true);
	EntityManager::ForEach<Illumination, TreeIndex>(_BranchNodeQuery, [](int i, Entity leafEntity, Illumination* illumination, TreeIndex* index) 
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
}


Entity TreeUtilities::TreeManager::CreateTree(Material* treeSurfaceMaterial)
{
	auto entity = EntityManager::CreateEntity(_TreeArchetype);
	TreeInfo treeInfo;
	treeInfo.GravitropismLevelVal = new std::vector<float>();
	treeInfo.ApicalDominanceTimeVal = new std::vector<float>();
	treeInfo.ApicalControlTimeVal = new std::vector<float>();
	treeInfo.ApicalControlTimeLevelVal = new std::vector<std::vector<float>>();
	treeInfo.Vertices = new std::vector<Vertex>();
	treeInfo.Indices = new std::vector<unsigned>();
	EntityManager::SetComponentData(entity, treeInfo);
	EntityManager::SetComponentData(entity, _TreeIndex);
	MeshMaterialComponent* mmc = new MeshMaterialComponent();
	mmc->_Material = treeSurfaceMaterial;
	mmc->_Mesh = nullptr;
	EntityManager::SetSharedComponent(entity, mmc);
	_TreeIndex.Value++;
	return entity;
}

void TreeUtilities::TreeManager::DeleteTree(Entity treeEntity)
{
	auto treeInfo = EntityManager::GetComponentData<TreeInfo>(treeEntity);
	delete treeInfo.GravitropismLevelVal;
	delete treeInfo.ApicalDominanceTimeVal;
	delete treeInfo.ApicalControlTimeVal;
	delete treeInfo.ApicalControlTimeLevelVal;
	delete treeInfo.Vertices;
	delete treeInfo.Indices;
	auto* mmc = EntityManager::GetSharedComponent<MeshMaterialComponent>(treeEntity);
	delete mmc->_Mesh;
	delete mmc;

	if (EntityManager::HasComponentData<BranchNodeInfo>(EntityManager::GetChildren(treeEntity).at(0))) {
		BranchNodeCleaner(EntityManager::GetChildren(treeEntity).at(0));
	}
	EntityManager::DeleteEntity(treeEntity);
}

Entity TreeUtilities::TreeManager::CreateBranchNode(TreeIndex treeIndex, Entity parentEntity)
{
	auto entity = EntityManager::CreateEntity(_BranchNodeArchetype);
	BudList ob = BudList();
	ob.Buds = new std::vector<Bud>();
	RingMeshList rml = RingMeshList();
	rml.Rings = new std::vector<RingMesh>();
	EntityManager::SetComponentData(entity, treeIndex);
	EntityManager::SetParent(entity, parentEntity);
	EntityManager::SetComponentData(entity, _BranchNodeIndex);
	EntityManager::SetComponentData(entity, ob);
	EntityManager::SetComponentData(entity, rml);
	_BranchNodeIndex.Value++;
	BranchNodeInfo branchInfo;
	branchInfo.MaxActivatedChildLevel = 0;
	EntityManager::SetComponentData(entity, branchInfo);
	return entity;
}

void TreeUtilities::TreeManager::ExportMeshToOBJ(Entity treeEntity, std::string filename)
{
	TreeInfo info = EntityManager::GetComponentData<TreeInfo>(treeEntity);

	auto vertices = info.Vertices;
	auto indices = info.Indices;

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

Mesh* TreeUtilities::TreeManager::GetMeshForTree(Entity treeEntity)
{
	if (!_Ready) {
		Debug::Error("TreeManager: Not initialized!");
		return nullptr;
	}
	return EntityManager::GetSharedComponent<MeshMaterialComponent>(treeEntity)->_Mesh;
}
#pragma endregion


void TreeUtilities::TreeManager::GenerateSimpleMeshForTree(Entity treeEntity, float resolution)
{
	if (resolution <= 0.0f) {
		Debug::Error("TreeManager: Resolution must be larger than 0!");
		return;
	}
	if (!_Ready) {
		Debug::Error("TreeManager: Not initialized!");
		return;
	}
	//Prepare ring mesh.
	EntityManager::ForEach<BranchNodeInfo, RingMeshList>(_BranchNodeQuery, [resolution](int i, Entity branchNode, BranchNodeInfo* info, RingMeshList* list) 
		{
			if (EntityManager::HasComponentData<TreeInfo>(EntityManager::GetParent(branchNode))) return;
			std::vector<RingMesh>* rings = list->Rings;
			rings->clear();
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
#pragma region Subdivision branch here.
			auto distance = glm::distance(parentTranslation, translation);

			int step = (parentThickness / resolution);
			if (step < 4) step = 4;
			if (step % 2 != 0) step++;
			list->step = step;
			int amount = distance / ((info->Thickness + parentThickness) / 2.0f) + 0.5f;
			if (amount % 2 != 0) amount++;
			BezierCurve curve = BezierCurve(parentTranslation, parentTranslation + distance / 3.0f * fromDir, translation - distance / 3.0f * dir, translation);
			float posStep = 1.0f / (float)amount;
			glm::vec3 dirStep = (dir - fromDir) / (float)amount;
			float radiusStep = (info->Thickness - parentThickness) / (float)amount;

			//list->NormalDir = fromDir == dir ? glm::cross(dir, glm::vec3(0, 0, 1)) : glm::cross(fromDir, dir);

			for (int i = 1; i < amount; i++) {
				rings->push_back(RingMesh(
					curve.GetPoint(posStep * (i - 1)), curve.GetPoint(posStep * i),
					fromDir + (float)(i - 1) * dirStep, fromDir + (float)i * dirStep,
					parentThickness + (float)(i - 1) * radiusStep, parentThickness + (float)i * radiusStep));
			}
			if (amount > 1)rings->push_back(RingMesh(curve.GetPoint(1.0f - posStep), translation, dir - dirStep, dir, info->Thickness - radiusStep, info->Thickness));
			else rings->push_back(RingMesh(parentTranslation, translation, fromDir, dir, parentThickness, info->Thickness));
#pragma endregion
		}

	);

	MeshMaterialComponent* mmc = EntityManager::GetSharedComponent<MeshMaterialComponent>(treeEntity);
	TreeInfo treeInfo = EntityManager::GetComponentData<TreeInfo>(treeEntity);
	treeInfo.Vertices->clear();
	treeInfo.Indices->clear();

	auto branchNode = EntityManager::GetChildren(treeEntity).at(0);
	
	if(EntityManager::GetChildrenAmount(branchNode) != 0){
		SimpleMeshGenerator(EntityManager::GetChildren(branchNode).at(0), *treeInfo.Vertices, *treeInfo.Indices, glm::vec3(1, 0, 0), resolution);
		if (mmc->_Mesh != nullptr) delete mmc->_Mesh;
		mmc->_Mesh = new Mesh();
		mmc->_Mesh->SetVertices(17, *treeInfo.Vertices, *treeInfo.Indices);
		treeInfo.MeshGenerated = true;
	}

	
	EntityManager::SetComponentData(treeEntity, treeInfo);
}
