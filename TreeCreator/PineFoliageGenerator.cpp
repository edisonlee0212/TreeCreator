#include "PineFoliageGenerator.h"
#include "TreeManager.h"
TreeUtilities::PineFoliageGenerator::PineFoliageGenerator()
{
	_DefaultFoliageInfo = PineFoliageInfo();
	_Archetype = EntityManager::CreateEntityArchetype("Pine Foliage", Transform(), GlobalTransform(), TreeIndex(), PineFoliageInfo());

	_LeafMaterial = std::make_shared<Material>();
	_LeafMaterial->SetMaterialProperty("material.shininess", 32.0f);
	_LeafMaterial->SetProgram(Default::GLPrograms::StandardInstancedProgram);
	_LeafMaterial->TransparentDiscard = true;
	_LeafMaterial->TransparentDiscardLimit = 0.1f;
	_LeafMaterial->CullingMode = MaterialCullingMode::OFF;
	_LeafSurfaceTex = FileManager::LoadTexture("../Resources/Textures/Leaf/Pine/level0.png");
	_LeafMaterial->SetTexture(_LeafSurfaceTex, TextureType::DIFFUSE);
}

void TreeUtilities::PineFoliageGenerator::Generate()
{
	const auto tree = GetOwner();
	Entity foliageEntity;
	GlobalTransform treeTransform = EntityManager::GetComponentData<GlobalTransform>(tree);
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
		particleSys->ReceiveShadow = false;
		Transform ltp;
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
	EntityManager::ForEach<InternodeInfo, Illumination, TreeIndex>(TreeManager::GetInternodeQuery(), [&m, ti, &internodeInfos, this](int i, Entity internode, InternodeInfo* info, Illumination* illumination, TreeIndex* index)
		{
			if (info->Inhibitor > _DefaultFoliageInfo.InhibitorLimit) return;
			if (illumination->Value < _DefaultFoliageInfo.IlluminationLimit) return;
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
		glm::vec3 ls = _DefaultFoliageInfo.LeafSize;
		auto branchFront = rotation * glm::vec3(0, 0, -1);
		auto branchUp = rotation * glm::vec3(0, 1, 0);
		if (glm::abs(glm::dot(branchFront, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f) continue;
		for(int j = 0; j < _DefaultFoliageInfo.SideLeafAmount; j++)
		{
			glm::mat4 leftTransform;
			glm::mat4 rightTransform;
			glm::vec3 position = (parentTranslation - translation) * (static_cast<float>(j) / _DefaultFoliageInfo.SideLeafAmount);
			
			glm::vec3 leftFront = glm::cross(branchFront, glm::vec3(0.0f, 1.0f, 0.0f));
			leftFront = glm::rotate(leftFront, glm::radians(-glm::gaussRand(_DefaultFoliageInfo.BendAngleMean, _DefaultFoliageInfo.BendAngleVariance)), branchFront);
			glm::vec3 rightFront = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), branchFront);
			rightFront = glm::rotate(rightFront, glm::radians(glm::gaussRand(_DefaultFoliageInfo.BendAngleMean, _DefaultFoliageInfo.BendAngleVariance)), branchFront);

			leftTransform = glm::inverse(treeTransform.Value) *
				(glm::translate(glm::mat4(1.0f), translation + position - leftFront * _DefaultFoliageInfo.LeafSize.z) * glm::mat4_cast(glm::quatLookAt(leftFront, glm::vec3(0.0f, 1.0f, 0.0f))) * glm::scale(ls));
			rightTransform = glm::inverse(treeTransform.Value) *
				(glm::translate(glm::mat4(1.0f), translation + position - rightFront * _DefaultFoliageInfo.LeafSize.z) * glm::mat4_cast(glm::quatLookAt(rightFront, glm::vec3(0.0f, 1.0f, 0.0f))) * glm::scale(ls));

			particleSys->Matrices.push_back(leftTransform);
			particleSys->Matrices.push_back(rightTransform);
		}
		
	}
}

void TreeUtilities::PineFoliageGenerator::OnGui()
{
	if (ImGui::Button("Regenerate")) Generate();
	ImGui::DragFloat("Inhibitor Limit", &_DefaultFoliageInfo.InhibitorLimit, 0.01f, 0);
	ImGui::DragFloat("Illumination Limit", &_DefaultFoliageInfo.IlluminationLimit, 0.01f, 0);
	ImGui::DragFloat3("Leaf Size", (float*)(void*)&_DefaultFoliageInfo.LeafSize, 0.01f, 0);
	ImGui::DragInt("Side Leaf Amount", &_DefaultFoliageInfo.SideLeafAmount, 1, 0);
	ImGui::DragFloat("BendAngleMean", &_DefaultFoliageInfo.BendAngleMean, 0.01f);
	ImGui::DragFloat("BendAngleVar", &_DefaultFoliageInfo.BendAngleVariance, 0.01f);
}
