#include "WillowFoliageGenerator.h"

#include <gtx/matrix_decompose.hpp>

#include "TreeManager.h"



TreeUtilities::WillowFoliageGenerator::WillowFoliageGenerator()
{
	_DefaultFoliageInfo = WillowFoliageInfo();
	_Archetype = EntityManager::CreateEntityArchetype("Willow Foliage", LocalToParent(), LocalToWorld(), TreeIndex(), WillowFoliageInfo());

	_BranchletMaterial = std::make_shared<Material>();
	_BranchletMaterial->SetMaterialProperty("material.shininess", 32.0f);
	_BranchletMaterial->SetProgram(Default::GLPrograms::StandardProgram);
	_BranchletSurfaceTex = AssetManager::LoadTexture("../Resources/Textures/BarkMaterial/Bark_Pine_baseColor.jpg");
	_BranchletMaterial->SetTexture(_BranchletSurfaceTex, TextureType::DIFFUSE);

	_LeafMaterial = std::make_shared<Material>();
	_LeafMaterial->SetMaterialProperty("material.shininess", 32.0f);
	_LeafMaterial->TransparentDiscard = true;
	_LeafMaterial->TransparentDiscardLimit = 0.2f;
	_LeafMaterial->CullingMode = MaterialCullingMode::OFF;
	_LeafMaterial->SetProgram(Default::GLPrograms::StandardInstancedProgram);
	_LeafSurfaceTex = AssetManager::LoadTexture("../Resources/Textures/Leaf/Willow/level0.png");
	_LeafMaterial->SetTexture(_LeafSurfaceTex, TextureType::DIFFUSE);
}

