#include "MaskTrimmer.h"
#include "TreeManager.h"

Entity TreeUtilities::MaskTrimmer::_CameraEntity;
unsigned TreeUtilities::MaskTrimmer::_ResolutionX;
unsigned TreeUtilities::MaskTrimmer::_ResolutionY;
std::unique_ptr<GLProgram> TreeUtilities::MaskTrimmer::_FilterProgram;
std::unique_ptr<RenderTarget> TreeUtilities::MaskTrimmer::_Filter;
std::unique_ptr<GLRenderBuffer> TreeUtilities::MaskTrimmer::_DepthStencilBuffer;

void TreeUtilities::MaskTrimmer::ShotInternodes() const
{
	if (!_CameraEntity.IsValid()) return;
	const auto targetTreeIndex = GetOwner().GetComponentData<TreeIndex>();
	const auto cameraTransform = _CameraEntity.GetComponentData<GlobalTransform>();
	std::vector<glm::mat4> matrices = std::vector<glm::mat4>();
	std::vector<Entity> internodeEntities = std::vector<Entity>();
	std::mutex m;
	EntityManager::ForEach<TreeIndex, GlobalTransform>(TreeManager::GetInternodeQuery(), [&m, targetTreeIndex, &matrices, &internodeEntities, this](int i, Entity entity, TreeIndex* index, GlobalTransform* globalTransform)
		{
			if (targetTreeIndex.Value != index->Value) return;
			glm::vec3 position = globalTransform->GetPosition();
			glm::mat4 input = glm::translate(position) * glm::scale(glm::vec3(_InternodeSize));
			std::lock_guard<std::mutex> lock(m);
			matrices.push_back(input);
			internodeEntities.push_back(entity);
		}
	);
	auto mesh = Default::Primitives::Sphere;

	const GLVBO indicesBuffer;
	const size_t count = matrices.size();
	mesh->Enable();

	indicesBuffer.SetData((GLsizei)count * sizeof(Entity), internodeEntities.data(), GL_DYNAMIC_DRAW);
	mesh->VAO()->EnableAttributeArray(11);
	mesh->VAO()->SetAttributeIntPointer(11, 1, GL_UNSIGNED_INT, sizeof(Entity), (void*)0);
	mesh->VAO()->SetAttributeDivisor(11, 1);

	const GLVBO matricesBuffer;
	matricesBuffer.SetData((GLsizei)count * sizeof(glm::mat4), matrices.data(), GL_DYNAMIC_DRAW);

	mesh->VAO()->EnableAttributeArray(12);
	mesh->VAO()->SetAttributePointer(12, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
	mesh->VAO()->EnableAttributeArray(13);
	mesh->VAO()->SetAttributePointer(13, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
	mesh->VAO()->EnableAttributeArray(14);
	mesh->VAO()->SetAttributePointer(14, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
	mesh->VAO()->EnableAttributeArray(15);
	mesh->VAO()->SetAttributePointer(15, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));
	mesh->VAO()->SetAttributeDivisor(12, 1);
	mesh->VAO()->SetAttributeDivisor(13, 1);
	mesh->VAO()->SetAttributeDivisor(14, 1);
	mesh->VAO()->SetAttributeDivisor(15, 1);

	const glm::mat4 translation = glm::translate(glm::identity<glm::mat4>(), glm::vec3(0.0f));
	const glm::mat4 scale = glm::scale(glm::identity<glm::mat4>(), glm::vec3(1.0f));
	const glm::mat4 model = translation * scale;
	_Filter->Bind();
	_FilterProgram->Bind();
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	_Filter->AttachTexture(_Result.get(), GL_COLOR_ATTACHMENT0);
	_Filter->Clear();
	if (count != 0) {
		const auto position = cameraTransform.GetPosition();
		const auto rotation = cameraTransform.GetRotation();
		_FilterProgram->SetFloat4x4("lightSpaceMatrix", _CameraEntity.GetPrivateComponent<CameraComponent>()->GetProjection() * glm::lookAt(position, position + rotation * glm::vec3(0, 0, -1), rotation * glm::vec3(0, 1, 0)));
		_FilterProgram->SetFloat4x4("model", model);
		glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)mesh->Size(), GL_UNSIGNED_INT, 0, (GLsizei)count);
	}

	mesh->VAO()->DisableAttributeArray(11);
	mesh->VAO()->DisableAttributeArray(12);
	mesh->VAO()->DisableAttributeArray(13);
	mesh->VAO()->DisableAttributeArray(14);
	mesh->VAO()->DisableAttributeArray(15);
	RenderTarget::BindDefault();
}

void TreeUtilities::MaskTrimmer::Filter()
{
	if (!_Mask || !_CameraEntity.IsValid()) return;
	ShotInternodes();
	Entity tree = GetOwner();

}

TreeUtilities::MaskTrimmer::MaskTrimmer()
{
	_Result = std::make_unique<GLTexture2D>(1, GL_R32F, _ResolutionX, _ResolutionY);
	_Result->SetInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	_Result->SetInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	_Result->SetInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	_Result->SetInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	_Mask = nullptr;
}

void TreeUtilities::MaskTrimmer::Trim()
{
	if (!_Mask || !_CameraEntity.IsValid()) return;
	Filter();
	auto& cameraComponent = _CameraEntity.GetPrivateComponent<CameraComponent>();
	Entity tree = GetOwner();

}

void TreeUtilities::MaskTrimmer::OnGui()
{
	ImGui::DragFloat("Internode size", &_InternodeSize);
	if (ImGui::Button("ShotInternodes"))
	{
		ShotInternodes();
	}
	if (ImGui::Button("Filter"))
	{
		Filter();
	}
	if (ImGui::Button("Trim"))
	{
		Trim();
	}
	ImGui::Text("TargetMask: ");
	EditorManager::DragAndDrop(_Mask);
	ImGui::Text("Content: ");
	if (_Mask)ImGui::Image((ImTextureID)_Mask->Texture()->ID(), ImVec2(320, 320), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::Separator();
	ImGui::Text("Result: ");
	ImGui::Image((ImTextureID)_Result->ID(), ImVec2(320, 320), ImVec2(0, 1), ImVec2(1, 0));
}
