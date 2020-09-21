#include "PlantSimulationSystem.h"
#include "TreeScene.h"

#include "quickhull/quickhull.hpp"

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
					for(auto i = currentSize; i < _NewTreeAmount; i++)
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

					ImGui::InputFloat2("Node Len Base/Age", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].BranchNodeLengthBase);

					ImGui::InputFloat4("AC Base/Age/Lvl/Dist", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].ApicalControlBase);

					ImGui::InputInt("Max Bud Age", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].MaxBudAge);

					ImGui::InputFloat("Phototropism", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].Phototropism);

					ImGui::InputFloat2("Gravitropism Base/Age", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].GravitropismBase);

					ImGui::InputFloat("PruningFactor", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].PruningFactor);

					ImGui::InputFloat("LowBranchPruningFactor", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].LowBranchPruningFactor);

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
						CreateTree(_DefaultTreeSurfaceMaterial1, _NewTreeParameters[i], _NewTreePositions[i], true);
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
		if (ImGui::Button(_Growing ? "Pause growing" : "Resume growing")) {
			_Growing = !_Growing;
		}
		if(ImGui::SliderFloat("Gravity", &_Gravity, 0.0f, 20.0f))
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
		if(ImGui::Button("Generate Convex Hull for all trees"))
		{
			std::vector<Entity> trees;
			_TreeQuery.ToEntityArray(trees);
			for(auto& tree : trees)
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
		for(auto i = 0; i < _NewTreeAmount; i++)
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
			output += "\nBranchNodeLengthBase\n";  output += std::to_string(_NewTreeParameters[i].BranchNodeLengthBase);
			output += "\nBranchNodeLengthAgeFactor\n";  output += std::to_string(_NewTreeParameters[i].BranchNodeLengthAgeFactor);
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
		for(auto i = 0; i < _NewTreeAmount; i++)
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

			ifs >> temp; ifs >> _NewTreeParameters[i].BranchNodeLengthBase; //Fixed
			ifs >> temp; ifs >> _NewTreeParameters[i].BranchNodeLengthAgeFactor; // Training target


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

	if(_GravityChanged)
	{
		_GravityChanged = false;
		CalculatePhysics(trees);
	}
}

bool TreeUtilities::PlantSimulationSystem::GrowTree(Entity& treeEntity)
{
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
	Entity rootBranchNode = EntityManager::GetChildren(treeEntity).at(0);
	UpdateBranchNodeLength(rootBranchNode);
	BranchNodeInfo tempBNInfo = EntityManager::GetComponentData<BranchNodeInfo>(rootBranchNode);
	tempBNInfo.Level = 0;
	tempBNInfo.MaxActivatedChildLevel = 0;
	tempBNInfo.DistanceToBranchStart = 0;
	EntityManager::SetComponentData(rootBranchNode, tempBNInfo);
	UpdateBranchNodeActivatedLevel(rootBranchNode);
#pragma endregion
	bool growed = GrowShoots(rootBranchNode, treeData, treeAge, treeParameters, treeIndex);
	treeAge.Value++;
	treeAge.ToGrowIteration--;
	EntityManager::SetComponentData(treeEntity, treeAge);
	if (growed) {
		UpdateBranchNodeResource(rootBranchNode, treeParameters, treeAge);
		EvaluatePruning(rootBranchNode, treeParameters, treeAge, treeData);
	}
	return growed;
}

