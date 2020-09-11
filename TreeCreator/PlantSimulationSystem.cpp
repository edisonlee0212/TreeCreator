#include "PlantSimulationSystem.h"

#include <gtx/matrix_decompose.hpp>

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
				ImGui::BeginChild("ChildL", ImVec2(300, 200), true, ImGuiWindowFlags_None | ImGuiWindowFlags_MenuBar);
				if (ImGui::BeginMenuBar())
				{
					if (ImGui::BeginMenu("Settings"))
					{
						
					}
					ImGui::EndMenuBar();
				}
				ImGui::Columns(1);
				ImGui::PushItemWidth(200);
				ImGui::InputFloat3("Position", &_NewTreePosition.x);
				ImGui::PopItemWidth();
				ImGui::NextColumn();
				ImGui::ColorEdit4("Tree Color", &_NewTreeColor.Color.x);
				ImGui::EndChild();
				ImGui::PopStyleVar();
				ImGui::SameLine();
				ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
				ImGui::BeginChild("ChildR", ImVec2(400, 200), true, ImGuiWindowFlags_None | ImGuiWindowFlags_MenuBar);
				if (ImGui::BeginMenuBar())
				{
					if (ImGui::BeginMenu("Tree parameters")) {
						ImGui::Checkbox("Show full parameters", &_DisplayFullParam);
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Load from..."))
					{
						if (ImGui::Button("Load preset 1")) {
							LoadDefaultTreeParameters(1);
						}
						if (ImGui::Button("Load preset 2")) {
							LoadDefaultTreeParameters(2);
						}

						ImGui::Text("Path: /Resources/TreeParameters/");
						ImGui::SameLine();
						ImGui::InputText("", _TempFilePath, 256);
						if (ImGui::Button("Load paremeters")) {
							//TODO: Load and apply parameters
						}
						if (ImGui::Button("Save paremeters")) {
							//TODO: Save parameters
						}
						ImGui::EndMenu();
					}
					ImGui::EndMenuBar();
				}
				ImGui::Columns(1);
				ImGui::PushItemWidth(200);
				if (_DisplayFullParam) {
					ImGui::InputInt("Seed", &_NewTreeParameters.Seed);
#pragma region Show params
					ImGui::InputInt("Lateral Bud Number", &_NewTreeParameters.LateralBudNumber);
					ImGui::NextColumn();
					ImGui::InputFloat("Apical Angle Var", &_NewTreeParameters.VarianceApicalAngle);
					ImGui::NextColumn();
					ImGui::InputFloat2("Branching Angle M/Var", &_NewTreeParameters.MeanBranchingAngle);
					ImGui::NextColumn();
					ImGui::InputFloat2("Roll Angle M/Var", &_NewTreeParameters.MeanRollAngle);
					ImGui::NextColumn();
					ImGui::InputFloat2("Extintion Prob A/L", &_NewTreeParameters.ApicalBudExtintionRate);
					ImGui::NextColumn();
					ImGui::InputFloat3("AD Base/Dis/Age", &_NewTreeParameters.ApicalDominanceBase);
					ImGui::NextColumn();
					ImGui::InputFloat("Growth Rate", &_NewTreeParameters.GrowthRate);
					ImGui::NextColumn();
					ImGui::InputFloat2("Node Len Base/Age", &_NewTreeParameters.InternodeLengthBase);
					ImGui::NextColumn();
					ImGui::InputFloat4("AC Base/Age/Lvl/Dist", &_NewTreeParameters.ApicalControl);
					ImGui::NextColumn();
					ImGui::InputInt("Max Bud Age", &_NewTreeParameters.MaxBudAge);
					ImGui::NextColumn();
					ImGui::InputFloat("Phototropism", &_NewTreeParameters.Phototropism);
					ImGui::NextColumn();
					ImGui::InputFloat2("Gravitropism Base/Age", &_NewTreeParameters.GravitropismBase);
					ImGui::NextColumn();
					ImGui::InputFloat("PruningFactor", &_NewTreeParameters.PruningFactor);
					ImGui::NextColumn();
					ImGui::InputFloat("LowBranchPruningFactor", &_NewTreeParameters.LowBranchPruningFactor);
					ImGui::NextColumn();
					ImGui::InputFloat("GravityBendingStrength", &_NewTreeParameters.GravityBendingStrength);
					ImGui::NextColumn();
					ImGui::InputFloat2("Lighting Factor A/L", &_NewTreeParameters.ApicalBudLightingFactor);
					ImGui::NextColumn();
					ImGui::InputFloat2("Gravity Base/BPCo", &_NewTreeParameters.GravityFactor);
					ImGui::NextColumn();
					ImGui::InputInt("Age", &_NewTreeParameters.Age);
					ImGui::NextColumn();
					ImGui::InputFloat2("Thickness End/Fac", &_NewTreeParameters.EndNodeThickness);
					ImGui::NextColumn();
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
					CreateTree(_DefaultTreeSurfaceMaterial1, _NewTreeParameters, _NewTreeColor, _NewTreePosition, true);
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
		static int iterations;
		ImGui::InputInt("Iteration", &iterations);
		ImGui::SameLine();
		if (ImGui::Button("Push iterations") && iterations > 0) {
			_GrowIterationCount += iterations;
		}
		ImGui::Text(("Current iteration left: " + std::to_string(_GrowIterationCount)).c_str());
		ImGui::SliderFloat("Gravity", &_Gravity, 0.0f, 20.0f);
	}
	
	ImGui::End();

}

