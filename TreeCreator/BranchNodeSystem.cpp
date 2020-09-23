#include "BranchNodeSystem.h"
#include "TreeManager.h"

#include <gtx/matrix_decompose.hpp>

void TreeUtilities::BranchNodeSystem::DrawGui()
{
	ImGui::Begin("Tree Utilities");
	if (ImGui::CollapsingHeader("Branch Node System", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Branch Nodes Amount: %d ", _BranchNodeQuery.GetEntityAmount());
		ImGui::Separator();
		ImGui::InputFloat("Branch Node Connection Width", &_ConnectionWidth);
		if (ImGui::Button("Regenerate connections for Branch Nodes")) {
			RefreshConnections();
		}
		ImGui::Separator();
		ImGui::CheckboxFlags("Draw Branch Nodes", &_ConfigFlags, BranchNodeSystem_DrawBranchNodes);
		ImGui::CheckboxFlags("Draw Branch Node Connections", &_ConfigFlags, BranchNodeSystem_DrawConnections);
	}
	ImGui::End();
}

void TreeUtilities::BranchNodeSystem::RaySelection()
{
	_RaySelectedEntity.Index = 0;
	_RaySelectedEntity.Version = 0;
	std::mutex writeMutex;
	float minDistance = FLT_MAX;
	auto cameraLtw = Application::GetMainCameraEntity().GetComponentData<LocalToWorld>();
	const Ray cameraRay = Application::GetMainCameraComponent()->Value->ScreenPointToRay(
		cameraLtw, InputManager::GetMouseScreenPosition());
	EntityManager::ForEach<LocalToWorld, BranchNodeInfo>(_BranchNodeQuery, [this, cameraLtw, &writeMutex, &minDistance, cameraRay](int i, Entity entity, LocalToWorld* ltw, BranchNodeInfo* info)
		{
			const float distance = glm::distance(glm::vec3(cameraLtw.Value[3]), glm::vec3(ltw->Value[3]));
			if (cameraRay.Intersect(ltw->Value[3], 0.1f))
			{
				std::lock_guard<std::mutex> lock(writeMutex);
				if (distance < minDistance)
				{
					minDistance = distance;
					this->_RaySelectedEntity = entity;
				}
			}
		}
	);
	if(InputManager::GetKey(GLFW_KEY_F))
	{
		if (!_RaySelectedEntity.IsNull())
		{
			EntityEditorSystem::SetSelectedEntity(_RaySelectedEntity);
		}
	}
}

void TreeUtilities::BranchNodeSystem::OnCreate()
{
	_BranchNodeLTWList.clear();
	_BranchNodeQuery = TreeManager::GetBranchNodeQuery();
	_ConfigFlags = BranchNodeSystem_DrawBranchNodes;
	_ConfigFlags = BranchNodeSystem_DrawConnections;
	Enable();
}

void TreeUtilities::BranchNodeSystem::OnDestroy()
{
}

void TreeUtilities::BranchNodeSystem::Update()
{
	_BranchNodeLTWList.clear();
	if (_ConfigFlags & BranchNodeSystem_DrawBranchNodes) {
		_BranchNodeQuery.ToComponentDataArray(_BranchNodeLTWList);
		if (!_BranchNodeLTWList.empty())RenderManager::DrawGizmoCubeInstanced(glm::vec4(0, 0, 1, 1), (glm::mat4*)_BranchNodeLTWList.data(), _BranchNodeLTWList.size(), Application::GetMainCameraComponent()->Value.get(), glm::mat4(1.0f), 0.02f);
	}
	if (_ConfigFlags & BranchNodeSystem_DrawConnections) {
		_ConnectionList.clear();
		_BranchNodeQuery.ToComponentDataArray(_ConnectionList);
		if (!_ConnectionList.empty())RenderManager::DrawGizmoMeshInstanced(Default::Primitives::Cylinder.get(), glm::vec4(0.6f, 0.3f, 0, 1), (glm::mat4*)_ConnectionList.data(), _ConnectionList.size(), Application::GetMainCameraComponent()->Value.get(), glm::mat4(1.0f), 1.0f);
	}
	DrawGui();
	RaySelection();
	if(!_RaySelectedEntity.IsNull())
	{
		RenderManager::DrawGizmoPoint(glm::vec4(1, 1, 0, 1), Application::GetMainCameraComponent()->Value.get(),
			_RaySelectedEntity.GetComponentData<LocalToWorld>().Value, 0.1f);
	}
}

void TreeUtilities::BranchNodeSystem::RefreshConnections() const
{
	float lineWidth = _ConnectionWidth;
	EntityManager::ForEach<LocalToWorld, Connection, BranchNodeInfo>(_BranchNodeQuery, [lineWidth](int i, Entity entity, LocalToWorld* ltw, Connection* c, BranchNodeInfo* info) {
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(ltw->Value, scale, rotation, translation, skew, perspective);
		glm::vec3 parentTranslation = translation + (rotation * glm::vec3(0, 0, 1)) * info->DistanceToParent; //EntityManager::GetComponentData<LocalToWorld>(EntityManager::GetParent(entity)).Value[3];
		glm::vec3 front;
		glm::vec3 up;
		rotation *= glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, 0.0f));
		glm::mat4 rotationMat = glm::mat4_cast(rotation);
		if(EntityManager::HasComponentData<TreeData>(EntityManager::GetParent(entity))) c->Value = glm::translate((translation + parentTranslation) / 2.0f) * rotationMat * glm::scale(glm::vec3(0.0f));
		else c->Value = glm::translate((translation + parentTranslation) / 2.0f) * rotationMat * glm::scale(glm::vec3(info->Thickness * lineWidth, glm::distance(translation, parentTranslation) / 2.0f, info->Thickness * lineWidth));
		});
}
