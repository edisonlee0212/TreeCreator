#include "CakeTower.h"

glm::ivec2 CakeTower::SelectSlice(glm::vec3 position) const
{
	glm::ivec2 retVal = glm::ivec2(0);
	const float heightLevel = MaxHeight / LayerAmount;
	const float sliceAngle = 360.0f / SectorAmount;
	auto x = static_cast<int>(position.y / heightLevel);
	if (x < 0) x = 0;
	retVal.x = x;
	if (retVal.x >= LayerAmount) retVal.x = LayerAmount - 1;
	if (position.x == 0 && position.z == 0) retVal.y = 0;
	else retVal.y = static_cast<int>((glm::degrees(glm::atan(position.x, position.z)) + 180.0f) / sliceAngle);
	if (retVal.y >= SectorAmount) retVal.y = SectorAmount - 1;
	return retVal;
}

void CakeTower::GenerateMesh()
{
	_BoundMeshes.clear();
	if (CakeTiers.empty()) return;
	for (int tierIndex = 0; tierIndex < LayerAmount; tierIndex++)
	{
		auto mesh = std::make_shared<Mesh>();
		std::vector<Vertex> vertices;
		std::vector<unsigned> indices;

		const float sliceAngle = 360.0f / SectorAmount;
		const int totalAngleStep = 3;
		const int totalLevelStep = 2;
		const float stepAngle = sliceAngle / (totalAngleStep - 1);
		const float heightLevel = MaxHeight / LayerAmount;
		const float stepLevel = heightLevel / (totalLevelStep - 1);
		vertices.resize(totalLevelStep * SectorAmount * totalAngleStep * 2 + totalLevelStep);
		indices.resize((12 * (totalLevelStep - 1) * totalAngleStep) * SectorAmount);
		for (int levelStep = 0; levelStep < totalLevelStep; levelStep++) {
			const float currentHeight = heightLevel * tierIndex + stepLevel * levelStep;
			for (int sliceIndex = 0; sliceIndex < SectorAmount; sliceIndex++)
			{
				for (int angleStep = 0; angleStep < totalAngleStep; angleStep++) {
					const int actualAngleStep = sliceIndex * totalAngleStep + angleStep;

					float currentAngle = sliceAngle * sliceIndex + stepAngle * angleStep;
					if (currentAngle >= 360) currentAngle = 0;
					float x = glm::abs(glm::tan(glm::radians(currentAngle)));
					float z = 1.0f;
					if (currentAngle >= 0 && currentAngle <= 90)
					{
						z *= -1;
						x *= -1;
					}
					else if (currentAngle > 90 && currentAngle <= 180)
					{
						x *= -1;
					}
					else if (currentAngle > 270 && currentAngle <= 360)
					{
						z *= -1;
					}
					glm::vec3 position = glm::normalize(glm::vec3(x, 0.0f, z)) * CakeTiers[tierIndex][sliceIndex].MaxDistance;
					position.y = currentHeight;
					vertices[levelStep * totalAngleStep * SectorAmount + actualAngleStep].m_position = position;
					vertices[levelStep * totalAngleStep * SectorAmount + actualAngleStep].m_texCoords0 = glm::vec2((float)levelStep / (totalLevelStep - 1), (float)angleStep / (totalAngleStep - 1));
					vertices[levelStep * totalAngleStep * SectorAmount + actualAngleStep].m_normal = glm::normalize(position);
					vertices[totalLevelStep * SectorAmount * totalAngleStep + levelStep * totalAngleStep * SectorAmount + actualAngleStep].m_position = position;
					vertices[totalLevelStep * SectorAmount * totalAngleStep + levelStep * totalAngleStep * SectorAmount + actualAngleStep].m_texCoords0 = glm::vec2((float)levelStep / (totalLevelStep - 1), (float)angleStep / (totalAngleStep - 1));
					vertices[totalLevelStep * SectorAmount * totalAngleStep + levelStep * totalAngleStep * SectorAmount + actualAngleStep].m_normal = glm::vec3(0, levelStep == 0 ? -1 : 1, 0);
				}
			}
			vertices[vertices.size() - totalLevelStep + levelStep].m_position = glm::vec3(0, currentHeight, 0);
			vertices[vertices.size() - totalLevelStep + levelStep].m_normal = glm::vec3(0, levelStep == 0 ? -1 : 1, 0);
			vertices[vertices.size() - totalLevelStep + levelStep].m_texCoords0 = glm::vec2(0.0f);
		}
		for (int levelStep = 0; levelStep < totalLevelStep - 1; levelStep++)
		{
			for (int sliceIndex = 0; sliceIndex < SectorAmount; sliceIndex++)
			{
				for (int angleStep = 0; angleStep < totalAngleStep; angleStep++) {
					const int actualAngleStep = sliceIndex * totalAngleStep + angleStep; //0-5
					//Fill a quad here.
					if (actualAngleStep < SectorAmount * totalAngleStep - 1) {
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep)] = levelStep * totalAngleStep * SectorAmount + actualAngleStep;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 1] = levelStep * totalAngleStep * SectorAmount + actualAngleStep + 1;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 2] = (levelStep + 1) * totalAngleStep * SectorAmount + actualAngleStep;

						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 3] = (levelStep + 1) * totalAngleStep * SectorAmount + actualAngleStep + 1;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 4] = (levelStep + 1) * totalAngleStep * SectorAmount + actualAngleStep;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 5] = levelStep * totalAngleStep * SectorAmount + actualAngleStep + 1;
						//Connect with center here.
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 6] = totalLevelStep * SectorAmount * totalAngleStep + levelStep * totalAngleStep * SectorAmount + actualAngleStep;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 7] = vertices.size() - totalLevelStep + levelStep;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 8] = totalLevelStep * SectorAmount * totalAngleStep + levelStep * totalAngleStep * SectorAmount + actualAngleStep + 1;

						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 9] = totalLevelStep * SectorAmount * totalAngleStep + (levelStep + 1) * totalAngleStep * SectorAmount + actualAngleStep + 1;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 10] = vertices.size() - totalLevelStep + (levelStep + 1);
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 11] = totalLevelStep * SectorAmount * totalAngleStep + (levelStep + 1) * totalAngleStep * SectorAmount + actualAngleStep;
					}
					else
					{
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep)] = levelStep * totalAngleStep * SectorAmount + actualAngleStep;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 1] = levelStep * totalAngleStep * SectorAmount;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 2] = (levelStep + 1) * totalAngleStep * SectorAmount + actualAngleStep;

						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 3] = (levelStep + 1) * totalAngleStep * SectorAmount;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 4] = (levelStep + 1) * totalAngleStep * SectorAmount + actualAngleStep;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 5] = levelStep * totalAngleStep * SectorAmount;
						//Connect with center here.
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 6] = totalLevelStep * SectorAmount * totalAngleStep + levelStep * totalAngleStep * SectorAmount + actualAngleStep;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 7] = vertices.size() - totalLevelStep + levelStep;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 8] = totalLevelStep * SectorAmount * totalAngleStep + levelStep * totalAngleStep * SectorAmount;

						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 9] = totalLevelStep * SectorAmount * totalAngleStep + (levelStep + 1) * totalAngleStep * SectorAmount;
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 10] = vertices.size() - totalLevelStep + (levelStep + 1);
						indices[12 * (levelStep * totalAngleStep * SectorAmount + actualAngleStep) + 11] = totalLevelStep * SectorAmount * totalAngleStep + (levelStep + 1) * totalAngleStep * SectorAmount + actualAngleStep;
					}

				}
			}
		}
		mesh->SetVertices(19, vertices, indices);
		_BoundMeshes.push_back(std::move(mesh));
	}
	_MeshGenerated = true;
}

