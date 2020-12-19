#include "MaskTrimmer.h"
#include "TreeManager.h"
#include "PlantSimulationSystem.h"
Entity TreeUtilities::MaskTrimmer::_CameraEntity;
unsigned TreeUtilities::MaskTrimmer::_ResolutionX;
unsigned TreeUtilities::MaskTrimmer::_ResolutionY;
std::unique_ptr<GLProgram> TreeUtilities::MaskTrimmer::_InternodeCaptureProgram;
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
	_InternodeCaptureProgram->Bind();
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	_Filter->AttachTexture(_InternodeCaptureResult.get(), GL_COLOR_ATTACHMENT0);
	_Filter->Clear();
	if (count != 0) {
		const auto position = cameraTransform.GetPosition();
		const auto rotation = cameraTransform.GetRotation();
		_InternodeCaptureProgram->SetFloat4x4("lightSpaceMatrix", _CameraEntity.GetPrivateComponent<CameraComponent>()->GetProjection() * glm::lookAt(position, position + rotation * glm::vec3(0, 0, -1), rotation * glm::vec3(0, 1, 0)));
		_InternodeCaptureProgram->SetFloat4x4("model", model);
		glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)mesh->Size(), GL_UNSIGNED_INT, 0, (GLsizei)count);
	}

	mesh->VAO()->DisableAttributeArray(11);
	mesh->VAO()->DisableAttributeArray(12);
	mesh->VAO()->DisableAttributeArray(13);
	mesh->VAO()->DisableAttributeArray(14);
	mesh->VAO()->DisableAttributeArray(15);
	RenderTarget::BindDefault();
}

void TreeUtilities::MaskTrimmer::Filter() const
{
	if (!_Mask || !_CameraEntity.IsValid()) return;
	ShotInternodes();
	_Filter->AttachTexture(_FilteredResult.get(), GL_COLOR_ATTACHMENT0);
	_Filter->GetFrameBuffer()->DrawBuffer(GL_COLOR_ATTACHMENT0);
	_Filter->Bind();
	_FilterProgram->Bind();
	_InternodeCaptureResult->Bind(0);
	_Mask->Texture()->Bind(1);
	_FilterProgram->SetInt("InputTex", 0);
	_FilterProgram->SetInt("MaskTex", 1);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	Default::GLPrograms::ScreenVAO->Bind();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	_FilteredResult->Bind(0);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, (void*)_Data.data());
}

TreeUtilities::MaskTrimmer::MaskTrimmer()
{
	_InternodeCaptureResult = std::make_unique<GLTexture2D>(1, GL_R32F, _ResolutionX, _ResolutionY);
	_InternodeCaptureResult->SetInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	_InternodeCaptureResult->SetInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	_InternodeCaptureResult->SetInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	_InternodeCaptureResult->SetInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	_FilteredResult = std::make_unique<GLTexture2D>(1, GL_R32F, _ResolutionX, _ResolutionY);
	_FilteredResult->SetInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	_FilteredResult->SetInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	_FilteredResult->SetInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	_FilteredResult->SetInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	_Data.resize(_ResolutionX * _ResolutionY);
	_Mask = nullptr;
}

void TreeUtilities::MaskTrimmer::Trim()
{
	if (!_Mask || !_CameraEntity.IsValid()) return;
	Filter();
	for(const auto i : _Data)
	{
		if(i != 0)
		{
			Entity entity = EntityManager::GetEntity(i);
			if (!entity.IsDeleted()) EntityManager::DeleteEntity(entity);
		}
	}
	if(EntityManager::HasPrivateComponent<FoliageGeneratorBase>(GetOwner()))EntityManager::GetPrivateComponent<FoliageGeneratorBase>(GetOwner())->Generate();
	TreeManager::GenerateSimpleMeshForTree(GetOwner(), PlantSimulationSystem::_MeshGenerationResolution, PlantSimulationSystem::_MeshGenerationSubdivision);
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
	ImGui::Text("Target Mask: ");
	EditorManager::DragAndDrop(_Mask);
	ImGui::Text("Content: ");
	if (_Mask)ImGui::Image((ImTextureID)_Mask->Texture()->ID(), ImVec2(160, 160), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::Separator();
	ImGui::Text("Internode Capture: ");
	ImGui::Image((ImTextureID)_InternodeCaptureResult->ID(), ImVec2(160, 160), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::Text("Filtered Result: ");
	ImGui::Image((ImTextureID)_FilteredResult->ID(), ImVec2(160, 160), ImVec2(0, 1), ImVec2(1, 0));
}