void TreeUtilities::PlantSimulationSystem::FixedUpdate()
{
	auto trees = std::vector<Entity>();
	_TreeQuery.ToEntityArray(&trees);
	if (_GrowIterationCount > 0) {
		_GrowIterationCount--;
		TryGrowAllTrees(trees);
	}
	CalculatePhysics(trees);

	while (!_MeshGenerationQueue.empty()) {
		Entity targetTree = _MeshGenerationQueue.front();
		TreeManager::GenerateSimpleMeshForTree(targetTree, 0.01f);
		TreeInfo treeInfo = EntityManager::GetComponentData<TreeInfo>(targetTree);
		_MeshGenerationQueue.pop();
	}
}

bool TreeUtilities::PlantSimulationSystem::GrowTree(Entity& tree)
{
#pragma region Collect tree data
	TreeInfo treeInfo = EntityManager::GetComponentData<TreeInfo>(tree);
	TreeAge treeAge = EntityManager::GetComponentData<TreeAge>(tree);
	TreeParameters treeParameters = EntityManager::GetComponentData<TreeParameters>(tree);
	TreeIndex treeIndex = EntityManager::GetComponentData<TreeIndex>(tree);
	LocalToWorld treeLocalToWorld = EntityManager::GetComponentData<LocalToWorld>(tree);
	Rotation treeRotation = EntityManager::GetComponentData<Rotation>(tree);
#pragma endregion
	if (treeAge.Value >= treeParameters.Age) {
		return false;
	}
#pragma region Prepare info
	if (treeAge.Value == 0) {
		treeInfo.CurrentSeed = treeParameters.Seed;
		srand(treeInfo.CurrentSeed);
	}
	else {
		srand(treeInfo.CurrentSeed);
		treeInfo.CurrentSeed = glm::linearRand(0.0f, 1.0f) * INT_MAX;
	}
	treeInfo.MaxBranchingDepth = 3;
	treeInfo.ActiveLength = 0;
	int timeOff = 4;
	treeInfo.ApicalDominanceTimeVal->resize(treeParameters.Age + timeOff);
	treeInfo.GravitropismLevelVal->resize(treeParameters.Age + timeOff);
	treeInfo.GravitropismLevelVal->resize(treeParameters.Age + timeOff);
	treeInfo.ApicalControlTimeVal->resize(treeParameters.Age + timeOff);
	treeInfo.ApicalControlTimeLevelVal->resize(treeParameters.Age + timeOff);
	for (size_t t = 0; t < treeParameters.Age + timeOff; t++) {
		treeInfo.ApicalDominanceTimeVal->at(t) = glm::pow(treeParameters.ApicalDominanceAgeFactor, t);
		treeInfo.GravitropismLevelVal->at(t) = treeParameters.GravitropismBase + t * treeParameters.GravitropismLevelFactor;
		treeInfo.ApicalControlTimeVal->at(t) = treeParameters.ApicalControl * glm::pow(treeParameters.ApicalControlAgeDescFactor, t);

		treeInfo.ApicalControlTimeLevelVal->at(t).resize(treeParameters.Age + timeOff);
		float baseApicalControlVal = treeInfo.ApicalControlTimeVal->at(t);
		treeInfo.ApicalControlTimeLevelVal->at(t).at(0) = 1.0f;
		float currentVal = 1;
		for (size_t level = 1; level < treeParameters.Age + timeOff; level++) {
			if (baseApicalControlVal >= 1) {
				currentVal *= 1.0f + (baseApicalControlVal - 1.0f) * glm::pow(treeParameters.ApicalControlLevelFactor, level);
				treeInfo.ApicalControlTimeLevelVal->at(t).at(level) = 1.0f / currentVal;
			}
			else {
				currentVal *= 1.0f - (1.0f - baseApicalControlVal) * glm::pow(treeParameters.ApicalControlLevelFactor, level);
				treeInfo.ApicalControlTimeLevelVal->at(t).at(level) = currentVal;
			}
		}
	}
#pragma endregion
#pragma region Update branch structure information
	Entity rootBranchNode = EntityManager::GetChildren(tree).at(0);
	UpdateBranchNodeLength(rootBranchNode);
	BranchNodeInfo tempBNInfo = EntityManager::GetComponentData<BranchNodeInfo>(rootBranchNode);
	tempBNInfo.Level = 0;
	tempBNInfo.MaxActivatedChildLevel = 0;
	tempBNInfo.DistanceToBranchStart = 0;
	EntityManager::SetComponentData(rootBranchNode, tempBNInfo);
	UpdateBranchNodeActivatedLevel(rootBranchNode);
#pragma endregion
	bool growed = GrowShoots(rootBranchNode, treeInfo, treeAge, treeParameters, treeIndex);
	treeAge.Value++;
	EntityManager::SetComponentData(tree, treeAge);
	if (growed) {
		UpdateBranchNodeResource(rootBranchNode, treeParameters, treeAge);
		EvaluatePruning(rootBranchNode, treeParameters, treeAge, treeInfo);
	}
	EntityManager::SetComponentData(tree, treeInfo);
	return growed;
}