bool TreeUtilities::PlantSimulationSystem::GrowShoots(Entity& branchNode, std::shared_ptr<TreeData>& treeInfo, TreeAge& treeAge, TreeParameters& treeParameters, TreeIndex& treeIndex)
{
	BranchNodeInfo branchNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(branchNode);
	if (branchNodeInfo.Pruned) return false;
#pragma region Grow child and collect Inhibitor
	branchNodeInfo.Inhibitor = 0;
	bool ret = false;
	EntityManager::ForEachChild(branchNode, [&ret, this, &branchNodeInfo, &treeInfo, &treeAge, &treeParameters, &treeIndex](Entity childNode)
		{
			if (GrowShoots(childNode, treeInfo, treeAge, treeParameters, treeIndex)) ret = true;
			auto childBranchNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(childNode);
			branchNodeInfo.Inhibitor += childBranchNodeInfo.Inhibitor * childBranchNodeInfo.ParentInhibitorFactor;
		}
	);
	if (branchNodeInfo.ActivatedBudsAmount == 0) return ret;
#pragma endregion
	auto branchNodeIllumination = EntityManager::GetComponentData<Illumination>(branchNode);
	auto budsList = EntityManager::GetComponentData<BudList>(branchNode);
	auto branchNodeIndex = EntityManager::GetComponentData<BranchNodeIndex>(branchNode);
	float lateralInhibitorToAdd = 0;
	for (auto& bud : *budsList.Buds) {
		if (!bud.IsActive) continue;
#pragma region Bud kill probability
		float budKillProbability = 0;
		if (bud.IsApical) {
			budKillProbability = EntityManager::HasComponentData<TreeData>(EntityManager::GetParent(branchNode)) ? 0 : treeParameters.ApicalBudKillProbability;
		}
		else {
			budKillProbability = treeParameters.LateralBudKillProbability;
		}
		if (glm::linearRand(0.0f, 1.0f) < budKillProbability) {
			DeactivateBud(branchNodeInfo, bud);
			continue;
		}
#pragma endregion
#pragma region Flush check
		//compute probability that the given bud can grow
		float budGrowProbability = 1.0f;
		// first take into account the apical dominance
		if (branchNodeInfo.Inhibitor > 0) budGrowProbability *= glm::exp(-branchNodeInfo.Inhibitor);
		// now take into consideration the light on the bud
		float illumination = branchNodeIllumination.Value / TreeManager::GetLightEstimator()->GetMaxIllumination();
		if (illumination < 1.0f) {
			budGrowProbability *= glm::pow(illumination, bud.IsApical ? treeParameters.ApicalBudLightingFactor : treeParameters.LateralBudLightingFactor);
		}

		// now check whether the bud is going to flush or not
		bool flush = treeAge.Value < 2 ? true : budGrowProbability >= glm::linearRand(0.0f, 1.0f);
#pragma endregion
		bool growSucceed = false;
		if (flush) {
			bool isLateral = !(bud.IsApical && EntityManager::GetChildrenAmount(branchNode) == 0);
#pragma region Compute total grow distance and internodes amount.
			int level = branchNodeInfo.Level;
			if (isLateral) level++;
			float apicalControl = GetApicalControl(treeInfo, branchNodeInfo, treeParameters, treeAge, level);
			float distanceToGrow = treeParameters.GrowthRate * apicalControl;
			int branchNodesToGrow = glm::floor(distanceToGrow + 0.5f);
			if (branchNodesToGrow != 0) {
				growSucceed = true;
			}
#pragma endregion
#pragma region Grow new shoot
			if (growSucceed) {
				float branchNodeLength = distanceToGrow / static_cast<float>(branchNodesToGrow);
				branchNodeLength *= treeParameters.BranchNodeLengthBase * glm::pow(treeParameters.BranchNodeLengthAgeFactor, treeAge.Value);
				int level = branchNodeInfo.Level;
				if (!bud.IsApical) {
					level++;
				}
				Entity prevBranchNode = branchNode;
				BranchNodeInfo prevBranchNodeInfo = branchNodeInfo;
				glm::vec3 prevEulerAngle = bud.EulerAngles;
				glm::quat prevBranchNodeRotation;
				prevBranchNodeRotation = branchNodeInfo.DesiredGlobalRotation;
#pragma region Create branch nodes
				for (int selectedNewNodeIndex = 0; selectedNewNodeIndex < branchNodesToGrow; selectedNewNodeIndex++) {
#pragma region Setup branch node
					Entity newBranchNode = TreeManager::CreateBranchNode(treeIndex, prevBranchNode);
					BudList newBranchNodeBudList = EntityManager::GetComponentData<BudList>(newBranchNode);
					BranchNodeInfo newBranchNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(newBranchNode);
					newBranchNodeInfo.ApicalBudExist = true;
					newBranchNodeInfo.ActivatedBudsAmount = treeParameters.LateralBudPerNode + 1;
					newBranchNodeInfo.DistanceToParent = branchNodeLength;
					newBranchNodeInfo.Level = level;
					newBranchNodeInfo.Pruned = false;
					newBranchNodeInfo.IsApical = prevBranchNodeInfo.IsApical;
					if (newBranchNodeInfo.IsApical) newBranchNodeInfo.Level = prevBranchNodeInfo.Level;
					else newBranchNodeInfo.Level = prevBranchNodeInfo.Level + 1;
					newBranchNodeInfo.MaxActivatedChildLevel = level;
					newBranchNodeInfo.ParentInhibitorFactor = glm::pow(treeParameters.ApicalDominanceDistanceFactor, newBranchNodeInfo.DistanceToParent);

#pragma endregion
#pragma region Transforms for branch node
					newBranchNodeInfo.DesiredLocalRotation = glm::quat(prevEulerAngle);
#pragma region Roll branch node
					glm::vec3 rollAngles = glm::vec3(0.0f, 0.0f, glm::radians(treeParameters.RollAngleMean + treeParameters.RollAngleVariance * glm::linearRand(-1, 1)));
					newBranchNodeInfo.DesiredLocalRotation *= glm::quat(rollAngles);
#pragma endregion

#pragma region Apply phototropism and gravitropism
					float gravitropism = treeInfo->GravitropismLevelVal.at(newBranchNodeInfo.Level);
					glm::quat globalRawRotation = prevBranchNodeRotation * newBranchNodeInfo.DesiredLocalRotation;
					glm::vec3 rawFront = globalRawRotation * glm::vec3(0.0f, 0.0f, -1.05f);
					glm::vec3 rawUp = globalRawRotation * glm::vec3(0.0f, 1.05f, 0.0f);
					glm::vec3 gravityDir = glm::vec3(0.0f, -1.0f, 0.0f);
					rawFront += gravityDir * gravitropism;
					if (branchNodeIllumination.Value > 0) {
						rawFront += glm::normalize(-branchNodeIllumination.LightDir) * treeParameters.Phototropism;
					}
					rawFront = glm::normalize(rawFront);
					rawUp = glm::normalize(glm::cross(glm::cross(rawFront, rawUp), rawFront));
					globalRawRotation = glm::quatLookAt(rawFront, rawUp);
					newBranchNodeInfo.DesiredLocalRotation = glm::inverse(prevBranchNodeRotation) * globalRawRotation;
					newBranchNodeInfo.DesiredGlobalRotation = globalRawRotation;
					prevBranchNodeRotation = globalRawRotation;
#pragma endregion
#pragma endregion
#pragma region Create Apical Bud
					Bud newApicalBud;
					newApicalBud.EulerAngles = glm::vec3(glm::gaussRand(glm::vec2(0.0f), glm::vec2(glm::radians(treeParameters.VarianceApicalAngle / 2.0f))), 0.0f);
					newApicalBud.IsActive = true;
					newApicalBud.IsApical = true;
					newApicalBud.StartAge = treeAge.Value;
					newBranchNodeBudList.Buds->push_back(newApicalBud);
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
						newBranchNodeBudList.Buds->push_back(newLateralBud);
					}
#pragma endregion
					prevEulerAngle = newApicalBud.EulerAngles;
					prevBranchNode = newBranchNode;
					prevBranchNodeInfo = newBranchNodeInfo;
#pragma region Apply new branch node info
					EntityManager::SetComponentData(newBranchNode, newBranchNodeBudList);
					EntityManager::SetComponentData(newBranchNode, newBranchNodeInfo);
#pragma endregion
				}
#pragma endregion
				DeactivateBud(branchNodeInfo, bud);
#pragma region Add inhibitor to this branchnode.
				float localInhibitor = 0;
				if (treeAge.Value <= 1) localInhibitor += treeParameters.ApicalDominanceBase;
				else {
					localInhibitor += treeParameters.ApicalDominanceBase * treeInfo->ApicalDominanceTimeVal.at(treeAge.Value);
				}
				if (bud.IsApical) {
					branchNodeInfo.Inhibitor += localInhibitor;
					EntityManager::SetComponentData(branchNode, branchNodeInfo);
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
				DeactivateBud(branchNodeInfo, bud);
			}
		}
		else {
			ret = true;
		}
#pragma endregion
	}
	branchNodeInfo.Inhibitor += lateralInhibitorToAdd;
	return ret;
}

