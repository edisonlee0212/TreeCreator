#include "PlantSimulationSystem.h"
#include "TreeScene.h"

#include "quickhull/quickhull.hpp"
#include <gtx/vector_angle.hpp>
#include <gtx/matrix_decompose.hpp>
#include <direct.h>
void TreeUtilities::PlantSimulationSystem::DrawGUI()
{
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Plant Simulation")) {
			if (ImGui::Button("Create...")) {
				ImGui::OpenPopup("New tree wizard");
			}
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

			if (ImGui::BeginPopupModal("New tree wizard", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
				ImGui::BeginChild("ChildL", ImVec2(300, 400), true, ImGuiWindowFlags_None | ImGuiWindowFlags_MenuBar);
				if (ImGui::BeginMenuBar())
				{
					if (ImGui::BeginMenu("Settings"))
					{
						ImGui::InputInt("New Tree Amount", &_NewTreeAmount);
						if (_NewTreeAmount < 1) _NewTreeAmount = 1;
						if (ImGui::BeginMenu("Export settings"))
						{
							ImGui::Text((std::string(_CurrentWorkingDir) + "\\").c_str());
							ImGui::SameLine();
							ImGui::InputText("", _TempExportFilePath, 256);
							if (ImGui::Button("Save")) {
								ExportSettings((std::string(_CurrentWorkingDir) + "\\") + std::string(_TempExportFilePath));
							}
							ImGui::EndMenu();
						}
						if (ImGui::BeginMenu("Import settings"))
						{
							ImGui::Text((std::string(_CurrentWorkingDir) + "\\").c_str());
							ImGui::SameLine();
							ImGui::InputText("", _TempImportFilePath, 256);
							if (ImGui::Button("Load")) {
								ImportSettings((std::string(_CurrentWorkingDir) + "\\") + std::string(_TempImportFilePath));
							}
							ImGui::EndMenu();
						}
						ImGui::EndMenu();
					}
					ImGui::EndMenuBar();
				}
				ImGui::Columns(1);
				if (_NewTreePositions.size() < _NewTreeAmount) {
					const auto currentSize = _NewTreePositions.size();
					_NewTreeParameters.resize(_NewTreeAmount);
					for (auto i = currentSize; i < _NewTreeAmount; i++)
					{
						LoadDefaultTreeParameters(1, _NewTreeParameters[i]);
					}
					_NewTreePositions.resize(_NewTreeAmount);
				}
				for (auto i = 0; i < _NewTreeAmount; i++)
				{
					std::string title = "New Tree No.";

					title += std::to_string(i);

					bool opened = ImGui::TreeNodeEx(title.c_str(), ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_NoAutoOpenOnLog | (_CurrentFocusedNewTreeIndex == i ? ImGuiTreeNodeFlags_Framed : ImGuiTreeNodeFlags_FramePadding));

					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
						_CurrentFocusedNewTreeIndex = i;
					}
					if (opened) {
						ImGui::TreePush();
						ImGui::InputFloat3(("Position##" + std::to_string(i)).c_str(), &_NewTreePositions[i].x);
						ImGui::TreePop();
					}
				}

				ImGui::EndChild();
				ImGui::PopStyleVar();
				ImGui::SameLine();
				ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
				ImGui::BeginChild("ChildR", ImVec2(400, 400), true, ImGuiWindowFlags_None | ImGuiWindowFlags_MenuBar);
				if (ImGui::BeginMenuBar())
				{
					if (ImGui::BeginMenu("Tree parameters")) {
						ImGui::Checkbox("Show full parameters", &_DisplayFullParam);

						if (ImGui::BeginMenu("Export"))
						{
							ImGui::Text((std::string(_CurrentWorkingDir) + "\\").c_str());
							ImGui::SameLine();
							ImGui::InputText("", _TempExportFilePath, 256);
							if (ImGui::Button("Save")) {
								//TODO: Save parameters
								ExportTreeParameters((std::string(_CurrentWorkingDir) + "\\") + std::string(_TempExportFilePath), _NewTreeParameters[_CurrentFocusedNewTreeIndex]);
							}
							ImGui::EndMenu();
						}
						if (ImGui::BeginMenu("Import"))
						{
							if (ImGui::Button("Load preset 1")) {
								LoadDefaultTreeParameters(1, _NewTreeParameters[_CurrentFocusedNewTreeIndex]);
							}
							if (ImGui::Button("Load preset 2")) {
								LoadDefaultTreeParameters(2, _NewTreeParameters[_CurrentFocusedNewTreeIndex]);
							}
							if (ImGui::Button("Load preset F6a")) {
								LoadDefaultTreeParameters(3, _NewTreeParameters[_CurrentFocusedNewTreeIndex]);
							}
							if (ImGui::Button("Load preset F6b")) {
								LoadDefaultTreeParameters(4, _NewTreeParameters[_CurrentFocusedNewTreeIndex]);
							}
							if (ImGui::Button("Load preset F6c")) {
								LoadDefaultTreeParameters(5, _NewTreeParameters[_CurrentFocusedNewTreeIndex]);
							}
							if (ImGui::Button("Load preset F6d")) {
								LoadDefaultTreeParameters(6, _NewTreeParameters[_CurrentFocusedNewTreeIndex]);
							}
							if (ImGui::Button("Load preset F6e")) {
								LoadDefaultTreeParameters(7, _NewTreeParameters[_CurrentFocusedNewTreeIndex]);
							}
							if (ImGui::Button("Load preset F6f")) {
								LoadDefaultTreeParameters(8, _NewTreeParameters[_CurrentFocusedNewTreeIndex]);
							}

							ImGui::Text((std::string(_CurrentWorkingDir) + "\\").c_str());
							ImGui::SameLine();
							ImGui::InputText("", _TempImportFilePath, 256);
							if (ImGui::Button("Load")) {
								//TODO: Load and apply parameters
								_NewTreeParameters[_CurrentFocusedNewTreeIndex] = ImportTreeParameters((std::string(_CurrentWorkingDir) + "\\") + std::string(_TempImportFilePath));
							}
							ImGui::EndMenu();
						}

						ImGui::EndMenu();
					}

					ImGui::EndMenuBar();
				}
				ImGui::Columns(1);
				ImGui::PushItemWidth(200);
				if (_DisplayFullParam) {
#pragma region Show params
					ImGui::InputInt("Seed", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].Seed);

					ImGui::InputInt("Lateral Bud Number", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].LateralBudPerNode);

					ImGui::InputFloat("Apical Angle Var", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].VarianceApicalAngle);

					ImGui::InputFloat2("Branching Angle M/Var", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].BranchingAngleMean);

					ImGui::InputFloat2("Roll Angle M/Var", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].RollAngleMean);

					ImGui::InputFloat2("Extinction Prob A/L", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].ApicalBudKillProbability);

					ImGui::InputFloat3("AD Base/Dis/Age", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].ApicalDominanceBase);

					ImGui::InputFloat("Growth Rate", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].GrowthRate);

					ImGui::InputFloat2("Node Len Base/Age", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].InternodeLengthBase);

					ImGui::InputFloat4("AC Base/Age/Lvl/Dist", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].ApicalControlBase);

					ImGui::InputInt("Max Bud Age", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].MaxBudAge);

					ImGui::InputFloat("Phototropism", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].Phototropism);

					ImGui::InputFloat2("Gravitropism Base/Age", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].GravitropismBase);

					ImGui::InputFloat("PruningFactor", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].PruningFactor);
					
					ImGui::InputFloat("LowBranchPruningFactor", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].LowBranchPruningFactor);
					
					ImGui::InputFloat("RemovalFactor Thickness", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].ThicknessRemovalFactor);
					
					ImGui::InputFloat("GravityBendingStrength", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].GravityBendingStrength);

					ImGui::InputFloat2("Lighting Factor A/L", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].ApicalBudLightingFactor);

					ImGui::InputFloat2("Gravity Base/BPCo", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].SaggingFactor);

					ImGui::InputFloat2("Thickness End/Fac", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].EndNodeThickness);

