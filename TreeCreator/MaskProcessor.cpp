#include "MaskProcessor.h"
#include "TreeManager.h"
#include "PlantSimulationSystem.h"
#include "Ray.h"
Entity TreeUtilities::MaskProcessor::_CameraEntity;
unsigned TreeUtilities::MaskProcessor::_ResolutionX;
unsigned TreeUtilities::MaskProcessor::_ResolutionY;
std::unique_ptr<GLProgram> TreeUtilities::MaskProcessor::_InternodeCaptureProgram;
std::unique_ptr<GLProgram> TreeUtilities::MaskProcessor::_FilterProgram;
std::unique_ptr<GLProgram> TreeUtilities::MaskProcessor::_MaskPreprocessProgram;
std::unique_ptr<RenderTarget> TreeUtilities::MaskProcessor::_Filter;
std::unique_ptr<GLRenderBuffer> TreeUtilities::MaskProcessor::_DepthStencilBuffer;

void MaskProcessor::Trim(int& totalChild, int& trimmedChild, std::map<int, Entity>& map, Entity internode)
{
	EntityManager::ForEachChild(internode, [&map, this, &totalChild, &trimmedChild](Entity child)
		{
			Trim(totalChild, trimmedChild, map, child);
		}
	);
	if (EntityManager::GetChildrenAmount(internode) == 0)
	{
		if (map.find(internode.Index) != map.end())
		{
			trimmedChild++;
			totalChild++;
			return;
		}
	}
	totalChild++;
	if (internode.GetComponentData<InternodeInfo>().Order > _MainBranchOrderProtection && map.find(internode.Index) != map.end())
	{
		if (_TrimFactor == 0.0f || (float)trimmedChild / totalChild > _TrimFactor) {
			EntityManager::DeleteEntity(internode);
			trimmedChild++;
		}
	}
}

void MaskProcessor::PlaceAttractionPoints()
{
	if(_Skeleton == nullptr || _Mask == nullptr)
	{
		Debug::Error("Skeleton or mask not ready!");
		return;
	}
	const auto treeIndex = GetOwner().GetComponentData<TreeIndex>();
	_Mask->Texture()->Bind(0);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, (void*)_MaskData.data());
	for (int x = 0; x < _ResolutionX; x++)
	{
		for (int y = 0; y < _ResolutionY; y++)
		{
			if (_MaskData[x * _ResolutionY + y] == glm::vec3(1, 0, 0))
			{
				GlobalTransform cameraTransform = _CameraEntity.GetComponentData<GlobalTransform>();
				glm::vec3 position;
				Ray cameraRay = _CameraEntity.GetPrivateComponent<CameraComponent>()->ScreenPointToRay(cameraTransform, glm::vec2(y - (int)_ResolutionY, _ResolutionX - x));
				auto start = cameraRay.Start;
				auto direction = glm::normalize(cameraRay.Direction);
				direction /= direction.z;
				position = start - direction * start.z;
				TreeManager::CreateAttractionPoint(treeIndex, position, GetOwner());
			}
		}
	}
}

void MaskProcessor::PreprocessMask() const
{
	_Filter->AttachTexture(_ProcessedMask.get(), GL_COLOR_ATTACHMENT0);
	_Filter->GetFrameBuffer()->DrawBuffer(GL_COLOR_ATTACHMENT0);
	glDisable(GL_DEPTH_TEST);
	_Filter->Bind();
	_MaskPreprocessProgram->Bind();
	_Mask->Texture()->Bind(0);
	_MaskPreprocessProgram->SetInt("InputTex", 0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	Default::GLPrograms::ScreenVAO->Bind();
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void TreeUtilities::MaskProcessor::ShotInternodes() const
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

void TreeUtilities::MaskProcessor::Filter() const
{
	if (!_Mask || !_CameraEntity.IsValid()) return;
	ShotInternodes();
	PreprocessMask();
	_Filter->AttachTexture(_FilteredResult.get(), GL_COLOR_ATTACHMENT0);
	_Filter->GetFrameBuffer()->DrawBuffer(GL_COLOR_ATTACHMENT0);
	_Filter->Bind();
	_FilterProgram->Bind();
	_InternodeCaptureResult->Bind(0);
	_ProcessedMask->Bind(1);
	_FilterProgram->SetInt("InputTex", 0);
	_FilterProgram->SetInt("MaskTex", 1);
	_FilterProgram->SetFloat("IgnoreMaxHeight", _IgnoreMaxHeight);
	_FilterProgram->SetFloat("IgnoreWidth", _IgnoreWidth);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	Default::GLPrograms::ScreenVAO->Bind();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	_FilteredResult->Bind(0);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, (void*)_Data.data());
}

TreeUtilities::MaskProcessor::MaskProcessor()
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

	_ProcessedMask = std::make_unique<GLTexture2D>(1, GL_RGB32F, _ResolutionX, _ResolutionY);
	_ProcessedMask->SetInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	_ProcessedMask->SetInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	_ProcessedMask->SetInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	_ProcessedMask->SetInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);


	_Data.resize(_ResolutionX * _ResolutionY);
	_MaskData.resize(_ResolutionX * _ResolutionY);
	_SkeletonData.resize(_ResolutionX * _ResolutionY);
	_Mask = nullptr;
	_Skeleton = nullptr;
}

