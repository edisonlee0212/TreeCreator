#include "WillowFoliageGenerator.h"

#include <gtx/matrix_decompose.hpp>

#include "TreeManager.h"



TreeUtilities::WillowFoliageGenerator::WillowFoliageGenerator()
{
	_Archetype = EntityManager::CreateEntityArchetype("Willow Foliage", LocalToParent(), LocalToWorld(), TreeIndex(), WillowFoliageInfo());

	_BranchletMaterial = std::make_shared<Material>();
	_BranchletMaterial->SetMaterialProperty("material.shininess", 32.0f);
	_BranchletMaterial->SetProgram(Default::GLPrograms::StandardProgram);
	_BranchletMaterial->SetTexture(_BranchletSurfaceTex, TextureType::DIFFUSE);

	_LeafMaterial = std::make_shared<Material>();
	_LeafMaterial->SetMaterialProperty("material.shininess", 32.0f);
	_LeafMaterial->SetProgram(Default::GLPrograms::StandardInstancedProgram);
	_LeafMaterial->SetTexture(_LeafSurfaceTex, TextureType::DIFFUSE);
}

void TreeUtilities::WillowFoliageGenerator::Generate(Entity tree)
{
	Entity foliageEntity;
	bool found = false;
	TreeIndex ti = EntityManager::GetComponentData<TreeIndex>(tree);
	EntityManager::ForEachChild(tree, [&found, &foliageEntity](Entity child)
		{
			if(child.HasComponentData<WillowFoliageInfo>())
			{
				found = true;
				foliageEntity = child;
			}
		}
	);
	if (!found)
	{
		foliageEntity = EntityManager::CreateEntity(_Archetype, "Foliage");
		auto mmc = std::make_unique<MeshRenderer>();
		mmc->Material = _BranchletMaterial;

		auto particleSys = std::make_unique<ParticleSystem>();
		particleSys->Material = _LeafMaterial;

		WillowFoliageInfo info;
		
		LocalToParent ltp;
		ltp.Value = glm::translate(glm::vec3(0.0f)) * glm::scale(glm::vec3(1.0f));
		foliageEntity.SetPrivateComponent(std::move(mmc));
		foliageEntity.SetPrivateComponent(std::move(particleSys));
		foliageEntity.SetComponentData(ltp);
		foliageEntity.SetComponentData(info);
		foliageEntity.SetComponentData(ti);
		EntityManager::SetParent(foliageEntity, tree);
	}
	auto* mmc = foliageEntity.GetPrivateComponent<MeshRenderer>();
	auto* particleSys = foliageEntity.GetPrivateComponent<ParticleSystem>();
	particleSys->get()->Matrices.clear();
	std::vector<Entity> internodes;
	std::mutex m;
	EntityManager::ForEach<InternodeInfo, TreeIndex>(TreeManager::GetInternodeQuery(), [&m, ti, &internodes](int i, Entity internode, InternodeInfo* info, TreeIndex* index)
		{
			if (!info->IsEndNode) return;
			if (ti.Value != index->Value) return;
			std::lock_guard<std::mutex> lock(m);
			internodes.push_back(internode);
		}
	);
	if (internodes.empty()) return;
	glm::mat4 treeTransform = EntityManager::GetComponentData<LocalToWorld>(tree).Value;
	std::vector<Branchlet> branchlets;
	branchlets.resize(internodes.size());
	for(int i = 0; i < internodes.size(); i++)
	{
		auto internodeInfo = internodes[i].GetComponentData<InternodeInfo>();
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(treeTransform * internodeInfo.GlobalTransform, scale, rotation, translation, skew, perspective);
		branchlets[i].Rings.clear();
		branchlets[i].LeafLocalTransforms.clear();
		glm::vec3 fromDir = rotation * glm::vec3(0, 0, -1);
		glm::vec3 up = rotation * glm::vec3(0, 1, 0);
		branchlets[i].Normal = glm::cross(up, fromDir);
		glm::vec3 dir = glm::vec3(0, -1, 0);
		glm::vec3 parentTranslation = translation;
		translation = parentTranslation + glm::vec3(fromDir.x, -2.0f, fromDir.z);
		float distance = glm::distance(parentTranslation, translation);
		int amount = 6;
		if (amount % 2 != 0) amount++;
		BezierCurve curve = BezierCurve(parentTranslation, parentTranslation + distance / 3.0f * fromDir, translation - distance / 3.0f * dir, translation);
		float posStep = 1.0f / (float)amount;
		glm::vec3 dirStep = (dir - fromDir) / (float)amount;
		float thickness = 0.01f;
		
		for (int j = 1; j < amount; j++) {
			branchlets[i].Rings.emplace_back(
				curve.GetPoint(posStep * (j - 1)), curve.GetPoint(posStep * j),
				fromDir + (float)(j - 1) * dirStep, fromDir + (float)j * dirStep,
				thickness, thickness);
		}
		if (amount > 1)branchlets[i].Rings.emplace_back(curve.GetPoint(1.0f - posStep), translation, dir - dirStep, dir, thickness, thickness);
		else branchlets[i].Rings.emplace_back(parentTranslation, translation, fromDir, dir, thickness, thickness);
	}

	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;
	vertices.clear();
	indices.clear();
	for (int i = 0; i < internodes.size(); i++)
	{
		SimpleMeshGenerator(branchlets[i], vertices, indices);
	}
	mmc->get()->Mesh = std::make_shared<Mesh>();
	mmc->get()->Mesh->SetVertices(17, vertices, indices, true);
}

