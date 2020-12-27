#include "TreeManager.h"
#include "LightEstimator.h"
#include <gtx/matrix_decompose.hpp>
#include <utility>
#include "AcaciaFoliageGenerator.h"
#include "MapleFoliageGenerator.h"
#include "PineFoliageGenerator.h"
#include "WillowFoliageGenerator.h"
#include "AppleFoliageGenerator.h"
#include "OakFoliageGenerator.h"
#include "BirchFoliageGenerator.h"
#include "CakeTower.h"
#include "KDop.h"
#include "MaskTrimmer.h"
#include "rapidjson/prettywriter.h"
#include "Ray.h"
using namespace TreeUtilities;

LightEstimator* TreeUtilities::TreeManager::_LightEstimator;

TreeSystem* TreeUtilities::TreeManager::_TreeSystem;
InternodeSystem* TreeUtilities::TreeManager::_InternodeSystem;
EntityArchetype TreeUtilities::TreeManager::_AttractionPointArchetype;
EntityArchetype TreeUtilities::TreeManager::_InternodeArchetype;
EntityArchetype TreeUtilities::TreeManager::_TreeArchetype;

EntityQuery TreeUtilities::TreeManager::_TreeQuery;
EntityQuery TreeUtilities::TreeManager::_AttractionPointQuery;
EntityQuery TreeUtilities::TreeManager::_InternodeQuery;

TreeIndex TreeUtilities::TreeManager::_TreeIndex;
InternodeIndex TreeUtilities::TreeManager::_InternodeIndex;
std::shared_ptr<Material> TreeUtilities::TreeManager::SemanticTreeBranchMaterial;
std::shared_ptr<Material> TreeUtilities::TreeManager::SemanticTreeLeafMaterial;
bool TreeUtilities::TreeManager::_Ready;
#pragma region Helpers

void InternodeData::OnGui()
{
	for (int i = 0; i < Buds.size(); i++)
	{
		ImGui::Text(("Bud " + std::to_string(i)).c_str());
		ImGui::Text(Buds[i].IsApical ? "Apical" : "Lateral");
		ImGui::Spacing();
	}
}



void TreeData::OnGui()
{
	ImGui::Text(("MeshGenerated: " + std::string(MeshGenerated ? "Yes" : "No")).c_str());
	ImGui::Text(("FoliageGenerated: " + std::string(FoliageGenerated ? "Yes" : "No")).c_str());
	ImGui::Text(("ActiveLength: " + std::to_string(ActiveLength)).c_str());
}