void TreeUtilities::MaskProcessor::Trim()
{
	if (!_Mask || !_CameraEntity.IsValid()) return;
	Filter();
	std::map<int, Entity> _Candidates;
	for (const auto i : _Data)
	{
		if (i != 0)
		{
			Entity entity = EntityManager::GetEntity(i);
			if (!entity.IsDeleted()) {
				//EntityManager::DeleteEntity(entity);
				_Candidates.insert({ entity.Index, entity });
			}
		}
	}
	Entity rootNode;
	EntityManager::ForEachChild(GetOwner(), [&rootNode](Entity child)
		{
			if (child.HasComponentData<InternodeInfo>()) rootNode = child;
		}
	);
	if (!rootNode.IsNull()) {
		int totalChild = 0;
		int trimmedChild = 0;
		Trim(totalChild, trimmedChild, _Candidates, rootNode);
	}
	if (EntityManager::HasPrivateComponent<FoliageGeneratorBase>(GetOwner()))EntityManager::GetPrivateComponent<FoliageGeneratorBase>(GetOwner())->Generate();
	TreeManager::GenerateSimpleMeshForTree(GetOwner(), PlantSimulationSystem::_MeshGenerationResolution, PlantSimulationSystem::_MeshGenerationSubdivision);
}

void TreeUtilities::MaskProcessor::ClearAttractionPoints() const
{
	const auto treeIndex = GetOwner().GetComponentData<TreeIndex>();
	std::vector<Entity> points;
	TreeManager::GetAttractionPointQuery().ToEntityArray(treeIndex, points);
	for (auto& i : points) EntityManager::DeleteEntity(i);
}

void TreeUtilities::MaskProcessor::OnGui()
{
	ImGui::DragFloat("Remove Distance", &_RemoveDistance, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("Attract Distance", &_AttractDistance, 0.01f, _RemoveDistance, 10.0f);
	ImGui::DragInt("AP Count", &_AttractionPointsCount);
	ImGui::Checkbox("Display attraction points", &_Display);
	if (ImGui::Button("Generate attraction points"))
	{
		ClearAttractionPoints();
		PlaceAttractionPoints();
	}
	if (_Display)
	{
		const auto treeIndex = GetOwner().GetComponentData<TreeIndex>();
		std::vector<GlobalTransform> points;
		TreeManager::GetAttractionPointQuery().ToComponentDataArray(treeIndex, points);
		RenderManager::DrawGizmoPointInstanced(glm::vec4(1.0f, 0.0f, 0.0f, 0.2f), (glm::mat4*)(void*)points.data(), points.size(), glm::mat4(1.0f), _RemoveDistance / 2.0f);
		RenderManager::DrawGizmoPointInstanced(glm::vec4(1.0f, 1.0f, 1.0f, 0.2f), (glm::mat4*)(void*)points.data(), points.size(), glm::mat4(1.0f), _AttractDistance / 2.0f);
	}

	ImGui::DragFloat("Internode size", &_InternodeSize, 0.001f, 0.02f, 1.0f);
	ImGui::DragFloat("Height ignore", &_IgnoreMaxHeight, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Trim factor", &_TrimFactor, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Width ignore", &_IgnoreWidth, 0.01f, 0.0f, 0.5f);
	ImGui::DragInt("Order protection", &_MainBranchOrderProtection, 1, 0, 99);
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
	ImGui::Text("Target Skeleton: ");
	EditorManager::DragAndDrop(_Skeleton);
	ImGui::Text("Target Mask: ");
	EditorManager::DragAndDrop(_Mask);
	ImGui::Text("Content: ");
	if (_Mask)ImGui::Image((ImTextureID)_Mask->Texture()->ID(), ImVec2(160, 160), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::Text("Processed mask: ");
	ImGui::Image((ImTextureID)_ProcessedMask->ID(), ImVec2(160, 160), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::Text("Internode Capture: ");
	ImGui::Image((ImTextureID)_InternodeCaptureResult->ID(), ImVec2(160, 160), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::Text("Filtered Result: ");
	ImGui::Image((ImTextureID)_FilteredResult->ID(), ImVec2(160, 160), ImVec2(0, 1), ImVec2(1, 0));
}