#pragma endregion

				}
				else {

				}
				ImGui::PopItemWidth();

				ImGui::EndChild();
				ImGui::PopStyleVar();

				ImGui::Separator();


				if (ImGui::Button("OK", ImVec2(120, 0))) {
					//Create tree here.
					for (auto i = 0; i < _NewTreeAmount; i++) {
						CreateTree(_DefaultTreeSurfaceMaterial1, _DefaultTreeLeafMaterial1, _DefaultTreeLeafMesh, _NewTreeParameters[i], _NewTreePositions[i], true);
					}
					ImGui::CloseCurrentPopup();
				}
				ImGui::SetItemDefaultFocus();
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
				ImGui::EndPopup();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	ImGui::Begin("Tree Utilities");
	if (ImGui::CollapsingHeader("Tree Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::InputFloat("Limit angle", &_DirectionPruningLimitAngle, 0.0f, 0.0f, "%.1f");
		ImGui::Checkbox("Enable direction pruning", &_EnableDirectionPruning);
		if (ImGui::SliderFloat("Gravity", &_Gravity, 0.0f, 20.0f))
		{
			_GravityChanged = true;
		}
		ImGui::InputFloat("Mesh resolution", &_MeshGenerationResolution, 0.0f, 0.0f, "%.5f");
		ImGui::InputFloat("Branch subdivision", &_MeshGenerationSubdivision, 0.0f, 0.0f, "%.5f");
		if (ImGui::Button("Generate mesh for all trees")) {
			auto trees = std::vector<Entity>();
			_TreeQuery.ToEntityArray(trees);
			for (auto& tree : trees)
			{
				TreeManager::GenerateSimpleMeshForTree(tree, _MeshGenerationResolution, _MeshGenerationSubdivision);
			}

		}
		ImGui::Separator();

		ImGui::Checkbox("Display convex hulls", &_DisplayConvexHull);
		if (ImGui::Button("Generate Convex Hull for all trees"))
		{
			std::vector<Entity> trees;
			_TreeQuery.ToEntityArray(trees);
			for (auto& tree : trees)
			{
				BuildConvexHullForTree(tree);
			}
		}

		ImGui::Separator();
		ImGui::InputInt("Iterations", &_NewPushIteration);
		if (ImGui::Button("Push iterations to all trees"))
		{
			EntityManager::ForEach<TreeAge>(_TreeQuery, [this](int i, Entity tree, TreeAge* age)
				{
					age->ToGrowIteration += _NewPushIteration;
				});
		}
		if (ImGui::Button(_Growing ? "Pause growing" : "Resume growing")) {
			_Growing = !_Growing;
		}
		ImGui::Separator();
		if (ImGui::Button("Delete all trees")) {
			ImGui::OpenPopup("Delete Warning");
		}
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		if (ImGui::BeginPopupModal("Delete Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure? All trees will be removed!");
			if (ImGui::Button("Yes, delete all!", ImVec2(120, 0))) {
				TreeManager::DeleteAllTrees();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
			ImGui::EndPopup();
		}
	}
	ImGui::Separator();
	ImGui::Spacing();
	if (ImGui::CollapsingHeader("I/O", ImGuiTreeNodeFlags_DefaultOpen)) {
		static char sceneOutputName[256] = {};
		ImGui::InputText("File name", sceneOutputName, 255);
		if (ImGui::Button(("Export scene as " + std::string(sceneOutputName) + ".obj").c_str())) {
			TreeScene::ExportSceneAsOBJ(sceneOutputName);
		}
	}
	ImGui::End();

}

void TreeUtilities::PlantSimulationSystem::ExportSettings(const std::string& path)
{
	std::ofstream of;
	of.open((path + ".ts").c_str(), std::ofstream::out | std::ofstream::trunc);
	if (of.is_open())
	{
		std::string output = "";
		output += "Amount:\n"; output += std::to_string(_NewTreeAmount);
		for (auto i = 0; i < _NewTreeAmount; i++)
		{
			output += "\nPosition\n";
			output += std::to_string(_NewTreePositions[i].x);
			output += "\n";
			output += std::to_string(_NewTreePositions[i].y);
			output += "\n";
			output += std::to_string(_NewTreePositions[i].z);
			output += "\nSeed\n"; output += std::to_string(_NewTreeParameters[i].Seed);
#pragma region Geometric
			output += "\nLateralBudPerNode\n"; output += std::to_string(_NewTreeParameters[i].LateralBudPerNode);
			output += "\nVarianceApicalAngle\n";  output += std::to_string(_NewTreeParameters[i].VarianceApicalAngle);
			output += "\nBranchingAngleMean\n";  output += std::to_string(_NewTreeParameters[i].BranchingAngleMean);
			output += "\nBranchingAngleVariance\n";  output += std::to_string(_NewTreeParameters[i].BranchingAngleVariance);
			output += "\nRollAngleMean\n";  output += std::to_string(_NewTreeParameters[i].RollAngleMean);
			output += "\nRollAngleVariance\n";  output += std::to_string(_NewTreeParameters[i].RollAngleVariance);
#pragma endregion
#pragma region Bud fate
			output += "\nApicalBudKillProbability\n"; output += std::to_string(_NewTreeParameters[i].ApicalBudKillProbability);
			output += "\nLateralBudKillProbability\n"; output += std::to_string(_NewTreeParameters[i].LateralBudKillProbability);
			output += "\nApicalDominanceBase\n";  output += std::to_string(_NewTreeParameters[i].ApicalDominanceBase);
			output += "\nApicalDominanceDistanceFactor\n";  output += std::to_string(_NewTreeParameters[i].ApicalDominanceDistanceFactor);
			output += "\nApicalDominanceAgeFactor\n";  output += std::to_string(_NewTreeParameters[i].ApicalDominanceAgeFactor);
			output += "\nGrowthRate\n"; output += std::to_string(_NewTreeParameters[i].GrowthRate);
			output += "\nBranchNodeLengthBase\n";  output += std::to_string(_NewTreeParameters[i].InternodeLengthBase);
			output += "\nBranchNodeLengthAgeFactor\n";  output += std::to_string(_NewTreeParameters[i].InternodeLengthAgeFactor);
			output += "\nApicalControlBase\n";  output += std::to_string(_NewTreeParameters[i].ApicalControlBase);
			output += "\nApicalControlAgeFactor\n";  output += std::to_string(_NewTreeParameters[i].ApicalControlAgeFactor);
			output += "\nApicalControlLevelFactor\n";  output += std::to_string(_NewTreeParameters[i].ApicalControlLevelFactor);
			output += "\nApicalControlDistanceFactor\n";  output += std::to_string(_NewTreeParameters[i].ApicalControlDistanceFactor);
			output += "\nMaxBudAge\n"; output += std::to_string(_NewTreeParameters[i].MaxBudAge);
#pragma endregion
#pragma region Environmental
			output += "\nPhototropism\n"; output += std::to_string(_NewTreeParameters[i].Phototropism);
			output += "\nGravitropismBase\n"; output += std::to_string(_NewTreeParameters[i].GravitropismBase);
			output += "\nGravitropismLevelFactor\n"; output += std::to_string(_NewTreeParameters[i].GravitropismLevelFactor);
			output += "\nPruningFactor\n"; output += std::to_string(_NewTreeParameters[i].PruningFactor);
			output += "\nLowBranchPruningFactor\n"; output += std::to_string(_NewTreeParameters[i].LowBranchPruningFactor);
			output += "\nThicknessRemovalFactor\n"; output += std::to_string(_NewTreeParameters[i].ThicknessRemovalFactor);
			output += "\nGravityBendingStrength\n"; output += std::to_string(_NewTreeParameters[i].GravityBendingStrength);
			output += "\nApicalBudLightingFactor\n"; output += std::to_string(_NewTreeParameters[i].ApicalBudLightingFactor);
			output += "\nLateralBudLightingFactor\n"; output += std::to_string(_NewTreeParameters[i].LateralBudLightingFactor);
#pragma endregion
#pragma region Sagging
			output += "\nSaggingFactor\n"; output += std::to_string(_NewTreeParameters[i].SaggingFactor);
			output += "\nSaggingForceBackPropagateFixedCoefficient\n"; output += std::to_string(_NewTreeParameters[i].SaggingForceBackPropagateFixedCoefficient);
#pragma endregion
			//output += "\nAge\n"; output += std::to_string(_NewTreeParameters[i].Age);
			output += "\nEndNodeThickness\n"; output += std::to_string(_NewTreeParameters[i].EndNodeThickness);
			output += "\nThicknessControlFactor\n"; output += std::to_string(_NewTreeParameters[i].ThicknessControlFactor);
		}
		of.write(output.c_str(), output.size());
		of.flush();
		of.close();
		Debug::Log("Tree group saved: " + path + ".ts");
	}
	else
	{
		Debug::Error("Can't open file!");
	}
}

void PlantSimulationSystem::ImportSettings(const std::string& path)
{
	std::ifstream ifs;
	ifs.open((path + ".ts").c_str());
	if (ifs.is_open())
	{
		std::string temp;
		ifs >> temp; ifs >> _NewTreeAmount;
		_NewTreeParameters.resize(_NewTreeAmount);
		_NewTreePositions.resize(_NewTreeAmount);
		_NewTreePositions.resize(_NewTreeAmount);
		for (auto i = 0; i < _NewTreeAmount; i++)
		{
			ifs >> temp; ifs >> _NewTreePositions[i].x >> _NewTreePositions[i].y >> _NewTreePositions[i].z;
			ifs >> temp; ifs >> _NewTreeParameters[i].Seed;
#pragma region Geometric
			ifs >> temp; ifs >> _NewTreeParameters[i].LateralBudPerNode;

			ifs >> temp; ifs >> _NewTreeParameters[i].VarianceApicalAngle; // Training target

			ifs >> temp; ifs >> _NewTreeParameters[i].BranchingAngleMean; // Training target
			ifs >> temp; ifs >> _NewTreeParameters[i].BranchingAngleVariance; // Training target

			ifs >> temp; ifs >> _NewTreeParameters[i].RollAngleMean; // Training target
			ifs >> temp; ifs >> _NewTreeParameters[i].RollAngleVariance; // Training target
#pragma endregion
#pragma region Bud fate
			ifs >> temp; ifs >> _NewTreeParameters[i].ApicalBudKillProbability; // Useless.
			ifs >> temp; ifs >> _NewTreeParameters[i].LateralBudKillProbability; //Useless.

			ifs >> temp; ifs >> _NewTreeParameters[i].ApicalDominanceBase;
			ifs >> temp; ifs >> _NewTreeParameters[i].ApicalDominanceDistanceFactor; // Training target
			ifs >> temp; ifs >> _NewTreeParameters[i].ApicalDominanceAgeFactor; // Training target

			ifs >> temp; ifs >> _NewTreeParameters[i].GrowthRate;

			ifs >> temp; ifs >> _NewTreeParameters[i].InternodeLengthBase; //Fixed
			ifs >> temp; ifs >> _NewTreeParameters[i].InternodeLengthAgeFactor; // Training target


			ifs >> temp; ifs >> _NewTreeParameters[i].ApicalControlBase; // Training target
			ifs >> temp; ifs >> _NewTreeParameters[i].ApicalControlAgeFactor; // Training target
			ifs >> temp; ifs >> _NewTreeParameters[i].ApicalControlLevelFactor; // Training target
			ifs >> temp; ifs >> _NewTreeParameters[i].ApicalControlDistanceFactor; // Training target

			ifs >> temp; ifs >> _NewTreeParameters[i].MaxBudAge;
#pragma endregion
#pragma region Environmental
			ifs >> temp; ifs >> _NewTreeParameters[i].Phototropism; // Based on tree leaf properties.
			ifs >> temp; ifs >> _NewTreeParameters[i].GravitropismBase; //Based on tree material properties.
			ifs >> temp; ifs >> _NewTreeParameters[i].GravitropismLevelFactor;  //Based on tree material properties.

			ifs >> temp; ifs >> _NewTreeParameters[i].PruningFactor;
			ifs >> temp; ifs >> _NewTreeParameters[i].LowBranchPruningFactor;
			
			ifs >> temp; ifs >> _NewTreeParameters[i].ThicknessRemovalFactor;

			ifs >> temp; ifs >> _NewTreeParameters[i].GravityBendingStrength;

			ifs >> temp; ifs >> _NewTreeParameters[i].ApicalBudLightingFactor;
			ifs >> temp; ifs >> _NewTreeParameters[i].LateralBudLightingFactor;
#pragma endregion

#pragma region Sagging
			ifs >> temp; ifs >> _NewTreeParameters[i].SaggingFactor;
			ifs >> temp; ifs >> _NewTreeParameters[i].SaggingForceBackPropagateFixedCoefficient;
#pragma endregion
			ifs >> temp; ifs >> _NewTreeParameters[i].EndNodeThickness;
			ifs >> temp; ifs >> _NewTreeParameters[i].ThicknessControlFactor;
		}
	}
	else
	{
		Debug::Error("Can't open file!");
	}
}

void TreeUtilities::PlantSimulationSystem::FixedUpdate()
{
	auto trees = std::vector<Entity>();
	_TreeQuery.ToEntityArray(trees);

	TryGrowAllTrees(trees);

	if (_GravityChanged)
	{
		_GravityChanged = false;
		CalculatePhysics(trees);
	}
}

bool TreeUtilities::PlantSimulationSystem::GrowTree(Entity& treeEntity)
{
	if (EntityManager::GetChildrenAmount(treeEntity) == 0) return false;
#pragma region Collect tree data
	auto treeData = EntityManager::GetSharedComponent<TreeData>(treeEntity);
	TreeAge treeAge = EntityManager::GetComponentData<TreeAge>(treeEntity);
	TreeParameters treeParameters = EntityManager::GetComponentData<TreeParameters>(treeEntity);
	TreeIndex treeIndex = EntityManager::GetComponentData<TreeIndex>(treeEntity);
	LocalToWorld treeLocalToWorld = EntityManager::GetComponentData<LocalToWorld>(treeEntity);
	Rotation treeRotation = EntityManager::GetComponentData<Rotation>(treeEntity);
#pragma endregion
	if (treeAge.ToGrowIteration == 0) {
		return false;
	}

#pragma region Prepare info
	if (treeAge.Value == 0) {
		treeData->CurrentSeed = treeParameters.Seed;
		srand(treeData->CurrentSeed);
	}
	else {
		srand(treeData->CurrentSeed);
		treeData->CurrentSeed = glm::linearRand(0.0f, 1.0f) * INT_MAX;
	}
	treeData->MaxBranchingDepth = 3;
	const auto timeOff = treeAge.Value + treeAge.ToGrowIteration + 4;
	treeData->ApicalDominanceTimeVal.resize(timeOff);
	treeData->GravitropismLevelVal.resize(timeOff);
	treeData->GravitropismLevelVal.resize(timeOff);
	treeData->ApicalControlTimeVal.resize(timeOff);
	treeData->ApicalControlTimeLevelVal.resize(timeOff);
	for (auto t = 0; t < timeOff; t++) {
		treeData->ApicalDominanceTimeVal.at(t) = glm::pow(treeParameters.ApicalDominanceAgeFactor, t);
		treeData->GravitropismLevelVal.at(t) = treeParameters.GravitropismBase + t * treeParameters.GravitropismLevelFactor;
		treeData->ApicalControlTimeVal.at(t) = treeParameters.ApicalControlBase * glm::pow(treeParameters.ApicalControlAgeFactor, t);

		treeData->ApicalControlTimeLevelVal.at(t).resize(timeOff);
		float baseApicalControlVal = treeData->ApicalControlTimeVal.at(t);
		treeData->ApicalControlTimeLevelVal.at(t).at(0) = 1.0f;
		float currentVal = 1;
		for (auto level = 1; level < timeOff; level++) {
			if (baseApicalControlVal >= 1) {
				currentVal *= 1.0f + (baseApicalControlVal - 1.0f) * glm::pow(treeParameters.ApicalControlLevelFactor, level);
				treeData->ApicalControlTimeLevelVal.at(t).at(level) = 1.0f / currentVal;
			}
			else {
				currentVal *= 1.0f - (1.0f - baseApicalControlVal) * glm::pow(treeParameters.ApicalControlLevelFactor, level);
				treeData->ApicalControlTimeLevelVal.at(t).at(level) = currentVal;
			}
		}
	}
#pragma endregion
#pragma region Update branch structure information
	Entity rootInternode = EntityManager::GetChildren(treeEntity).at(0);
	bool growed = GrowShoots(rootInternode, treeData, treeAge, treeParameters, treeIndex);

	UpdateInternodeLength(rootInternode);
	InternodeInfo tempBNInfo = EntityManager::GetComponentData<InternodeInfo>(rootInternode);
	tempBNInfo.Level = 0;
	tempBNInfo.MaxActivatedChildLevel = 0;
	tempBNInfo.DistanceToBranchStart = 0;
	EntityManager::SetComponentData(rootInternode, tempBNInfo);
	UpdateInternodeActivatedLevel(rootInternode);
#pragma endregion

	treeAge.Value++;
	treeAge.ToGrowIteration--;
	EntityManager::SetComponentData(treeEntity, treeAge);
	if (growed) {
		UpdateInternodeResource(rootInternode, treeParameters, treeAge);
		EvaluatePruning(rootInternode, treeParameters, treeAge, treeData);
		EvaluateRemoval(rootInternode, treeParameters);
		if (_EnableDirectionPruning) EvaluateDirectionPruning(rootInternode, glm::normalize(glm::vec3(treeLocalToWorld.Value[3])), _DirectionPruningLimitAngle);
	}
	return growed;
}

bool TreeUtilities::PlantSimulationSystem::GrowShoots(Entity& internode, std::shared_ptr<TreeData>& treeInfo, TreeAge& treeAge, TreeParameters& treeParameters, TreeIndex& treeIndex)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	if (internodeInfo.Pruned) return false;
#pragma region Grow child and collect Inhibitor
	internodeInfo.Inhibitor = 0;
	bool ret = false;
	EntityManager::ForEachChild(internode, [&ret, this, &internodeInfo, &treeInfo, &treeAge, &treeParameters, &treeIndex](Entity childNode)
		{
			if (GrowShoots(childNode, treeInfo, treeAge, treeParameters, treeIndex)) ret = true;
			auto childInternodeInfo = EntityManager::GetComponentData<InternodeInfo>(childNode);
			internodeInfo.Inhibitor += childInternodeInfo.Inhibitor * childInternodeInfo.ParentInhibitorFactor;
		}
	);
	if (internodeInfo.ActivatedBudsAmount == 0) return ret;
#pragma endregion
	auto internodeIllumination = EntityManager::GetComponentData<Illumination>(internode);
	auto internodeData = EntityManager::GetSharedComponent<InternodeData>(internode);
	//auto internodeIndex = EntityManager::GetComponentData<InternodeIndex>(internode);
	float lateralInhibitorToAdd = 0;
	for (auto& bud : internodeData->Buds) {
#pragma region Bud kill probability
		float budKillProbability = 0;
		if (bud.IsApical) {
			budKillProbability = EntityManager::HasComponentData<TreeInfo>(EntityManager::GetParent(internode)) ? 0 : treeParameters.ApicalBudKillProbability;
		}
		else {
			budKillProbability = treeParameters.LateralBudKillProbability;
		}
		if (glm::linearRand(0.0f, 1.0f) < budKillProbability) {
			DeactivateBud(internodeInfo, bud);
			continue;
		}
#pragma endregion
#pragma region Flush check
		//compute probability that the given bud can grow
		float budGrowProbability = 1.0f;
		// first take into account the apical dominance
		if (internodeInfo.Inhibitor > 0) budGrowProbability *= glm::exp(-internodeInfo.Inhibitor);
		// now take into consideration the light on the bud
		float illumination = internodeIllumination.Value;
		if (illumination < 1.0f) {
			budGrowProbability *= glm::pow(illumination, bud.IsApical ? treeParameters.ApicalBudLightingFactor : treeParameters.LateralBudLightingFactor);
		}

		// now check whether the bud is going to flush or not
		bool flush = treeAge.Value < 2 ? true : budGrowProbability >= glm::linearRand(0.0f, 1.0f);
#pragma endregion
		bool growSucceed = false;
		if (flush) {
			bool isLateral = !(bud.IsApical && EntityManager::GetChildrenAmount(internode) == 0);
#pragma region Compute total grow distance and internodes amount.
			int level = internodeInfo.Level;
			if (isLateral) level++;
			float apicalControl = GetApicalControl(treeInfo, internodeInfo, treeParameters, treeAge, level);
			float distanceToGrow = treeParameters.GrowthRate * apicalControl;
			int internodesToGrow = glm::floor(distanceToGrow + 0.5f);
			if (internodesToGrow != 0) {
				growSucceed = true;
			}
#pragma endregion
#pragma region Grow new shoot
			if (growSucceed) {
				float internodeLength = distanceToGrow / static_cast<float>(internodesToGrow);
				internodeLength *= treeParameters.InternodeLengthBase * glm::pow(treeParameters.InternodeLengthAgeFactor, treeAge.Value);
				int level = internodeInfo.Level;
				if (!bud.IsApical) {
					level++;
				}
				Entity prevInternode = internode;
				InternodeInfo prevInternodeInfo = internodeInfo;
				glm::vec3 prevEulerAngle = bud.EulerAngles;
				glm::quat prevInternodeRotation;
				prevInternodeRotation = internodeInfo.DesiredGlobalRotation;
#pragma region Create internodes
				for (int selectedNewNodeIndex = 0; selectedNewNodeIndex < internodesToGrow; selectedNewNodeIndex++) {
#pragma region Setup internode
					Entity newInternode = TreeManager::CreateInternode(treeIndex, prevInternode);
					auto newInternodeData = EntityManager::GetSharedComponent<InternodeData>(newInternode);
					InternodeInfo newInternodeInfo = EntityManager::GetComponentData<InternodeInfo>(newInternode);
					newInternodeInfo.ApicalBudExist = true;
					newInternodeInfo.ActivatedBudsAmount = treeParameters.LateralBudPerNode + 1;
					newInternodeInfo.DistanceToParent = internodeLength;
					newInternodeInfo.Level = level;
					newInternodeInfo.Pruned = false;
					newInternodeInfo.IsApical = prevInternodeInfo.IsApical;
					//if (newInternodeInfo.IsApical) newInternodeInfo.Level = prevInternodeInfo.Level;
					//else newInternodeInfo.Level = prevInternodeInfo.Level + 1;
					//newInternodeInfo.MaxActivatedChildLevel = level;
					newInternodeInfo.ParentInhibitorFactor = glm::pow(treeParameters.ApicalDominanceDistanceFactor, newInternodeInfo.DistanceToParent);

#pragma endregion
#pragma region Transforms for internode
					newInternodeInfo.DesiredLocalRotation = glm::quat(prevEulerAngle);
#pragma region Roll internode
					glm::vec3 rollAngles = glm::vec3(0.0f, 0.0f, glm::radians(treeParameters.RollAngleMean + treeParameters.RollAngleVariance * glm::linearRand(-1, 1)));
					newInternodeInfo.DesiredLocalRotation *= glm::quat(rollAngles);
#pragma endregion

#pragma region Apply phototropism and gravitropism
					float gravitropism = treeInfo->GravitropismLevelVal.at(newInternodeInfo.Level);
					glm::quat globalRawRotation = prevInternodeRotation * newInternodeInfo.DesiredLocalRotation;
					glm::vec3 rawFront = globalRawRotation * glm::vec3(0.0f, 0.0f, -1.05f);
					glm::vec3 rawUp = globalRawRotation * glm::vec3(0.0f, 1.05f, 0.0f);
					glm::vec3 gravityDir = glm::vec3(0.0f, -1.0f, 0.0f);
					rawFront += gravityDir * gravitropism;
					if (internodeIllumination.Value > 0) {
						rawFront += glm::normalize(-internodeIllumination.LightDir) * treeParameters.Phototropism;
					}
					rawFront = glm::normalize(rawFront);
					rawUp = glm::normalize(glm::cross(glm::cross(rawFront, rawUp), rawFront));
					globalRawRotation = glm::quatLookAt(rawFront, rawUp);
					newInternodeInfo.DesiredLocalRotation = glm::inverse(prevInternodeRotation) * globalRawRotation;
					newInternodeInfo.DesiredGlobalRotation = globalRawRotation;
					prevInternodeRotation = globalRawRotation;
#pragma endregion
#pragma endregion
#pragma region Create Apical Bud
					Bud newApicalBud;
					newApicalBud.EulerAngles = glm::vec3(glm::gaussRand(glm::vec2(0.0f), glm::vec2(glm::radians(treeParameters.VarianceApicalAngle / 2.0f))), 0.0f);
					newApicalBud.IsActive = true;
					newApicalBud.IsApical = true;
					newApicalBud.StartAge = treeAge.Value;
					newInternodeData->Buds.push_back(newApicalBud);
#pragma endregion
#pragma region Create Lateral Buds
					for (int selectedNewBudIndex = 0; selectedNewBudIndex < treeParameters.LateralBudPerNode; selectedNewBudIndex++) {
						Bud newLateralBud;
						float rollAngle = 360.0f * (selectedNewBudIndex + 1) / treeParameters.LateralBudPerNode + treeParameters.BranchingAngleVariance * glm::linearRand(-1, 1);
						float branchAngle = treeParameters.BranchingAngleMean + treeParameters.BranchingAngleVariance * glm::gaussRand(0.0f, 0.5f);
						newLateralBud.EulerAngles = glm::vec3(glm::radians(branchAngle), 0.0f, glm::radians(rollAngle));
						newLateralBud.IsActive = true;
						newLateralBud.IsApical = false;
						newLateralBud.StartAge = treeAge.Value;
						newInternodeData->Buds.push_back(newLateralBud);
					}
#pragma endregion
					prevEulerAngle = newApicalBud.EulerAngles;
					prevInternode = newInternode;
					prevInternodeInfo = newInternodeInfo;
#pragma region Apply new internode info
					EntityManager::SetComponentData(newInternode, newInternodeInfo);
#pragma endregion
				}
#pragma endregion
				DeactivateBud(internodeInfo, bud);
#pragma region Add inhibitor to this internode.
				float localInhibitor = 0;
				if (treeAge.Value <= 1) localInhibitor += treeParameters.ApicalDominanceBase;
				else {
					localInhibitor += treeParameters.ApicalDominanceBase * treeInfo->ApicalDominanceTimeVal.at(treeAge.Value);
				}
				if (bud.IsApical) {
					internodeInfo.Inhibitor += localInhibitor;
					EntityManager::SetComponentData(internode, internodeInfo);
				}
				else {
					lateralInhibitorToAdd += localInhibitor;
				}
#pragma endregion
			}
#pragma endregion
		}
#pragma region If the bud didnt flush then check whether we should remove it because of the old age.
		if (!growSucceed) {
			int budAge = treeAge.Value - bud.StartAge;
			if (budAge > treeParameters.MaxBudAge) {
				DeactivateBud(internodeInfo, bud);
			}
		}
		else {
			ret = true;
		}
#pragma endregion
	}
	for (int i = 0; i < internodeData->Buds.size(); i++)
	{
		if (!internodeData->Buds[i].IsActive)
		{
			internodeData->Buds.erase(internodeData->Buds.begin() + i);
			i--;
		}
	}
	internodeInfo.Inhibitor += lateralInhibitorToAdd;
	EntityManager::SetComponentData(internode, internodeInfo);
	return ret;
}

