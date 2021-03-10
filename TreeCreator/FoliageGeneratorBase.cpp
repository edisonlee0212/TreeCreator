﻿#include "FoliageGeneratorBase.h"
#include "PlantSimulationSystem.h"
#include <gtx/matrix_decompose.hpp>

#include "TreeManager.h"
std::shared_ptr<Texture2D> TreeUtilities::DefaultFoliageGenerator::_LeafSurfaceTex = nullptr;

void TreeUtilities::DefaultFoliageGenerator::GenerateLeaves(Entity& internode, 
	glm::mat4& treeTransform, std::vector<glm::mat4>& leafTransforms, bool isLeft)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	Illumination internodeIllumination = EntityManager::GetComponentData<Illumination>(internode);
	if (internodeIllumination.Value > _DefaultFoliageInfo.LeafIlluminationLimit) {
		if (glm::linearRand(0.0f, 1.0f) >= internodeInfo.Inhibitor * _DefaultFoliageInfo.LeafInhibitorFactor)
		{
			glm::vec3 translation;
			glm::quat rotation;
			glm::vec3 scale;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose(treeTransform * internodeInfo.GlobalTransform, scale, rotation, translation, skew, perspective);
			//x, 向阳轴，y: 横轴，z：roll
			glm::vec3 ls = glm::vec3(_DefaultFoliageInfo.LeafSize.x, 1.0f, _DefaultFoliageInfo.LeafSize.y);
			auto branchFront = rotation * glm::vec3(0, 0, -1);
			auto branchUp = rotation * glm::vec3(0, 1, 0);
			if (_DefaultFoliageInfo.IsBothSide || isLeft)
			{
				for (int i = 0; i < _DefaultFoliageInfo.SideLeafAmount; i++)
				{
					auto front = glm::rotate(branchFront,
						glm::radians(_DefaultFoliageInfo.StartBendingAngle + i * _DefaultFoliageInfo.BendingAngleIncrement), branchUp);
					auto up = branchUp;
					PlantSimulationSystem::ApplyTropism(internodeIllumination.LightDir, _DefaultFoliageInfo.LeafPhotoTropism, up, front);
					PlantSimulationSystem::ApplyTropism(glm::vec3(0, -1, 0), _DefaultFoliageInfo.LeafGravitropism, front, up);
					auto localPosition = _DefaultFoliageInfo.LeafDistance * front;
					auto leafTransform = glm::translate(glm::mat4(1.0f), localPosition + translation) * glm::mat4_cast(glm::quatLookAt(front, up)) * glm::scale(ls);
					leafTransform = glm::inverse(treeTransform) * leafTransform;
					leafTransforms.push_back(leafTransform);
				}
			}
			if (_DefaultFoliageInfo.IsBothSide || !isLeft)
			{
				for (int i = 0; i < _DefaultFoliageInfo.SideLeafAmount; i++)
				{
					for (int i = 0; i < _DefaultFoliageInfo.SideLeafAmount; i++)
					{
						auto front = glm::rotate(branchFront,
							glm::radians(-(_DefaultFoliageInfo.StartBendingAngle + i * _DefaultFoliageInfo.BendingAngleIncrement)), branchUp);
						auto up = branchUp;
						PlantSimulationSystem::ApplyTropism(internodeIllumination.LightDir, _DefaultFoliageInfo.LeafPhotoTropism, up, front);
						PlantSimulationSystem::ApplyTropism(glm::vec3(0, -1, 0), _DefaultFoliageInfo.LeafGravitropism, front, up);
						auto localPosition = _DefaultFoliageInfo.LeafDistance * front;
						auto leafTransform = glm::translate(glm::mat4(1.0f), localPosition + translation) * glm::mat4_cast(glm::quatLookAt(front, up)) * glm::scale(ls);
						leafTransform = glm::inverse(treeTransform) * leafTransform;
						leafTransforms.push_back(leafTransform);
					}
				}
			}
			if (internodeInfo.Level == internodeInfo.MaxChildLevel)
			{
				auto front = branchFront;
				auto up = branchUp;
				PlantSimulationSystem::ApplyTropism(internodeIllumination.LightDir, _DefaultFoliageInfo.LeafPhotoTropism, up, front);
				PlantSimulationSystem::ApplyTropism(glm::vec3(0, -1, 0), _DefaultFoliageInfo.LeafGravitropism, front, up);
				auto localPosition = _DefaultFoliageInfo.LeafDistance * front;
				auto leafTransform = glm::translate(glm::mat4(1.0f), localPosition + translation) * glm::mat4_cast(glm::quatLookAt(front, up)) * glm::scale(ls);
				leafTransform = glm::inverse(treeTransform) * leafTransform;
				leafTransforms.push_back(leafTransform);
			}
		}
	}
	EntityManager::ForEachChild(internode, [&treeTransform, &leafTransforms, isLeft, this](Entity child)
		{
			GenerateLeaves(child, treeTransform, leafTransforms, !isLeft);
		});
}

