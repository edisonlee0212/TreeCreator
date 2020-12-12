#include "CakeTower.h"

glm::ivec2 CakeTower::SelectSlice(glm::vec3 position) const
{
	glm::ivec2 retVal = glm::ivec2(0);
	const float heightLevel = _MaxHeight / TierAmount;
	const float sliceAngle = 360.0f / SliceAmount;
	retVal.x = static_cast<int>(position.y / heightLevel);
	if (retVal.x >= TierAmount) retVal.x = TierAmount - 1;
	if (position.x == 0 && position.z == 0) retVal.y = 0;
	else retVal.y = static_cast<int>((glm::degrees(glm::atan(position.x, position.z)) + 180.0f) / sliceAngle);
	if (retVal.y >= SliceAmount) retVal.y = SliceAmount - 1;
	return retVal;
}

void CakeTower::GenerateMesh()
{
	_BoundMesh = std::make_shared<Mesh>();
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;

	std::vector<std::vector<glm::vec3>> rings;

	const float sliceAngle = 360.0f / SliceAmount;
	const int totalAngleStep = 3;
	const int totalLevelStep = 2;
	const float stepAngle = sliceAngle / (totalAngleStep - 1);
	const float heightLevel = _MaxHeight / TierAmount;
	const float stepLevel = heightLevel / (totalLevelStep - 1);
	rings.resize(TierAmount * totalLevelStep);
	vertices.resize(TierAmount * totalLevelStep * SliceAmount * totalAngleStep);

	indices.resize(6 * (TierAmount * totalLevelStep - 1) * SliceAmount * totalAngleStep);
	for (int tierIndex = 0; tierIndex < TierAmount; tierIndex++)
	{
		for (int levelStep = 0; levelStep < totalLevelStep; levelStep++) {
			const int actualLevelStep = tierIndex * totalLevelStep + levelStep;
			rings[actualLevelStep].resize(SliceAmount * totalAngleStep);
			for (int sliceIndex = 0; sliceIndex < SliceAmount; sliceIndex++)
			{
				for (int angleStep = 0; angleStep < totalAngleStep; angleStep++) {
					const int actualAngleStep = sliceIndex * totalAngleStep + angleStep;
					const float currentHeight = heightLevel * tierIndex + stepLevel * levelStep;
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
					rings[actualLevelStep][actualAngleStep] = position;
					vertices[actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep].Position = position;
					vertices[actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep].TexCoords0 = glm::vec2(0.0f);
				}
			}
		}
		for (int levelStep = 0; levelStep < totalLevelStep; levelStep++)
		{
			const int actualLevelStep = tierIndex * totalLevelStep + levelStep; //0-2
			if (actualLevelStep == TierAmount * totalLevelStep - 1) break;
			for (int sliceIndex = 0; sliceIndex < SliceAmount; sliceIndex++)
			{
				for (int angleStep = 0; angleStep < totalAngleStep; angleStep++) {
					const int actualAngleStep = sliceIndex * totalAngleStep + angleStep; //0-5
					//Fill a quad here.
					if (actualAngleStep < SliceAmount * totalAngleStep - 1) {
						indices[6 * (actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep)] = actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep;
						indices[6 * (actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep) + 1] = actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep + 1;
						indices[6 * (actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep) + 2] = (actualLevelStep + 1) * totalAngleStep * SliceAmount + actualAngleStep;

						indices[6 * (actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep) + 3] = (actualLevelStep + 1) * totalAngleStep * SliceAmount + actualAngleStep + 1;
						indices[6 * (actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep) + 4] = (actualLevelStep + 1) * totalAngleStep * SliceAmount + actualAngleStep;
						indices[6 * (actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep) + 5] = actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep + 1;
					}
					else
					{
						indices[6 * (actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep)] = actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep;
						indices[6 * (actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep) + 1] = actualLevelStep * totalAngleStep * SliceAmount;
						indices[6 * (actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep) + 2] = (actualLevelStep + 1) * totalAngleStep * SliceAmount + actualAngleStep;

						indices[6 * (actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep) + 3] = (actualLevelStep + 1) * totalAngleStep * SliceAmount;
						indices[6 * (actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep) + 4] = (actualLevelStep + 1) * totalAngleStep * SliceAmount + actualAngleStep;
						indices[6 * (actualLevelStep * totalAngleStep * SliceAmount + actualAngleStep) + 5] = actualLevelStep * totalAngleStep * SliceAmount;
					}
				}
			}
		}
	}

	_BoundMesh->SetVertices(17, vertices, indices);
	_MeshGenerated = true;
}