#pragma region Helpers
void TreeUtilities::PlantSimulationSystem::CalculatePhysics(std::vector<Entity>& trees)
{
	for (auto& tree : trees) {
		Rotation rotation = EntityManager::GetComponentData<Rotation>(tree);
		TreeParameters treeParameters = EntityManager::GetComponentData<TreeParameters>(tree);
		Entity rootBranchNode = EntityManager::GetChildren(tree).at(0);
		CalculateDirectGravityForce(tree, _Gravity);
		BackPropagateForce(rootBranchNode, treeParameters.SaggingForceBackPropagateFixedCoefficient);
		glm::mat4 transform = glm::identity<glm::mat4>(); //glm::translate(glm::mat4(1.0f), glm::vec3(0.0f))* glm::mat4_cast(glm::quatLookAt(glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)))* glm::scale(glm::vec3(1.0f));
		UpdateLocalTransform(rootBranchNode, treeParameters, transform, rotation.Value);
		ApplyLocalTransform(tree);
		TreeManager::GetBranchNodeSystem()->RefreshConnections();
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
		tps.BranchNodeLengthBase = 0.9238272f;
		tps.BranchNodeLengthAgeFactor = 0.9559f;

		tps.ApicalControlBase = 0.93576f;
		tps.ApicalControlAgeFactor = 0.918157f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.4244511f;
		tps.GravitropismBase = 0.2396032f;
		tps.GravitropismLevelFactor = 0.0f;
		tps.PruningFactor = 0.05f;
		tps.LowBranchPruningFactor = 0.639226f;
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
		tps.BranchNodeLengthBase = 1.02f;
		tps.BranchNodeLengthAgeFactor = 0.97f;

		tps.ApicalControlBase = 2.4f;
		tps.ApicalControlAgeFactor = 0.85f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.29f;
		tps.GravitropismBase = 0.061f;
		tps.GravitropismLevelFactor = 0.0f;
		tps.PruningFactor = 0.05f;
		tps.LowBranchPruningFactor = 1.3f;
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
		tps.BranchNodeLengthBase = 0.55f;
		tps.BranchNodeLengthAgeFactor = 0.97f;

		tps.ApicalControlBase = 2.2f;
		tps.ApicalControlAgeFactor = 0.5f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.29f;
		tps.GravitropismBase = -0.41f;
		tps.GravitropismLevelFactor = 0.15f;
		tps.PruningFactor = 0.7f;
		tps.LowBranchPruningFactor = 1.3f;
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
		tps.BranchNodeLengthBase = 0.49f;
		tps.BranchNodeLengthAgeFactor = 0.98f;

		tps.ApicalControlBase = 2.27f;
		tps.ApicalControlAgeFactor = 0.9f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.15f;
		tps.GravitropismBase = -0.47f;//-0.41f;
		tps.GravitropismLevelFactor = 0.14f;
		tps.PruningFactor = 0.82f;
		tps.LowBranchPruningFactor = 2.83f;
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
		tps.BranchNodeLengthBase = 0.4f;
		tps.BranchNodeLengthAgeFactor = 0.96f;

		tps.ApicalControlBase = 6.2f;
		tps.ApicalControlAgeFactor = 0.9f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.42f;
		tps.GravitropismBase = -0.43f;//-0.43f;
		tps.GravitropismLevelFactor = 0.73f;
		tps.PruningFactor = 0.12f;
		tps.LowBranchPruningFactor = 1.25f;
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
		tps.BranchNodeLengthBase = 0.55f;
		tps.BranchNodeLengthAgeFactor = 0.95f;

		tps.ApicalControlBase = 5.5f;
		tps.ApicalControlAgeFactor = 0.91f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.32f;
		tps.GravitropismBase = -0.21f;//-0.43f;
		tps.GravitropismLevelFactor = 0.15f;
		tps.PruningFactor = 0.48f;
		tps.LowBranchPruningFactor = 5.5f;
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
		tps.BranchNodeLengthBase = 0.4f;
		tps.BranchNodeLengthAgeFactor = 0.97f;

		tps.ApicalControlBase = 5.5f;
		tps.ApicalControlAgeFactor = 0.92f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.05f;
		tps.GravitropismBase = -0.15f;//-0.43f;
		tps.GravitropismLevelFactor = 0.12f;
		tps.PruningFactor = 0.22f;
		tps.LowBranchPruningFactor = 1.11f;
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
		tps.BranchNodeLengthBase = 0.51f;
		tps.BranchNodeLengthAgeFactor = 0.98f;

		tps.ApicalControlBase = 3.25f;
		tps.ApicalControlAgeFactor = 0.7f;
		tps.ApicalControlLevelFactor = 1.0f;
		tps.Phototropism = 0.15f;
		tps.GravitropismBase = -0.13f;//-0.43f;
		tps.GravitropismLevelFactor = 0.14f;
		tps.PruningFactor = 0.8f;
		tps.LowBranchPruningFactor = 2.9f;
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
		TreeManager::CalculateBranchNodeIllumination();
		for (auto& tree : trees) {

			Rotation rotation = EntityManager::GetComponentData<Rotation>(tree);
			LocalToWorld ltw = EntityManager::GetComponentData<LocalToWorld>(tree);
			TreeParameters treeParameters = EntityManager::GetComponentData<TreeParameters>(tree);
			Entity rootBranchNode = EntityManager::GetChildren(tree).at(0);

			if (GrowTree(tree)) {
				growed = true;
				CalculatePhysics(trees);
				CalculateCrownShyness(1.5f);
			}
		}
	}
	if (growed == false) {
		_Growing = false;
	}
}