void CakeTower::FormEntity()
{
	if (!_MeshGenerated) CalculateVolume();
	if (!_MeshGenerated) return;
	auto children = EntityManager::GetChildren(GetOwner());
	for (auto& child : children)
	{
		if (child.HasComponentData<CakeTowerInfo>()) EntityManager::DeleteEntity(child);
	}
	children.clear();
	Transform transform;
	GlobalTransform globalTransform;
	transform.m_value = glm::translate(glm::vec3(0.0f)) * glm::scale(glm::vec3(1.0f));
	globalTransform.m_value = transform.m_value;
	const auto targetEntity = EntityManager::CreateEntity(_CakeTowerArchetype, "CakeTowers");
	targetEntity.SetComponentData(transform);
	targetEntity.SetComponentData(globalTransform);
	EntityManager::SetParent(targetEntity, GetOwner());
	
	for (int i = 0; i < _BoundMeshes.size(); i++)
	{
		auto slice = EntityManager::CreateEntity(_CakeTowerArchetype, "CakeTower" + std::to_string(i));
		auto mmc = std::make_unique<MeshRenderer>();
		mmc->m_material = _CakeTowerMaterial;
		mmc->m_forwardRendering = true;
		mmc->m_mesh = _BoundMeshes[i];
		slice.SetPrivateComponent(std::move(mmc));
		slice.SetComponentData(transform);
		slice.SetComponentData(globalTransform);
		EntityManager::SetParent(slice, targetEntity);
	}
}