#pragma region Helpers
void TreeUtilities::PlantSimulationSystem::CalculatePhysics(std::vector<Entity>& trees)
{
	for (auto& tree : trees) {
		Rotation rotation = EntityManager::GetComponentData<Rotation>(tree);
		TreeParameters treeParameters = EntityManager::GetComponentData<TreeParameters>(tree);
		Entity rootBranchNode = EntityManager::GetChildren(tree).at(0);
		CalculateDirectGravityForce(tree, _Gravity);
		BackPropagateForce(rootBranchNode, treeParameters.GravityBackPropageteFixedCoefficient);
		glm::mat4 transform = glm::identity<glm::mat4>(); //glm::translate(glm::mat4(1.0f), glm::vec3(0.0f))* glm::mat4_cast(glm::quatLookAt(glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)))* glm::scale(glm::vec3(1.0f));
		UpdateLocalTransform(rootBranchNode, treeParameters, transform, rotation.Value);
		ApplyLocalTransform(tree);
		TreeManager::GetBranchNodeSystem()->RefreshConnections();
	}
}

void TreeUtilities::PlantSimulationSystem::LoadDefaultTreeParameters(int preset)
{
	switch (preset)
	{
	case 1:
#pragma region Set default tree param
		_NewTreeParameters.Seed = 1;
		_NewTreeParameters.VarianceApicalAngle = 0.42990970562500003;
		_NewTreeParameters.LateralBudNumber = 2;
		_NewTreeParameters.MeanBranchingAngle = 27.198200000000000;
		_NewTreeParameters.VarianceBranchingAngle = 0.037388089600000000;
		_NewTreeParameters.MeanRollAngle = 113.11000000000000;
		_NewTreeParameters.VarianceRollAngle = 13.090141080900001;

		_NewTreeParameters.ApicalBudExtintionRate = 0.99903945880000000;
		_NewTreeParameters.LateralBudEntintionRate = 0.0062681600000000001;
		_NewTreeParameters.ApicalBudLightingFactor = 1.0;// 0.099225700000000000;
		_NewTreeParameters.LateralBudLightingFactor = 1.0005922199999999;
		_NewTreeParameters.ApicalDominanceBase = 5.0524730000000000;
		_NewTreeParameters.ApicalDominanceDistanceFactor = 0.37777800000000000;
		_NewTreeParameters.ApicalDominanceAgeFactor = 0.44704700000000003;
		_NewTreeParameters.GrowthRate = 1.3069500000000001;
		_NewTreeParameters.InternodeLengthBase = 0.92382719999999996;
		_NewTreeParameters.InternodeLengthAgeFactor = 0.95584000000000002;

		_NewTreeParameters.ApicalControl = 0.93576000000000004;
		_NewTreeParameters.ApicalControlAgeDescFactor = 0.91815700000000000;
		_NewTreeParameters.ApicalControlLevelFactor = 1.0000000000000000;
		_NewTreeParameters.Phototropism = 0.42445109999999999;
		_NewTreeParameters.GravitropismBase = 0.239603199999999998;
		_NewTreeParameters.PruningFactor = 0.05f;
		_NewTreeParameters.LowBranchPruningFactor = 0.63922599999999996;
		_NewTreeParameters.GravityBendingStrength = 0.2f;
		_NewTreeParameters.Age = 14;
		_NewTreeParameters.GravityFactor = 0.050594199999999999;
		_NewTreeParameters.MaxBudAge = 10;

		_NewTreeParameters.EndNodeThickness = 0.02f;
		_NewTreeParameters.ThicknessControlFactor = 0.75f;
		_NewTreeParameters.GravityBackPropageteFixedCoefficient = 0.5f;
#pragma endregion
		break;
	case 2:
		_NewTreeParameters.Seed = 1;
		_NewTreeParameters.VarianceApicalAngle = 38;
		_NewTreeParameters.LateralBudNumber = 4;
		_NewTreeParameters.MeanBranchingAngle = 38;
		_NewTreeParameters.VarianceBranchingAngle = 2;
		_NewTreeParameters.MeanRollAngle = 91;
		_NewTreeParameters.VarianceRollAngle = 1;

		_NewTreeParameters.ApicalBudExtintionRate = 0;
		_NewTreeParameters.LateralBudEntintionRate = 0.21f;
		_NewTreeParameters.ApicalBudLightingFactor = 0.39f;
		_NewTreeParameters.LateralBudLightingFactor = 1.13f;
		_NewTreeParameters.ApicalDominanceBase = 3.13f;
		_NewTreeParameters.ApicalDominanceDistanceFactor = 0.13f;
		_NewTreeParameters.ApicalDominanceAgeFactor = 0.82;
		_NewTreeParameters.GrowthRate = 0.98f;
		_NewTreeParameters.InternodeLengthBase = 1.02f;
		_NewTreeParameters.InternodeLengthAgeFactor = 0.97f;

		_NewTreeParameters.ApicalControl = 2.4f;
		_NewTreeParameters.ApicalControlAgeDescFactor = 0.85f;
		_NewTreeParameters.ApicalControlLevelFactor = 1.0f;
		_NewTreeParameters.Phototropism = 0.29f;
		_NewTreeParameters.GravitropismBase = 0.061f;
		_NewTreeParameters.PruningFactor = 0.05;
		_NewTreeParameters.LowBranchPruningFactor = 1.3f;
		_NewTreeParameters.GravityBendingStrength = 0.73f;
		_NewTreeParameters.Age = 14;
		_NewTreeParameters.GravityFactor = 0.050594199999999999;
		_NewTreeParameters.MaxBudAge = 8;
		_NewTreeParameters.EndNodeThickness = 0.02f;
		_NewTreeParameters.ThicknessControlFactor = 0.6f;
		_NewTreeParameters.GravityBackPropageteFixedCoefficient = 0.5f;
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
			}
			else {
				TreeInfo treeInfo = EntityManager::GetComponentData<TreeInfo>(tree);
				if (!treeInfo.MeshGenerated) {
					_MeshGenerationQueue.push(tree);
				}
			}
		}
	}
	if (growed == false) {
		_Growing = false;
	}
}