std::string CakeTower::Serialize()
{
	if (!_MeshGenerated) CalculateVolume();
	std::string output;
	output += std::to_string(TierAmount) + "\n";
	output += std::to_string(SliceAmount) + "\n";
	output += std::to_string(_MaxHeight) + "\n";
	int tierIndex = 0;
	for (const auto& tier : CakeTiers)
	{
		int sliceIndex = 0;
		for (const auto& slice : tier)
		{
			output += std::to_string(slice.MaxDistance);
			if(tierIndex != TierAmount - 1 || sliceIndex != SliceAmount - 1)
			{
				output += "\n";
			}
			sliceIndex++;
		}
		tierIndex++;
	}
	output += "\n";
	return output;
}

void CakeTower::Deserialize(const std::string& path)
{
	std::ifstream ifs;
	ifs.open(path.c_str());
	Debug::Log("Loading from " + path);
	if(ifs.is_open())
	{
		ifs >> TierAmount;
		ifs >> SliceAmount;
		ifs >> _MaxHeight;
		CakeTiers.resize(TierAmount);
		for (auto& tier : CakeTiers)
		{
			tier.resize(SliceAmount);
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
	internodeDataQuery.ToComponentDataArray<InternodeInfo, TreeIndex>(internodeInfos, [targetTreeIndex](TreeIndex& treeIndex)
		{
			return treeIndex.Value == targetTreeIndex.Value;
		}
	);
	_MaxHeight = 0;
	//_MaxRadius = 0;
	for (auto& i : internodeInfos)
	{
		const glm::vec3 position = i.GlobalTransform[3];
		if (position.y > _MaxHeight) _MaxHeight = position.y;
		const float radius = glm::length(glm::vec2(position.x, position.z));
		//if (radius > _MaxRadius) _MaxRadius = radius;
	}

	CakeTiers.resize(TierAmount);
	for (auto& tier : CakeTiers)
	{
		tier.resize(SliceAmount);
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
		if(currentDistance <= i.Thickness)
		{
			for(auto& slice : CakeTiers[sliceIndex.x])
			{
				if (slice.MaxDistance < currentDistance + i.Thickness) slice.MaxDistance = currentDistance + i.Thickness;
			}
		}
		else if (CakeTiers[sliceIndex.x][sliceIndex.y].MaxDistance < currentDistance) CakeTiers[sliceIndex.x][sliceIndex.y].MaxDistance = currentDistance;
	}
	GenerateMesh();
}

bool CakeTower::InVolume(glm::vec3 position) const
{
	const auto sliceIndex = SelectSlice(position);
	const float currentDistance = glm::length(glm::vec2(position.x, position.z));
	return CakeTiers[sliceIndex.x][sliceIndex.y].MaxDistance < currentDistance && position.y <= _MaxHeight;
}

void CakeTower::OnGui()
{
	ImGui::Checkbox("Prune Buds", &_PruneBuds);
	ImGui::Checkbox("Display bounds", &_Display);
	ImGui::ColorEdit4("Display Color", &DisplayColor.x);
	ImGui::DragFloat("Display Scale", &DisplayScale, 0.01f, 0.01f, 1.0f);
	bool edited = false;
	if(ImGui::DragInt("Tier Amount", &TierAmount, 1, 1, 100)) edited = true;
	if(ImGui::DragInt("Slice Amount", &SliceAmount, 1, 1, 100)) edited = true;
	if (ImGui::Button("Calculate Bounds") || edited) CalculateVolume();
	if (ImGui::Button("Save..."))
	{
		const std::string path = FileIO::SaveFile("CakeTower (*.ct)\0*.ct\0").value();
		if (!path.empty())
		{
			const std::string data = Serialize();
			std::ofstream ofs;
			ofs.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
			ofs.write(data.c_str(), data.length());
			ofs.flush();
			ofs.close();
		}
	}
	if(ImGui::Button("Load..."))
	{
		const std::string path = FileIO::OpenFile("CakeTower (*.ct)\0*.ct\0").value();
		if (!path.empty())
		{
			Deserialize(path);
		}
	}
	if (_Display && _MeshGenerated)
	{
		RenderManager::DrawGizmoMesh(_BoundMesh.get(), DisplayColor, GetOwner().GetComponentData<GlobalTransform>().Value);
	}
}
