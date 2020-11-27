#include "OakFoliageGenerator.h"

#include "PlantSimulationSystem.h"


TreeUtilities::OakFoliageGenerator::OakFoliageGenerator()

{
	_DefaultFoliageInfo = OakFoliageInfo();
	_Archetype = EntityManager::CreateEntityArchetype("Oak Foliage", LocalToParent(), LocalToWorld(), TreeIndex(), OakFoliageInfo());

	_LeafMaterial = std::make_shared<Material>();
	_LeafMaterial->SetMaterialProperty("material.shininess", 32.0f);
	_LeafMaterial->SetProgram(Default::GLPrograms::StandardInstancedProgram);
	_LeafMaterial->SetTransparentDiscard(true);
	_LeafMaterial->SetTransparentDiscardLimit(0.7f);
	_LeafSurfaceTex = AssetManager::LoadTexture("../Resources/Textures/Leaf/PrunusAvium/A/level0.png");
	_LeafMaterial->SetTexture(_LeafSurfaceTex, TextureType::DIFFUSE);
}


void TreeUtilities::OakFoliageGenerator::Generate()
{
	const auto tree = GetOwner();
	LocalToWorld treeLocalToWorld = EntityManager::GetComponentData<LocalToWorld>(tree);
	Entity foliageEntity;
	LocalToWorld treeTransform = EntityManager::GetComponentData<LocalToWorld>(tree);
	bool found = false;
	TreeIndex ti = EntityManager::GetComponentData<TreeIndex>(tree);
	EntityManager::ForEachChild(tree, [&found, &foliageEntity](Entity child)
		{
			if (child.HasComponentData<OakFoliageInfo>())
			{
				found = true;
				foliageEntity = child;
			}
		}
	);
	if (!found)
	{
		foliageEntity = EntityManager::CreateEntity(_Archetype, "Foliage");
		auto particleSys = std::make_unique<Particles>();
		particleSys->Material = _LeafMaterial;
		particleSys->Mesh = Default::Primitives::Quad;
		particleSys->ForwardRendering = true;
		particleSys->BackCulling = false;
		particleSys->ReceiveShadow = false;
		LocalToParent ltp;
		ltp.Value = glm::translate(glm::vec3(0.0f)) * glm::scale(glm::vec3(1.0f));
		foliageEntity.SetPrivateComponent(std::move(particleSys));
		foliageEntity.SetComponentData(ltp);
		foliageEntity.SetComponentData(_DefaultFoliageInfo);
		foliageEntity.SetComponentData(ti);
		EntityManager::SetParent(foliageEntity, tree);
	}
	auto& particleSys = foliageEntity.GetPrivateComponent<Particles>();
	particleSys->Matrices.clear();
	std::vector<InternodeInfo> internodeInfos;
	std::mutex m;
	
	EntityManager::ForEach<InternodeInfo, TreeIndex>(TreeManager::GetInternodeQuery(), [&m, ti, &internodeInfos, this](int i, Entity internode, InternodeInfo* info, TreeIndex* index)
		{
			if (info->Order < _DefaultFoliageInfo.OrderLimit) return;
			if (ti.Value != index->Value) return;
			std::lock_guard<std::mutex> lock(m);
			internodeInfos.push_back(*info);
		}
	);
	for (int i = 0; i < internodeInfos.size(); i++)
	{
		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(treeTransform.Value * internodeInfos[i].GlobalTransform, scale, rotation, translation, skew, perspective);
		glm::vec3 parentTranslation = treeTransform.Value * glm::vec4(internodeInfos[i].ParentTranslation, 1.0f);
		//x, �����ᣬy: ���ᣬz��roll
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
			leafTransform = glm::inverse(treeTransform.Value) *
				(glm::translate(glm::mat4(1.0f), translation + position) * glm::mat4_cast(rotation) * glm::scale(ls));

			particleSys->Matrices.push_back(leafTransform);
		}
	}
}

void TreeUtilities::OakFoliageGenerator::OnGui()
{
	ImGui::DragInt("Order Limit", &_DefaultFoliageInfo.OrderLimit, 1.0f, 0);
	ImGui::DragFloat3("Leaf Size", (float*)(void*)&_DefaultFoliageInfo.LeafSize, 0.01f, 0);
	ImGui::DragInt("Leaf Amount", &_DefaultFoliageInfo.LeafAmount, 1, 0);
	ImGui::DragFloat("GenerationRadius", &_DefaultFoliageInfo.GenerationRadius, 0.01f);
}