inline float TreeUtilities::PlantSimulationSystem::GetApicalControl(std::shared_ptr<TreeData>& treeInfo, BranchNodeInfo& branchNodeInfo, TreeParameters& treeParameters, TreeAge& treeAge, int level) const
{
	float apicalControl = treeParameters.ApicalControlBase * glm::pow(treeParameters.ApicalControlAgeFactor, treeAge.Value);
	if (treeInfo->ApicalControlTimeVal.at(treeAge.Value) < 1.0f) {
		int reversedLevel = branchNodeInfo.MaxActivatedChildLevel - level + 1;
		return treeInfo->ApicalControlTimeLevelVal.at(treeAge.Value)[reversedLevel];
	}
	return treeInfo->ApicalControlTimeLevelVal.at(treeAge.Value)[level];
}

void TreeUtilities::PlantSimulationSystem::UpdateBranchNodeLength(Entity& branchNode)
{
	BranchNodeInfo branchNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(branchNode);
	branchNodeInfo.DistanceToBranchEnd = 0;
	branchNodeInfo.TotalDistanceToBranchEnd = 0;
	if (branchNodeInfo.Pruned) EntityManager::SetComponentData(branchNode, branchNodeInfo);
	EntityManager::ForEachChild(branchNode, [this, &branchNodeInfo](Entity child) {
		UpdateBranchNodeLength(child);
		BranchNodeInfo childNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(child);
		float d = childNodeInfo.DistanceToBranchEnd + childNodeInfo.DistanceToParent;
		branchNodeInfo.TotalDistanceToBranchEnd += childNodeInfo.DistanceToParent + childNodeInfo.TotalDistanceToBranchEnd;
		if (d > branchNodeInfo.DistanceToBranchEnd) branchNodeInfo.DistanceToBranchEnd = d;
		});
	EntityManager::SetComponentData(branchNode, branchNodeInfo);
}