void TreeUtilities::TreeManager::SimpleMeshGenerator(Entity& internode, std::vector<Vertex>& vertices, std::vector<unsigned>& indices, glm::vec3 normal, float resolution, int parentStep)
{
	InternodeInfo info = EntityManager::GetComponentData<InternodeInfo>(internode);
	glm::vec3 newNormalDir = normal;
	//glm::vec3 dir = info.DesiredGlobalRotation * glm::vec3(0.0f, 0.0f, -1.0f);
	//newNormalDir = glm::cross(glm::cross(dir, newNormalDir), dir);

	auto& list = EntityManager::GetPrivateComponent<InternodeData>(internode);
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
				indices.push_back(vertexIndex + (ringIndex)*step + i);
				//Up triangle
				indices.push_back(vertexIndex + (ringIndex)*step + i + 1);
				indices.push_back(vertexIndex + (ringIndex)*step + i);
				indices.push_back(vertexIndex + (ringIndex - 1) * step + i + 1);
			}
			//Down triangle
			indices.push_back(vertexIndex + (ringIndex - 1) * step + step - 1);
			indices.push_back(vertexIndex + (ringIndex - 1) * step);
			indices.push_back(vertexIndex + (ringIndex)*step + step - 1);
			//Up triangle
			indices.push_back(vertexIndex + (ringIndex)*step);
			indices.push_back(vertexIndex + (ringIndex)*step + step - 1);
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
	std::string vertShaderCode = std::string("#version 460 core\n")
		+ *Default::ShaderIncludes::Uniform +
		+"\n"
		+ FileIO::LoadFileAsString(FileIO::GetResourcePath("Shaders/Vertex/Standard.vert"));
	std::string fragShaderCode = std::string("#version 460 core\n")
		+ *Default::ShaderIncludes::Uniform
		+ "\n"
		+ FileIO::LoadFileAsString(FileIO::GetAssetFolderPath() + "Shaders/Fragment/SemanticBranch.frag");

	auto standardvert = std::make_shared<GLShader>(ShaderType::Vertex);
	standardvert->Compile(vertShaderCode);
	auto standardfrag = std::make_shared<GLShader>(ShaderType::Fragment);
	standardfrag->Compile(fragShaderCode);
	auto branchProgram = std::make_shared<GLProgram>(standardvert, standardfrag);
	

	vertShaderCode = std::string("#version 460 core\n")
		+ *Default::ShaderIncludes::Uniform +
		+"\n"
		+ FileIO::LoadFileAsString(FileIO::GetResourcePath("Shaders/Vertex/StandardInstanced.vert"));
	fragShaderCode = std::string("#version 460 core\n")
		+ *Default::ShaderIncludes::Uniform
		+ "\n"
		+ FileIO::LoadFileAsString(FileIO::GetAssetFolderPath() + "Shaders/Fragment/SemanticLeaf.frag");
	standardvert = std::make_shared<GLShader>(ShaderType::Vertex);
	standardvert->Compile(vertShaderCode);
	standardfrag = std::make_shared<GLShader>(ShaderType::Fragment);
	standardfrag->Compile(fragShaderCode);
	auto leafProgram = std::make_shared<GLProgram>(standardvert, standardfrag);
	
	SemanticTreeBranchMaterial = std::make_shared<Material>();
	SemanticTreeBranchMaterial->SetProgram(branchProgram);
	SemanticTreeLeafMaterial = std::make_shared<Material>();
	SemanticTreeLeafMaterial->CullingMode = MaterialCullingMode::OFF;
	SemanticTreeLeafMaterial->SetProgram(leafProgram);

	_InternodeArchetype = EntityManager::CreateEntityArchetype(
		"Internode",
		GlobalTransform(), Connection(), Ray(),
		Illumination(),
		InternodeIndex(), InternodeInfo(), TreeIndex()
	);
	_TreeArchetype = EntityManager::CreateEntityArchetype(
		"Tree", Transform(),
		GlobalTransform(),
		TreeIndex(), TreeInfo(), TreeAge(),
		TreeParameters()
	);
	_AttractionPointArchetype = EntityManager::CreateEntityArchetype("Attraction Point", Transform(),
		GlobalTransform(), TreeIndex(), AttractionPointInfo());

	_AttractionPointQuery = EntityManager::CreateEntityQuery();
	_AttractionPointQuery.SetAllFilters(AttractionPointInfo());
	
	_InternodeQuery = EntityManager::CreateEntityQuery();
	EntityManager::SetEntityQueryAllFilters(_InternodeQuery, GlobalTransform(), Connection(), Illumination(), InternodeIndex(), InternodeInfo(), TreeIndex());
	_TreeQuery = EntityManager::CreateEntityQuery();
	EntityManager::SetEntityQueryAllFilters(_TreeQuery, TreeInfo(), TreeAge(), TreeIndex(), TreeParameters(), GlobalTransform());

	_TreeSystem = Application::GetCurrentWorld()->CreateSystem<TreeSystem>(SystemGroup::SimulationSystemGroup);
	_InternodeSystem = Application::GetCurrentWorld()->CreateSystem<InternodeSystem>(SystemGroup::SimulationSystemGroup);



	_TreeIndex.Value = 0;
	_InternodeIndex.Value = 0;

	_LightEstimator = new LightEstimator();

	EditorManager::RegisterComponentDataInspector<TreeAge>(
		[](ComponentBase* data, bool isRoot)
		{
			ImGui::Text(("Current age: " + std::to_string(*(int*)data)).c_str());
			ImGui::DragInt("Iterations left", (int*)((char*)data + sizeof(int)));
		}
	);

	EditorManager::RegisterComponentDataInspector<TreeIndex>(
		[](ComponentBase* data, bool isRoot)
		{
			ImGui::Text(("Value: " + std::to_string(*(int*)data)).c_str());
		}
	);

	EditorManager::RegisterComponentDataInspector<TreeParameters>(
		[](ComponentBase* data, bool isRoot)
		{
			auto tps = static_cast<TreeParameters*>(data);
			ImGui::DragInt("Seed", &tps->Seed);
			ImGui::DragInt("Age", &tps->Age);

			ImGui::DragInt("Lateral Bud Number", &tps->LateralBudPerNode);
			ImGui::DragFloat("Apical Angle Var", &tps->VarianceApicalAngle);
			ImGui::DragFloat2("Branching Angle M/Var", &tps->BranchingAngleMean);
			ImGui::DragFloat2("Roll Angle M/Var", &tps->RollAngleMean);
			ImGui::DragFloat2("Extinction Prob A/L", &tps->ApicalBudKillProbability);
			ImGui::DragFloat3("AD Dis/Age", &tps->ApicalControlDistanceFactor);
			ImGui::DragFloat("Growth Rate", &tps->GrowthRate);
			ImGui::DragFloat2("Node Len Base/Age", &tps->InternodeLengthBase);
			ImGui::DragFloat4("AC Base/Age/Lvl/Dist", &tps->ApicalControlBase);
			ImGui::DragInt("Max Bud Age", &tps->MaxBudAge);
			ImGui::DragFloat("InternodeSize", &tps->InternodeSize);
			ImGui::DragFloat("Phototropism", &tps->Phototropism);
			ImGui::DragFloat2("Gravitropism Base/Age", &tps->GravitropismBase);
			ImGui::DragFloat("PruningFactor", &tps->PruningFactor);
			ImGui::DragFloat("LowBranchPruningFactor", &tps->LowBranchPruningFactor);
			ImGui::DragFloat2("Gravity Strength/Angle", &tps->GravityBendingStrength);
			ImGui::DragFloat2("Lighting Factor A/L", &tps->ApicalBudLightingFactor);
			ImGui::DragFloat2("Thickness End/Fac", &tps->EndNodeThickness);
			ImGui::Spacing();
			ImGui::DragInt("FoliageType", &tps->FoliageType);
		}
	);

	EditorManager::RegisterComponentDataInspector<InternodeInfo>(
		[](ComponentBase* data, bool isRoot)
		{
			auto internodeInfo = static_cast<InternodeInfo*>(data);
			if (ImGui::TreeNode("General")) {
				ImGui::Checkbox("Activated", &internodeInfo->Activated);
				ImGui::Text(("StartAge: " + std::to_string(internodeInfo->StartAge)).c_str());
				ImGui::Text(("Order: " + std::to_string(internodeInfo->Order)).c_str());
				ImGui::Text(("Level: " + std::to_string(internodeInfo->Level)).c_str());
				ImGui::Text(("DistanceToParent: " + std::to_string(internodeInfo->DistanceToParent)).c_str());
				ImGui::Spacing();
				ImGui::InputFloat3("BranchEndPosition", (float*)(void*)&internodeInfo->BranchEndPosition);
				ImGui::InputFloat3("BranchStartPosition", (float*)(void*)&internodeInfo->BranchStartPosition);
				ImGui::Text(("DistanceToBranchEnd: " + std::to_string(internodeInfo->DistanceToBranchEnd)).c_str());
				ImGui::Text(("LongestDistanceToEnd: " + std::to_string(internodeInfo->LongestDistanceToEnd)).c_str());
				ImGui::Text(("TotalDistanceToEnd: " + std::to_string(internodeInfo->TotalDistanceToEnd)).c_str());
				ImGui::Text(("DistanceToBranchStart: " + std::to_string(internodeInfo->DistanceToBranchStart)).c_str());
				ImGui::Text(("DistanceToRoot: " + std::to_string(internodeInfo->DistanceToRoot)).c_str());
				ImGui::Spacing();
				ImGui::Text(("AccumulatedLength: " + std::to_string(internodeInfo->AccumulatedLength)).c_str());
				ImGui::Text(("AccumulatedLight: " + std::to_string(internodeInfo->AccumulatedLight)).c_str());
				ImGui::Text(("AccumulatedActivatedBudsAmount: " + std::to_string(internodeInfo->AccumulatedActivatedBudsAmount)).c_str());
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Growth")) {
				ImGui::Text(("MaxChildOrder: " + std::to_string(internodeInfo->MaxChildOrder)).c_str());
				ImGui::Text(("MaxChildLevel: " + std::to_string(internodeInfo->MaxChildLevel)).c_str());
				ImGui::Text(("Inhibitor: " + std::to_string(internodeInfo->Inhibitor)).c_str());
				ImGui::Text(("ParentInhibitorFactor: " + std::to_string(internodeInfo->ParentInhibitorFactor)).c_str());
				ImGui::Text(("ActivatedBudsAmount: " + std::to_string(internodeInfo->ActivatedBudsAmount)).c_str());
				ImGui::Text(("BranchEndInternodeAmount: " + std::to_string(internodeInfo->BranchEndInternodeAmount)).c_str());
				ImGui::Text((std::string("Pruned: ") + (internodeInfo->Pruned ? "Yes" : "No")).c_str());
				std::string reason;
				switch (internodeInfo->PruneReason)
				{
				case 0:
					reason = "Low Branch";
					break;
				case 1:
					reason = "Pruning Factor";
					break;
				}
				ImGui::Text(("Pruned reason: " + reason).c_str());
				ImGui::Text(("IsMaxChild: " + std::string(internodeInfo->IsMaxChild ? "Yes" : "No")).c_str());
				ImGui::Text(("ApicalBudExist: " + std::to_string(internodeInfo->ApicalBudExist)).c_str());
				ImGui::Text(("IsEndNode: " + std::to_string(internodeInfo->IsEndNode)).c_str());
				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Geometric")) {
				ImGui::InputFloat3("MeanChildPosition", (float*)(void*)&internodeInfo->ChildBranchesMeanPosition, "%.3f", ImGuiInputTextFlags_ReadOnly);
				ImGui::Text(("MeanWeight: " + std::to_string(internodeInfo->MeanWeight)).c_str());
				ImGui::Text(("Sagging: " + std::to_string(internodeInfo->Sagging)).c_str());
				ImGui::Text(("Length: " + std::to_string(internodeInfo->Length)).c_str());
				ImGui::Text(("Thickness: " + std::to_string(internodeInfo->Thickness)).c_str());
				ImGui::Text(("ParentThickness: " + std::to_string(internodeInfo->ParentThickness)).c_str());
				ImGui::Text(("Deformation: " + std::to_string(internodeInfo->Deformation)).c_str());
				ImGui::Text(("Straightness: " + std::to_string(internodeInfo->Straightness)).c_str());
				ImGui::Text(("Slope: " + std::to_string(internodeInfo->Slope)).c_str());
				ImGui::Text(("SiblingAngle: " + std::to_string(internodeInfo->SiblingAngle)).c_str());
				ImGui::Text(("ParentAngle: " + std::to_string(internodeInfo->ParentAngle)).c_str());
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Transform related")) {
				ImGui::Text(("IsMainChild: " + std::string(internodeInfo->IsMainChild ? "Yes" : "No")).c_str());
				ImGui::InputFloat4("Parent translation", (float*)(void*)&internodeInfo->ParentTranslation.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
				ImGui::InputFloat4("LT0", (float*)(void*)&internodeInfo->LocalTransform[0], "%.3f", ImGuiInputTextFlags_ReadOnly);
				ImGui::InputFloat4("LT1", (float*)(void*)&internodeInfo->LocalTransform[1], "%.3f", ImGuiInputTextFlags_ReadOnly);
				ImGui::InputFloat4("LT2", (float*)(void*)&internodeInfo->LocalTransform[2], "%.3f", ImGuiInputTextFlags_ReadOnly);
				ImGui::InputFloat4("LT3", (float*)(void*)&internodeInfo->LocalTransform[3], "%.3f", ImGuiInputTextFlags_ReadOnly);
				ImGui::Spacing();

				ImGui::InputFloat4("GT0", (float*)(void*)&internodeInfo->GlobalTransform[0], "%.3f", ImGuiInputTextFlags_ReadOnly);
				ImGui::InputFloat4("GT1", (float*)(void*)&internodeInfo->GlobalTransform[1], "%.3f", ImGuiInputTextFlags_ReadOnly);
				ImGui::InputFloat4("GT2", (float*)(void*)&internodeInfo->GlobalTransform[2], "%.3f", ImGuiInputTextFlags_ReadOnly);
				ImGui::InputFloat4("GT3", (float*)(void*)&internodeInfo->GlobalTransform[3], "%.3f", ImGuiInputTextFlags_ReadOnly);

				ImGui::TreePop();
			}
			if (ImGui::TreeNode("Crown Shyness"))
			{
				ImGui::Text(("CrownShyness: " + std::to_string(internodeInfo->CrownShyness)).c_str());
				ImGui::TreePop();
			}
		}
	);

	EditorManager::RegisterComponentDataInspector<Illumination>(
		[](ComponentBase* data, bool isRoot)
		{
			auto illumination = static_cast<Illumination*>(data);
			ImGui::Text(("Value: " + std::to_string(illumination->Value)).c_str());
		}
	);

	EditorManager::RegisterComponentDataInspector<TreeInfo>(
		[](ComponentBase* data, bool isRoot)
		{
			const auto info = static_cast<TreeInfo*>(data);
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

EntityQuery TreeManager::GetAttractionPointQuery()
{
	return _AttractionPointQuery;
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
	if (internodes.empty()) return;
	const auto snapShots = _LightEstimator->GetSnapShots();
	float maxIllumination = 0;
	for (const auto& shot : *snapShots) {
		size_t resolution = shot->Resolution();
		std::vector<std::shared_future<void>> futures;
		std::mutex writeMutex;
		std::mutex maxIlluminationMutex;
		for (size_t i = 0; i < resolution; i++) {
			futures.push_back(JobManager::GetThreadPool().Push([i, shot, resolution, &writeMutex, &maxIlluminationMutex, &maxIllumination](int id)
				{
					float localMaxIllumination = 0;
					for (size_t j = 0; j < resolution; j++) {
						unsigned index = shot->GetEntityIndex(i, j);
						if (index != 0) {
							std::lock_guard<std::mutex> lock(writeMutex);
							auto illumination = EntityManager::GetComponentData<Illumination>(index);
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
		for (const auto& i : futures) i.wait();
	}
	_LightEstimator->SetMaxIllumination(maxIllumination);
	EntityManager::ForEach<Illumination, TreeIndex>(_InternodeQuery, [maxIllumination](int i, Entity leafEntity, Illumination* illumination, TreeIndex* index)
		{
			illumination->LightDir = glm::normalize(illumination->LightDir);
			illumination->Value /= maxIllumination;
		}
	);
}


Entity TreeUtilities::TreeManager::CreateTree(std::shared_ptr<Material> treeSurfaceMaterial, TreeParameters& treeParameters)
{
	const auto entity = EntityManager::CreateEntity(_TreeArchetype);
	
	EntityManager::SetPrivateComponent(entity, std::make_unique<TreeData>());
	//EntityManager::SetPrivateComponent(entity, std::move(std::make_unique<KDop>()));
	EntityManager::SetPrivateComponent(entity, std::make_unique<CakeTower>());
	EntityManager::SetPrivateComponent(entity, std::make_unique<MaskTrimmer>());
	EntityManager::SetComponentData(entity, _TreeIndex);
	auto mmc = std::make_unique<MeshRenderer>();
	mmc->Material = std::move(treeSurfaceMaterial);
	EntityManager::SetPrivateComponent(entity, std::move(mmc));
	switch (treeParameters.FoliageType)
	{
	case 0:
		EntityManager::SetPrivateComponent(entity, std::make_unique<DefaultFoliageGenerator>());
		entity.SetName("Default Tree No." + std::to_string(_TreeIndex.Value));
		break;
	case 1:
		EntityManager::SetPrivateComponent(entity, std::make_unique<AcaciaFoliageGenerator>());
		entity.SetName("Acacia Tree No." + std::to_string(_TreeIndex.Value));
		break;
	case 2:
		EntityManager::SetPrivateComponent(entity, std::make_unique<WillowFoliageGenerator>());
		entity.SetName("Willow Tree No." + std::to_string(_TreeIndex.Value));
		break;
	case 3:
		EntityManager::SetPrivateComponent(entity, std::make_unique<PineFoliageGenerator>());
		entity.SetName("Pine Tree No." + std::to_string(_TreeIndex.Value));
		break;
	case 4:
		EntityManager::SetPrivateComponent(entity, std::make_unique<MapleFoliageGenerator>());
		entity.SetName("Maple Tree No." + std::to_string(_TreeIndex.Value));
		break;
	case 5:
		EntityManager::SetPrivateComponent(entity, std::make_unique<AppleFoliageGenerator>());
		entity.SetName("Apple Tree No." + std::to_string(_TreeIndex.Value));
		break;
	case 6:
		EntityManager::SetPrivateComponent(entity, std::make_unique<OakFoliageGenerator>());
		entity.SetName("Oak Tree No." + std::to_string(_TreeIndex.Value));
		break;
	case 7:
		EntityManager::SetPrivateComponent(entity, std::make_unique<BirchFoliageGenerator>());
		entity.SetName("Birch Tree No." + std::to_string(_TreeIndex.Value));
		break;
	}
	_TreeIndex.Value++;
	return entity;
}



void TreeManager::DeleteAllTrees()
{
	std::vector<Entity> trees;
	_TreeQuery.ToEntityArray(trees);
	for (auto& tree : trees)
	{
		EntityManager::DeleteEntity(tree);
	}
}

Entity TreeUtilities::TreeManager::CreateInternode(TreeIndex treeIndex, Entity parentEntity)
{
	const auto entity = EntityManager::CreateEntity(_InternodeArchetype);
	entity.SetName("Internode");
	auto internodeData = std::make_unique<InternodeData>();
	EntityManager::SetComponentData(entity, treeIndex);
	EntityManager::SetParent(entity, parentEntity);
	EntityManager::SetComponentData(entity, _InternodeIndex);
	EntityManager::SetPrivateComponent(entity, std::move(internodeData));
	_InternodeIndex.Value++;
	InternodeInfo internodeInfo;
	internodeInfo.IsEndNode = false;
	internodeInfo.MaxChildOrder = 0;
	EntityManager::SetComponentData(entity, internodeInfo);
	return entity;
}

Entity TreeManager::CreateAttractionPoint(const TreeIndex& treeIndex, const glm::vec3& position, const Entity& tree)
{
	const auto entity = EntityManager::CreateEntity(_AttractionPointArchetype);
	entity.SetName("Attraction Point");
	entity.SetComponentData(treeIndex);
	Transform transform;
	transform.SetPosition(position);
	entity.SetComponentData(transform);
	EntityManager::SetParent(entity, tree);
	return entity;
}

void TreeUtilities::TreeManager::ExportTreeAsModel(Entity treeEntity, std::string filename, bool includeFoliage)
{
	auto mesh = EntityManager::GetPrivateComponent<MeshRenderer>(treeEntity)->Mesh;
	auto vertices = mesh->GetVerticesUnsafe();
	auto indices = mesh->GetIndicesUnsafe();
	
	if (vertices.size() == 0) {
		Debug::Log("Mesh not generated!");
		return;
	}

	std::ofstream of;
	of.open((filename + ".obj").c_str(), std::ofstream::out | std::ofstream::trunc);
	if (of.is_open())
	{
		std::string branchVert = "";
		std::string branchletVert = "";
		std::string leafVert = "";
		std::string branchIndices = "";
		std::string branchletIndices = "";
		std::string leafIndices = "";
#pragma region Data collection
		for (const auto& vertex : vertices) {
			branchVert += "v " + std::to_string(vertex.Position.x)
				+ " " + std::to_string(vertex.Position.y)
				+ " " + std::to_string(vertex.Position.z)
				+ "\n";
		}
		for (int i = 0; i < indices.size() / 3; i++) {
			branchIndices += "f " + std::to_string(indices.at(i * 3) + 1)
				+ " " + std::to_string(indices.at(i * 3 + 1) + 1)
				+ " " + std::to_string(indices.at(i * 3 + 2) + 1)
				+ "\n";
		}
#pragma endregion
		size_t branchVertSize = vertices.size();
		if(includeFoliage)
		{
			Entity foliageEntity;
			EntityManager::ForEachChild(treeEntity, [&foliageEntity](Entity child)
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
			size_t branchletVertSize = 0;
			if(foliageEntity.HasPrivateComponent<MeshRenderer>())
			{
				mesh = EntityManager::GetPrivateComponent<MeshRenderer>(foliageEntity)->Mesh;
				vertices = mesh->GetVerticesUnsafe();
				indices = mesh->GetIndicesUnsafe();
				branchletVertSize += vertices.size();
#pragma region Data collection
				for (const auto& vertex : vertices) {
					branchletVert += "v " + std::to_string(vertex.Position.x)
						+ " " + std::to_string(vertex.Position.y)
						+ " " + std::to_string(vertex.Position.z)
						+ "\n";
				}
				for (int i = 0; i < indices.size() / 3; i++) {
					branchletIndices += "f " + std::to_string(indices.at(i * 3) + 1 + branchVertSize)
						+ " " + std::to_string(indices.at(i * 3 + 1) + 1 + branchVertSize)
						+ " " + std::to_string(indices.at(i * 3 + 2) + 1 + branchVertSize)
						+ "\n";
				}
#pragma endregion
			}
			
			if(foliageEntity.HasPrivateComponent<Particles>())
			{
				auto& particles = EntityManager::GetPrivateComponent<Particles>(foliageEntity);
				mesh = particles->Mesh;
				vertices = mesh->GetVerticesUnsafe();
				indices = mesh->GetIndicesUnsafe();
				auto& matrices = particles->Matrices;
				size_t offset = 0;
				for(auto& matrix : matrices)
				{
					for (const auto& vertex : vertices) {
						glm::vec3 position = matrix * glm::vec4(vertex.Position, 1);
						leafVert += "v " + std::to_string(position.x)
							+ " " + std::to_string(position.y)
							+ " " + std::to_string(position.z)
							+ "\n";
					}
				}
				for (auto& matrix : matrices)
				{
					for (int i = 0; i < indices.size() / 3; i++) {
						leafIndices += "f " + std::to_string(indices.at(i * 3) + 1 + offset + branchVertSize + branchletVertSize)
							+ " " + std::to_string(indices.at(i * 3 + 1) + 1 + offset + branchVertSize + branchletVertSize)
							+ " " + std::to_string(indices.at(i * 3 + 2) + 1 + offset + branchVertSize + branchletVertSize)
							+ "\n";
					}
					offset += vertices.size();
				}
			}
		}
		of.write(branchVert.c_str(), branchVert.size());
		of.flush();
		if (branchletVert.size() != 0) {
			of.write(branchletVert.c_str(), branchletVert.size());
			of.flush();
		}
		if (leafVert.size() != 0) {
			of.write(leafVert.c_str(), leafVert.size());
			of.flush();
		}
		std::string group = "o branches\n";
		of.write(group.c_str(), group.size());
		of.write(branchIndices.c_str(), branchIndices.size());
		of.flush();
		if (branchletVert.size() != 0) {
			group = "o branchlets\n";
			of.write(group.c_str(), group.size());
			of.write(branchletIndices.c_str(), branchletIndices.size());
			of.flush();
		}
		if (leafVert.size() != 0) {
			group = "o leaves\n";
			of.write(group.c_str(), group.size());
			of.write(leafIndices.c_str(), leafIndices.size());
			of.flush();
		}
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
	glm::mat4 treeTransform = EntityManager::GetComponentData<GlobalTransform>(treeEntity).Value;
	//Prepare ring mesh.
	EntityManager::ForEach<InternodeInfo>(_InternodeQuery, [&creationM, resolution, subdivision, treeTransform](int i, Entity internode, InternodeInfo* info)
		{
			if (EntityManager::HasComponentData<TreeInfo>(EntityManager::GetParent(internode))) return;
			auto& list = EntityManager::GetPrivateComponent<InternodeData>(internode);

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

	auto& mmc = EntityManager::GetPrivateComponent<MeshRenderer>(treeEntity);
	auto& treeData = EntityManager::GetPrivateComponent<TreeData>(treeEntity);
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;

	auto internodeEntity = EntityManager::GetChildren(treeEntity).at(0);

	if (EntityManager::GetChildrenAmount(internodeEntity) != 0) {
		SimpleMeshGenerator(EntityManager::GetChildren(internodeEntity).at(0), vertices, indices, glm::vec3(1, 0, 0), resolution);
		mmc->Mesh = std::make_shared<Mesh>();
		mmc->Mesh->SetVertices(17, vertices, indices, true);
		treeData->MeshGenerated = true;
	}
}
void TreeManager::SerializeTreeGraph(std::string path, Entity tree)
{
	std::ofstream ofs;
	ofs.open((path + ".json").c_str(), std::ofstream::out | std::ofstream::trunc);
	if (!ofs.is_open())
	{
		Debug::Error("Can't open file!");
		return;
	}
	
	TreeIndex treeIndex = tree.GetComponentData<TreeIndex>();
	std::vector<InternodeInfo> internodeInfos;
	std::vector<Entity> internodes;
	_InternodeQuery.ToEntityArray<TreeIndex>(internodes, [treeIndex](Entity entity, TreeIndex& compareIndex)
		{
			return treeIndex.Value == compareIndex.Value;
		}
	);
	_InternodeQuery.ToComponentDataArray<InternodeInfo, TreeIndex>(internodeInfos, [treeIndex](TreeIndex& compareIndex)
		{
			return treeIndex.Value == compareIndex.Value;
		}
	);
	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	buffer.Clear();
	writer.StartObject();
	writer.String("Count");
	writer.Int(internodes.size());
	writer.String("Internodes");
	writer.StartArray();
	for(size_t i = 0; i < internodes.size(); i++)
	{
		//Internode
		writer.StartObject();
		writer.String("Index");
		writer.Int(internodes[i].Index);
		writer.String("Parent");
		writer.Int(EntityManager::GetParent(internodes[i]).Index);
		writer.String("Gravelius Order");
		writer.Int(internodeInfos[i].Order);
		writer.String("Thickness");
		writer.Double(internodeInfos[i].Thickness);
		writer.String("Level");
		writer.Int(internodeInfos[i].Level);
		writer.String("Start Age");
		writer.Int(internodeInfos[i].StartAge);
		//Position
		writer.String("Position");
		auto& transform = internodeInfos[i].GlobalTransform;
		//xyz
		writer.StartArray();
		writer.Double(transform[3].x);
		writer.Double(transform[3].y);
		writer.Double(transform[3].z);
		writer.EndArray();
		//End xyz
		//End Position
		writer.String("Children");
		writer.StartArray();
		for(auto& child : EntityManager::GetChildren(internodes[i]))
		{
			writer.Int(child.Index);
		}
		writer.EndArray();
		writer.EndObject();
		//End internode
	}
	writer.EndArray();
	writer.EndObject();
	std::string output = buffer.GetString();
	ofs.write(output.c_str(), output.size());
	ofs.flush();
	ofs.close();
	Debug::Log("Tree graph saved: " + path + ".json");
}