#pragma region Helpers
void TreeUtilities::PlantSimulationSystem::CalculatePhysics(std::vector<Entity>& trees)
{
	for (auto& tree : trees) {
		Rotation rotation = EntityManager::GetComponentData<Rotation>(tree);
		TreeParameters treeParameters = EntityManager::GetComponentData<TreeParameters>(tree);
		if (EntityManager::GetChildrenAmount(tree) == 0) continue;
		Entity rootInternode = EntityManager::GetChildren(tree).at(0);
		CalculateDirectGravityForce(tree, _Gravity);
		BackPropagateForce(rootInternode, treeParameters.SaggingForceBackPropagateFixedCoefficient);
		glm::mat4 transform = glm::identity<glm::mat4>(); //glm::translate(glm::mat4(1.0f), glm::vec3(0.0f))* glm::mat4_cast(glm::quatLookAt(glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)))* glm::scale(glm::vec3(1.0f));

		auto immc = EntityManager::GetSharedComponent<InstancedMeshMaterialComponent>(tree);
		immc->Matrices.clear();
		UpdateLocalTransform(rootInternode, treeParameters, transform, rotation.Value, immc->Matrices);
		ApplyLocalTransform(tree);
		TreeManager::GetInternodeSystem()->RefreshConnections();
	}
}

