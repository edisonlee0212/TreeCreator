#include "FoliageGeneratorBase.h"
#include "PlantSimulationSystem.h"
#include <gtx/matrix_decompose.hpp>

#include "TreeManager.h"

void TreeUtilities::DefaultFoliageGenerator::GenerateLeaves(Entity& internode, TreeParameters& treeParameters,
	glm::mat4& treeTransform, std::vector<glm::mat4>& leafTransforms, bool isLeft)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	Illumination internodeIllumination = EntityManager::GetComponentData<Illumination>(internode);
	auto internodeData = EntityManager::GetSharedComponent<InternodeData>(internode);
	internodeData->LeafLocalTransforms.clear();
	if (internodeIllumination.Value > treeParameters.LeafIlluminationLimit) {
		if (glm::linearRand(0.0f, 1.0f) >= internodeInfo.Inhibitor * treeParameters.LeafInhibitorFactor)
		{
			glm::vec3 translation;
			glm::quat rotation;
			glm::vec3 scale;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose(treeTransform * internodeInfo.GlobalTransform, scale, rotation, translation, skew, perspective);
			//x, 向阳轴，y: 横轴，z：roll
			glm::vec3 ls = glm::vec3(treeParameters.LeafSize.x, 1.0f, treeParameters.LeafSize.y);
			auto branchFront = rotation * glm::vec3(0, 0, -1);
			auto branchUp = rotation * glm::vec3(0, 1, 0);
			if (treeParameters.IsBothSide || isLeft)
			{
				for (int i = 0; i < treeParameters.SideLeafAmount; i++)
				{
					auto front = glm::rotate(branchFront,
						glm::radians(treeParameters.StartBendingAngle + i * treeParameters.BendingAngleIncrement), branchUp);
					auto up = branchUp;
					PlantSimulationSystem::ApplyTropism(internodeIllumination.LightDir, treeParameters.LeafPhotoTropism, up, front);
					PlantSimulationSystem::ApplyTropism(glm::vec3(0, -1, 0), treeParameters.LeafGravitropism, front, up);
					auto localPosition = treeParameters.LeafDistance * front;
					auto leafTransform = glm::translate(glm::mat4(1.0f), localPosition + translation) * glm::mat4_cast(glm::quatLookAt(front, up)) * glm::scale(ls);
					internodeData->LeafLocalTransforms.push_back(leafTransform);
					leafTransforms.push_back(leafTransform);
				}
			}
			if (treeParameters.IsBothSide || !isLeft)
			{
				for (int i = 0; i < treeParameters.SideLeafAmount; i++)
				{
					for (int i = 0; i < treeParameters.SideLeafAmount; i++)
					{
						auto front = glm::rotate(branchFront,
							glm::radians(-(treeParameters.StartBendingAngle + i * treeParameters.BendingAngleIncrement)), branchUp);
						auto up = branchUp;
						PlantSimulationSystem::ApplyTropism(internodeIllumination.LightDir, treeParameters.LeafPhotoTropism, up, front);
						PlantSimulationSystem::ApplyTropism(glm::vec3(0, -1, 0), treeParameters.LeafGravitropism, front, up);
						auto localPosition = treeParameters.LeafDistance * front;
						auto leafTransform = glm::translate(glm::mat4(1.0f), localPosition + translation) * glm::mat4_cast(glm::quatLookAt(front, up)) * glm::scale(ls);
						internodeData->LeafLocalTransforms.push_back(leafTransform);
						leafTransforms.push_back(leafTransform);
					}
				}
			}
			if (internodeInfo.Level == internodeInfo.MaxChildLevel)
			{
				auto front = branchFront;
				auto up = branchUp;
				PlantSimulationSystem::ApplyTropism(internodeIllumination.LightDir, treeParameters.LeafPhotoTropism, up, front);
				PlantSimulationSystem::ApplyTropism(glm::vec3(0, -1, 0), treeParameters.LeafGravitropism, front, up);
				auto localPosition = treeParameters.LeafDistance * front;
				auto leafTransform = glm::translate(glm::mat4(1.0f), localPosition + translation) * glm::mat4_cast(glm::quatLookAt(front, up)) * glm::scale(ls);
				internodeData->LeafLocalTransforms.push_back(leafTransform);
				leafTransforms.push_back(leafTransform);
			}
		}
	}
	EntityManager::ForEachChild(internode, [&treeParameters, &treeTransform, &leafTransforms, isLeft, this](Entity child)
		{
			GenerateLeaves(child, treeParameters, treeTransform, leafTransforms, !isLeft);
		});
}

void TreeUtilities::DefaultFoliageGenerator::Generate(Entity tree)
{
	TreeParameters treeParameters = EntityManager::GetComponentData<TreeParameters>(tree);
	LocalToWorld treeLocalToWorld = EntityManager::GetComponentData<LocalToWorld>(tree);
	auto immc = EntityManager::GetSharedComponent<InstancedMeshRenderer>(tree);
	GenerateLeaves(EntityManager::GetChildren(tree)[0], treeParameters, treeLocalToWorld.Value, immc->Matrices, true);
	Debug::Log(std::to_string(immc->Matrices.size()));
}

void TreeUtilities::DefaultFoliageGenerator::OnGui()
{
	
}

void AcaciaFoliageGenerator::Generate(Entity tree)
{
}

void AcaciaFoliageGenerator::OnGui()
{
}

void WillowFoliageGenerator::Generate(Entity tree)
{
}

void WillowFoliageGenerator::OnGui()
{
}