void TreeUtilities::WillowFoliageGenerator::Generate()
{
	const auto tree = GetOwner();
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
		mmc->ForwardRendering = true;
		
		auto particleSys = std::make_unique<Particles>();
		particleSys->Material = _LeafMaterial;
		particleSys->Mesh = Default::Primitives::Quad;
		particleSys->ForwardRendering = true;
		particleSys->ReceiveShadow = false;
		LocalToWorld ltp;
		ltp.Value = glm::translate(glm::vec3(0.0f)) * glm::scale(glm::vec3(1.0f));
		foliageEntity.SetPrivateComponent(std::move(mmc));
		foliageEntity.SetPrivateComponent(std::move(particleSys));
		foliageEntity.SetComponentData(ltp);
		foliageEntity.SetComponentData(_DefaultFoliageInfo);
		foliageEntity.SetComponentData(ti);
		EntityManager::SetParent(foliageEntity, tree);
	}
	auto& mmc = foliageEntity.GetPrivateComponent<MeshRenderer>();
	auto& particleSys = foliageEntity.GetPrivateComponent<Particles>();
	particleSys->Matrices.clear();
	std::vector<Entity> internodes;
	std::mutex m;
	EntityManager::ForEach<InternodeInfo, TreeIndex>(TreeManager::GetInternodeQuery(), [&m, ti, &internodes, this](int i, Entity internode, InternodeInfo* info, TreeIndex* index)
		{
			if (info->Inhibitor > _DefaultFoliageInfo.InhibitorLimit) return;
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
		float downDistance = glm::gaussRand(_DefaultFoliageInfo.DownDistanceMean, _DefaultFoliageInfo.DownDistanceVariance);
		float pushDistance = _DefaultFoliageInfo.PushDistance;
		if(translation.y - downDistance < _DefaultFoliageInfo.LowLimit)
		{
			float newDownDistance = translation.y - _DefaultFoliageInfo.LowLimit;
			pushDistance *= newDownDistance / downDistance;
			downDistance = newDownDistance;
		}
		translation = parentTranslation + glm::vec3(fromDir.x * pushDistance, -downDistance, fromDir.z * pushDistance);
		float distance = glm::distance(parentTranslation, translation);
		int amount = _DefaultFoliageInfo.SubdivisionAmount;
		if (amount % 2 != 0) amount++;
		BezierCurve curve = BezierCurve(parentTranslation, parentTranslation + distance / 3.0f * fromDir, translation - distance / 3.0f * dir, translation);
		float posStep = 1.0f / (float)amount;
		glm::vec3 dirStep = (dir - fromDir) / (float)amount;
		float thickness = _DefaultFoliageInfo.Thickness;
		for (int j = 1; j < amount; j++) {
			auto startPos = curve.GetPoint(posStep * (j - 1));
			auto endPos = curve.GetPoint(posStep * j);
			auto startDir = fromDir + (float)(j - 1) * dirStep;
			auto endDir = fromDir + (float)j * dirStep;
			branchlets[i].Rings.emplace_back(startPos, endPos,
				startDir, endDir,
				thickness, thickness);
		}
		if (amount > 1)branchlets[i].Rings.emplace_back(curve.GetPoint(1.0f - posStep), translation, dir - dirStep, dir, thickness, thickness);
		else branchlets[i].Rings.emplace_back(parentTranslation, translation, fromDir, dir, thickness, thickness);
		
		amount = _DefaultFoliageInfo.LeafAmount;
		posStep = 1.0f / (float)amount;
		dirStep = (dir - fromDir) / (float)amount;
		for (int j = 1; j < amount; j++) {
			auto endPos = curve.GetPoint(posStep * j);
			auto endDir = fromDir + (float)j * dirStep;
			auto currUp = glm::cross(endDir, branchlets[i].Normal);
			auto l = glm::rotate(endDir, glm::radians(glm::gaussRand(_DefaultFoliageInfo.BendAngleMean, _DefaultFoliageInfo.BendAngleVariance)), currUp);
			auto r = glm::rotate(endDir, glm::radians(-glm::gaussRand(_DefaultFoliageInfo.BendAngleMean, _DefaultFoliageInfo.BendAngleVariance)), currUp);
			glm::vec3 s = _DefaultFoliageInfo.LeafSize;// *(semantic ? 2.0f : 1.0f);
			branchlets[i].LeafLocalTransforms.push_back(glm::translate(endPos + s.z * 2.0f * l) * glm::mat4_cast(glm::quatLookAt(-l, currUp)) * glm::scale(s));
			branchlets[i].LeafLocalTransforms.push_back(glm::translate(endPos + s.z * 2.0f * r) * glm::mat4_cast(glm::quatLookAt(-r, currUp)) * glm::scale(s));
		}
	}
	for (int i = 0; i < internodes.size(); i++)
	{
		particleSys->Matrices.insert(particleSys->Matrices.end(), branchlets[i].LeafLocalTransforms.begin(), branchlets[i].LeafLocalTransforms.end());
	}
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;
	vertices.clear();
	indices.clear();
	for (int i = 0; i < internodes.size(); i++)
	{
		SimpleMeshGenerator(branchlets[i], vertices, indices);
	}
	mmc->Mesh = std::make_shared<Mesh>();
	mmc->Mesh->SetVertices(17, vertices, indices, true);
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

void TreeUtilities::WillowFoliageGenerator::OnGui()
{
	ImGui::DragFloat("Inhibitor Limit", &_DefaultFoliageInfo.InhibitorLimit, 0.01f);
	ImGui::DragFloat("Dist Mean", &_DefaultFoliageInfo.DownDistanceMean, 0.01f);
	ImGui::DragFloat("Dist Var", &_DefaultFoliageInfo.DownDistanceVariance, 0.01f);
	ImGui::DragFloat("Low Limit", &_DefaultFoliageInfo.LowLimit, 0.01f);
	ImGui::DragFloat("Push Dist", &_DefaultFoliageInfo.PushDistance, 0.01f);
	ImGui::DragFloat("Thickness", &_DefaultFoliageInfo.Thickness, 0.01f);
	ImGui::DragFloat("Bend Angle", &_DefaultFoliageInfo.BendAngleMean, 0.01f);
	ImGui::DragFloat("Bend Var", &_DefaultFoliageInfo.BendAngleVariance, 0.01f);
	ImGui::DragInt("Subdiv Amount", &_DefaultFoliageInfo.SubdivisionAmount);
	ImGui::DragInt("Leaf Amount", &_DefaultFoliageInfo.LeafAmount);
	ImGui::DragFloat3("Leaf Size", (float*)(void*)&_DefaultFoliageInfo.LeafSize, 0.01f);
}