CakeTower::CakeTower()
{
	_CakeTowerArchetype = EntityManager::CreateEntityArchetype("CakeTower", Transform(), GlobalTransform(), CakeTowerInfo());
	_CakeTowerMaterial = std::make_shared<Material>();
	_CakeTowerMaterial->SetProgram(Default::GLPrograms::StandardProgram);
	SetEnabled(true);
}

std::string CakeTower::Save()
{
	if (!_MeshGenerated) CalculateVolume();
	std::string output;
	output += std::to_string(LayerAmount) + "\n";
	output += std::to_string(SectorAmount) + "\n";
	output += std::to_string(MaxHeight) + "\n";
	output += std::to_string(MaxRadius) + "\n";
	int tierIndex = 0;
	for (const auto& tier : CakeTiers)
	{
		int sliceIndex = 0;
		for (const auto& slice : tier)
		{
			output += std::to_string(slice.MaxDistance);
			output += "\n";
			sliceIndex++;
		}
		tierIndex++;
	}
	output += "\n";
	for (const auto& tier : CakeTiers)
	{
		int sliceIndex = 0;
		for (const auto& slice : tier)
		{
			output += std::to_string(slice.MaxDistance);
			output += ",";
			sliceIndex++;
		}
		tierIndex++;
	}
	output += "\n";
	return output;
}

void CakeTower::ExportAsObj(const std::string& filename)
{
	if (!_MeshGenerated) CalculateVolume();
	auto& meshes = _BoundMeshes;
	
	std::ofstream of;
	of.open((filename + ".obj").c_str(), std::ofstream::out | std::ofstream::trunc);
	if (of.is_open())
	{
		std::string o = "o ";
		o += "RBV\n";
		of.write(o.c_str(), o.size());
		of.flush();
		std::string data;
		int offset = 1;
#pragma region Data collection
		for (auto& mesh : meshes) {
			for (const auto& vertex : mesh->GetVerticesUnsafe()) {
				data += "v " + std::to_string(vertex.m_position.x)
					+ " " + std::to_string(-vertex.m_position.y)
					+ " " + std::to_string(vertex.m_position.z)
					+ "\n";
			}
		}
		for (auto& mesh : meshes)
		{
			data += "# List of indices for faces vertices, with (x, y, z).\n";
			for (auto i = 0; i < mesh->GetIndicesUnsafe().size() / 3; i++) {
				auto f1 = mesh->GetIndicesUnsafe().at(3l * i) + offset;
				auto f2 = mesh->GetIndicesUnsafe().at(3l * i + 1) + offset;
				auto f3 = mesh->GetIndicesUnsafe().at(3l * i + 2) + offset;
				data += "f " + std::to_string(f1)
					+ " " + std::to_string(f2)
					+ " " + std::to_string(f3)
					+ "\n";
			}
			offset += mesh->GetVerticesUnsafe().size();
		}
#pragma endregion
		of.write(data.c_str(), data.size());
		of.flush();
	}
	
}

void CakeTower::Load(const std::string& path)
{
	std::ifstream ifs;
	ifs.open(path.c_str());
	Debug::Log("Loading from " + path);
	if (ifs.is_open())
	{
		ifs >> LayerAmount;
		ifs >> SectorAmount;
		ifs >> MaxHeight;
		ifs >> MaxRadius;
		CakeTiers.resize(LayerAmount);
		for (auto& tier : CakeTiers)
		{
			tier.resize(SectorAmount);
			for (auto& slice : tier)
			{
				ifs >> slice.MaxDistance;
			}
		}
		GenerateMesh();
	}

}

