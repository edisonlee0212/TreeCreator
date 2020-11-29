#include "TreeVolume.h"
void TreeVolume::SetPruneBuds(bool value)
{
	_PruneBuds = value;
}

bool TreeVolume::PruneBuds() const
{
	return _PruneBuds;
}

bool TreeVolume::InVolume(glm::vec3 position) const
{
	switch (_Type)
	{
	case TreeVolumeType::Cube:
		return _Bound.InBound(position);
	case TreeVolumeType::Sphere:
		return glm::distance(position, _Bound.Center) <= _Bound.Radius;
	}
	return true;
}

void TreeVolume::OnGui()
{
	ImGui::Checkbox("Prune Buds", &_PruneBuds);
	static const char* TVTypes[]{ "Default", "Cube", "Sphere" };
	ImGui::Combo("Display mode", (int*)(void*)&_Type, TVTypes, IM_ARRAYSIZE(TVTypes));
	switch (_Type)
	{
	case TreeVolumeType::Cube:
		ImGui::Text("Type: Cube");
		ImGui::Checkbox("Display bounds", &_Display);
		if (_Display)
		{
			ImGui::ColorEdit4("Color: ", (float*)(void*)&_DisplayColor);
			RenderManager::DrawGizmoCube(_DisplayColor, glm::translate(_Bound.Center) * glm::scale(_Bound.Size), 1);
		}
		ImGui::DragFloat3("Center: ", (float*)(void*)&_Bound.Center);
		ImGui::DragFloat3("Size: ", (float*)(void*)&_Bound.Size);
		break;
	case TreeVolumeType::Sphere:
		ImGui::Text("Type: Sphere");
		ImGui::Checkbox("Display bounds", &_Display);
		if (_Display)
		{
			ImGui::ColorEdit4("Color: ", (float*)(void*)&_DisplayColor);
			RenderManager::DrawGizmoPoint(_DisplayColor,
				glm::translate(_Bound.Center) * glm::scale(glm::vec3(_Bound.Radius)), 1);
		}
		ImGui::DragFloat3("Center: ", (float*)(void*)&_Bound.Center);
		ImGui::DragFloat("Radius: ", (float*)(void*)&_Bound.Radius);
		break;
	default:
		ImGui::Text("Type: Default");
		break;
	}
}