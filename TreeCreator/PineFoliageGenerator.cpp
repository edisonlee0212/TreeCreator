#include "PineFoliageGenerator.h"
#include "TreeManager.h"
std::shared_ptr<Texture2D> TreeUtilities::PineFoliageGenerator::_LeafSurfaceTex = nullptr;

TreeUtilities::PineFoliageGenerator::PineFoliageGenerator()
{
	_DefaultFoliageInfo = PineFoliageInfo();
	_Archetype = EntityManager::CreateEntityArchetype("Pine Foliage", Transform(), GlobalTransform(), TreeIndex(), PineFoliageInfo());

	_LeafMaterial = std::make_shared<Material>();
	_LeafMaterial->Shininess = 32.0f;
	_LeafMaterial->SetProgram(Default::GLPrograms::StandardInstancedProgram);
	_LeafMaterial->AlphaDiscardEnabled = true;
	_LeafMaterial->AlphaDiscardOffset = 0.1f;
	_LeafMaterial->CullingMode = MaterialCullingMode::OFF;
	if (!_LeafSurfaceTex) _LeafSurfaceTex = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Leaf/Pine/level0.png");
	//_LeafMaterial->SetTexture(_LeafSurfaceTex);
	_LeafMaterial->AlbedoColor = glm::normalize(glm::vec3(60.0f / 256.0f, 140.0f / 256.0f, 0.0f));
	_LeafMaterial->Metallic = 0.0f;
	_LeafMaterial->Roughness = 0.3f;
	_LeafMaterial->AmbientOcclusion = glm::linearRand(0.5f, 0.8f);
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
		EntityManager::SetParent(foliageEntity, tree);
		auto particleSys = std::make_unique<Particles>();
		particleSys->Material = _LeafMaterial;
		particleSys->Mesh = Default::Primitives::Quad;
		particleSys->ForwardRendering = false;
		Transform ltp;
		ltp.Value = glm::translate(glm::vec3(0.0f)) * glm::scale(glm::vec3(1.0f));
		foliageEntity.SetPrivateComponent(std::move(particleSys));
		foliageEntity.SetComponentData(ltp);
		foliageEntity.SetComponentData(_DefaultFoliageInfo);
		foliageEntity.SetComponentData(ti);
		
	}
	auto& particleSys = foliageEntity.GetPrivateComponent<Particles>();
	particleSys->Matrices.clear();
	std::vector<InternodeInfo> internodeInfos;
	std::mutex m;
	EntityManager::ForEach<InternodeInfo, Illumination, TreeIndex>(TreeManager::GetInternodeQuery(), [&m, ti, &internodeInfos, this](int i, Entity internode, InternodeInfo* info, Illumination* illumination, TreeIndex* index)
		{
			if (info->AccumulatedLength > _DefaultFoliageInfo.LengthLimit) return;
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
				(glm::translate(glm::mat4(1.0f), translation + position - leftFront * _DefaultFoliageInfo.LeafSize.z) * glm::mat4_cast(glm::quatLookAt(leftFront, glm::sphericalRand(1.0f))) * glm::scale(ls));
			rightTransform = glm::inverse(treeTransform.Value) *
				(glm::translate(glm::mat4(1.0f), translation + position - rightFront * _DefaultFoliageInfo.LeafSize.z) * glm::mat4_cast(glm::quatLookAt(rightFront, glm::sphericalRand(1.0f))) * glm::scale(ls));

			if(!glm::any(glm::isnan(leftTransform[3])))particleSys->Matrices.push_back(leftTransform);
			if (!glm::any(glm::isnan(rightTransform[3])))particleSys->Matrices.push_back(rightTransform);
		}
	}
}

void TreeUtilities::PineFoliageGenerator::OnGui()
{
	if (ImGui::Button("Regenerate")) Generate();
	ImGui::DragFloat("Length Limit", &_DefaultFoliageInfo.LengthLimit, 0.01f, 0);
	ImGui::DragFloat3("Leaf Size", (float*)(void*)&_DefaultFoliageInfo.LeafSize, 0.01f, 0);
	ImGui::DragInt("Side Leaf Amount", &_DefaultFoliageInfo.SideLeafAmount, 1, 0);
	ImGui::DragFloat("BendAngleMean", &_DefaultFoliageInfo.BendAngleMean, 0.01f);
	ImGui::DragFloat("BendAngleVar", &_DefaultFoliageInfo.BendAngleVariance, 0.01f);
}
