#include "InternodeSystem.h"
#include "TreeManager.h"

#include <gtx/matrix_decompose.hpp>

void TreeUtilities::InternodeSystem::DrawGui()
{
	ImGui::Begin("Tree Utilities");
	if (ImGui::CollapsingHeader("Internode System", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Internodes Amount: %d ", _InternodeQuery.GetEntityAmount());
		ImGui::Separator();
		ImGui::InputFloat("Internode Connection Width", &_ConnectionWidth);
		if (ImGui::Button("Regenerate connections for internodes")) {
			RefreshConnections();
		}
		ImGui::Separator();
		ImGui::CheckboxFlags("Draw internodes", &_ConfigFlags, InternodeSystem_DrawInternodes);
		ImGui::CheckboxFlags("Draw internode Connections", &_ConfigFlags, InternodeSystem_DrawConnections);
	}
	ImGui::End();
}

void TreeUtilities::InternodeSystem::RaySelection()
{
	_RaySelectedEntity.Index = 0;
	_RaySelectedEntity.Version = 0;
	std::mutex writeMutex;
	float minDistance = FLT_MAX;
	auto mainCamera = RenderManager::GetMainCamera();
	if (mainCamera) {
		auto cameraLtw = mainCamera->GetOwner().GetComponentData<GlobalTransform>();
		glm::vec2 mousePos;
		if (InputManager::GetMousePosition(mousePos)) {
			const Ray cameraRay = mainCamera->GetCamera()->ScreenPointToRay(
				cameraLtw, mousePos);
			EntityManager::ForEach<GlobalTransform, InternodeInfo>(_InternodeQuery, [this, cameraLtw, &writeMutex, &minDistance, cameraRay](int i, Entity entity, GlobalTransform* ltw, InternodeInfo* info)
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
		}
	}
	if(InputManager::GetKey(GLFW_KEY_F))
	{
		if (!_RaySelectedEntity.IsNull())
		{
			EditorManager::SetSelectedEntity(_RaySelectedEntity);
		}
	}
}

void TreeUtilities::InternodeSystem::OnCreate()
{
	_InternodeLTWList.clear();
	_InternodeQuery = TreeManager::GetInternodeQuery();
	_ConfigFlags = InternodeSystem_DrawInternodes;
	_ConfigFlags = InternodeSystem_DrawConnections;
	Enable();
}

void TreeUtilities::InternodeSystem::OnDestroy()
{
}

void TreeUtilities::InternodeSystem::Update()
{
	_InternodeLTWList.clear();
	if (_ConfigFlags & InternodeSystem_DrawInternodes) {
		_InternodeQuery.ToComponentDataArray(_InternodeLTWList);
		if (!_InternodeLTWList.empty())RenderManager::DrawGizmoCubeInstanced(glm::vec4(0, 0, 1, 1), (glm::mat4*)_InternodeLTWList.data(), _InternodeLTWList.size(), glm::mat4(1.0f), 0.02f);
	}
	if (_ConfigFlags & InternodeSystem_DrawConnections) {
		_ConnectionList.clear();
		_InternodeQuery.ToComponentDataArray(_ConnectionList);
		if (!_ConnectionList.empty())RenderManager::DrawGizmoMeshInstanced(Default::Primitives::Cylinder.get(), glm::vec4(0.6f, 0.3f, 0, 1), (glm::mat4*)_ConnectionList.data(), _ConnectionList.size(), glm::mat4(1.0f), 1.0f);
	}
	DrawGui();
	RaySelection();
	if(!_RaySelectedEntity.IsNull())
	{
		RenderManager::DrawGizmoPoint(glm::vec4(1, 1, 0, 1), 
			_RaySelectedEntity.GetComponentData<GlobalTransform>().Value, 0.1f);
	}
}

void TreeUtilities::InternodeSystem::RefreshConnections() const
{
	float lineWidth = _ConnectionWidth;
	EntityManager::ForEach<GlobalTransform, Connection, InternodeInfo>(_InternodeQuery, [lineWidth](int i, Entity entity, GlobalTransform* ltw, Connection* c, InternodeInfo* info) {
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(ltw->Value, scale, rotation, translation, skew, perspective);
		glm::vec3 parentTranslation = translation + (rotation * glm::vec3(0, 0, 1)) * info->DistanceToParent;
		rotation *= glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, 0.0f));
		glm::mat4 rotationMat = glm::mat4_cast(rotation);
		if(EntityManager::HasPrivateComponent<TreeData>(EntityManager::GetParent(entity))) c->Value = glm::translate((translation + parentTranslation) / 2.0f) * rotationMat * glm::scale(glm::vec3(0.0f));
		else c->Value = glm::translate((translation + parentTranslation) / 2.0f) * rotationMat * glm::scale(glm::vec3(info->Thickness * lineWidth, glm::distance(translation, parentTranslation) / 2.0f, info->Thickness * lineWidth));
		});
}