void TreeUtilities::PlantSimulationSystem::UpdateBranchNodeActivatedLevel(Entity& branchNode)
{
	BranchNodeInfo branchNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(branchNode);
	if (branchNodeInfo.Pruned) EntityManager::SetComponentData(branchNode, branchNodeInfo);
	// go through node children and update their level accordingly
	float maxChildLength = 0;
	Entity maxChild;
	EntityManager::ForEachChild(branchNode, [this, &branchNodeInfo, &maxChild, &maxChildLength](Entity child) {
		BranchNodeInfo childNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(child);
		if (branchNodeInfo.Pruned) return;
		float d = childNodeInfo.TotalDistanceToBranchEnd + childNodeInfo.DistanceToParent;
		if (d > maxChildLength) {
			maxChildLength = d;
			maxChild = child;
		}
		});
	int maxChildLevel = branchNodeInfo.Level;

	EntityManager::ForEachChild(branchNode, [this, &branchNodeInfo, &maxChild](Entity child)
		{
			BranchNodeInfo childNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(child);
			if (branchNodeInfo.Pruned || child.Index == maxChild.Index) return;
			childNodeInfo.DistanceToBranchStart = childNodeInfo.DistanceToParent;
			childNodeInfo.Level = branchNodeInfo.Level + 1;
			childNodeInfo.MaxActivatedChildLevel = childNodeInfo.Level;
			childNodeInfo.IsApical = false;
			EntityManager::SetComponentData(child, childNodeInfo);
			UpdateBranchNodeActivatedLevel(child);
			childNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(child);
			if (childNodeInfo.MaxActivatedChildLevel > branchNodeInfo.MaxActivatedChildLevel) branchNodeInfo.MaxActivatedChildLevel = childNodeInfo.MaxActivatedChildLevel;
		}
	);
	EntityManager::SetComponentData(branchNode, branchNodeInfo);
	if (maxChild.Index != 0) {
		BranchNodeInfo childNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(maxChild);
		childNodeInfo.DistanceToBranchStart = branchNodeInfo.DistanceToBranchStart + childNodeInfo.DistanceToParent;
		childNodeInfo.Level = branchNodeInfo.Level;
		childNodeInfo.MaxActivatedChildLevel = branchNodeInfo.MaxActivatedChildLevel;
		childNodeInfo.IsApical = true;
		EntityManager::SetComponentData(maxChild, childNodeInfo);
		UpdateBranchNodeActivatedLevel(maxChild);
	}
}

void TreeUtilities::PlantSimulationSystem::UpdateLocalTransform(Entity& branchNode, TreeParameters& treeParameters, glm::mat4& parentLTW, glm::quat& treeRotation)
{
	BranchNodeInfo branchNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(branchNode);
	glm::quat actualLocalRotation = branchNodeInfo.DesiredLocalRotation;

	glm::vec3 scale;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(parentLTW, scale, branchNodeInfo.ParentRotation, branchNodeInfo.ParentTranslation, skew, perspective);

#pragma region Apply force here.
	glm::quat newGlobalRotation = treeRotation * branchNodeInfo.ParentRotation * branchNodeInfo.DesiredLocalRotation;
	glm::vec3 front = newGlobalRotation * glm::vec3(0, 0, -1);
	glm::vec3 up = newGlobalRotation * glm::vec3(0, 1, 0);
	float gravityBending = treeParameters.GravityBendingStrength * branchNodeInfo.AccumulatedGravity;
	front += gravityBending * glm::vec3(0, -1, 0);
	front = glm::normalize(front);
	up = glm::cross(glm::cross(front, up), front);
	newGlobalRotation = glm::quatLookAt(front, up);
	actualLocalRotation = glm::inverse(branchNodeInfo.ParentRotation) * glm::inverse(treeRotation) * newGlobalRotation;
#pragma endregion

	branchNodeInfo.LocalTransform = glm::translate(glm::mat4(1.0f), actualLocalRotation * glm::vec3(0, 0, -1)
		* branchNodeInfo.DistanceToParent) * glm::mat4_cast(actualLocalRotation)
		* glm::scale(glm::vec3(1.0f));

	branchNodeInfo.GlobalTransform = parentLTW * branchNodeInfo.LocalTransform;
	float mainChildThickness = 0;
	BranchNodeInfo mainChildInfo;
	unsigned mainChildEntityIndex = 0;
	EntityManager::ForEachChild(branchNode, [this, &mainChildInfo, &mainChildEntityIndex, &mainChildThickness, &treeParameters, &branchNodeInfo, &treeRotation](Entity child)
		{
			UpdateLocalTransform(child, treeParameters, branchNodeInfo.GlobalTransform, treeRotation);
			BranchNodeInfo childNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(child);
			childNodeInfo.ParentThickness = branchNodeInfo.Thickness;
			if (mainChildThickness < childNodeInfo.Thickness) {
				glm::vec3 cScale;
				glm::quat cRotation;
				glm::vec3 cTranslation;
				glm::vec3 cSkew;
				glm::vec4 cPerspective;
				glm::decompose(childNodeInfo.GlobalTransform, cScale, cRotation, cTranslation, cSkew, cPerspective);
				branchNodeInfo.MainChildRotation = cRotation;
				mainChildEntityIndex = child.Index;
				mainChildInfo = childNodeInfo;
				mainChildInfo.IsMainChild = true;
			}
			EntityManager::SetComponentData(child, childNodeInfo);
		}
	);
	if (EntityManager::GetChildrenAmount(branchNode) == 0) branchNodeInfo.MainChildRotation = glm::inverse(treeRotation) * newGlobalRotation;
	else {
		EntityManager::SetComponentData(mainChildEntityIndex, mainChildInfo);
	}
	EntityManager::ForEachChild(branchNode, [this, &branchNodeInfo](Entity child)
		{
			BranchNodeInfo childNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(child);
			childNodeInfo.ParentMainChildRotation = branchNodeInfo.MainChildRotation;
			EntityManager::SetComponentData(child, childNodeInfo);
		}
	);
	EntityManager::SetComponentData(branchNode, branchNodeInfo);
}


