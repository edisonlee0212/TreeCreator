#include "pch.h"
#include "TreeSystem.h"
#include "TreeManager.h"

void TreeUtilities::TreeSystem::DrawGUI()
{
	ImGui::Begin("Tree Utilities");
	if (ImGui::CollapsingHeader("Tree System", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Tree Amount: %d ", _TreeEntities.size());
		ImGui::Separator();
	}
	ImGui::End();
	auto _SelectedTreeEntity = EntityEditorSystem::GetSelectedEntity();

	ImGui::Begin("Tree Inspector");
	
	if (!_SelectedTreeEntity.IsNull() && EntityManager::HasComponentData<TreeData>(_SelectedTreeEntity)) {
		TreeIndex index = EntityManager::GetComponentData<TreeIndex>(_SelectedTreeEntity);
		TreeParameters tps = EntityManager::GetComponentData<TreeParameters>(_SelectedTreeEntity);
		std::string title = "Tree ";
		title += std::to_string(index.Value);
		ImGui::Text(title.c_str());
		ImGui::InputFloat("Mesh resolution", &_MeshGenerationResolution, 0.0f, 0.0f, "%.5f");
		ImGui::InputFloat("Branch subdivision", &_MeshGenerationSubdivision, 0.0f, 0.0f, "%.5f");
		if (ImGui::Button("Regenerate mesh")) {
			TreeManager::GenerateSimpleMeshForTree(_SelectedTreeEntity, _MeshGenerationResolution, _MeshGenerationSubdivision);
		}
		ImGui::InputText("File name", _MeshOBJFileName, 255);
		if(ImGui::Button(("Export mesh as " + std::string(_MeshOBJFileName) + ".obj").c_str())) {
			TreeManager::ExportMeshToOBJ(_SelectedTreeEntity, _MeshOBJFileName);
		}
		ImGui::Separator();
		title = "Rewards Estimation##";
		title += std::to_string(index.Value);
		if (ImGui::CollapsingHeader(title.c_str())) {
			if (ImGui::Button("Estimate lighting")) {
				TreeManager::CalculateRewards(_SelectedTreeEntity);
			}
			else {
				TreeManager::GetLightEstimator()->TakeSnapShot(false);
			}

			auto estimation = EntityManager::GetComponentData<RewardEstimation>(_SelectedTreeEntity);
			title = "Light Rewards: " + std::to_string(estimation.LightEstimationResult);
			ImGui::Text(title.c_str());
			auto* ss = TreeManager::GetLightEstimator()->GetSnapShots();
			for (auto i : *ss) {
				ImGui::Image((ImTextureID)i->SnapShotTexture()->ID(), ImVec2(500, 500));
			}
		}
		ImGui::Separator();
	}
	ImGui::End();
}
void TreeUtilities::TreeSystem::OnCreate()
{
	_TreeQuery = TreeManager::GetTreeQuery();


	_ConfigFlags = 0;
	_ConfigFlags += TreeSystem_DrawTrees;
	_ConfigFlags += TreeSystem_DrawTreeMeshes;

	Enable();
}

void TreeUtilities::TreeSystem::OnDestroy()
{
}

void TreeUtilities::TreeSystem::Update()
{
	_TreeEntities.clear();
	_TreeQuery.ToEntityArray(_TreeEntities);
	DrawGUI();
}

void TreeUtilities::TreeSystem::FixedUpdate()
{
	
}

std::vector<Entity>* TreeUtilities::TreeSystem::GetTreeEntities()
{
	return &_TreeEntities;
}