void CakeTower::CalculateVolume()
{
	Entity tree = GetOwner();
	EntityQuery internodeDataQuery = EntityManager::CreateEntityQuery();
	EntityManager::SetEntityQueryAllFilters(internodeDataQuery, InternodeInfo());
	std::vector<InternodeInfo> internodeInfos;
	TreeIndex targetTreeIndex = tree.GetComponentData<TreeIndex>();
	internodeDataQuery.ToComponentDataArray<InternodeInfo, TreeIndex>(internodeInfos, [targetTreeIndex](const TreeIndex& treeIndex)
		{
			return treeIndex.Value == targetTreeIndex.Value;
		}
	);
	MaxHeight = 0;
	MaxRadius = 0;
	for (auto& i : internodeInfos)
	{
		const glm::vec3 position = i.GlobalTransform[3];
		if (position.y > MaxHeight) MaxHeight = position.y;
		const float radius = glm::length(glm::vec2(position.x, position.z));
		if (radius > MaxRadius) MaxRadius = radius;
	}

	CakeTiers.resize(LayerAmount);
	for (auto& tier : CakeTiers)
	{
		tier.resize(SectorAmount);
		for (auto& slice : tier)
		{
			slice.MaxDistance = 0.0f;
		}
	}
	for (auto& i : internodeInfos)
	{
		const glm::vec3 position = i.GlobalTransform[3];
		const auto sliceIndex = SelectSlice(position);
		const float currentDistance = glm::length(glm::vec2(position.x, position.z));
		if (currentDistance <= i.Thickness)
		{
			for (auto& slice : CakeTiers[sliceIndex.x])
			{
				if (slice.MaxDistance < currentDistance + i.Thickness) slice.MaxDistance = currentDistance + i.Thickness;
			}
		}
		else if (CakeTiers[sliceIndex.x][sliceIndex.y].MaxDistance < currentDistance) CakeTiers[sliceIndex.x][sliceIndex.y].MaxDistance = currentDistance;
	}
	GenerateMesh();
}

void CakeTower::CalculateVolume(float maxHeight)
{
	Entity tree = GetOwner();
	EntityQuery internodeDataQuery = EntityManager::CreateEntityQuery();
	EntityManager::SetEntityQueryAllFilters(internodeDataQuery, InternodeInfo());
	std::vector<InternodeInfo> internodeInfos;
	TreeIndex targetTreeIndex = tree.GetComponentData<TreeIndex>();
	internodeDataQuery.ToComponentDataArray<InternodeInfo, TreeIndex>(internodeInfos, [targetTreeIndex](const TreeIndex& treeIndex)
		{
			return treeIndex.Value == targetTreeIndex.Value;
		}
	);
	MaxHeight = maxHeight;
	MaxRadius = 0;
	for (auto& i : internodeInfos)
	{
		const glm::vec3 position = i.GlobalTransform[3];
		const float radius = glm::length(glm::vec2(position.x, position.z));
		if (radius > MaxRadius) MaxRadius = radius;
	}

	CakeTiers.resize(LayerAmount);
	for (auto& tier : CakeTiers)
	{
		tier.resize(SectorAmount);
		for (auto& slice : tier)
		{
			slice.MaxDistance = 0.0f;
		}
	}
	const auto threadsAmount = JobManager::GetInstance().PrimaryWorkers().Size();
	std::vector<std::vector<std::vector<CakeSlice>>> tempCakeTowers;
	tempCakeTowers.resize(threadsAmount);
	for (int i = 0; i < threadsAmount; i++)
	{
		tempCakeTowers[i].resize(LayerAmount);
		for (auto& tier : tempCakeTowers[i])
		{
			tier.resize(SectorAmount);
			for (auto& slice : tier)
			{
				slice.MaxDistance = 0.0f;
			}
		}
	}
	std::vector<std::shared_future<void>> results;
	for (int threadIndex = 0; threadIndex < threadsAmount; threadIndex++)
	{
		results.push_back(JobManager::GetInstance().PrimaryWorkers().Push([&tempCakeTowers, threadIndex, &internodeInfos, this](int id)
			{
				auto& cakeTower = tempCakeTowers[threadIndex];
				for (auto& i : internodeInfos)
				{
					const glm::vec3 position = i.GlobalTransform[3];
					const auto sliceIndex = SelectSlice(position);
					const float currentDistance = glm::length(glm::vec2(position.x, position.z));
					if (currentDistance <= i.Thickness)
					{
						for (auto& slice : cakeTower[sliceIndex.x])
						{
							if (slice.MaxDistance < currentDistance + i.Thickness) slice.MaxDistance = currentDistance + i.Thickness;
						}
					}
					else if (cakeTower[sliceIndex.x][sliceIndex.y].MaxDistance < currentDistance) cakeTower[sliceIndex.x][sliceIndex.y].MaxDistance = currentDistance;
				}

			}
		).share());
	}
	for (const auto& i : results) i.wait();
	for (int threadIndex = 0; threadIndex < threadsAmount; threadIndex++)
	{
		auto& cakeTower = tempCakeTowers[threadIndex];
		for(int i = 0; i < LayerAmount; i++)
		{
			for(int j = 0; j < SectorAmount; j++)
			{
				if (CakeTiers[i][j].MaxDistance < cakeTower[i][j].MaxDistance) CakeTiers[i][j].MaxDistance = cakeTower[i][j].MaxDistance;
			}
		}
	}
	/*
	for (auto& i : internodeInfos)
	{
		const glm::vec3 position = i.GlobalTransform[3];
		const auto sliceIndex = SelectSlice(position);
		const float currentDistance = glm::length(glm::vec2(position.x, position.z));
		if (currentDistance <= i.Thickness)
		{
			for (auto& slice : CakeTiers[sliceIndex.x])
			{
				if (slice.MaxDistance < currentDistance + i.Thickness) slice.MaxDistance = currentDistance + i.Thickness;
			}
		}
		else if (CakeTiers[sliceIndex.x][sliceIndex.y].MaxDistance < currentDistance) CakeTiers[sliceIndex.x][sliceIndex.y].MaxDistance = currentDistance;
	}*/


	GenerateMesh();
}