void TreeUtilities::PlantSimulationSystem::LoadDefaultTreeParameters(int preset, TreeParameters& tps)
{
	switch (preset)
	{
	case 1:
#pragma region Set default tree param
		tps.Seed = 1;
		tps.VarianceApicalAngle = 0.43f;
		tps.LateralBudPerNode = 2;
		tps.BranchingAngleMean = 27.2f;
		tps.BranchingAngleVariance = 0.0374f;
		tps.RollAngleMean = 113.11f;
		tps.RollAngleVariance = 13.09f;

		tps.ApicalBudKillProbability = 0.99904f;
		tps.LateralBudKillProbability = 0.006269f;
		tps.ApicalBudLightingFactor = 1.0;
		tps.LateralBudLightingFactor = 1.0006f;
		tps.ApicalDominanceBase = 1.0f;
		tps.ApicalDominanceDistanceFactor = 0.377778f;
		tps.ApicalDominanceAgeFactor = 0.447047f;
		tps.GrowthRate = 1.307f;
		tps.InternodeLengthBase = 0.9238272f;
		tps.InternodeLengthAgeFactor = 0.9559f;

		tps.ApicalControlBase = 0.93576f;
		tps.ApicalControlAgeFactor = 0.918157f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.4244511f;
		tps.GravitropismBase = 0.2396032f;
		tps.GravitropismLevelFactor = 0.0f;
		tps.PruningFactor = 0.05f;
		tps.LowBranchPruningFactor = 0.639226f;
		tps.ThicknessRemovalFactor = 99999;
		tps.GravityBendingStrength = 0.2f;
		tps.SaggingFactor = 0.0506f;
		tps.MaxBudAge = 10;

		tps.EndNodeThickness = 0.02f;
		tps.ThicknessControlFactor = 0.75f;
		tps.SaggingForceBackPropagateFixedCoefficient = 0.5f;
#pragma endregion
		break;
	case 2:
		tps.Seed = 1;
		tps.VarianceApicalAngle = 38;
		tps.LateralBudPerNode = 4;
		tps.BranchingAngleMean = 38;
		tps.BranchingAngleVariance = 2;
		tps.RollAngleMean = 91;
		tps.RollAngleVariance = 1;

		tps.ApicalBudKillProbability = 0;
		tps.LateralBudKillProbability = 0.21f;
		tps.ApicalBudLightingFactor = 0.39f;
		tps.LateralBudLightingFactor = 1.13f;
		tps.ApicalDominanceBase = 1.0f;
		tps.ApicalDominanceDistanceFactor = 0.13f;
		tps.ApicalDominanceAgeFactor = 0.82f;
		tps.GrowthRate = 0.98f;
		tps.InternodeLengthBase = 1.02f;
		tps.InternodeLengthAgeFactor = 0.97f;

		tps.ApicalControlBase = 2.4f;
		tps.ApicalControlAgeFactor = 0.85f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.29f;
		tps.GravitropismBase = 0.061f;
		tps.GravitropismLevelFactor = 0.0f;
		tps.PruningFactor = 0.05f;
		tps.LowBranchPruningFactor = 1.3f;
		tps.ThicknessRemovalFactor = 99999;
		tps.GravityBendingStrength = 0.73f;
		tps.SaggingFactor = 0.0506f;
		tps.MaxBudAge = 8;
		tps.EndNodeThickness = 0.02f;
		tps.ThicknessControlFactor = 0.6f;
		tps.SaggingForceBackPropagateFixedCoefficient = 0.5f;
		break;
	case 3:
		//F6a
		tps.Seed = 1;
		tps.VarianceApicalAngle = 38;
		tps.LateralBudPerNode = 2;
		tps.BranchingAngleMean = 38;
		tps.BranchingAngleVariance = 2;
		tps.RollAngleMean = 91;
		tps.RollAngleVariance = 1;

		tps.ApicalBudKillProbability = 0;
		tps.LateralBudKillProbability = 0.21f;
		tps.ApicalBudLightingFactor = 0.39f;
		tps.LateralBudLightingFactor = 1.13f;
		tps.ApicalDominanceBase = 3.13f;
		tps.ApicalDominanceDistanceFactor = 0.13f;
		tps.ApicalDominanceAgeFactor = 0.82f;
		tps.GrowthRate = 2.98f;
		tps.InternodeLengthBase = 0.55f;
		tps.InternodeLengthAgeFactor = 0.97f;

		tps.ApicalControlBase = 2.2f;
		tps.ApicalControlAgeFactor = 0.5f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.29f;
		tps.GravitropismBase = -0.41f;
		tps.GravitropismLevelFactor = 0.15f;
		tps.PruningFactor = 0.7f;
		tps.LowBranchPruningFactor = 1.3f;
		tps.ThicknessRemovalFactor = 99999;
		tps.GravityBendingStrength = 0.73f;
		tps.SaggingFactor = 0.0506f;
		tps.MaxBudAge = 8;
		tps.EndNodeThickness = 0.02f;
		tps.ThicknessControlFactor = 0.6f;
		tps.SaggingForceBackPropagateFixedCoefficient = 0.5f;
		break;
	case 4:
		//F6b
		tps.Seed = 1;
		tps.VarianceApicalAngle = 0;
		tps.LateralBudPerNode = 2;
		tps.BranchingAngleMean = 41;
		tps.BranchingAngleVariance = 3;
		tps.RollAngleMean = 87;
		tps.RollAngleVariance = 2;

		tps.ApicalBudKillProbability = 0;
		tps.LateralBudKillProbability = 0.21f;
		tps.ApicalBudLightingFactor = 1.05f;
		tps.LateralBudLightingFactor = 0.37f;
		tps.ApicalDominanceBase = 0.37f;
		tps.ApicalDominanceDistanceFactor = 0.31f;
		tps.ApicalDominanceAgeFactor = 0.9f;
		tps.GrowthRate = 1.9f;
		tps.InternodeLengthBase = 0.49f;
		tps.InternodeLengthAgeFactor = 0.98f;

		tps.ApicalControlBase = 2.27f;
		tps.ApicalControlAgeFactor = 0.9f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.15f;
		tps.GravitropismBase = -0.47f;//-0.41f;
		tps.GravitropismLevelFactor = 0.14f;
		tps.PruningFactor = 0.82f;
		tps.LowBranchPruningFactor = 2.83f;
		tps.ThicknessRemovalFactor = 99999;
		tps.GravityBendingStrength = 0.795f;
		tps.SaggingFactor = 0.0506f;
		tps.MaxBudAge = 8;
		tps.EndNodeThickness = 0.02f;
		tps.ThicknessControlFactor = 0.6f;
		tps.SaggingForceBackPropagateFixedCoefficient = 0.5f;
		break;

	case 5:
		//F6c
		tps.Seed = 1;
		tps.VarianceApicalAngle = 10;
		tps.LateralBudPerNode = 3;
		tps.BranchingAngleMean = 51;
		tps.BranchingAngleVariance = 4;
		tps.RollAngleMean = 100;
		tps.RollAngleVariance = 30;

		tps.ApicalBudKillProbability = 0;
		tps.LateralBudKillProbability = 0.015f;
		tps.ApicalBudLightingFactor = 0.36f;
		tps.LateralBudLightingFactor = 0.65f;
		tps.ApicalDominanceBase = 6.29f;
		tps.ApicalDominanceDistanceFactor = 0.9f;
		tps.ApicalDominanceAgeFactor = 0.87f;
		tps.GrowthRate = 3.26f;
		tps.InternodeLengthBase = 0.4f;
		tps.InternodeLengthAgeFactor = 0.96f;

		tps.ApicalControlBase = 6.2f;
		tps.ApicalControlAgeFactor = 0.9f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.42f;
		tps.GravitropismBase = -0.43f;//-0.43f;
		tps.GravitropismLevelFactor = 0.73f;
		tps.PruningFactor = 0.12f;
		tps.LowBranchPruningFactor = 1.25f;
		tps.ThicknessRemovalFactor = 99999;
		tps.GravityBendingStrength = 0.73f;
		tps.SaggingFactor = 0.0506f;
		tps.MaxBudAge = 8;
		tps.EndNodeThickness = 0.02f;
		tps.ThicknessControlFactor = 0.6f;
		tps.SaggingForceBackPropagateFixedCoefficient = 0.5f;
		break;
	case 6:
		//F6d
		tps.Seed = 1;
		tps.VarianceApicalAngle = 5;
		tps.LateralBudPerNode = 1;
		tps.BranchingAngleMean = 55;
		tps.BranchingAngleVariance = 5;
		tps.RollAngleMean = 130;
		tps.RollAngleVariance = 30;

		tps.ApicalBudKillProbability = 0;
		tps.LateralBudKillProbability = 0.01f;
		tps.ApicalBudLightingFactor = 0.5f;
		tps.LateralBudLightingFactor = 0.03f;
		tps.ApicalDominanceBase = 5.59f;
		tps.ApicalDominanceDistanceFactor = 0.979f;
		tps.ApicalDominanceAgeFactor = 0.5f;
		tps.GrowthRate = 4.25f;
		tps.InternodeLengthBase = 0.55f;
		tps.InternodeLengthAgeFactor = 0.95f;

		tps.ApicalControlBase = 5.5f;
		tps.ApicalControlAgeFactor = 0.91f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.32f;
		tps.GravitropismBase = -0.21f;//-0.43f;
		tps.GravitropismLevelFactor = 0.15f;
		tps.PruningFactor = 0.48f;
		tps.LowBranchPruningFactor = 5.5f;
		tps.ThicknessRemovalFactor = 99999;
		tps.GravityBendingStrength = 0.79f;
		tps.SaggingFactor = 0.0506f;
		tps.MaxBudAge = 8;
		tps.EndNodeThickness = 0.02f;
		tps.ThicknessControlFactor = 0.6f;
		tps.SaggingForceBackPropagateFixedCoefficient = 0.5f;
		break;
	case 7:
		//F6e
		tps.Seed = 1;
		tps.VarianceApicalAngle = 2;
		tps.LateralBudPerNode = 2;
		tps.BranchingAngleMean = 60;
		tps.BranchingAngleVariance = 3;
		tps.RollAngleMean = 130;
		tps.RollAngleVariance = 10;

		tps.ApicalBudKillProbability = 0;
		tps.LateralBudKillProbability = 0.18f;
		tps.ApicalBudLightingFactor = 0.03f;
		tps.LateralBudLightingFactor = 0.21f;
		tps.ApicalDominanceBase = 6.5f;
		tps.ApicalDominanceDistanceFactor = 0.91f;
		tps.ApicalDominanceAgeFactor = 0.55f;
		tps.GrowthRate = 2.4f;
		tps.InternodeLengthBase = 0.4f;
		tps.InternodeLengthAgeFactor = 0.97f;

		tps.ApicalControlBase = 5.5f;
		tps.ApicalControlAgeFactor = 0.92f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.05f;
		tps.GravitropismBase = -0.15f;//-0.43f;
		tps.GravitropismLevelFactor = 0.12f;
		tps.PruningFactor = 0.22f;
		tps.LowBranchPruningFactor = 1.11f;
		tps.ThicknessRemovalFactor = 99999;
		tps.GravityBendingStrength = 0.78f;
		tps.SaggingFactor = 0.0506f;
		tps.MaxBudAge = 8;
		tps.EndNodeThickness = 0.02f;
		tps.ThicknessControlFactor = 0.6f;
		tps.SaggingForceBackPropagateFixedCoefficient = 0.5f;
		break;
	case 8:
		//F6f
		tps.Seed = 1;
		tps.VarianceApicalAngle = 12;
		tps.LateralBudPerNode = 2;
		tps.BranchingAngleMean = 43;
		tps.BranchingAngleVariance = 3;
		tps.RollAngleMean = 80;
		tps.RollAngleVariance = 4;

		tps.ApicalBudKillProbability = 0;
		tps.LateralBudKillProbability = 0.21f;
		tps.ApicalBudLightingFactor = 0.36f;
		tps.LateralBudLightingFactor = 1.05f;
		tps.ApicalDominanceBase = 0.38f;
		tps.ApicalDominanceDistanceFactor = 0.9f;
		tps.ApicalDominanceAgeFactor = 0.31f;
		tps.GrowthRate = 1.9f;
		tps.InternodeLengthBase = 0.51f;
		tps.InternodeLengthAgeFactor = 0.98f;

		tps.ApicalControlBase = 3.25f;
		tps.ApicalControlAgeFactor = 0.7f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.15f;
		tps.GravitropismBase = -0.13f;//-0.43f;
		tps.GravitropismLevelFactor = 0.14f;
		tps.PruningFactor = 0.8f;
		tps.LowBranchPruningFactor = 2.9f;
		tps.ThicknessRemovalFactor = 99999;
		tps.GravityBendingStrength = 0.19f;
		tps.SaggingFactor = 0.0506f;
		tps.MaxBudAge = 8;
		tps.EndNodeThickness = 0.02f;
		tps.ThicknessControlFactor = 0.6f;
		tps.SaggingForceBackPropagateFixedCoefficient = 0.5f;
		break;
	}
}

