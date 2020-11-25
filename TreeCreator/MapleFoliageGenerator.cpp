#include "MapleFoliageGenerator.h"
#include "TreeManager.h"
TreeUtilities::MapleFoliageGenerator::MapleFoliageGenerator()
{
	_DefaultFoliageInfo = MapleFoliageInfo();
	_Archetype = EntityManager::CreateEntityArchetype("Pine Foliage", LocalToParent(), LocalToWorld(), TreeIndex(), MapleFoliageInfo());

	_LeafMaterial = std::make_shared<Material>();
	_LeafMaterial->SetMaterialProperty("material.shininess", 32.0f);
	_LeafMaterial->SetTransparentDiscard(true);
	_LeafMaterial->SetTransparentDiscardLimit(0.5f);
	_LeafMaterial->SetProgram(Default::GLPrograms::StandardInstancedProgram);
	_LeafSurfaceTex = AssetManager::LoadTexture("../Resources/Textures/Leaf/maple.png");
	_LeafMaterial->SetTexture(_LeafSurfaceTex, TextureType::DIFFUSE);
}

void TreeUtilities::MapleFoliageGenerator::Generate(Entity tree)
{
	Entity foliageEntity;
	LocalToWorld treeTransform = EntityManager::GetComponentData<LocalToWorld>(tree);
	bool found = false;
	TreeIndex ti = EntityManager::GetComponentData<TreeIndex>(tree);
	EntityManager::ForEachChild(tree, [&found, &foliageEntity](Entity child)
		{
			if (child.HasComponentData<MapleFoliageInfo>())
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
		LocalToParent ltp;
		ltp.Value = glm::translate(glm::vec3(0.0f)) * glm::scale(glm::vec3(1.0f));
		foliageEntity.SetPrivateComponent(std::move(particleSys));
		foliageEntity.SetComponentData(ltp);
		foliageEntity.SetComponentData(_DefaultFoliageInfo);
		foliageEntity.SetComponentData(ti);
		EntityManager::SetParent(foliageEntity, tree);
	}
	auto* particleSys = foliageEntity.GetPrivateComponent<Particles>();
	particleSys->get()->Matrices.clear();
	std::vector<InternodeInfo> internodeInfos;
	std::vector<Illumination> illuminations;
	std::mutex m;
	MapleFoliageInfo mapleFoliageInfo = foliageEntity.GetComponentData<MapleFoliageInfo>();
	EntityManager::ForEach<InternodeInfo, Illumination, TreeIndex>(TreeManager::GetInternodeQuery(), [&m, ti, &internodeInfos, &mapleFoliageInfo, &illuminations](int i, Entity internode, InternodeInfo* info, Illumination* illumination, TreeIndex* index)
		{
			if (info->Inhibitor > mapleFoliageInfo.InhibitorLimit) return;
			if (illumination->Value < mapleFoliageInfo.IlluminationLimit) return;
			if (ti.Value != index->Value) return;
			std::lock_guard<std::mutex> lock(m);
			internodeInfos.push_back(*info);
			illuminations.push_back(*illumination);
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
		//x, ÏòÑôÖá£¬y: ºáÖá£¬z£ºroll
		glm::vec3 ls = mapleFoliageInfo.LeafSize;
		auto branchFront = rotation * glm::vec3(0, 0, -1);
		auto branchUp = rotation * glm::vec3(0, 1, 0);
		if (glm::abs(glm::dot(branchFront, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f) continue;
		for (int j = 0; j < mapleFoliageInfo.LeafAmount; j++)
		{
			glm::mat4 leafTransform;
			
			glm::vec3 position = glm::linearRand(glm::vec3(0.0f), parentTranslation - translation);
			position += glm::ballRand(mapleFoliageInfo.GenerationRadius);
			//glm::quat rotation = glm::quat(glm::radians(glm::linearRand(glm::vec3(-180.0f), glm::vec3(180.0f))));
			glm::quat rotation = glm::quatLookAt(glm::sphericalRand(1.0f), -glm::gaussRand(illuminations[i].LightDir, glm::vec3(0.01f)));
			leafTransform = glm::inverse(treeTransform.Value) *
				(glm::translate(glm::mat4(1.0f), translation + position) * glm::mat4_cast(rotation) * glm::scale(ls));
				
			particleSys->get()->Matrices.push_back(leafTransform);
		}

	}
}

void TreeUtilities::MapleFoliageGenerator::OnParamGui()
{
	ImGui::DragFloat("Inhibitor Limit", &_DefaultFoliageInfo.InhibitorLimit, 0.01f, 0);
	ImGui::DragFloat("Illumination Limit", &_DefaultFoliageInfo.IlluminationLimit, 0.01f, 0);
	ImGui::DragFloat3("Leaf Size", (float*)(void*)&_DefaultFoliageInfo.LeafSize, 0.01f, 0);
	ImGui::DragInt("Leaf Amount", &_DefaultFoliageInfo.LeafAmount, 1, 0);
	ImGui::DragFloat("GenerationRadius", &_DefaultFoliageInfo.GenerationRadius, 0.01f);
}
