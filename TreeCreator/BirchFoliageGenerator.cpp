#include "BirchFoliageGenerator.h"
#include "PlantSimulationSystem.h"
std::shared_ptr<Texture2D> TreeUtilities::BirchFoliageGenerator::_LeafSurfaceTex = nullptr;

TreeUtilities::BirchFoliageGenerator::BirchFoliageGenerator()
{
	_DefaultFoliageInfo = BirchFoliageInfo();
	_Archetype = EntityManager::CreateEntityArchetype("Birch Foliage", Transform(), GlobalTransform(), TreeIndex(), BirchFoliageInfo());

	_LeafMaterial = std::make_shared<Material>();
	_LeafMaterial->m_shininess = 32.0f;
	_LeafMaterial->SetProgram(Default::GLPrograms::StandardInstancedProgram);
	_LeafMaterial->m_alphaDiscardEnabled = true;
	_LeafMaterial->m_alphaDiscardOffset = 0.7f;
	_LeafMaterial->m_cullingMode = MaterialCullingMode::Off;
	if(!_LeafSurfaceTex)_LeafSurfaceTex = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Leaf/PrunusAvium/A/level0.png");
	//_LeafMaterial->SetTexture(_LeafSurfaceTex);
	_LeafMaterial->m_albedoColor = glm::normalize(glm::vec3(60.0f / 256.0f, 140.0f / 256.0f, 0.0f));
	_LeafMaterial->m_metallic = 0.0f;
	_LeafMaterial->m_roughness = 0.3f;
	_LeafMaterial->m_ambientOcclusion = glm::linearRand(0.4f, 0.8f);

}

void TreeUtilities::BirchFoliageGenerator::Generate()
{
	const auto tree = GetOwner();
	Entity foliageEntity;
	GlobalTransform treeTransform = EntityManager::GetComponentData<GlobalTransform>(tree);
	bool found = false;
	TreeIndex ti = EntityManager::GetComponentData<TreeIndex>(tree);
	EntityManager::ForEachChild(tree, [&found, &foliageEntity](Entity child)
		{
			if (child.HasComponentData<BirchFoliageInfo>())
			{
				found = true;
				foliageEntity = child;
			}
		}
	);
	if (!found)
	{
		foliageEntity = EntityManager::CreateEntity(_Archetype, "Foliage");
		EntityManager::SetParent(foliageEntity, tree);
		auto particleSys = std::make_unique<Particles>();
		particleSys->m_material = _LeafMaterial;
		particleSys->m_mesh = Default::Primitives::Quad;
		particleSys->m_forwardRendering = false;
		Transform transform;
		transform.m_value = glm::translate(glm::vec3(0.0f)) * glm::scale(glm::vec3(1.0f));
		foliageEntity.SetPrivateComponent(std::move(particleSys));
		foliageEntity.SetComponentData(transform);
		foliageEntity.SetComponentData(_DefaultFoliageInfo);
		foliageEntity.SetComponentData(ti);
		
	}
	auto& particleSys = foliageEntity.GetPrivateComponent<Particles>();
	particleSys->m_matrices.clear();
	std::vector<InternodeInfo> internodeInfos;
	std::mutex m;
	EntityManager::ForEach<InternodeInfo, TreeIndex>(JobManager::PrimaryWorkers(), TreeManager::GetInternodeQuery(), [&m, ti, &internodeInfos, this](int i, Entity internode, InternodeInfo& info, TreeIndex& index)
		{
			if (info.DistanceToBranchEnd > _DefaultFoliageInfo.DistanceLimit) return;
			if (ti.Value != index.Value) return;
			std::lock_guard<std::mutex> lock(m);
			internodeInfos.push_back(info);
		}
	);
	for (int i = 0; i < internodeInfos.size(); i++)
	{
		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(treeTransform.m_value * internodeInfos[i].GlobalTransform, scale, rotation, translation, skew, perspective);
		glm::vec3 parentTranslation = treeTransform.m_value * glm::vec4(internodeInfos[i].ParentTranslation, 1.0f);
		//x, ÏòÑôÖá£¬y: ºáÖá£¬z£ºroll
		glm::vec3 ls = _DefaultFoliageInfo.LeafSize;
		auto branchFront = rotation * glm::vec3(0, 0, -1);
		auto branchUp = rotation * glm::vec3(0, 1, 0);
		if (glm::abs(glm::dot(branchFront, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f) continue;
		for (int j = 0; j < _DefaultFoliageInfo.LeafAmount; j++)
		{
			glm::mat4 leafTransform;
			glm::vec3 position = glm::linearRand(glm::vec3(0.0f), parentTranslation - translation);
			position += glm::ballRand(_DefaultFoliageInfo.GenerationRadius);
			glm::quat rotation = glm::quat(glm::radians(glm::linearRand(glm::vec3(-180.0f), glm::vec3(180.0f))));
			//glm::quat rotation = glm::quatLookAt(glm::sphericalRand(1.0f), -glm::gaussRand(illuminations[i].LightDir, glm::vec3(0.01f)));
			leafTransform = glm::inverse(treeTransform.m_value) *
				(glm::translate(glm::mat4(1.0f), translation + position) * glm::mat4_cast(rotation) * glm::scale(ls));

			particleSys->m_matrices.push_back(leafTransform);
		}
	}
}

void TreeUtilities::BirchFoliageGenerator::OnGui()
{
	if (ImGui::Button("Regenerate")) Generate();
	ImGui::DragFloat("Distance Limit", &_DefaultFoliageInfo.DistanceLimit, 0.1f, 0);
	ImGui::DragFloat3("Leaf Size", (float*)(void*)&_DefaultFoliageInfo.LeafSize, 0.01f, 0);
	ImGui::DragInt("Leaf Amount", &_DefaultFoliageInfo.LeafAmount, 1, 0);
	ImGui::DragFloat("GenerationRadius", &_DefaultFoliageInfo.GenerationRadius, 0.01f);
}