bool CakeTower::InVolume(glm::vec3 position) const
{
	if (glm::any(glm::isnan(position))) return true;
	if (_MeshGenerated) {
		const auto sliceIndex = SelectSlice(position);
		const float currentDistance = glm::length(glm::vec2(position.x, position.z));
		return CakeTiers[sliceIndex.x][sliceIndex.y].MaxDistance >= currentDistance && position.y <= MaxHeight;
	}
	return true;
}

void CakeTower::OnGui()
{
	if(ImGui::Button("To OBJ"))
	{
		ExportAsObj("caketower");
	}
	ImGui::DragFloat("Remove Distance", &RemovalDistance, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("Attract Distance", &AttractionDistance, 0.01f, RemovalDistance, 10.0f);
	ImGui::DragInt("AP Count", &_AttractionPointsCount);
	ImGui::Checkbox("Enable Space Colonization", &EnableSpaceColonization);
	if (ImGui::Button("Generate attraction points"))
	{
		ClearAttractionPoints();
		GenerateAttractionPoints();
	}
	if (_Display)
	{
		const auto treeIndex = GetOwner().GetComponentData<TreeIndex>();
		std::vector<GlobalTransform> points;
		TreeManager::GetAttractionPointQuery().ToComponentDataArray(treeIndex, points);
		//RenderManager::DrawGizmoPointInstanced(glm::vec4(1.0f, 0.0f, 0.0f, 0.2f), (glm::mat4*)(void*)points.data(), points.size(), glm::mat4(1.0f), RemovalDistance / 3.0f);
		//RenderManager::DrawGizmoPointInstanced(glm::vec4(1.0f, 1.0f, 1.0f, 0.2f), (glm::mat4*)(void*)points.data(), points.size(), glm::mat4(1.0f), AttractionDistance / 2.0f);
	}
	ImGui::Checkbox("Prune Buds", &_PruneBuds);
	ImGui::Checkbox("Display bounds", &_Display);
	ImGui::ColorEdit4("Display Color", &DisplayColor.x);
	ImGui::DragFloat("Display Scale", &DisplayScale, 0.01f, 0.01f, 1.0f);
	bool edited = false;
	if (ImGui::DragInt("Layer Amount", &LayerAmount, 1, 1, 100)) edited = true;
	if (ImGui::DragInt("Slice Amount", &SectorAmount, 1, 1, 100)) edited = true;
	if (ImGui::Button("Calculate Bounds") || edited) CalculateVolume();
	if (ImGui::Button("Form Entity"))
	{
		FormEntity();
	}
	if (ImGui::Button("Save..."))
	{
		auto temp = FileIO::SaveFile("CakeTower (*.ct)\0*.ct\0");
		if (temp.has_value()) {
			const std::string path = temp.value();
			if (!path.empty())
			{
				const std::string data = Save();
				std::ofstream ofs;
				ofs.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
				ofs.write(data.c_str(), data.length());
				ofs.flush();
				ofs.close();
			}
		}
	}
	if (ImGui::Button("Load..."))
	{
		auto temp = FileIO::OpenFile("CakeTower (*.ct)\0*.ct\0");
		if (temp.has_value())
		{
			const std::string path = temp.value();
			if (!path.empty())
			{
				Load(path);
			}
		}

	}
	if (_Display && _MeshGenerated)
	{
		for (auto& i : _BoundMeshes) {
			RenderManager::DrawGizmoMesh(i.get(), DisplayColor, GetOwner().GetComponentData<GlobalTransform>().m_value);
		}
	}
}

void CakeTower::GenerateAttractionPoints()
{
	GenerateAttractionPoints(_AttractionPointsCount);
}

void CakeTower::GenerateAttractionPoints(int amount)
{
	if (!_MeshGenerated) CalculateVolume();
	glm::vec3 min = glm::vec3(-MaxRadius, 0, -MaxRadius);
	glm::vec3 max = glm::vec3(MaxRadius, MaxHeight, MaxRadius);
	TreeVolume::GenerateAttractionPoints(min, max, amount);
}
