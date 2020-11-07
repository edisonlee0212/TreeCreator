#include "TreeSystem.h"
#include "TreeManager.h"
#include "PlantSimulationSystem.h"
void TreeUtilities::TreeSystem::OnGui()
{
	auto treeEntity = EditorManager::GetSelectedEntity();
	ImGui::Begin("Tree Inspector");
	
	if (!treeEntity.IsNull() && EntityManager::HasPrivateComponent<TreeData>(treeEntity)) {
		TreeIndex index = EntityManager::GetComponentData<TreeIndex>(treeEntity);
		std::string title = "Tree ";
		title += std::to_string(index.Value);
		ImGui::Text(title.c_str());

		if (ImGui::Button("Enable all nodes"))
		{
			PlantSimulationSystem::SetAllInternodeActivated(treeEntity, true);
		}
		if (ImGui::Button("Disable all nodes"))
		{
			PlantSimulationSystem::SetAllInternodeActivated(treeEntity, false);
		}

		ImGui::Spacing();
		ImGui::Separator();
		
		ImGui::InputFloat("Mesh resolution", &_MeshGenerationResolution, 0.0f, 0.0f, "%.5f");
		ImGui::InputFloat("Branch subdivision", &_MeshGenerationSubdivision, 0.0f, 0.0f, "%.5f");
		if (ImGui::Button("Regenerate mesh")) {
			TreeManager::GenerateSimpleMeshForTree(treeEntity, _MeshGenerationResolution, _MeshGenerationSubdivision);
		}
		ImGui::InputText("File name", _MeshOBJFileName, 255);
		if(ImGui::Button(("Export mesh as " + std::string(_MeshOBJFileName) + ".obj").c_str())) {
			TreeManager::ExportMeshToOBJ(treeEntity, _MeshOBJFileName);
		}
		ImGui::Separator();
		title = "Light Estimation##";
		title += std::to_string(index.Value);
		if (ImGui::CollapsingHeader(title.c_str())) {
			auto* ss = TreeManager::GetLightEstimator()->GetSnapShots();
			for (auto i : *ss) {
				ImGui::Image((ImTextureID)i->SnapShotTexture()->ID(), ImVec2(200, 200));
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
	OnGui();
}

void TreeUtilities::TreeSystem::FixedUpdate()
{
	
}