inline float TreeUtilities::PlantSimulationSystem::GetApicalControl(TreeInfo& treeInfo, BranchNodeInfo& branchNodeInfo, TreeParameters& treeParameters, TreeAge& treeAge, int level)
{
	float apicalControl = treeParameters.ApicalControl * glm::pow(treeParameters.ApicalControlAgeDescFactor, treeAge.Value);
	if (treeInfo.ApicalControlTimeVal->at(treeAge.Value) < 1.0f) {
		int reversedLevel = branchNodeInfo.MaxActivatedChildLevel - level + 1;
		return treeInfo.ApicalControlTimeLevelVal->at(treeAge.Value)[reversedLevel];
	}
	else {
		return treeInfo.ApicalControlTimeLevelVal->at(treeAge.Value)[level];
	}

	return 1.0f;
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
	float gravityBending = treeParameters.GravityBendingStrength * branchNodeInfo.AccmulatedGravity;
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
				glm::vec3 cscale;
				glm::quat crotation;
				glm::vec3 ctranslation;
				glm::vec3 cskew;
				glm::vec4 cperspective;
				glm::decompose(childNodeInfo.GlobalTransform, cscale, crotation, ctranslation, cskew, cperspective);
				branchNodeInfo.MainChildRotation = crotation;
				mainChildEntityIndex = child.Index;
				mainChildInfo = childNodeInfo;
				mainChildInfo.IsMainChild = true;
			}
			EntityManager::SetComponentData(child, childNodeInfo);
		}
	);
	if (EntityManager::GetChildrenAmount(branchNode) == 0) branchNodeInfo.MainChildRotation = glm::inverse(treeRotation) * newGlobalRotation;
	else EntityManager::SetComponentData(mainChildEntityIndex, mainChildInfo);
	EntityManager::SetComponentData(branchNode, branchNodeInfo);
}

