#include "TreeVolume.h"
void TreeVolume::SetPruneBuds(bool value)
{
	_PruneBuds = value;
}

bool TreeVolume::PruneBuds() const
{
	return _PruneBuds;
}

void TreeVolume::CalculateVolume()
{
}

bool TreeVolume::InVolume(glm::vec3 position) const
{
	switch (_Type)
	{
	case TreeVolumeType::Sphere:
		return glm::distance(position, _Center) <= _SphereRadius;
	}
	return true;
}

void TreeVolume::OnGui()
{
	ImGui::Checkbox("Prune Buds", &_PruneBuds);
	ImGui::Checkbox("Display bounds", &_Display);
	static const char* TVTypes[]{ "Default", "Cube", "Sphere" };
	ImGui::Combo("Display mode", (int*)(void*)&_Type, TVTypes, IM_ARRAYSIZE(TVTypes));
	switch (_Type)
	{
	
	case TreeVolumeType::Sphere:
		ImGui::Text("Type: Sphere");
		if (_Display)
		{
			ImGui::ColorEdit4("Color: ", (float*)(void*)&_DisplayColor);
			RenderManager::DrawGizmoPoint(_DisplayColor,
				glm::translate(_Center) * glm::scale(glm::vec3(_SphereRadius)), 1);
		}
		ImGui::DragFloat3("Center: ", (float*)(void*)&_Center.x);
		ImGui::DragFloat("Radius: ", (float*)(void*)&_SphereRadius);
		break;
	default:
		ImGui::Text("Type: Default");
		break;
	}
}