#include "CakeTower.h"

glm::ivec2 CakeTower::SelectSlice(glm::vec3 position) const
{
	glm::ivec2 retVal = glm::ivec2(0);
	const float heightLevel = _MaxHeight / TierAmount;
	const float sliceAngle = 360.0f / SliceAmount;
	retVal.x = position.y / heightLevel;
	if (retVal.x >= TierAmount) retVal.x = TierAmount - 1;
	retVal.y = (glm::degrees(glm::atan(position.x, position.z)) + 180.0f) / sliceAngle;
	if (retVal.y >= SliceAmount) retVal.y = SliceAmount - 1;
	return retVal;
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
	_MaxRadius = 0;
	for (auto& i : internodeInfos)
	{
		const glm::vec3 position = i.GlobalTransform[3];
		if (position.y > _MaxHeight) _MaxHeight = position.y;
		const float radius = glm::length(glm::vec2(position.x, position.z));
		if (radius > _MaxRadius) _MaxRadius = radius;
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
		const float currentDistance = glm::length(glm::vec2(position.x, position.z)) + i.Thickness;
		if (CakeTiers[sliceIndex.x][sliceIndex.y].MaxDistance < currentDistance) CakeTiers[sliceIndex.x][sliceIndex.y].MaxDistance = currentDistance;
	}
}

bool CakeTower::InVolume(glm::vec3 position) const
{
	const auto sliceIndex = SelectSlice(position);
	const float currentDistance = glm::length(glm::vec2(position.x, position.z));
	return CakeTiers[sliceIndex.x][sliceIndex.y].MaxDistance < currentDistance;
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
	if (_Display)
	{
		float tierIndex = 0;
		std::vector<glm::mat4> models;
		const float heightLevel = _MaxHeight / TierAmount;
		const float sliceAngle = 360.0f / SliceAmount;
		for (auto& tier : CakeTiers)
		{
			float sliceIndex = 0;
			tier.resize(SliceAmount);
			for (auto& slice : tier)
			{
				const float currentHeight = heightLevel * (tierIndex + 0.5f);
				const float currentAngle = sliceAngle * (sliceIndex + 0.5f);
				float x = glm::abs(glm::tan(glm::radians(currentAngle)));
				float z = 1.0f;
				if (currentAngle > 0 && currentAngle <= 90)
				{
					z *= -1;
					x *= -1;
				}else if (currentAngle > 90 && currentAngle <= 180)
				{
					x *= -1;
				} else if (currentAngle > 270 && currentAngle <= 360)
				{
					z *= -1;
				}
				glm::vec3 direction = glm::normalize(glm::vec3(x, 0.0f, z)) * slice.MaxDistance;
				
				glm::vec3 front = glm::vec3(0, 1, 0);
				const glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), glm::vec3(direction.x, currentHeight, direction.z)) * glm::mat4_cast(glm::quatLookAt(front, glm::normalize(direction))) * glm::scale(glm::vec3(DisplayScale));
				models.push_back(model);
				sliceIndex++;
			}
			tierIndex++;
		}
		RenderManager::DrawGizmoCubeInstanced(DisplayColor, models.data(), models.size(), glm::mat4(1.0f), 1.0f);
	}
}