DefaultFoliageGenerator::DefaultFoliageGenerator()
{
	_DefaultFoliageInfo = DefaultFoliageInfo();
	_Archetype = EntityManager::CreateEntityArchetype("Pine Foliage", Transform(), GlobalTransform(), TreeIndex(), DefaultFoliageInfo());

	_LeafMaterial = std::make_shared<Material>();
	_LeafMaterial->m_shininess = 32.0f;
	_LeafMaterial->SetProgram(Default::GLPrograms::StandardInstancedProgram);
	_LeafMaterial->m_alphaDiscardEnabled = true;
	_LeafMaterial->m_alphaDiscardOffset = 0.1f;
	_LeafMaterial->m_cullingMode = MaterialCullingMode::Off;
	if (!_LeafSurfaceTex) _LeafSurfaceTex = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Leaf/Pine/level0.png");
	//_LeafMaterial->SetTexture(_LeafSurfaceTex);
	_LeafMaterial->m_albedoColor = glm::normalize(glm::vec3(60.0f / 256.0f, 140.0f / 256.0f, 0.0f));
	_LeafMaterial->m_metallic = 0.0f;
	_LeafMaterial->m_roughness = 0.3f;
	_LeafMaterial->m_ambientOcclusion = glm::linearRand(0.4f, 0.8f);

}

void TreeUtilities::DefaultFoliageGenerator::Generate()
{
	const auto tree = GetOwner();
	GlobalTransform treeTransform = EntityManager::GetComponentData<GlobalTransform>(tree);
	Entity foliageEntity;
	bool found = false;
	TreeIndex ti = EntityManager::GetComponentData<TreeIndex>(tree);
	EntityManager::ForEachChild(tree, [&found, &foliageEntity](Entity child)
		{
			if (child.HasComponentData<DefaultFoliageInfo>())
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
	GenerateLeaves(EntityManager::GetChildren(tree)[0], treeTransform.m_value, particleSys->m_matrices, true);
}

void TreeUtilities::DefaultFoliageGenerator::OnGui()
{
	if (ImGui::Button("Regenerate")) Generate();
	ImGui::DragFloat2("Leaf Size XY", (float*)(void*)&_DefaultFoliageInfo.LeafSize, 0.01f);
	ImGui::DragFloat("LeafIlluminationLimit", &_DefaultFoliageInfo.LeafIlluminationLimit, 0.01f);
	ImGui::DragFloat("LeafInhibitorFactor", &_DefaultFoliageInfo.LeafInhibitorFactor, 0.01f);
	ImGui::Checkbox("IsBothSide", &_DefaultFoliageInfo.IsBothSide);
	ImGui::DragInt("SideLeafAmount", &_DefaultFoliageInfo.SideLeafAmount, 0.01f);
	ImGui::DragFloat("StartBendingAngle", &_DefaultFoliageInfo.StartBendingAngle, 0.01f);
	ImGui::DragFloat("BendingAngleIncrement", &_DefaultFoliageInfo.BendingAngleIncrement, 0.01f);
	ImGui::DragFloat("LeafPhotoTropism", &_DefaultFoliageInfo.LeafPhotoTropism, 0.01f);
	ImGui::DragFloat("LeafGravitropism", &_DefaultFoliageInfo.LeafGravitropism, 0.01f);
	ImGui::DragFloat("LeafDistance", &_DefaultFoliageInfo.LeafDistance, 0.01f);
}