void TreeUtilities::PlantSimulationSystem::TryGrowAllTrees(std::vector<Entity>& trees)
{
	bool growed = false;
	if (_Growing) {
		TreeManager::CalculateInternodeIllumination();
		for (auto& tree : trees) {

			Rotation rotation = EntityManager::GetComponentData<Rotation>(tree);
			LocalToWorld ltw = EntityManager::GetComponentData<LocalToWorld>(tree);
			TreeParameters treeParameters = EntityManager::GetComponentData<TreeParameters>(tree);
			if (GrowTree(tree)) {
				growed = true;
				CalculatePhysics(trees);
				CalculateCrownShyness(2.0f);
			}
		}
	}
	if (growed == false) {
		_Growing = false;
	}
}


inline float TreeUtilities::PlantSimulationSystem::GetApicalControl(std::shared_ptr<TreeData>& treeInfo, InternodeInfo& internodeInfo, TreeParameters& treeParameters, TreeAge& treeAge, int level) const
{
	float apicalControl = treeParameters.ApicalControlBase * glm::pow(treeParameters.ApicalControlAgeFactor, treeAge.Value);
	if (treeInfo->ApicalControlTimeVal.at(treeAge.Value) < 1.0f) {
		const int reversedLevel = internodeInfo.MaxActivatedChildLevel - level + 1;
		return treeInfo->ApicalControlTimeLevelVal.at(treeAge.Value)[reversedLevel];
	}
	return treeInfo->ApicalControlTimeLevelVal.at(treeAge.Value)[level];
}

void TreeUtilities::PlantSimulationSystem::UpdateInternodeLength(Entity& internode)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	internodeInfo.DistanceToBranchEnd = 0;
	internodeInfo.TotalDistanceToBranchEnd = 0;
	EntityManager::ForEachChild(internode, [this, &internodeInfo](Entity child)
		{
			UpdateInternodeLength(child);
			const InternodeInfo childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			float d = childNodeInfo.DistanceToBranchEnd + childNodeInfo.DistanceToParent;
			internodeInfo.TotalDistanceToBranchEnd += childNodeInfo.DistanceToParent + childNodeInfo.TotalDistanceToBranchEnd;
			if (d > internodeInfo.DistanceToBranchEnd) internodeInfo.DistanceToBranchEnd = d;
		}
	);
	EntityManager::SetComponentData(internode, internodeInfo);
}

void TreeUtilities::PlantSimulationSystem::UpdateInternodeActivatedLevel(Entity& internode)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	// go through node children and update their level accordingly
	float maxChildLength = 0;
	Entity maxChild;
	EntityManager::ForEachChild(internode, [this, &internodeInfo, &maxChild, &maxChildLength](Entity child)
		{
			const InternodeInfo childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			const float d = childNodeInfo.TotalDistanceToBranchEnd + childNodeInfo.DistanceToParent;
			if (d > maxChildLength) {
				maxChildLength = d;
				maxChild = child;
			}
		}
	);
	EntityManager::ForEachChild(internode, [this, &internodeInfo, &maxChild](Entity child)
		{
			InternodeInfo childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			childNodeInfo.DistanceToBranchStart = childNodeInfo.DistanceToParent;
			childNodeInfo.MaxActivatedChildLevel = childNodeInfo.Level;
			childNodeInfo.IsApical = child.Index == maxChild.Index;
			EntityManager::SetComponentData(child, childNodeInfo);
			UpdateInternodeActivatedLevel(child);
			childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			if (childNodeInfo.MaxActivatedChildLevel > internodeInfo.MaxActivatedChildLevel) internodeInfo.MaxActivatedChildLevel = childNodeInfo.MaxActivatedChildLevel;
		}
	);
	EntityManager::SetComponentData(internode, internodeInfo);
}

