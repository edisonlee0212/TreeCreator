#include "KDop.h"

void KDop::CalculateVolume()
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
	for(auto& i : DirectionalDistance)
	{
		i = FLT_MIN;
	}
	
	for(auto& i : internodeInfos)
	{
		int index = 0;
		glm::vec3 position = i.GlobalTransform[3];
		for(int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				for (int z = -1; z <= 1; z++)
				{
					if(x != 0 || y != 0 || z != 0)
					{
						glm::vec3 direction = glm::vec3(x, y, z);
						glm::vec3 pointOnLine = glm::closestPointOnLine(position, direction * 100.0f, direction * -100.0f);
						float distance = glm::dot(direction, pointOnLine) > 0 ? 1.0f : -1.0f;
						distance *= glm::length(pointOnLine);
						if(DirectionalDistance[index] < distance)
						{
							DirectionalDistance[index] = distance;
						}
						index++;
					}
				}
			}
		}
	}
}

bool KDop::InVolume(glm::vec3 position) const
{
	int index = 0;
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			for (int z = -1; z <= 1; z++)
			{
				if (x != 0 || y != 0 || z != 0)
				{
					glm::vec3 direction = glm::vec3(x, y, z);
					glm::vec3 pointOnLine = glm::closestPointOnLine(position, direction * 10000.0f, direction * -10000.0f);
					float distance = glm::dot(direction, pointOnLine) > 0 ? 1.0f : -1.0f;
					distance *= glm::length(pointOnLine);
					if (DirectionalDistance[index] < distance)
					{
						return false;
					}
					index++;
				}
			}
		}
	}
	return true;
}

void KDop::OnGui()
{
	ImGui::Checkbox("Prune Buds", &_PruneBuds);
	ImGui::Checkbox("Display bounds", &_Display);
	if (ImGui::Button("Calculate Bounds")) CalculateVolume();
	if (_Display)
	{
		int index = 0;
		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				for (int z = -1; z <= 1; z++)
				{
					if (x != 0 || y != 0 || z != 0)
					{
						glm::vec3 direction = glm::normalize(glm::vec3(x, y, z)) * DirectionalDistance[index];
						glm::vec3 front = glm::vec3(0, 0, -1);
						if (direction.x == 0 && y == 0) front = glm::vec3(0, 1, 0);
						front = glm::cross(glm::cross(front, direction), direction);
						glm::mat4 model = glm::translate(glm::identity<glm::mat4>(), direction) * glm::mat4_cast(glm::quatLookAt(glm::normalize(front), glm::normalize(direction))) * glm::scale(glm::vec3(2.0f));
						RenderManager::DrawGizmoQuad(glm::vec4(0, 1, 0, 0.5), model);
						index++;
					}
				}
			}
		}
	}
}