void TreeUtilities::PlantSimulationSystem::DeactivateBud(BranchNodeInfo& branchNodeInfo, Bud& bud)
{
	branchNodeInfo.ActivatedBudsAmount--;
	bud.IsActive = false;
	if (bud.IsApical) branchNodeInfo.ApicalBudExist = false;
}


void TreeUtilities::PlantSimulationSystem::EvaluatePruning(Entity& branchNode, TreeParameters& treeParameters, TreeAge& treeAge, std::shared_ptr<TreeData>& treeInfo)
{
	BranchNodeInfo branchNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(branchNode);
	if (EntityManager::GetChildrenAmount(branchNode) == 0)
	{
		branchNodeInfo.IsActivatedEndNode = true;
		EntityManager::SetComponentData(branchNode, branchNodeInfo);
	}
	if (branchNodeInfo.Pruned) {
		return;
	}
	if (branchNodeInfo.Level == 0 && treeAge.Value < 3) return;
	if (branchNodeInfo.Level == 1 && !branchNodeInfo.IsApical) {
		float height = EntityManager::GetComponentData<LocalToWorld>(branchNode).Value[3].y;
		if (height < treeParameters.LowBranchPruningFactor && height < treeInfo->Height) {
			PruneBranchNode(branchNode, &branchNodeInfo);
			return;
		}
	}
	float normalL = branchNodeInfo.AccumulatedLength / treeParameters.BranchNodeLengthBase;
	float ratioScale = 1;
	float factor = ratioScale / glm::sqrt(branchNodeInfo.AccumulatedLength);
	//factor *= branchNodeInfo.AccumulatedLight;
	if (factor < treeParameters.PruningFactor) {
		PruneBranchNode(branchNode, &branchNodeInfo);
		return;
	}
	
	EntityManager::ForEachChild(branchNode, [this, &treeParameters, &treeAge, &treeInfo](Entity child)
		{
			EvaluatePruning(child, treeParameters, treeAge, treeInfo);
		}
	);
}