void TreeUtilities::PlantSimulationSystem::UpdateLocalTransform(Entity& internode, TreeParameters& treeParameters, glm::mat4& parentLTW, glm::quat& treeRotation, std::vector<glm::mat4>& leafTransforms)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	glm::quat actualLocalRotation = internodeInfo.DesiredLocalRotation;

	glm::vec3 scale;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(parentLTW, scale, internodeInfo.ParentRotation, internodeInfo.ParentTranslation, skew, perspective);

#pragma region Apply force here.
	glm::quat newGlobalRotation = treeRotation * internodeInfo.ParentRotation * internodeInfo.DesiredLocalRotation;
	glm::vec3 front = newGlobalRotation * glm::vec3(0, 0, -1);
	glm::vec3 up = newGlobalRotation * glm::vec3(0, 1, 0);
	float gravityBending = treeParameters.GravityBendingStrength * internodeInfo.AccumulatedGravity;
	front += gravityBending * glm::vec3(0, -1, 0);
	front = glm::normalize(front);
	up = glm::cross(glm::cross(front, up), front);
	newGlobalRotation = glm::quatLookAt(front, up);
	actualLocalRotation = glm::inverse(internodeInfo.ParentRotation) * glm::inverse(treeRotation) * newGlobalRotation;
#pragma endregion

	internodeInfo.LocalTransform = glm::translate(glm::mat4(1.0f), actualLocalRotation * glm::vec3(0, 0, -1)
		* internodeInfo.DistanceToParent) * glm::mat4_cast(actualLocalRotation)
		* glm::scale(glm::vec3(1.0f));

	internodeInfo.GlobalTransform = parentLTW * internodeInfo.LocalTransform;

	auto internodeData = EntityManager::GetSharedComponent<InternodeData>(internode);
	internodeData->LeafLocalTransforms.clear();
	float illumination = EntityManager::GetComponentData<Illumination>(internode).Value;
	if (illumination > 0.04f) {
		if (internodeInfo.Level > internodeInfo.MaxChildLevel - 6)
		{
			glm::vec3 lp = glm::vec3(0.0f);
			//x, ÏòÑôÖá£¬y: ºáÖá£¬z£ºroll
			glm::quat lr;
			glm::vec3 ls = glm::vec3(0.1f, 1.0f, 0.2f);
			glm::mat4 localTransform;
			
			lr = glm::quat(glm::vec3(glm::radians(45.0f), 0.0f, 0.0f));
			localTransform = glm::translate(glm::mat4(1.0f), lp) * glm::mat4_cast(lr) * glm::scale(ls);
			//Generate leaf here.
			internodeData->LeafLocalTransforms.push_back(localTransform);
			leafTransforms.push_back(internodeInfo.GlobalTransform * localTransform);

			lr = glm::quat(glm::vec3(glm::radians(-45.0f), 0.0f, 0.0f));
			localTransform = glm::translate(glm::mat4(1.0f), lp) * glm::mat4_cast(lr) * glm::scale(ls);
			//Generate leaf here.
			internodeData->LeafLocalTransforms.push_back(localTransform);
			leafTransforms.push_back(internodeInfo.GlobalTransform * localTransform);
		}
	}

	float mainChildThickness = 0;
	InternodeInfo mainChildInfo;
	unsigned mainChildEntityIndex = 0;
	EntityManager::ForEachChild(internode, [this, &mainChildInfo, &mainChildEntityIndex, &mainChildThickness, &treeParameters, &internodeInfo, &treeRotation, &leafTransforms](Entity child)
		{
			UpdateLocalTransform(child, treeParameters, internodeInfo.GlobalTransform, treeRotation, leafTransforms);
			InternodeInfo childInternodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			childInternodeInfo.ParentThickness = internodeInfo.Thickness;
			if (mainChildThickness < childInternodeInfo.Thickness) {
				glm::vec3 cScale;
				glm::quat cRotation;
				glm::vec3 cTranslation;
				glm::vec3 cSkew;
				glm::vec4 cPerspective;
				glm::decompose(childInternodeInfo.GlobalTransform, cScale, cRotation, cTranslation, cSkew, cPerspective);
				internodeInfo.MainChildRotation = cRotation;
				mainChildEntityIndex = child.Index;
				mainChildInfo = childInternodeInfo;
				mainChildInfo.IsMainChild = true;
			}
			EntityManager::SetComponentData(child, childInternodeInfo);
		}
	);
	if (EntityManager::GetChildrenAmount(internode) == 0) internodeInfo.MainChildRotation = glm::inverse(treeRotation) * newGlobalRotation;
	else {
		EntityManager::SetComponentData(mainChildEntityIndex, mainChildInfo);
	}
	EntityManager::ForEachChild(internode, [this, &internodeInfo](Entity child)
		{
			InternodeInfo childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			childNodeInfo.ParentMainChildRotation = internodeInfo.MainChildRotation;
			EntityManager::SetComponentData(child, childNodeInfo);
		}
	);



	EntityManager::SetComponentData(internode, internodeInfo);
}


void TreeUtilities::PlantSimulationSystem::DeactivateBud(InternodeInfo& internodeInfo, Bud& bud)
{
	internodeInfo.ActivatedBudsAmount--;
	bud.IsActive = false;
	if (bud.IsApical) internodeInfo.ApicalBudExist = false;
}


void TreeUtilities::PlantSimulationSystem::EvaluatePruning(Entity& internode, TreeParameters& treeParameters, TreeAge& treeAge, std::shared_ptr<TreeData>& treeInfo)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	if (EntityManager::GetChildrenAmount(internode) == 0)
	{
		internodeInfo.IsActivatedEndNode = true;
		EntityManager::SetComponentData(internode, internodeInfo);
	}
	if (internodeInfo.Pruned) {
		return;
	}
	if (internodeInfo.Level == 0 && treeAge.Value < 3) return;
	if (internodeInfo.Level == 1 && !internodeInfo.IsApical) {
		float height = EntityManager::GetComponentData<LocalToWorld>(internode).Value[3].y;
		if (height < treeParameters.LowBranchPruningFactor && height < treeInfo->Height) {
			PruneInternode(internode, &internodeInfo);
			return;
		}
	}
	float normalL = internodeInfo.AccumulatedLength / treeParameters.InternodeLengthBase;
	float ratioScale = 1;
	float factor = ratioScale / glm::sqrt(normalL);
	//factor *= internodeInfo.AccumulatedLight;
	if (factor < treeParameters.PruningFactor) {
		PruneInternode(internode, &internodeInfo);
		return;
	}

	EntityManager::ForEachChild(internode, [this, &treeParameters, &treeAge, &treeInfo](Entity child)
		{
			EvaluatePruning(child, treeParameters, treeAge, treeInfo);
		}
	);
}

void PlantSimulationSystem::EvaluateRemoval(Entity& internode, TreeParameters& treeParameters)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	EntityManager::ForEachChild(internode, [&internodeInfo, this, &treeParameters](Entity child)
		{
			InternodeInfo childInternodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			if(internodeInfo.Thickness - childInternodeInfo.Thickness > treeParameters.ThicknessRemovalFactor)
			{
				EntityManager::DeleteEntity(child);
			}
			EvaluateRemoval(child, treeParameters);
		}
	);
}

void PlantSimulationSystem::EvaluateDirectionPruning(Entity& internode, glm::vec3 escapeDirection, float limitAngle)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	if (glm::angle(escapeDirection, internodeInfo.DesiredGlobalRotation * glm::vec3(0.0f, 0.0f, -1.0f)) < glm::radians(limitAngle))
	{
		PruneInternode(internode, &internodeInfo);
	}
	EntityManager::ForEachChild(internode, [this, &escapeDirection, &limitAngle](Entity child)
		{
			EvaluateDirectionPruning(child, escapeDirection, limitAngle);
		}
	);
}

void TreeUtilities::PlantSimulationSystem::ApplyLocalTransform(Entity& treeEntity)
{
	glm::mat4 treeTransform = EntityManager::GetComponentData<LocalToWorld>(treeEntity).Value;
	auto treeIndex = EntityManager::GetComponentData<TreeIndex>(treeEntity).Value;
	auto treeData = EntityManager::GetSharedComponent<TreeData>(treeEntity);
	std::mutex heightMutex;
	float treeHeight = 0.0f;
	EntityManager::ForEach<TreeIndex, LocalToWorld, InternodeInfo>(_InternodeQuery,
		[treeTransform, treeIndex, &treeHeight, &heightMutex](int i, Entity internode, TreeIndex* index, LocalToWorld* ltw, InternodeInfo* info)
		{
			if (index->Value == treeIndex) {
				auto nltw = treeTransform * info->GlobalTransform;
				ltw->Value = nltw;
				if (nltw[3].y > treeHeight) {
					std::lock_guard<std::mutex> lock(heightMutex);
					treeHeight = nltw[3].y;
				}
			}
		}
	);
	treeData->Height = treeHeight;
}

void TreeUtilities::PlantSimulationSystem::CalculateDirectGravityForce(Entity& treeEntity, float gravity)
{
	float gravityFactor = EntityManager::GetComponentData<TreeParameters>(treeEntity).SaggingFactor;
	EntityManager::ForEach<LocalToWorld, InternodeInfo, Gravity>(_InternodeQuery, [gravityFactor, gravity](int i, Entity internode, LocalToWorld* ltw, InternodeInfo* bni, Gravity* fs)
		{
			float thickness = bni->Thickness;
			fs->Value = gravity * gravityFactor * bni->DistanceToParent;
		});
}

void TreeUtilities::PlantSimulationSystem::BackPropagateForce(Entity& internode, float fixedPropagationCoefficient)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	float gfs = EntityManager::GetComponentData<Gravity>(internode).Value;
	internodeInfo.AccumulatedGravity = gfs;
	EntityManager::ForEachChild(internode, [this, &internodeInfo, fixedPropagationCoefficient](Entity child)
		{
			BackPropagateForce(child, fixedPropagationCoefficient);
			auto childInternodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			internodeInfo.AccumulatedGravity += fixedPropagationCoefficient * childInternodeInfo.AccumulatedGravity * childInternodeInfo.Thickness / internodeInfo.Thickness;
		});
	EntityManager::SetComponentData(internode, internodeInfo);
}

void PlantSimulationSystem::CalculateCrownShyness(float radius)
{
	std::vector<LocalToWorld> internodesLTWs;
	std::vector<TreeIndex> internodesTreeIndices;
	_InternodeQuery.ToComponentDataArray(internodesLTWs);
	_InternodeQuery.ToComponentDataArray(internodesTreeIndices);

	EntityManager::ForEach<LocalToWorld, InternodeInfo, TreeIndex>(_InternodeQuery, [radius, &internodesLTWs, &internodesTreeIndices, this](int i, Entity branchNode, LocalToWorld* ltw, InternodeInfo* info, TreeIndex* index)
		{
			if (info->Pruned) return;
			if (!info->IsActivatedEndNode) return;
			for (size_t bi = 0; bi < internodesLTWs.size(); bi++)
			{
				if (internodesTreeIndices[bi].Value != index->Value)
				{
					//auto position1 = glm::vec2(ltw->Value[3].x, ltw->Value[3].z);
					//auto position2 = glm::vec2(internodesLTWs[bi].Value[3].x, internodesLTWs[bi].Value[3].z);
					auto position1 = glm::vec3(ltw->Value[3].x, ltw->Value[3].y, ltw->Value[3].z);
					auto position2 = glm::vec3(internodesLTWs[bi].Value[3].x, internodesLTWs[bi].Value[3].y, internodesLTWs[bi].Value[3].z);
					if (glm::distance(position1, position2) < radius)
					{
						info->Pruned = true;
						info->IsActivatedEndNode = false;
					}
				}
			}
		}
	);
}

