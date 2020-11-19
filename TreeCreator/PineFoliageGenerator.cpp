#include "PineFoliageGenerator.h"
#include "TreeManager.h"
TreeUtilities::PineFoliageGenerator::PineFoliageGenerator()
{
	_DefaultFoliageInfo = PineFoliageInfo();
	_Archetype = EntityManager::CreateEntityArchetype("Pine Foliage", LocalToParent(), LocalToWorld(), TreeIndex(), PineFoliageInfo());

	_LeafMaterial = std::make_shared<Material>();
	_LeafMaterial->SetMaterialProperty("material.shininess", 32.0f);
	_LeafMaterial->SetProgram(Default::GLPrograms::StandardInstancedProgram);
	_LeafSurfaceTex = AssetManager::LoadTexture("../Resources/Textures/Leaf/Pine/level0.png");
	_LeafMaterial->SetTexture(_LeafSurfaceTex, TextureType::DIFFUSE);
}

void TreeUtilities::PineFoliageGenerator::Generate(Entity tree)
{
	Entity foliageEntity;
	LocalToWorld treeTransform = EntityManager::GetComponentData<LocalToWorld>(tree);
	bool found = false;
	TreeIndex ti = EntityManager::GetComponentData<TreeIndex>(tree);
	EntityManager::ForEachChild(tree, [&found, &foliageEntity](Entity child)
		{
			if (child.HasComponentData<PineFoliageInfo>())
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
	std::mutex m;
	PineFoliageInfo pineFoliageInfo = foliageEntity.GetComponentData<PineFoliageInfo>();
	EntityManager::ForEach<InternodeInfo, TreeIndex>(TreeManager::GetInternodeQuery(), [&m, ti, &internodeInfos, &pineFoliageInfo](int i, Entity internode, InternodeInfo* info, TreeIndex* index)
		{
			if (info->Inhibitor > pineFoliageInfo.InhibitorLimit) return;
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
		//x, ÏòÑôÖá£¬y: ºáÖá£¬z£ºroll
		glm::vec3 ls = pineFoliageInfo.LeafSize;
		auto branchFront = rotation * glm::vec3(0, 0, -1);
		auto branchUp = rotation * glm::vec3(0, 1, 0);
		if (glm::abs(glm::dot(branchFront, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f) continue;
		for(int j = 0; j < pineFoliageInfo.SideLeafAmount; j++)
		{
			glm::mat4 leftTransform;
			glm::mat4 rightTransform;
			glm::vec3 position = (parentTranslation - translation) * (static_cast<float>(j) / pineFoliageInfo.SideLeafAmount);
			
			glm::vec3 leftFront = glm::cross(branchFront, glm::vec3(0.0f, 1.0f, 0.0f));
			leftFront = glm::rotate(leftFront, glm::radians(-glm::gaussRand(pineFoliageInfo.BendAngleMean, pineFoliageInfo.BendAngleVariance)), branchFront);
			glm::vec3 rightFront = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), branchFront);
			rightFront = glm::rotate(rightFront, glm::radians(glm::gaussRand(pineFoliageInfo.BendAngleMean, pineFoliageInfo.BendAngleVariance)), branchFront);

			leftTransform = glm::inverse(treeTransform.Value) *
				(glm::translate(glm::mat4(1.0f), translation + position - leftFront * pineFoliageInfo.LeafSize.z) * glm::mat4_cast(glm::quatLookAt(leftFront, glm::vec3(0.0f, 1.0f, 0.0f))) * glm::scale(ls));
			rightTransform = glm::inverse(treeTransform.Value) *
				(glm::translate(glm::mat4(1.0f), translation + position - rightFront * pineFoliageInfo.LeafSize.z) * glm::mat4_cast(glm::quatLookAt(rightFront, glm::vec3(0.0f, 1.0f, 0.0f))) * glm::scale(ls));

			particleSys->get()->Matrices.push_back(leftTransform);
			particleSys->get()->Matrices.push_back(rightTransform);
		}
		
	}
}

void TreeUtilities::PineFoliageGenerator::OnParamGui()
{
	ImGui::DragFloat("Inhibitor Limit", &_DefaultFoliageInfo.InhibitorLimit, 0.01f);
	ImGui::DragFloat3("Leaf Size", (float*)(void*)&_DefaultFoliageInfo.LeafSize, 0.01f);
	ImGui::DragInt("Side Leaf Amount", &_DefaultFoliageInfo.SideLeafAmount);
	ImGui::DragFloat("BendAngleMean", &_DefaultFoliageInfo.BendAngleMean, 0.01f);
	ImGui::DragFloat("BendAngleVar", &_DefaultFoliageInfo.BendAngleVariance, 0.01f);
}