bool TreeUtilities::PlantSimulationSystem::GrowShoots(Entity& branchNode, TreeInfo& treeInfo, TreeAge& treeAge, TreeParameters& treeParameters, TreeIndex& treeIndex)
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
	Illumination branchNodeIllumination = EntityManager::GetComponentData<Illumination>(branchNode);
	BudList budsList = EntityManager::GetComponentData<BudList>(branchNode);
	BranchNodeIndex branchNodeIndex = EntityManager::GetComponentData<BranchNodeIndex>(branchNode);
	float lateralInhibitorToAdd = 0;
	for (auto& bud : *budsList.Buds) {
		if (!bud.IsActive) continue;
#pragma region Bud kill probability
		float budKillProbability = 0;
		if (bud.IsApical) {
			budKillProbability = EntityManager::HasComponentData<TreeInfo>(EntityManager::GetParent(branchNode)) ? 0 : treeParameters.ApicalBudExtintionRate;
		}
		else {
			budKillProbability = treeParameters.LateralBudEntintionRate;
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
			int internodesToGrow = glm::floor(distanceToGrow + 0.5f);
			if (internodesToGrow != 0) {
				growSucceed = true;
			}
#pragma endregion
#pragma region Grow new shoot
			if (growSucceed) {
				float internodeLength = distanceToGrow / (float)internodesToGrow;
				internodeLength *= treeParameters.InternodeLengthBase * glm::pow(treeParameters.InternodeLengthAgeFactor, treeAge.Value);
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
				for (int selectedNewNodeIndex = 0; selectedNewNodeIndex < internodesToGrow; selectedNewNodeIndex++) {
#pragma region Setup branch node
					Entity newBranchNode = TreeManager::CreateBranchNode(treeIndex, prevBranchNode);
					BudList newBranchNodeBudList = EntityManager::GetComponentData<BudList>(newBranchNode);
					BranchNodeInfo newBranchNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(newBranchNode);
					newBranchNodeInfo.ApicalBudExist = true;
					newBranchNodeInfo.ActivatedBudsAmount = treeParameters.LateralBudNumber + 1;
					newBranchNodeInfo.DistanceToParent = internodeLength;
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
					glm::vec3 rollAngles = glm::vec3(0.0f, 0.0f, glm::radians(treeParameters.MeanRollAngle + treeParameters.VarianceRollAngle * glm::linearRand(-1, 1)));
					newBranchNodeInfo.DesiredLocalRotation *= glm::quat(rollAngles);
#pragma endregion

#pragma region Apply phototropism and gravitropism
					float gravitropism = treeInfo.GravitropismLevelVal->at(newBranchNodeInfo.Level);
					glm::quat globalRawRotation = prevBranchNodeRotation * newBranchNodeInfo.DesiredLocalRotation;
					glm::vec3 rawFront = globalRawRotation * glm::vec3(0, 0, -1.05f);
					glm::vec3 rawUp = globalRawRotation * glm::vec3(0, 1.05f, 0);
					glm::vec3 gravityDir = glm::vec3(0, -1, 0);
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
					treeInfo.ActiveLength += newBranchNodeInfo.DistanceToParent;
#pragma region Create Apical Bud
					Bud newApicalBud;
					newApicalBud.EulerAngles = glm::vec3(glm::gaussRand(glm::vec2(0.0f), glm::vec2(glm::radians(treeParameters.VarianceApicalAngle / 2.0f))), 0.0f);
					newApicalBud.IsActive = true;
					newApicalBud.IsApical = true;
					newApicalBud.StartAge = treeAge.Value;
					newBranchNodeBudList.Buds->push_back(newApicalBud);
#pragma endregion
#pragma region Create Lateral Buds
					for (int selectedNewBudIndex = 0; selectedNewBudIndex < treeParameters.LateralBudNumber; selectedNewBudIndex++) {
						Bud newLateralBud;
						float rollAngle = 360.0f * (selectedNewBudIndex + 1) / treeParameters.LateralBudNumber + treeParameters.VarianceBranchingAngle * glm::linearRand(-1, 1);
						float branchAngle = treeParameters.MeanBranchingAngle + treeParameters.VarianceBranchingAngle * glm::gaussRand(0.0f, 0.5f);
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
					localInhibitor += treeParameters.ApicalDominanceBase * treeInfo.ApicalDominanceTimeVal->at(treeAge.Value);
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

void TreeUtilities::PlantSimulationSystem::DeactivateBud(BranchNodeInfo& branchNodeInfo, Bud& bud)
{
	branchNodeInfo.ActivatedBudsAmount--;
	bud.IsActive = false;
	if (bud.IsApical) branchNodeInfo.ApicalBudExist = false;
}

void TreeUtilities::PlantSimulationSystem::PruneBranchNode(Entity& branchNode, TreeInfo& treeInfo)
{
	BranchNodeInfo branchNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(branchNode);
	branchNodeInfo.Pruned = true;
	treeInfo.ActiveLength -= branchNodeInfo.DistanceToParent;
	EntityManager::ForEachChild(branchNode, [this, &treeInfo](Entity child)
		{
			PruneBranchNode(child, treeInfo);
		}
	);
	EntityManager::SetComponentData(branchNode, branchNodeInfo);
}

void TreeUtilities::PlantSimulationSystem::EvaluatePruning(Entity& branchNode, TreeParameters& treeParameters, TreeAge& treeAge, TreeInfo& treeInfo)
{
	BranchNodeInfo branchNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(branchNode);
	if (branchNodeInfo.Pruned) return;
	if (branchNodeInfo.Level == 0 && treeAge.Value < 3) return;
	if (branchNodeInfo.Level == 1 && !branchNodeInfo.IsApical) {
		float height = EntityManager::GetComponentData<LocalToWorld>(branchNode).Value[3].y;
		if (height < treeParameters.LowBranchPruningFactor && height < treeInfo.Height) {
			PruneBranchNode(branchNode, treeInfo);
			return;
		}
	}
	float normalL = branchNodeInfo.AccmulatedLength / treeParameters.InternodeLengthBase;
	float ratioScale = 1;
	float factor = ratioScale / glm::sqrt(branchNodeInfo.AccmulatedLength);
	//factor *= branchNodeInfo.AccmulatedLight;
	if (factor < treeParameters.PruningFactor) {
		PruneBranchNode(branchNode, treeInfo);
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
	auto treeInfo = EntityManager::GetComponentData<TreeInfo>(treeEntity);
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
	treeInfo.Height = treeHeight;
	EntityManager::SetComponentData(treeEntity, treeInfo);
}

void TreeUtilities::PlantSimulationSystem::CalculateDirectGravityForce(Entity& treeEntity, float gravity)
{
	float gravityFactor = EntityManager::GetComponentData<TreeParameters>(treeEntity).GravityFactor;
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
	branchNodeInfo.AccmulatedGravity = gfs;
	EntityManager::ForEachChild(branchNode, [this, &branchNodeInfo, fixedPropagationCoefficient](Entity child)
		{
			BackPropagateForce(child, fixedPropagationCoefficient);
			auto childNodeInfo = EntityManager::GetComponentData<BranchNodeInfo>(child);
			branchNodeInfo.AccmulatedGravity += fixedPropagationCoefficient * childNodeInfo.AccmulatedGravity * childNodeInfo.Thickness / branchNodeInfo.Thickness;
		});
	EntityManager::SetComponentData(branchNode, branchNodeInfo);
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
	branchNodeInfo.AccmulatedLight = branchNodeIllumination.Value / TreeManager::GetLightEstimator()->GetMaxIllumination();
	branchNodeInfo.AccmulatedLength = branchNodeInfo.DistanceToParent;
	branchNodeInfo.AccmulatedActivatedBudsAmount = branchNodeInfo.ActivatedBudsAmount;
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
			branchNodeInfo.AccmulatedLight += childNodeInfo.AccmulatedLight;
			branchNodeInfo.AccmulatedActivatedBudsAmount += childNodeInfo.AccmulatedActivatedBudsAmount;
			branchNodeInfo.AccmulatedLength += childNodeInfo.AccmulatedLength;
		}
	);
	if (mainChildThickness > branchNodeInfo.Thickness) branchNodeInfo.Thickness = mainChildThickness;
	EntityManager::SetComponentData(branchNode, branchNodeInfo);
}

void TreeUtilities::PlantSimulationSystem::UpdateBranchNodeResourceAllocation(Entity& branchNode)
{

}

void TreeUtilities::PlantSimulationSystem::OnCreate()
{
	_TreeQuery = TreeManager::GetTreeQuery();
	_BranchNodeQuery = TreeManager::GetBranchNodeQuery();
	_Gravity = 0;
	for (int i = 0; i < 256; i++) {
		_TempFilePath[i] = 0;
	}

	_DefaultTreeSurfaceMaterial1 = new Material();
	_DefaultTreeSurfaceMaterial1->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto textureDiffuse1 = new Texture2D(TextureType::DIFFUSE);
	textureDiffuse1->LoadTexture(FileIO::GetResourcePath("Textures/brown.png"), "");
	auto textureNormal1 = new Texture2D(TextureType::NORMAL);
	textureNormal1->LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Bark_Pine_normal.jpg"), "");
	_DefaultTreeSurfaceMaterial1->Textures2Ds()->push_back(textureDiffuse1);
	_DefaultTreeSurfaceMaterial1->Textures2Ds()->push_back(textureNormal1);

	_DefaultTreeSurfaceMaterial2 = new Material();
	_DefaultTreeSurfaceMaterial2->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto textureDiffuse2 = new Texture2D(TextureType::DIFFUSE);
	textureDiffuse2->LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Aspen_bark_001_COLOR.jpg"), "");
	auto textureNormal2 = new Texture2D(TextureType::NORMAL);
	textureNormal2->LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Aspen_bark_001_NORM.jpg"), "");
	_DefaultTreeSurfaceMaterial2->Textures2Ds()->push_back(textureDiffuse2);
	_DefaultTreeSurfaceMaterial2->Textures2Ds()->push_back(textureNormal2);
	_Gravity = 1.0f;
	_NewTreeColor.Color = glm::vec4(1.0f);
	LoadDefaultTreeParameters(1);
	Enable();
}

void TreeUtilities::PlantSimulationSystem::OnDestroy()
{
}

void TreeUtilities::PlantSimulationSystem::Update()
{
	DrawGUI();
}



Entity TreeUtilities::PlantSimulationSystem::CreateTree(Material* treeSurfaceMaterial, TreeParameters treeParameters, TreeColor color, glm::vec3 position, bool enabled)
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
	EntityManager::SetComponentData(treeEntity, color);
#pragma endregion
	TreeAge age;
	age.Value = 0;
	age.Enable = enabled;

	Bud bud;
	bud.EulerAngles = glm::vec3(0.0f);
	bud.IsActive = true;
	bud.IsApical = true;
	bud.StartAge = 0;

	BudList list = EntityManager::GetComponentData<BudList>(branchNodeEntity);
	list.Buds->push_back(bud);

	BranchNodeInfo branchNodeInfo;
	branchNodeInfo.ApicalBudExist = true;
	branchNodeInfo.Level = 0;
	branchNodeInfo.MaxActivatedChildLevel = 0;
	branchNodeInfo.ActivatedBudsAmount = 1;
	branchNodeInfo.Pruned = false;
	branchNodeInfo.DistanceToParent = 0;
	branchNodeInfo.DesiredGlobalRotation = glm::quatLookAt(glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
	branchNodeInfo.DesiredLocalRotation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
	TreeInfo treeInfo = EntityManager::GetComponentData<TreeInfo>(treeEntity);
	treeInfo.MeshGenerated = false;
	glm::mat4 transform = glm::identity<glm::mat4>();
	UpdateLocalTransform(branchNodeEntity, treeParameters, transform, r.Value);

	EntityManager::SetComponentData(branchNodeEntity, branchNodeInfo);
	EntityManager::SetComponentData(treeEntity, treeParameters);
	EntityManager::SetComponentData(treeEntity, treeInfo);
	EntityManager::SetComponentData(treeEntity, age);
	_Growing = true;
	return treeEntity;
}

#pragma endregion