inline void PlantSimulationSystem::PruneInternode(Entity& internode, InternodeInfo* internodeInfo)
{
	internodeInfo->Pruned = true;
	internodeInfo->IsActivatedEndNode = false;
	EntityManager::SetComponentData(internode, *internodeInfo);
}

void PlantSimulationSystem::BuildConvexHullForTree(Entity& tree)
{
	std::vector<LocalToWorld> internodeLTWs;
	std::vector<InternodeInfo> internodeInfos;
	const auto treeIndex = EntityManager::GetComponentData<TreeIndex>(tree);
	_InternodeQuery.ToComponentDataArray(treeIndex, internodeLTWs);
	_InternodeQuery.ToComponentDataArray(treeIndex, internodeInfos);

	quickhull::QuickHull<float> qh; // Could be double as well
	std::vector<quickhull::Vector3<float>> pointCloud;
	for (size_t i = 0; i < internodeLTWs.size(); i++)
	{
		//if(internodesInfos[i].IsActivatedEndNode)
		{
			const auto transform = internodeInfos[i].GlobalTransform;
			pointCloud.emplace_back(transform[3].x, transform[3].y, transform[3].z);
		}
	}
	auto hull = qh.getConvexHull(pointCloud, true, false);
	const auto& indexBuffer = hull.getIndexBuffer();
	const auto& vertexBuffer = hull.getVertexBuffer();
	std::vector<Vertex> vertices;
	std::vector<unsigned> indices;
	indices.resize(indexBuffer.size());
	vertices.resize(vertexBuffer.size());

	for (size_t i = 0; i < indexBuffer.size(); i++)
	{
		indices[i] = indexBuffer[i];
	}

	for (size_t i = 0; i < vertexBuffer.size(); i++)
	{
		const auto v = vertexBuffer[i];
		vertices[i].Position = glm::vec3(v.x, v.y, v.z);
		vertices[i].TexCoords0 = glm::vec2(0, 0);
	}
	auto treeInfo = EntityManager::GetSharedComponent<TreeData>(tree);
	treeInfo->ConvexHull = std::make_shared<Mesh>();
	treeInfo->ConvexHull->SetVertices(17, vertices, indices, true);
}

TreeParameters PlantSimulationSystem::ImportTreeParameters(const std::string& path)
{
	TreeParameters ret;
	std::ifstream ifs;
	ifs.open((path + ".tps").c_str());
	if (ifs.is_open())
	{
		std::string temp;
		ifs >> temp; ifs >> ret.Seed;
#pragma region Geometric
		ifs >> temp; ifs >> ret.LateralBudPerNode;

		ifs >> temp; ifs >> ret.VarianceApicalAngle; // Training target

		ifs >> temp; ifs >> ret.BranchingAngleMean; // Training target
		ifs >> temp; ifs >> ret.BranchingAngleVariance; // Training target

		ifs >> temp; ifs >> ret.RollAngleMean; // Training target
		ifs >> temp; ifs >> ret.RollAngleVariance; // Training target
#pragma endregion
#pragma region Bud fate
		ifs >> temp; ifs >> ret.ApicalBudKillProbability; // Useless.
		ifs >> temp; ifs >> ret.LateralBudKillProbability; //Useless.

		ifs >> temp; ifs >> ret.ApicalDominanceBase;
		ifs >> temp; ifs >> ret.ApicalDominanceDistanceFactor; // Training target
		ifs >> temp; ifs >> ret.ApicalDominanceAgeFactor; // Training target

		ifs >> temp; ifs >> ret.GrowthRate;

		ifs >> temp; ifs >> ret.InternodeLengthBase; //Fixed
		ifs >> temp; ifs >> ret.InternodeLengthAgeFactor; // Training target


		ifs >> temp; ifs >> ret.ApicalControlBase; // Training target
		ifs >> temp; ifs >> ret.ApicalControlAgeFactor; // Training target
		ifs >> temp; ifs >> ret.ApicalControlLevelFactor; // Training target
		ifs >> temp; ifs >> ret.ApicalControlDistanceFactor; // Training target

		ifs >> temp; ifs >> ret.MaxBudAge;
#pragma endregion
#pragma region Environmental
		ifs >> temp; ifs >> ret.Phototropism; // Based on tree leaf properties.
		ifs >> temp; ifs >> ret.GravitropismBase; //Based on tree material properties.
		ifs >> temp; ifs >> ret.GravitropismLevelFactor;  //Based on tree material properties.

		ifs >> temp; ifs >> ret.PruningFactor;
		ifs >> temp; ifs >> ret.LowBranchPruningFactor;
		
		ifs >> temp; ifs >> ret.ThicknessRemovalFactor;
		
		ifs >> temp; ifs >> ret.GravityBendingStrength;

		ifs >> temp; ifs >> ret.ApicalBudLightingFactor;
		ifs >> temp; ifs >> ret.LateralBudLightingFactor;
#pragma endregion

#pragma region Sagging
		ifs >> temp; ifs >> ret.SaggingFactor;
		ifs >> temp; ifs >> ret.SaggingForceBackPropagateFixedCoefficient;
#pragma endregion

		//ifs >> temp; ifs >> ret.Age;
		ifs >> temp; ifs >> ret.EndNodeThickness;
		ifs >> temp; ifs >> ret.ThicknessControlFactor;
	}
	else
	{
		Debug::Error("Can't open file!");
	}
	return ret;
}

void PlantSimulationSystem::ExportTreeParameters(const std::string& path, TreeParameters& treeParameters)
{
	std::ofstream of;
	of.open((path + ".tps").c_str(), std::ofstream::out | std::ofstream::trunc);
	if (of.is_open())
	{
		std::string output = "";
		output += "Seed\n"; output += std::to_string(treeParameters.Seed);
#pragma region Geometric
		output += "\nLateralBudPerNode\n"; output += std::to_string(treeParameters.LateralBudPerNode);
		output += "\nVarianceApicalAngle\n";  output += std::to_string(treeParameters.VarianceApicalAngle);
		output += "\nBranchingAngleMean\n";  output += std::to_string(treeParameters.BranchingAngleMean);
		output += "\nBranchingAngleVariance\n";  output += std::to_string(treeParameters.BranchingAngleVariance);
		output += "\nRollAngleMean\n";  output += std::to_string(treeParameters.RollAngleMean);
		output += "\nRollAngleVariance\n";  output += std::to_string(treeParameters.RollAngleVariance);
#pragma endregion
#pragma region Bud fate
		output += "\nApicalBudKillProbability\n"; output += std::to_string(treeParameters.ApicalBudKillProbability);
		output += "\nLateralBudKillProbability\n"; output += std::to_string(treeParameters.LateralBudKillProbability);
		output += "\nApicalDominanceBase\n";  output += std::to_string(treeParameters.ApicalDominanceBase);
		output += "\nApicalDominanceDistanceFactor\n";  output += std::to_string(treeParameters.ApicalDominanceDistanceFactor);
		output += "\nApicalDominanceAgeFactor\n";  output += std::to_string(treeParameters.ApicalDominanceAgeFactor);
		output += "\nGrowthRate\n"; output += std::to_string(treeParameters.GrowthRate);
		output += "\nBranchNodeLengthBase\n";  output += std::to_string(treeParameters.InternodeLengthBase);
		output += "\nBranchNodeLengthAgeFactor\n";  output += std::to_string(treeParameters.InternodeLengthAgeFactor);
		output += "\nApicalControlBase\n";  output += std::to_string(treeParameters.ApicalControlBase);
		output += "\nApicalControlAgeFactor\n";  output += std::to_string(treeParameters.ApicalControlAgeFactor);
		output += "\nApicalControlLevelFactor\n";  output += std::to_string(treeParameters.ApicalControlLevelFactor);
		output += "\nApicalControlDistanceFactor\n";  output += std::to_string(treeParameters.ApicalControlDistanceFactor);
		output += "\nMaxBudAge\n"; output += std::to_string(treeParameters.MaxBudAge);
#pragma endregion
#pragma region Environmental
		output += "\nPhototropism\n"; output += std::to_string(treeParameters.Phototropism);
		output += "\nGravitropismBase\n"; output += std::to_string(treeParameters.GravitropismBase);
		output += "\nGravitropismLevelFactor\n"; output += std::to_string(treeParameters.GravitropismLevelFactor);
		output += "\nPruningFactor\n"; output += std::to_string(treeParameters.PruningFactor);
		output += "\nLowBranchPruningFactor\n"; output += std::to_string(treeParameters.LowBranchPruningFactor);
		output += "\nThicknessRemovalFactor\n"; output += std::to_string(treeParameters.ThicknessRemovalFactor);
		output += "\nGravityBendingStrength\n"; output += std::to_string(treeParameters.GravityBendingStrength);
		output += "\nApicalBudLightingFactor\n"; output += std::to_string(treeParameters.ApicalBudLightingFactor);
		output += "\nLateralBudLightingFactor\n"; output += std::to_string(treeParameters.LateralBudLightingFactor);
#pragma endregion
#pragma region Sagging
		output += "\nSaggingFactor\n"; output += std::to_string(treeParameters.SaggingFactor);
		output += "\nSaggingForceBackPropagateFixedCoefficient\n"; output += std::to_string(treeParameters.SaggingForceBackPropagateFixedCoefficient);
#pragma endregion
		//output += "\nAge\n"; output += std::to_string(treeParameters.Age);
		output += "\nEndNodeThickness\n"; output += std::to_string(treeParameters.EndNodeThickness);
		output += "\nThicknessControlFactor\n"; output += std::to_string(treeParameters.ThicknessControlFactor);
		of.write(output.c_str(), output.size());
		of.flush();
		of.close();
		Debug::Log("Tree parameters saved: " + path + ".tps");
	}
	else
	{
		Debug::Error("Can't open file!");
	}
}