void TreeUtilities::WillowFoliageGenerator::SimpleMeshGenerator(Branchlet& branchlet, std::vector<Vertex>& vertices,
	std::vector<unsigned>& indices)
{
	glm::vec3 normal = branchlet.Normal;

	auto rings = branchlet.Rings;
	int step = 4;
	float angleStep = 360.0f / (float)(step);
	int vertexIndex = vertices.size();
	Vertex archetype;
	float textureXstep = 1.0f / step * 4.0f;
	for (int i = 0; i < step; i++) {
		archetype.Position = rings.at(0).GetPoint(normal, angleStep * i, true);
		float x = i < (step / 2) ? i * textureXstep : (step - i) * textureXstep;
		archetype.TexCoords0 = glm::vec2(x, 0.0f);
		vertices.push_back(archetype);
	}
	int ringSize = rings.size();
	float textureYstep = 1.0f / ringSize * 2.0f;
	for (int ringIndex = 0; ringIndex < ringSize; ringIndex++) {
		for (int i = 0; i < step; i++) {
			archetype.Position = rings.at(ringIndex).GetPoint(normal, angleStep * i, false);
			float x = i < (step / 2) ? i * textureXstep : (step - i) * textureXstep;
			float y = ringIndex < (ringSize / 2) ? (ringIndex + 1) * textureYstep : (ringSize - ringIndex - 1) * textureYstep;
			archetype.TexCoords0 = glm::vec2(x, y);
			vertices.push_back(archetype);
		}
		for (int i = 0; i < step - 1; i++) {
			//Down triangle
			indices.push_back(vertexIndex + ringIndex * step + i);
			indices.push_back(vertexIndex + ringIndex * step + i + 1);
			indices.push_back(vertexIndex + (ringIndex + 1) * step + i);
			//Up triangle
			indices.push_back(vertexIndex + (ringIndex + 1) * step + i + 1);
			indices.push_back(vertexIndex + (ringIndex + 1) * step + i);
			indices.push_back(vertexIndex + ringIndex * step + i + 1);
		}
		//Down triangle
		indices.push_back(vertexIndex + ringIndex * step + step - 1);
		indices.push_back(vertexIndex + ringIndex * step);
		indices.push_back(vertexIndex + (ringIndex + 1) * step + step - 1);
		//Up triangle
		indices.push_back(vertexIndex + (ringIndex + 1) * step);
		indices.push_back(vertexIndex + (ringIndex + 1) * step + step - 1);
		indices.push_back(vertexIndex + ringIndex * step);
	}
}

void TreeUtilities::WillowFoliageGenerator::OnParamGui()
{
}