void TreeUtilities::PlantSimulationSystem::ApplyLocalTransform(Entity& treeEntity)
{
	glm::mat4 treeTransform = EntityManager::GetComponentData<LocalToWorld>(treeEntity).Value;
	auto treeIndex = EntityManager::GetComponentData<TreeIndex>(treeEntity).Value;
	auto treeInfo = EntityManager::GetSharedComponent<TreeData>(treeEntity);
	std::mutex heightMutex;
	float treeHeight = 0.0f;
	EntityManager::ForEach<TreeIndex, LocalToWorld, BranchNodeInfo>(_BranchNodeQuery,
		[treeTransform, treeIndex, &treeHeight, &heightMutex](int i, Entity branchNode, TreeIndex* index, LocalToWorld* ltw, BranchNodeInfo* info)
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
	treeInfo->Height = treeHeight;
}

void TreeUtilities::PlantSimulationSystem::CalculateDirectGravityForce(Entity& treeEntity, float gravity)
{
	float gravityFactor = EntityManager::GetComponentData<TreeParameters>(treeEntity).SaggingFactor;
	EntityManager::ForEach<LocalToWorld, BranchNodeInfo, Gravity>(_BranchNodeQuery, [gravityFactor, gravity](int i, Entity branchNode, LocalToWorld* ltw, BranchNodeInfo* bni, Gravity* fs)
		{
			float thickness = bni->Thickness;
			fs->Value = gravity * gravityFactor * bni->DistanceToParent;
		});
}

void TreeUtilities::PlantSimulationSystem::BackPropagateForce(Entity& branchNode, float fixedPropagationCoefficient)
{
	BranchNodeInfo branchNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(branchNode);
	float gfs = EntityManager::GetComponentData<Gravity>(branchNode).Value;
	branchNodeInfo.AccumulatedGravity = gfs;
	EntityManager::ForEachChild(branchNode, [this, &branchNodeInfo, fixedPropagationCoefficient](Entity child)
		{
			BackPropagateForce(child, fixedPropagationCoefficient);
			auto childNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(child);
			branchNodeInfo.AccumulatedGravity += fixedPropagationCoefficient * childNodeInfo.AccumulatedGravity * childNodeInfo.Thickness / branchNodeInfo.Thickness;
		});
	EntityManager::SetComponentData(branchNode, branchNodeInfo);
}

void PlantSimulationSystem::CalculateCrownShyness(float radius)
{
	std::vector<LocalToWorld> branchNodesLTWs;
	std::vector<TreeIndex> branchNodesTreeIndices;
	_BranchNodeQuery.ToComponentDataArray(branchNodesLTWs);
	_BranchNodeQuery.ToComponentDataArray(branchNodesTreeIndices);

	EntityManager::ForEach<LocalToWorld, BranchNodeInfo, TreeIndex>(_BranchNodeQuery, [radius, &branchNodesLTWs, &branchNodesTreeIndices, this](int i, Entity branchNode, LocalToWorld* ltw, BranchNodeInfo* info, TreeIndex* index)
		{
			if (info->Pruned) return;
			if (!info->IsActivatedEndNode) return;
			for(size_t bi = 0; bi < branchNodesLTWs.size(); bi++)
			{
				if(branchNodesTreeIndices[bi].Value != index->Value)
				{
					//auto position1 = glm::vec2(ltw->Value[3].x, ltw->Value[3].z);
					//auto position2 = glm::vec2(branchNodesLTWs[bi].Value[3].x, branchNodesLTWs[bi].Value[3].z);
					auto position1 = glm::vec3(ltw->Value[3].x, ltw->Value[3].y, ltw->Value[3].z);
					auto position2 = glm::vec3(branchNodesLTWs[bi].Value[3].x, branchNodesLTWs[bi].Value[3].y, branchNodesLTWs[bi].Value[3].z);
					if(glm::distance(position1, position2) < radius)
					{
						info->Pruned = true;
						info->IsActivatedEndNode = false;
					}
				}
			}
		}
	);
}

inline void PlantSimulationSystem::PruneBranchNode(Entity& branchNode, BranchNodeInfo* branchNodeInfo)
{
	branchNodeInfo->Pruned = true;
	branchNodeInfo->IsActivatedEndNode = false;
	EntityManager::SetComponentData(branchNode, *branchNodeInfo);
}

void PlantSimulationSystem::BuildConvexHullForTree(Entity& tree)
{
	std::vector<LocalToWorld> branchNodesLTWs;
	std::vector<BranchNodeInfo> branchNodeInfos;
	const auto treeIndex = EntityManager::GetComponentData<TreeIndex>(tree);
	_BranchNodeQuery.ToComponentDataArray(treeIndex, branchNodesLTWs);
	_BranchNodeQuery.ToComponentDataArray(treeIndex, branchNodeInfos);

	quickhull::QuickHull<float> qh; // Could be double as well
	std::vector<quickhull::Vector3<float>> pointCloud;
	for(size_t i = 0; i < branchNodesLTWs.size(); i++)
	{
		//if(branchNodesInfos[i].IsActivatedEndNode)
		{
			const auto transform = branchNodeInfos[i].GlobalTransform;
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

	for(size_t i = 0; i < indexBuffer.size(); i++)
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

		ifs >> temp; ifs >> ret.BranchNodeLengthBase; //Fixed
		ifs >> temp; ifs >> ret.BranchNodeLengthAgeFactor; // Training target


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
		output += "\nBranchNodeLengthBase\n";  output += std::to_string(treeParameters.BranchNodeLengthBase);
		output += "\nBranchNodeLengthAgeFactor\n";  output += std::to_string(treeParameters.BranchNodeLengthAgeFactor);
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

void TreeUtilities::PlantSimulationSystem::UpdateBranchNodeResource(Entity& branchNode, TreeParameters& treeParameters, TreeAge& treeAge)
{
	BranchNodeInfo branchNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(branchNode);

	branchNodeInfo.DistanceToBranchEnd = 0;
	branchNodeInfo.NumValidChild = 0;

	if (EntityManager::GetChildrenAmount(branchNode) == 0) branchNodeInfo.Thickness = treeParameters.EndNodeThickness;
	else {
		branchNodeInfo.Thickness = 0;
	}

	Illumination branchNodeIllumination = EntityManager::GetComponentData<Illumination>(branchNode);
	branchNodeInfo.AccumulatedLight = branchNodeIllumination.Value / TreeManager::GetLightEstimator()->GetMaxIllumination();
	branchNodeInfo.AccumulatedLength = branchNodeInfo.DistanceToParent;
	branchNodeInfo.AccumulatedActivatedBudsAmount = branchNodeInfo.ActivatedBudsAmount;
	branchNodeInfo.BranchEndNodeAmount = EntityManager::GetChildrenAmount(branchNode) == 0 ? 1 : 0;

	float mainChildThickness = 0.0f;
	EntityManager::ForEachChild(branchNode, [this, &mainChildThickness, &branchNodeInfo, &treeParameters, &treeAge](Entity child)
		{
			UpdateBranchNodeResource(child, treeParameters, treeAge);
			BranchNodeInfo childNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(child);

			if (branchNodeInfo.MaxChildLevel < childNodeInfo.MaxChildLevel) {
				branchNodeInfo.MaxChildLevel = childNodeInfo.MaxChildLevel;
			}
			else if (branchNodeInfo.MaxChildLevel < childNodeInfo.Level) {
				branchNodeInfo.MaxChildLevel = childNodeInfo.Level;
			}
			float d = childNodeInfo.DistanceToBranchEnd + childNodeInfo.DistanceToParent;
			if (d > branchNodeInfo.DistanceToBranchEnd)branchNodeInfo.DistanceToBranchEnd = d;
			float cL = childNodeInfo.DistanceToParent;
			branchNodeInfo.NumValidChild += 1;

			branchNodeInfo.BranchEndNodeAmount += childNodeInfo.BranchEndNodeAmount;
			branchNodeInfo.Thickness += treeParameters.ThicknessControlFactor * childNodeInfo.Thickness;
			if (childNodeInfo.Thickness > mainChildThickness) {
				mainChildThickness = childNodeInfo.Thickness;
			}
			if (childNodeInfo.Pruned) return;
			branchNodeInfo.AccumulatedLight += childNodeInfo.AccumulatedLight;
			branchNodeInfo.AccumulatedActivatedBudsAmount += childNodeInfo.AccumulatedActivatedBudsAmount;
			branchNodeInfo.AccumulatedLength += childNodeInfo.AccumulatedLength;
		}
	);
	if (mainChildThickness > branchNodeInfo.Thickness) branchNodeInfo.Thickness = mainChildThickness;
	EntityManager::SetComponentData(branchNode, branchNodeInfo);
}

void TreeUtilities::PlantSimulationSystem::OnCreate()
{
	_getcwd(_CurrentWorkingDir, 256);
	_TreeQuery = TreeManager::GetTreeQuery();
	_BranchNodeQuery = TreeManager::GetBranchNodeQuery();
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
	if(_DisplayConvexHull)
	{
		std::vector<LocalToWorld> ltws;
		std::vector<Entity> trees;

		_TreeQuery.ToComponentDataArray(ltws);
		_TreeQuery.ToEntityArray(trees);

		
		for(size_t i = 0; i < trees.size(); i++)
		{
			auto data = EntityManager::GetSharedComponent<TreeData>(trees[i]);
			if (data->ConvexHull != nullptr) {
				RenderManager::DrawGizmoMesh(data->ConvexHull.get(), glm::vec4(0.5, 0.5, 0.5, 1.0), Application::GetMainCameraComponent()->Value.get(), ltws[i].Value);
			}
		}
		
	}
	DrawGUI();
}

Entity TreeUtilities::PlantSimulationSystem::CreateTree(std::shared_ptr<Material> treeSurfaceMaterial, TreeParameters treeParameters, glm::vec3 position, bool enabled)
{
	auto treeEntity = TreeManager::CreateTree(treeSurfaceMaterial);
	Entity branchNodeEntity = TreeManager::CreateBranchNode(EntityManager::GetComponentData<TreeIndex>(treeEntity), treeEntity);
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

	BudList list = EntityManager::GetComponentData<BudList>(branchNodeEntity);
	list.Buds->push_back(bud);

	BranchNodeInfo branchNodeInfo;
	branchNodeInfo.IsActivatedEndNode = false;
	branchNodeInfo.IsApical = true;
	branchNodeInfo.ApicalBudExist = true;
	branchNodeInfo.Level = 0;
	branchNodeInfo.MaxActivatedChildLevel = 0;
	branchNodeInfo.ActivatedBudsAmount = 1;
	branchNodeInfo.Pruned = false;
	branchNodeInfo.DistanceToParent = 0;
	branchNodeInfo.DesiredGlobalRotation = glm::quatLookAt(glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
	branchNodeInfo.DesiredLocalRotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
	auto treeInfo = EntityManager::GetSharedComponent<TreeData>(treeEntity);
	treeInfo->MeshGenerated = false;
	glm::mat4 transform = glm::identity<glm::mat4>();
	UpdateLocalTransform(branchNodeEntity, treeParameters, transform, r.Value);

	EntityManager::SetComponentData(branchNodeEntity, branchNodeInfo);
	EntityManager::SetComponentData(treeEntity, treeParameters);
	EntityManager::SetComponentData(treeEntity, age);
	_Growing = true;
	return treeEntity;
}

void PlantSimulationSystem::CreateDefaultTree()
{
	TreeParameters tps;
	LoadDefaultTreeParameters(1, tps);
	CreateTree(_DefaultTreeSurfaceMaterial1, tps, glm::vec3(0.0f), true);
}

#pragma endregion