void TreeUtilities::PlantSimulationSystem::UpdateInternodeResource(Entity& internode, TreeParameters& treeParameters, TreeAge& treeAge)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);

	internodeInfo.DistanceToBranchEnd = 0;
	internodeInfo.NumValidChild = 0;

	if (EntityManager::GetChildrenAmount(internode) == 0) internodeInfo.Thickness = treeParameters.EndNodeThickness;
	else {
		internodeInfo.Thickness = 0;
	}

	Illumination internodeIllumination = EntityManager::GetComponentData<Illumination>(internode);
	internodeInfo.AccumulatedLight = internodeIllumination.Value;
	internodeInfo.AccumulatedLength = internodeInfo.DistanceToParent;
	internodeInfo.AccumulatedActivatedBudsAmount = internodeInfo.ActivatedBudsAmount;
	internodeInfo.BranchEndInternodeAmount = EntityManager::GetChildrenAmount(internode) == 0 ? 1 : 0;
	internodeInfo.MaxChildLevel = internodeInfo.Level;
	float mainChildThickness = 0.0f;
	EntityManager::ForEachChild(internode, [this, &mainChildThickness, &internodeInfo, &treeParameters, &treeAge](Entity child)
		{
			UpdateInternodeResource(child, treeParameters, treeAge);
			InternodeInfo childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);

			if (internodeInfo.MaxChildLevel < childNodeInfo.MaxChildLevel) {
				internodeInfo.MaxChildLevel = childNodeInfo.MaxChildLevel;
			}
			else if (internodeInfo.MaxChildLevel < childNodeInfo.Level) {
				internodeInfo.MaxChildLevel = childNodeInfo.Level;
			}
			float d = childNodeInfo.DistanceToBranchEnd + childNodeInfo.DistanceToParent;
			if (d > internodeInfo.DistanceToBranchEnd)internodeInfo.DistanceToBranchEnd = d;
			float cL = childNodeInfo.DistanceToParent;
			internodeInfo.NumValidChild += 1;

			internodeInfo.BranchEndInternodeAmount += childNodeInfo.BranchEndInternodeAmount;
			internodeInfo.Thickness += treeParameters.ThicknessControlFactor * childNodeInfo.Thickness;
			if (childNodeInfo.Thickness > mainChildThickness) {
				mainChildThickness = childNodeInfo.Thickness;
			}

			if (childNodeInfo.Pruned) return;
			internodeInfo.AccumulatedLight += childNodeInfo.AccumulatedLight;
			internodeInfo.AccumulatedActivatedBudsAmount += childNodeInfo.AccumulatedActivatedBudsAmount;
			internodeInfo.AccumulatedLength += childNodeInfo.AccumulatedLength;
		}
	);
	if (mainChildThickness > internodeInfo.Thickness) internodeInfo.Thickness = mainChildThickness;

	EntityManager::SetComponentData(internode, internodeInfo);
}

void TreeUtilities::PlantSimulationSystem::OnCreate()
{
	_getcwd(_CurrentWorkingDir, 256);
	_TreeQuery = TreeManager::GetTreeQuery();
	_InternodeQuery = TreeManager::GetInternodeQuery();
	_Gravity = 0;
	for (int i = 0; i < 256; i++) {
		_TempImportFilePath[i] = 0;
	}

	_DefaultTreeSurfaceMaterial1 = std::make_shared<Material>();
	_DefaultTreeSurfaceMaterial1->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto textureDiffuse1 = new Texture2D(TextureType::DIFFUSE);
	textureDiffuse1->LoadTexture(FileIO::GetResourcePath("Textures/brown.png"), "");
	auto textureNormal1 = new Texture2D(TextureType::NORMAL);
	textureNormal1->LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Bark_Pine_normal.jpg"), "");
	_DefaultTreeSurfaceMaterial1->Textures2Ds()->push_back(textureDiffuse1);
	_DefaultTreeSurfaceMaterial1->Textures2Ds()->push_back(textureNormal1);

	_DefaultTreeSurfaceMaterial2 = std::make_shared<Material>();
	_DefaultTreeSurfaceMaterial2->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto textureDiffuse2 = new Texture2D(TextureType::DIFFUSE);
	textureDiffuse2->LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Aspen_bark_001_COLOR.jpg"), "");
	auto textureNormal2 = new Texture2D(TextureType::NORMAL);
	textureNormal2->LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Aspen_bark_001_NORM.jpg"), "");
	_DefaultTreeSurfaceMaterial2->Textures2Ds()->push_back(textureDiffuse2);
	_DefaultTreeSurfaceMaterial2->Textures2Ds()->push_back(textureNormal2);

	_DefaultTreeLeafMaterial1 = std::make_shared<Material>();
	_DefaultTreeLeafMaterial1->SetMaterialProperty("material.shininess", 32.0f);
	_DefaultTreeLeafMaterial1->Programs()->push_back(Default::GLPrograms::StandardInstancedProgram);
	auto textureDiffuseLeaf1 = new Texture2D(TextureType::DIFFUSE);
	textureDiffuseLeaf1->LoadTexture(FileIO::GetResourcePath("Textures/Leaf/PrunusAvium/A/level0.png"), "");
	//textureDiffuseLeaf1->LoadTexture(FileIO::GetResourcePath("Textures/green.png"), "");
	auto textureNormalLeaf1 = new Texture2D(TextureType::NORMAL);
	textureNormalLeaf1->LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Aspen_bark_001_NORM.jpg"), "");
	_DefaultTreeLeafMaterial1->Textures2Ds()->push_back(textureDiffuseLeaf1);
	//_DefaultTreeLeafMaterial1->Textures2Ds()->push_back(textureNormalLeaf1);

	std::vector<Vertex> leafVertices;
	std::vector<unsigned> leafIndices;
	Vertex v;
	v.Position = glm::vec3(-1, 0, 0);
	v.TexCoords0 = glm::vec2(0, 0);
	leafVertices.push_back(v);
	v.Position = glm::vec3(-1, 0, -2);
	v.TexCoords0 = glm::vec2(0, 1);
	leafVertices.push_back(v);
	v.Position = glm::vec3(1, 0, 0);
	v.TexCoords0 = glm::vec2(1, 0);
	leafVertices.push_back(v);
	v.Position = glm::vec3(1, 0, -2);
	v.TexCoords0 = glm::vec2(1, 1);
	leafVertices.push_back(v);

	leafIndices.push_back(0);
	leafIndices.push_back(1);
	leafIndices.push_back(2);

	leafIndices.push_back(3);
	leafIndices.push_back(2);
	leafIndices.push_back(1);
	_DefaultTreeLeafMesh = std::make_shared<Mesh>();
	_DefaultTreeLeafMesh->SetVertices((unsigned)VertexAttribute::Position | (unsigned)VertexAttribute::TexCoord0,
		leafVertices, leafIndices);
	
	//_DefaultTreeLeafMesh = Default::Primitives::Quad;

	_Gravity = 1.0f;
	_NewTreeParameters.resize(1);
	LoadDefaultTreeParameters(1, _NewTreeParameters[_CurrentFocusedNewTreeIndex]);
	Enable();
}

void TreeUtilities::PlantSimulationSystem::OnDestroy()
{
}

void TreeUtilities::PlantSimulationSystem::Update()
{
	if (_DisplayConvexHull)
	{
		std::vector<LocalToWorld> ltws;
		std::vector<Entity> trees;

		_TreeQuery.ToComponentDataArray(ltws);
		_TreeQuery.ToEntityArray(trees);


		for (size_t i = 0; i < trees.size(); i++)
		{
			auto data = EntityManager::GetSharedComponent<TreeData>(trees[i]);
			if (data->ConvexHull != nullptr) {
				RenderManager::DrawGizmoMesh(data->ConvexHull.get(), glm::vec4(0.5, 0.5, 0.5, 1.0), Application::GetMainCameraComponent()->Value.get(), ltws[i].Value);
			}
		}

	}
	DrawGUI();
}

Entity TreeUtilities::PlantSimulationSystem::CreateTree(std::shared_ptr<Material> treeSurfaceMaterial, std::shared_ptr<Material> treeLeafMaterial, std::shared_ptr<Mesh> treeLeafMesh, TreeParameters treeParameters, glm::vec3 position, bool enabled)
{
	auto treeEntity = TreeManager::CreateTree(treeSurfaceMaterial, treeLeafMaterial, treeLeafMesh);
	Entity internode = TreeManager::CreateInternode(EntityManager::GetComponentData<TreeIndex>(treeEntity), treeEntity);
#pragma region Position & Style
	Translation t;
	t.Value = position;
	Scale s;
	s.Value = glm::vec3(1.0f);
	Rotation r;
	r.Value = glm::quatLookAt(glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
	LocalToWorld ltw;
	ltw.Value = glm::translate(glm::mat4(1.0f), t.Value) * glm::mat4_cast(r.Value) * glm::scale(s.Value);

	EntityManager::SetComponentData(treeEntity, t);
	EntityManager::SetComponentData(treeEntity, s);
	EntityManager::SetComponentData(treeEntity, r);
	EntityManager::SetComponentData(treeEntity, ltw);
#pragma endregion
	TreeAge age;
	age.Value = 0;
	age.ToGrowIteration = 0;
	age.Enable = enabled;

	Bud bud;
	bud.EulerAngles = glm::vec3(0.0f);
	bud.IsActive = true;
	bud.IsApical = true;
	bud.StartAge = 0;

	auto internodeData = EntityManager::GetSharedComponent<InternodeData>(internode);
	internodeData->Buds.push_back(bud);

	InternodeInfo internodeInfo;
	internodeInfo.IsActivatedEndNode = false;
	internodeInfo.IsApical = true;
	internodeInfo.ApicalBudExist = true;
	internodeInfo.Level = 0;
	internodeInfo.MaxActivatedChildLevel = 0;
	internodeInfo.ActivatedBudsAmount = 1;
	internodeInfo.Pruned = false;
	internodeInfo.DistanceToParent = 0;
	internodeInfo.DesiredGlobalRotation = glm::quatLookAt(glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
	internodeInfo.DesiredLocalRotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
	auto treeInfo = EntityManager::GetSharedComponent<TreeData>(treeEntity);
	treeInfo->MeshGenerated = false;
	glm::mat4 transform = glm::identity<glm::mat4>();
	auto immc = EntityManager::GetSharedComponent<InstancedMeshMaterialComponent>(treeEntity);
	immc->Matrices.clear();
	UpdateLocalTransform(internode, treeParameters, transform, r.Value, immc->Matrices);

	EntityManager::SetComponentData(internode, internodeInfo);
	EntityManager::SetComponentData(treeEntity, treeParameters);
	EntityManager::SetComponentData(treeEntity, age);
	_Growing = true;
	return treeEntity;
}

void PlantSimulationSystem::CreateDefaultTree()
{
	TreeParameters tps;
	LoadDefaultTreeParameters(1, tps);
	CreateTree(_DefaultTreeSurfaceMaterial1, _DefaultTreeLeafMaterial1, _DefaultTreeLeafMesh, tps, glm::vec3(0.0f), true);
}

#pragma endregion
