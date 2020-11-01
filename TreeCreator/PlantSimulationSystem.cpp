#include "PlantSimulationSystem.h"
#include "TreeScene.h"

#include "quickhull/QuickHull.hpp"
#include <gtx/vector_angle.hpp>
#include <gtx/matrix_decompose.hpp>
#include <direct.h>

#include "pugixml/pugixml.hpp"

#include <CGAL/Point_set_3.h>

#include "CrownSurfaceRecon.h"

void TreeUtilities::PlantSimulationSystem::FixedUpdate()
{
	auto trees = std::vector<Entity>();
	_TreeQuery.ToEntityArray(trees);

	TryGrowAllTrees(trees);

	if (_GravityChanged)
	{
		_GravityChanged = false;
		for (auto& tree : trees) CalculatePhysics(tree);
		TreeManager::GetInternodeSystem()->RefreshConnections();
	}
}
void TreeUtilities::PlantSimulationSystem::TryGrowAllTrees(std::vector<Entity>& trees)
{
	bool growed = false;
	if (_Growing) {
		for (auto& tree : trees) {
			Rotation rotation = EntityManager::GetComponentData<Rotation>(tree);
			LocalToWorld ltw = EntityManager::GetComponentData<LocalToWorld>(tree);
			TreeParameters treeParameters = EntityManager::GetComponentData<TreeParameters>(tree);
			if (GrowTree(tree)) {
				growed = true;
			}
		}
	}
	if (growed == false) {
		if (_Growing)
		{
			_Growing = false;
		}

	}
	else
	{
		RefreshTrees();
	}
}
bool TreeUtilities::PlantSimulationSystem::GrowTree(Entity& treeEntity)
{
	if (EntityManager::GetChildrenAmount(treeEntity) == 0) return false;
#pragma region Collect tree data
	auto treeInfo = EntityManager::GetComponentData<TreeInfo>(treeEntity);
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
		treeInfo.CurrentSeed = treeParameters.Seed;
		srand(treeInfo.CurrentSeed);
	}
	else {
		srand(treeInfo.CurrentSeed);
		treeInfo.CurrentSeed = glm::linearRand(0.0f, 1.0f) * INT_MAX;
	}
	treeInfo.MaxBranchingDepth = 3;
	EntityManager::SetComponentData(treeEntity, treeInfo);
	const auto timeOff = treeAge.Value + treeAge.ToGrowIteration + 4;
	treeData->ApicalControlTimeVal.resize(timeOff);
	treeData->ApicalControlTimeLevelVal.resize(timeOff);
	for (auto t = 0; t < timeOff; t++) {
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
	const bool growed = GrowShoots(rootInternode, treeData, treeAge, treeParameters, treeIndex, treeLocalToWorld.Value);
	if (growed) {
		UpdateDistanceToBranchEnd(rootInternode, treeParameters, treeAge.Value);
		UpdateDistanceToBranchStart(rootInternode);
		CalculatePhysics(treeEntity);
		treeAge.Value++;
		treeAge.ToGrowIteration--;
		EntityManager::SetComponentData(treeEntity, treeAge);
	}
#pragma endregion
	return growed;
}
#pragma region Helpers
#pragma region I/O

void TreeUtilities::PlantSimulationSystem::ExportSettings(const std::string& path)
{
	std::ofstream ofs;
	ofs.open((path + ".xml").c_str(), std::ofstream::out | std::ofstream::trunc);
	if (ofs.is_open())
	{
		std::string output = "<TreeSettings amount=\"" + std::to_string(_NewTreeAmount) + "\">\n";
		ofs.write(output.c_str(), output.size());
		ofs.flush();
		for (auto i = 0; i < _NewTreeAmount; i++)
		{
			output = "\t<Instance index=\"" + std::to_string(i) + "\">\n";
			output += "\t\t<Position>\n";
			output += "\t\t\t<x>" + std::to_string(_NewTreePositions[i].x) + "</x>\n";
			output += "\t\t\t<y>" + std::to_string(_NewTreePositions[i].y) + "</y>\n";
			output += "\t\t\t<z>" + std::to_string(_NewTreePositions[i].z) + "</z>\n";
			output += "\t\t</Position>\n";
			ofs.write(output.c_str(), output.size());
			ofs.flush();
			TreeParameterExportHelper(ofs, _NewTreeParameters[i]);
			output = "\t</Instance>\n";
			ofs.write(output.c_str(), output.size());
			ofs.flush();
		}
		output = "</TreeSettings>\n";
		ofs.write(output.c_str(), output.size());
		ofs.flush();
		ofs.close();
		Debug::Log("Tree group saved: " + path + ".xml");
	}
	else
	{
		Debug::Error("Can't open file!");
	}
}
void PlantSimulationSystem::ImportSettings(const std::string& path)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file((path + ".xml").c_str());
	if (!result) {
		Debug::Error("Can't open file!");
		return;
	}
	_NewTreeAmount = doc.child("TreeSettings").attribute("amount").as_int();
	_NewTreeParameters.resize(_NewTreeAmount);
	_NewTreePositions.resize(_NewTreeAmount);
	int i = 0;
	for (const auto& instanceNode : doc.child("TreeSettings"))
	{
		const auto& positionNode = instanceNode.child("Position");
		_NewTreePositions[i].x = std::atof(positionNode.child("x").first_child().value());
		_NewTreePositions[i].y = std::atof(positionNode.child("y").first_child().value());
		_NewTreePositions[i].z = std::atof(positionNode.child("z").first_child().value());
		const auto& treeParametersNode = instanceNode.child("TreeParameters");
		for (const auto& parameterNode : treeParametersNode.children())
		{
			std::string name = parameterNode.name();
			if (name.compare("Seed") == 0)
			{
				_NewTreeParameters[i].Seed = std::atoi(parameterNode.first_child().value());
			}
			else if (name.compare("LateralBudPerNode") == 0)
			{
				_NewTreeParameters[i].LateralBudPerNode = std::atoi(parameterNode.first_child().value());
			}
			else if (name.compare("VarianceApicalAngle") == 0)
			{
				_NewTreeParameters[i].VarianceApicalAngle = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("BranchingAngleMean") == 0)
			{
				_NewTreeParameters[i].BranchingAngleMean = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("BranchingAngleVariance") == 0)
			{
				_NewTreeParameters[i].BranchingAngleVariance = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("RollAngleMean") == 0)
			{
				_NewTreeParameters[i].RollAngleMean = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("RollAngleVariance") == 0)
			{
				_NewTreeParameters[i].RollAngleVariance = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("ApicalBudKillProbability") == 0)
			{
				_NewTreeParameters[i].ApicalBudKillProbability = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("LateralBudKillProbability") == 0)
			{
				_NewTreeParameters[i].LateralBudKillProbability = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("ApicalDominanceBase") == 0)
			{
				_NewTreeParameters[i].ApicalDominanceBase = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("ApicalDominanceDistanceFactor") == 0)
			{
				_NewTreeParameters[i].ApicalDominanceDistanceFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("ApicalDominanceAgeFactor") == 0)
			{
				_NewTreeParameters[i].ApicalDominanceAgeFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("GrowthRate") == 0)
			{
				_NewTreeParameters[i].GrowthRate = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("InternodeLengthBase") == 0)
			{
				_NewTreeParameters[i].InternodeLengthBase = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("InternodeLengthAgeFactor") == 0)
			{
				_NewTreeParameters[i].InternodeLengthAgeFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("ApicalControlBase") == 0)
			{
				_NewTreeParameters[i].ApicalControlBase = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("ApicalControlAgeFactor") == 0)
			{
				_NewTreeParameters[i].ApicalControlAgeFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("ApicalControlLevelFactor") == 0)
			{
				_NewTreeParameters[i].ApicalControlLevelFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("ApicalControlDistanceFactor") == 0)
			{
				_NewTreeParameters[i].ApicalControlDistanceFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("MaxBudAge") == 0)
			{
				_NewTreeParameters[i].MaxBudAge = std::atoi(parameterNode.first_child().value());
			}
			else if (name.compare("Phototropism") == 0)
			{
				_NewTreeParameters[i].Phototropism = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("GravitropismBase") == 0)
			{
				_NewTreeParameters[i].GravitropismBase = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("GravitropismLevelFactor") == 0)
			{
				_NewTreeParameters[i].GravitropismLevelFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("PruningFactor") == 0)
			{
				_NewTreeParameters[i].PruningFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("LowBranchPruningFactor") == 0)
			{
				_NewTreeParameters[i].LowBranchPruningFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("ThicknessRemovalFactor") == 0)
			{
				_NewTreeParameters[i].ThicknessRemovalFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("GravityBendingStrength") == 0)
			{
				_NewTreeParameters[i].GravityBendingStrength = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("ApicalBudLightingFactor") == 0)
			{
				_NewTreeParameters[i].ApicalBudLightingFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("LateralBudLightingFactor") == 0)
			{
				_NewTreeParameters[i].LateralBudLightingFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("SaggingFactor") == 0)
			{
				_NewTreeParameters[i].SaggingFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("SaggingForceBackPropagateFixedCoefficient") == 0)
			{
				_NewTreeParameters[i].SaggingForceBackPropagateFixedCoefficient = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("EndNodeThickness") == 0)
			{
				_NewTreeParameters[i].EndNodeThickness = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("ThicknessControlFactor") == 0)
			{
				_NewTreeParameters[i].ThicknessControlFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("CrownShynessBase") == 0)
			{
				_NewTreeParameters[i].CrownShynessBase = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("CrownShynessFactor") == 0)
			{
				_NewTreeParameters[i].CrownShynessFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("LeafSizeX") == 0)
			{
				_NewTreeParameters[i].LeafSize.x = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("LeafSizeY") == 0)
			{
				_NewTreeParameters[i].LeafSize.y = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("LeafIlluminationLimit") == 0)
			{
				_NewTreeParameters[i].LeafIlluminationLimit = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("LeafInhibitorFactor") == 0)
			{
				_NewTreeParameters[i].LeafInhibitorFactor = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("IsBothSide") == 0)
			{
				_NewTreeParameters[i].IsBothSide = std::atoi(parameterNode.first_child().value()) == 1;
			}
			else if (name.compare("SideLeafAmount") == 0)
			{
				_NewTreeParameters[i].SideLeafAmount = std::atoi(parameterNode.first_child().value());
			}
			else if (name.compare("StartBendingAngle") == 0)
			{
				_NewTreeParameters[i].StartBendingAngle = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("BendingAngleIncrement") == 0)
			{
				_NewTreeParameters[i].BendingAngleIncrement = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("LeafPhotoTropism") == 0)
			{
				_NewTreeParameters[i].LeafPhotoTropism = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("LeafGravitropism") == 0)
			{
				_NewTreeParameters[i].LeafGravitropism = std::atof(parameterNode.first_child().value());
			}
			else if (name.compare("LeafDistance") == 0)
			{
				_NewTreeParameters[i].LeafDistance = std::atof(parameterNode.first_child().value());
			}
		}
		i++;
	}
}

void PlantSimulationSystem::TreeParameterExportHelper(std::ofstream& ofs, TreeParameters& treeParameters)
{
	std::string output;
	output += "\t\t<TreeParameters>\n";
	output += "\t\t\t<Seed>"; output += std::to_string(treeParameters.Seed) + "</Seed>\n";
#pragma region Geometric
	output += "\t\t\t<LateralBudPerNode>"; output += std::to_string(treeParameters.LateralBudPerNode) + "</LateralBudPerNode>\n";
	output += "\t\t\t<VarianceApicalAngle>";  output += std::to_string(treeParameters.VarianceApicalAngle) + "</VarianceApicalAngle>\n";
	output += "\t\t\t<BranchingAngleMean>";  output += std::to_string(treeParameters.BranchingAngleMean) + "</BranchingAngleMean>\n";
	output += "\t\t\t<BranchingAngleVariance>";  output += std::to_string(treeParameters.BranchingAngleVariance) + "</BranchingAngleVariance>\n";
	output += "\t\t\t<RollAngleMean>";  output += std::to_string(treeParameters.RollAngleMean) + "</RollAngleMean>\n";
	output += "\t\t\t<RollAngleVariance>";  output += std::to_string(treeParameters.RollAngleVariance) + "</RollAngleVariance>\n";
#pragma endregion
#pragma region Bud fate
	output += "\t\t\t<ApicalBudKillProbability>"; output += std::to_string(treeParameters.ApicalBudKillProbability) + "</ApicalBudKillProbability>\n";
	output += "\t\t\t<LateralBudKillProbability>"; output += std::to_string(treeParameters.LateralBudKillProbability) + "</LateralBudKillProbability>\n";
	output += "\t\t\t<ApicalDominanceBase>";  output += std::to_string(treeParameters.ApicalDominanceBase) + "</ApicalDominanceBase>\n";
	output += "\t\t\t<ApicalDominanceDistanceFactor>";  output += std::to_string(treeParameters.ApicalDominanceDistanceFactor) + "</ApicalDominanceDistanceFactor>\n";
	output += "\t\t\t<ApicalDominanceAgeFactor>";  output += std::to_string(treeParameters.ApicalDominanceAgeFactor) + "</ApicalDominanceAgeFactor>\n";
	output += "\t\t\t<GrowthRate>"; output += std::to_string(treeParameters.GrowthRate) + "</GrowthRate>\n";
	output += "\t\t\t<InternodeLengthBase>";  output += std::to_string(treeParameters.InternodeLengthBase) + "</InternodeLengthBase>\n";
	output += "\t\t\t<InternodeLengthAgeFactor>";  output += std::to_string(treeParameters.InternodeLengthAgeFactor) + "</InternodeLengthAgeFactor>\n";
	output += "\t\t\t<ApicalControlBase>";  output += std::to_string(treeParameters.ApicalControlBase) + "</ApicalControlBase>\n";
	output += "\t\t\t<ApicalControlAgeFactor>";  output += std::to_string(treeParameters.ApicalControlAgeFactor) + "</ApicalControlAgeFactor>\n";
	output += "\t\t\t<ApicalControlLevelFactor>";  output += std::to_string(treeParameters.ApicalControlLevelFactor) + "</ApicalControlLevelFactor>\n";
	output += "\t\t\t<ApicalControlDistanceFactor>";  output += std::to_string(treeParameters.ApicalControlDistanceFactor) + "</ApicalControlDistanceFactor>\n";
	output += "\t\t\t<MaxBudAge>"; output += std::to_string(treeParameters.MaxBudAge) + "</MaxBudAge>\n";
#pragma endregion
#pragma region Environmental
	output += "\t\t\t<Phototropism>"; output += std::to_string(treeParameters.Phototropism) + "</Phototropism>\n";
	output += "\t\t\t<GravitropismBase>"; output += std::to_string(treeParameters.GravitropismBase) + "</GravitropismBase>\n";
	output += "\t\t\t<GravitropismLevelFactor>"; output += std::to_string(treeParameters.GravitropismLevelFactor) + "</GravitropismLevelFactor>\n";
	output += "\t\t\t<PruningFactor>"; output += std::to_string(treeParameters.PruningFactor) + "</PruningFactor>\n";
	output += "\t\t\t<LowBranchPruningFactor>"; output += std::to_string(treeParameters.LowBranchPruningFactor) + "</LowBranchPruningFactor>\n";
	output += "\t\t\t<ThicknessRemovalFactor>"; output += std::to_string(treeParameters.ThicknessRemovalFactor) + "</ThicknessRemovalFactor>\n";
	output += "\t\t\t<GravityBendingStrength>"; output += std::to_string(treeParameters.GravityBendingStrength) + "</GravityBendingStrength>\n";
	output += "\t\t\t<ApicalBudLightingFactor>"; output += std::to_string(treeParameters.ApicalBudLightingFactor) + "</ApicalBudLightingFactor>\n";
	output += "\t\t\t<LateralBudLightingFactor>"; output += std::to_string(treeParameters.LateralBudLightingFactor) + "</LateralBudLightingFactor>\n";
#pragma endregion
#pragma region Sagging
	output += "\t\t\t<SaggingFactor>"; output += std::to_string(treeParameters.SaggingFactor) + "</SaggingFactor>\n";
	output += "\t\t\t<SaggingForceBackPropagateFixedCoefficient>"; output += std::to_string(treeParameters.SaggingForceBackPropagateFixedCoefficient) + "</SaggingForceBackPropagateFixedCoefficient>\n";
#pragma endregion
	output += "\t\t\t<EndNodeThickness>"; output += std::to_string(treeParameters.EndNodeThickness) + "</EndNodeThickness>\n";
	output += "\t\t\t<ThicknessControlFactor>"; output += std::to_string(treeParameters.ThicknessControlFactor) + "</ThicknessControlFactor>\n";

	output += "\t\t\t<CrownShynessBase>"; output += std::to_string(treeParameters.CrownShynessBase) + "</CrownShynessBase>\n";
	output += "\t\t\t<CrownShynessFactor>"; output += std::to_string(treeParameters.CrownShynessFactor) + "</CrownShynessFactor>\n";

	output += "\t\t\t<LeafSizeX>"; output += std::to_string(treeParameters.LeafSize.x) + "</LeafSizeX>\n";
	output += "\t\t\t<LeafSizeY>"; output += std::to_string(treeParameters.LeafSize.y) + "</LeafSizeY>\n";
	output += "\t\t\t<LeafIlluminationLimit>"; output += std::to_string(treeParameters.LeafIlluminationLimit) + "</LeafIlluminationLimit>\n";
	output += "\t\t\t<LeafInhibitorFactor>"; output += std::to_string(treeParameters.LeafInhibitorFactor) + "</LeafInhibitorFactor>\n";
	output += "\t\t\t<IsBothSide>"; output += std::string(treeParameters.IsBothSide ? "1" : "0") + "</IsBothSide>\n";
	output += "\t\t\t<SideLeafAmount>"; output += std::to_string(treeParameters.SideLeafAmount) + "</SideLeafAmount>\n";
	output += "\t\t\t<StartBendingAngle>"; output += std::to_string(treeParameters.StartBendingAngle) + "</StartBendingAngle>\n";
	output += "\t\t\t<BendingAngleIncrement>"; output += std::to_string(treeParameters.BendingAngleIncrement) + "</BendingAngleIncrement>\n";
	output += "\t\t\t<LeafPhotoTropism>"; output += std::to_string(treeParameters.LeafPhotoTropism) + "</LeafPhotoTropism>\n";
	output += "\t\t\t<LeafGravitropism>"; output += std::to_string(treeParameters.LeafGravitropism) + "</LeafGravitropism>\n";
	output += "\t\t\t<LeafDistance>"; output += std::to_string(treeParameters.LeafDistance) + "</LeafDistance>\n";

	output += "\t\t</TreeParameters>\n";
	ofs.write(output.c_str(), output.size());
	ofs.flush();
}
#pragma endregion
#pragma region Growth
Entity TreeUtilities::PlantSimulationSystem::CreateTree(std::shared_ptr<Material> treeLeafMaterial, std::shared_ptr<Mesh> treeLeafMesh, TreeParameters treeParameters, glm::vec3 position, bool enabled)
{
	auto mat = std::make_shared<Material>();
	mat->SetTexture(_DefaultTreeSurfaceTex1, TextureType::DIFFUSE);
	mat->SetTexture(_DefaultTreeSurfaceNTex1, TextureType::NORMAL);
	mat->SetTexture(_DefaultTreeSurfaceSTex1, TextureType::SPECULAR);
	auto treeEntity = TreeManager::CreateTree(mat, treeLeafMaterial, treeLeafMesh);
	Entity internode = TreeManager::CreateInternode(EntityManager::GetComponentData<TreeIndex>(treeEntity), treeEntity);
#pragma region Position & Style
	Translation t;
	t.Value = position;
	Scale s;
	s.Value = glm::vec3(1.0f);
	EulerRotation er;
	er.Value = glm::vec3(0);
	Rotation r;
	r.Value = glm::quat(glm::radians(er.Value));
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

	auto internodeData = EntityManager::GetSharedComponent<InternodeData>(internode);
	internodeData->Buds.push_back(bud);

	InternodeInfo internodeInfo;
	internodeInfo.IsActivatedEndNode = false;
	internodeInfo.IsMaxChild = true;
	internodeInfo.ApicalBudExist = true;
	internodeInfo.Order = 0;
	internodeInfo.MaxChildOrder = 0;
	internodeInfo.DistanceToBranchStart = 0;
	internodeInfo.MaxChildOrder = 0;
	internodeInfo.ActivatedBudsAmount = 1;
	internodeInfo.Pruned = false;
	internodeInfo.DistanceToParent = 0;
	internodeInfo.DesiredLocalRotation = glm::quat(glm::vec3(glm::radians(90.0f), 0.0f, 0.0f));
	EntityManager::SetComponentData(internode, internodeInfo);
	auto treeInfo = EntityManager::GetSharedComponent<TreeData>(treeEntity);
	treeInfo->MeshGenerated = false;
	auto id = glm::mat4(1.0f);
	UpdateLocalTransform(internode, treeParameters, id, ltw.Value);
	EntityManager::SetComponentData(treeEntity, treeParameters);
	EntityManager::SetComponentData(treeEntity, age);
	_Growing = true;
	return treeEntity;
}
bool TreeUtilities::PlantSimulationSystem::GrowShoots(Entity& internode, std::shared_ptr<TreeData>& treeData, TreeAge& treeAge, TreeParameters& treeParameters, TreeIndex& treeIndex, glm::mat4& treeTransform)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	internodeInfo.Inhibitor = 0;
	auto internodeData = EntityManager::GetSharedComponent<InternodeData>(internode);
	
#pragma region Grow child
	bool ret = false;
	EntityManager::ForEachChild(internode, [&ret, this, &internodeInfo, &treeData, &treeAge, &treeParameters, &treeIndex, &treeTransform](Entity childNode)
		{
			if (GrowShoots(childNode, treeData, treeAge, treeParameters, treeIndex, treeTransform)) ret = true;
			auto childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(childNode);
			internodeInfo.Inhibitor += childNodeInfo.Inhibitor * childNodeInfo.ParentInhibitorFactor;
		}
	);
	if (internodeInfo.Pruned)
	{
		internodeData->Buds.clear();
		internodeInfo.ActivatedBudsAmount = 0;
		internodeInfo.ApicalBudExist = false;
		EntityManager::SetComponentData(internode, internodeInfo);
		return false;
	}
	if (internodeInfo.ActivatedBudsAmount == 0) {
		EntityManager::SetComponentData(internode, internodeInfo);
		return ret;
	}
#pragma endregion
	auto internodeIllumination = EntityManager::GetComponentData<Illumination>(internode);
	float lateralBudsInhibitorToAdd = 0;
	for (auto& bud : internodeData->Buds) {
#pragma region Bud kill probability
		float budKillProbability = 0;
		if (bud.IsApical) {
			budKillProbability = EntityManager::HasComponentData<TreeInfo>(EntityManager::GetParent(internode)) ? 0 : treeParameters.ApicalBudKillProbability;
		}
		else {
			budKillProbability = treeParameters.LateralBudKillProbability;
		}
		float randomProb = glm::linearRand(0.0f, 1.0f);
		if (randomProb < budKillProbability) {
			bud.IsActive = false;
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
			budGrowProbability *= glm::pow(illumination, 
				bud.IsApical ? treeParameters.ApicalBudLightingFactor : treeParameters.LateralBudLightingFactor);
		}
		budGrowProbability *= internodeInfo.CrownShyness;
		// now check whether the bud is going to flush or not
		bool flush = treeAge.Value < 2 ? true : budGrowProbability >= glm::linearRand(0.0f, 1.0f);
#pragma endregion
		bool growSucceed = false;
		if (flush) {
			bool isLateral = !(bud.IsApical && EntityManager::GetChildrenAmount(internode) == 0);
#pragma region Compute total grow distance and internodes amount.
			int order = internodeInfo.Order;
			int level = internodeInfo.Level;
			if (isLateral) {
				order++;
				level++;
			}
			float apicalControl = GetApicalControl(treeData, internodeInfo, treeParameters, treeAge, order);
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
				Entity prevInternode = internode;
				InternodeInfo prevInternodeInfo = internodeInfo;
				glm::vec3 prevEulerAngle = bud.EulerAngles;
				glm::quat prevInternodeRotation;
				glm::vec3 scale;
				glm::vec3 trans;
				glm::quat rotation;
				glm::vec3 skew;
				glm::vec4 perspective;
				glm::decompose(internodeInfo.GlobalTransform, scale, rotation, trans, skew, perspective);
				prevInternodeRotation = rotation;
#pragma region Create internodes
				for (int selectedNewNodeIndex = 0; selectedNewNodeIndex < internodesToGrow; selectedNewNodeIndex++) {
#pragma region Setup internode
					Entity newInternode = TreeManager::CreateInternode(treeIndex, prevInternode);
					auto newInternodeData = EntityManager::GetSharedComponent<InternodeData>(newInternode);
					InternodeInfo newInternodeInfo = EntityManager::GetComponentData<InternodeInfo>(newInternode);
					if (selectedNewNodeIndex == internodesToGrow - 1) {
						newInternodeInfo.ApicalBudExist = true;
						newInternodeInfo.ActivatedBudsAmount = treeParameters.LateralBudPerNode + 1;
					}else
					{
						newInternodeInfo.ApicalBudExist = false;
						newInternodeInfo.ActivatedBudsAmount = treeParameters.LateralBudPerNode;
					}
					newInternodeInfo.DistanceToParent = internodeLength;
					treeData->ActiveLength += internodeLength;
					newInternodeInfo.Order = order;
					if(isLateral)
					{
						newInternodeInfo.Level = level;
						level++;
					}else
					{
						newInternodeInfo.Level = internodeInfo.Level; 
					}
					newInternodeInfo.Pruned = false;
					newInternodeInfo.StartAge = treeAge.Value;
					newInternodeInfo.ParentInhibitorFactor = glm::pow(treeParameters.ApicalDominanceDistanceFactor, newInternodeInfo.DistanceToParent);

#pragma endregion
#pragma region Transforms for internode
					newInternodeInfo.DesiredLocalRotation = glm::quat(prevEulerAngle);
#pragma region Roll internode
					glm::vec3 rollAngles = glm::vec3(0.0f, 0.0f, glm::radians(treeParameters.RollAngleMean + treeParameters.RollAngleVariance * glm::linearRand(-1, 1)));
					newInternodeInfo.DesiredLocalRotation *= glm::quat(rollAngles);
#pragma endregion
#pragma region Apply phototropism and gravitropism
					float gravitropism = treeParameters.GravitropismBase + prevInternodeInfo.DistanceToBranchStart * treeParameters.GravitropismLevelFactor;
					glm::vec3 scale;
					glm::vec3 skew;
					glm::vec4 perspective;
					glm::quat rotation;
					glm::vec3 translation;
					glm::decompose(treeTransform, scale, rotation, translation, skew, perspective);

					glm::quat globalRawRotation = rotation * prevInternodeRotation * newInternodeInfo.DesiredLocalRotation;
					glm::vec3 desiredFront = globalRawRotation * glm::vec3(0.0f, 0.0f, -1.0f);
					glm::vec3 desiredUp = globalRawRotation * glm::vec3(0.0f, 1.0f, 0.0f);
					ApplyTropism(glm::vec3(0, 1, 0), gravitropism, desiredFront, desiredUp);
					if (internodeIllumination.Value > 0) {
						ApplyTropism(-internodeIllumination.LightDir, treeParameters.Phototropism, desiredFront, desiredUp);
					}
					desiredFront = glm::normalize(desiredFront);
					desiredUp = glm::normalize(glm::cross(glm::cross(desiredFront, desiredUp), desiredFront));
					globalRawRotation = glm::quatLookAt(desiredFront, desiredUp);
					newInternodeInfo.DesiredLocalRotation = glm::inverse(rotation) * glm::inverse(prevInternodeRotation) * globalRawRotation;
					prevInternodeRotation = globalRawRotation;
#pragma endregion
#pragma endregion
					prevEulerAngle = glm::vec3(glm::gaussRand(glm::vec2(0.0f), glm::vec2(glm::radians(treeParameters.VarianceApicalAngle))), 0.0f);
#pragma region Create Apical Bud
					if (selectedNewNodeIndex == internodesToGrow - 1) {
						Bud newApicalBud;
						newApicalBud.EulerAngles = prevEulerAngle;
						newApicalBud.IsActive = true;
						newApicalBud.IsApical = true;
						newInternodeData->Buds.push_back(newApicalBud);
					}
#pragma endregion
#pragma region Create Lateral Buds
					for (int selectedNewBudIndex = 0; selectedNewBudIndex < treeParameters.LateralBudPerNode; selectedNewBudIndex++) {
						Bud newLateralBud;
						float rollAngle = 360.0f * (selectedNewBudIndex + 1) / treeParameters.LateralBudPerNode + treeParameters.BranchingAngleVariance * glm::linearRand(-1, 1);
						float branchAngle = treeParameters.BranchingAngleMean + treeParameters.BranchingAngleVariance * glm::gaussRand(0.0f, 0.5f);
						newLateralBud.EulerAngles = glm::vec3(glm::radians(branchAngle), 0.0f, glm::radians(rollAngle));
						newLateralBud.IsActive = true;
						newLateralBud.IsApical = false;
						newInternodeData->Buds.push_back(newLateralBud);
					}
#pragma endregion
					prevInternode = newInternode;
					prevInternodeInfo = newInternodeInfo;
#pragma region Apply new internode info
					EntityManager::SetComponentData(newInternode, newInternodeInfo);
#pragma endregion
				}
#pragma endregion
				bud.IsActive = false;
				if(isLateral)
				{
					lateralBudsInhibitorToAdd += treeParameters.ApicalDominanceBase * static_cast<float>(internodeInfo.ActivatedBudsAmount) * glm::pow(treeParameters.ApicalDominanceAgeFactor, treeAge.Value);
				}else
				{
					internodeInfo.Inhibitor += treeParameters.ApicalDominanceBase * static_cast<float>(internodeInfo.ActivatedBudsAmount) * glm::pow(treeParameters.ApicalDominanceAgeFactor, treeAge.Value);
				}
			}
#pragma endregion
		}
#pragma region If the bud didnt flush then check whether we should remove it because of the old age.
		if (!growSucceed) {
			int budAge = treeAge.Value - internodeInfo.StartAge;
			if (budAge > treeParameters.MaxBudAge) {
				bud.IsActive = false;
			}
		}
		else {
			ret = true;
		}
#pragma endregion
	}
	bool hasApicalBud = false;
	for (int i = 0; i < internodeData->Buds.size(); i++)
	{
		if (!internodeData->Buds[i].IsActive)
		{
			internodeData->Buds.erase(internodeData->Buds.begin() + i);
			i--;
		}
		else if (internodeData->Buds[i].IsApical)
		{
			hasApicalBud = true;
		}
	}
	internodeInfo.Inhibitor += lateralBudsInhibitorToAdd;
	internodeInfo.ActivatedBudsAmount = internodeData->Buds.size();
	internodeInfo.ApicalBudExist = hasApicalBud;
	EntityManager::SetComponentData(internode, internodeInfo);
	return ret;
}
inline float TreeUtilities::PlantSimulationSystem::GetApicalControl(std::shared_ptr<TreeData>& treeInfo, InternodeInfo& internodeInfo, TreeParameters& treeParameters, TreeAge& treeAge, int level) const
{
	float apicalControl = treeParameters.ApicalControlBase * glm::pow(treeParameters.ApicalControlAgeFactor, treeAge.Value);
	if (treeInfo->ApicalControlTimeVal.at(treeAge.Value) < 1.0f) {
		const int reversedLevel = internodeInfo.MaxChildOrder - level + 1;
		return treeInfo->ApicalControlTimeLevelVal.at(treeAge.Value)[reversedLevel];
	}
	return treeInfo->ApicalControlTimeLevelVal.at(treeAge.Value)[level];
}
#pragma endregion
#pragma region PostProcessing
#pragma region Pruning
void PlantSimulationSystem::CalculateCrownShyness(float detectionDistance)
{
	std::vector<LocalToWorld> internodesLTWs;
	std::vector<TreeIndex> internodesTreeIndices;
	std::vector<TreeIndex> treeIndices;
	std::vector<TreeParameters> treeParameters;
	_InternodeQuery.ToComponentDataArray<LocalToWorld, InternodeInfo>(internodesLTWs, [detectionDistance](InternodeInfo& info)
		{
			return info.LongestDistanceToEnd <= detectionDistance;
		});
	_InternodeQuery.ToComponentDataArray<TreeIndex, InternodeInfo>(internodesTreeIndices, [detectionDistance](InternodeInfo& info)
		{
			return info.LongestDistanceToEnd <= detectionDistance;
		});

	_TreeQuery.ToComponentDataArray(treeIndices);
	_TreeQuery.ToComponentDataArray(treeParameters);
	EntityManager::ForEach<LocalToWorld, InternodeInfo, TreeIndex>(_InternodeQuery, [detectionDistance, &treeIndices, &treeParameters, &internodesLTWs, &internodesTreeIndices, this](int i, Entity branchNode, LocalToWorld* ltw, InternodeInfo* info, TreeIndex* index)
		{
			if (info->Pruned) {
				info->CrownShyness = 1.0f;
				return;
			}
			if (info->LongestDistanceToEnd > detectionDistance) {
				info->CrownShyness = 1.0f;
				return;
			}
			//if (!info->IsActivatedEndNode) return;
			float crownShynessLimit = 0;
			float crownShynessFactor = 1.0f;
			for (size_t ii = 0; ii < treeIndices.size(); ii++)
			{
				if (treeIndices[ii].Value == index->Value)
				{
					crownShynessLimit = treeParameters[ii].CrownShynessBase;
					crownShynessFactor = treeParameters[ii].CrownShynessFactor;
					break;
				}
			}
			if (crownShynessLimit <= 0) return;
			float minDistance = FLT_MAX;
			for (size_t bi = 0; bi < internodesLTWs.size(); bi++)
			{
				if (internodesTreeIndices[bi].Value != index->Value)
				{
					auto position1 = glm::vec3(ltw->Value[3].x, ltw->Value[3].y, ltw->Value[3].z);
					auto position2 = glm::vec3(internodesLTWs[bi].Value[3].x, internodesLTWs[bi].Value[3].y, internodesLTWs[bi].Value[3].z);
					float d = glm::distance(position1, position2);
					if (minDistance > d) minDistance = d;
				}
			}
			minDistance /= crownShynessLimit;
			if (minDistance < 1.0f) minDistance = 1.0f;
			info->CrownShyness = glm::pow(1.0f - 1.0f / minDistance, crownShynessFactor);
		}
	);
}
void TreeUtilities::PlantSimulationSystem::EvaluatePruning(Entity& internode, TreeParameters& treeParameters, TreeAge& treeAge, TreeInfo& treeInfo)
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
	float height = EntityManager::GetComponentData<LocalToWorld>(internode).Value[3].y;
	if (height > 15)
	{
		PruneInternode(internode, &internodeInfo, 2);
		return;
	}
	if (internodeInfo.Order == 0 && treeAge.Value < 3) return;



	float ratioScale = 1;
	float factor = ratioScale / glm::sqrt(internodeInfo.AccumulatedLength);
	factor *= internodeInfo.AccumulatedLight;
	if (factor < treeParameters.PruningFactor) {
		PruneInternode(internode, &internodeInfo, 1);
		return;
	}

	EntityManager::ForEachChild(internode, [this, &treeParameters, &treeAge, &treeInfo](Entity child)
		{
			EvaluatePruning(child, treeParameters, treeAge, treeInfo);
		}
	);
}
void PlantSimulationSystem::EvaluateDirectionPruning(Entity& internode, glm::vec3 escapeDirection, float limitAngle)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	glm::vec3 scale;
	glm::vec3 trans;
	glm::quat rotation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(internodeInfo.GlobalTransform, scale, rotation, trans, skew, perspective);
	if (glm::angle(escapeDirection, rotation * glm::vec3(0.0f, 0.0f, -1.0f)) < glm::radians(limitAngle))
	{
		PruneInternode(internode, &internodeInfo, 2);
	}
	EntityManager::ForEachChild(internode, [this, &escapeDirection, &limitAngle](Entity child)
		{
			EvaluateDirectionPruning(child, escapeDirection, limitAngle);
		}
	);
}
inline void PlantSimulationSystem::PruneInternode(Entity& internode, InternodeInfo* internodeInfo, int PruneReason) const
{
	internodeInfo->Pruned = true;
	internodeInfo->IsActivatedEndNode = false;
	internodeInfo->PruneReason = PruneReason;
	EntityManager::SetComponentData(internode, *internodeInfo);
}
#pragma endregion
#pragma region Removal
bool PlantSimulationSystem::EvaluateRemoval(Entity& internode, TreeParameters& treeParameters)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	bool childLowCutoff = false;
	EntityManager::ForEachChild(internode, [&internodeInfo, this, &treeParameters, &childLowCutoff](Entity child)
		{
			InternodeInfo childInternodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			if (internodeInfo.Order != 0 && EntityManager::GetComponentData<LocalToWorld>(child).Value[3].y < treeParameters.LowBranchPruningFactor)
			{
				EntityManager::DeleteEntity(child);
				childLowCutoff = true;
				return;
			}
			if (internodeInfo.Thickness - childInternodeInfo.Thickness > treeParameters.ThicknessRemovalFactor)
			{
				EntityManager::DeleteEntity(child);
				return;
			}
			childLowCutoff = EvaluateRemoval(child, treeParameters);
			if (childLowCutoff)
			{
				EntityManager::DeleteEntity(child);
				if (internodeInfo.Order == 0)
				{
					childLowCutoff = false;
				}
			}
		}
	);
	return childLowCutoff;
}
#pragma endregion
#pragma region Physics
void TreeUtilities::PlantSimulationSystem::CalculatePhysics(Entity tree)
{
	LocalToWorld ltw = EntityManager::GetComponentData<LocalToWorld>(tree);
	TreeParameters treeParameters = EntityManager::GetComponentData<TreeParameters>(tree);
	if (EntityManager::GetChildrenAmount(tree) == 0) return;
	Entity rootInternode = EntityManager::GetChildren(tree).at(0);
	CalculateDirectGravityForce(tree, _Gravity);
	BackPropagateForce(rootInternode, treeParameters.SaggingForceBackPropagateFixedCoefficient);
	auto id = glm::mat4(1.0f);
	UpdateLocalTransform(rootInternode, treeParameters, id, ltw.Value);
	ApplyLocalTransform(tree);
}
void TreeUtilities::PlantSimulationSystem::ApplyLocalTransform(Entity& treeEntity) const
{
	glm::mat4 treeTransform = EntityManager::GetComponentData<LocalToWorld>(treeEntity).Value;
	auto treeIndex = EntityManager::GetComponentData<TreeIndex>(treeEntity).Value;
	auto treeInfo = EntityManager::GetComponentData<TreeInfo>(treeEntity);
	std::mutex heightMutex;
	float treeHeight = 0.0f;
	EntityManager::ForEach<TreeIndex, LocalToWorld, InternodeInfo>(_InternodeQuery,
		[treeTransform, treeIndex, &treeHeight, &heightMutex](int i, Entity internode, TreeIndex* index, LocalToWorld* ltw, InternodeInfo* info)
		{
			if (index->Value == treeIndex) {
				ltw->Value = treeTransform * info->GlobalTransform;
				if (info->GlobalTransform[3].y > treeHeight) {
					std::lock_guard<std::mutex> lock(heightMutex);
					treeHeight = info->GlobalTransform[3].y;
				}
			}
		}
	);
	treeInfo.Height = treeHeight;
	EntityManager::SetComponentData(treeEntity, treeInfo);
}
void TreeUtilities::PlantSimulationSystem::CalculateDirectGravityForce(Entity& treeEntity, float gravity) const
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

#pragma endregion
#pragma endregion
#pragma region Info Updates
void TreeUtilities::PlantSimulationSystem::UpdateDistanceToBranchEnd(Entity& internode, TreeParameters& treeParameters, int treeAge)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	internodeInfo.MaxChildOrder = internodeInfo.Order;
	internodeInfo.BranchEndPosition = internodeInfo.GlobalTransform[3];
	internodeInfo.DistanceToBranchEnd = 0;
	internodeInfo.LongestDistanceToEnd = 0;
	internodeInfo.TotalDistanceToEnd = 0;
	internodeInfo.BranchEndInternodeAmount = EntityManager::GetChildrenAmount(internode) == 0 ? 1 : 0;
	//internodeInfo.Inhibitor = treeParameters.ApicalDominanceBase * static_cast<float>(internodeInfo.ActivatedBudsAmount) * glm::pow(treeParameters.ApicalDominanceAgeFactor, (treeAge - internodeInfo.StartAge));
	if (EntityManager::GetChildrenAmount(internode) == 0) {
		internodeInfo.Thickness = treeParameters.EndNodeThickness;
	}
	else {
		internodeInfo.Thickness = 0;
	}
	float mainChildThickness = 0.0f;
	EntityManager::ForEachChild(internode, [this, &internodeInfo, &mainChildThickness, &treeParameters, treeAge](Entity child)
		{
			UpdateDistanceToBranchEnd(child, treeParameters, treeAge);
			const InternodeInfo childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			if (childNodeInfo.MaxChildOrder > internodeInfo.MaxChildOrder) internodeInfo.MaxChildOrder = childNodeInfo.MaxChildOrder;
			//internodeInfo.Inhibitor += childNodeInfo.Inhibitor * childNodeInfo.ParentInhibitorFactor;
			float d = childNodeInfo.LongestDistanceToEnd + childNodeInfo.DistanceToParent;
			internodeInfo.TotalDistanceToEnd += childNodeInfo.DistanceToParent + childNodeInfo.TotalDistanceToEnd;
			if (d > internodeInfo.LongestDistanceToEnd) internodeInfo.LongestDistanceToEnd = d;
			if (childNodeInfo.Order == internodeInfo.Order)
			{
				internodeInfo.DistanceToBranchEnd = childNodeInfo.DistanceToBranchEnd + childNodeInfo.DistanceToParent;
				internodeInfo.BranchEndPosition = childNodeInfo.BranchEndPosition;
			}
			internodeInfo.BranchEndInternodeAmount += childNodeInfo.BranchEndInternodeAmount;
			internodeInfo.Thickness += treeParameters.ThicknessControlFactor * childNodeInfo.Thickness;
			if (childNodeInfo.Thickness > mainChildThickness) {
				mainChildThickness = childNodeInfo.Thickness;
			}
		}
	);
	if (mainChildThickness > internodeInfo.Thickness) internodeInfo.Thickness = mainChildThickness;
	EntityManager::SetComponentData(internode, internodeInfo);
}
void TreeUtilities::PlantSimulationSystem::UpdateDistanceToBranchStart(Entity& internode)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	// go through node children and update their level accordingly
	float maxChildLength = 0;
	Entity maxChild;
	EntityManager::ForEachChild(internode, [this, &maxChild, &maxChildLength](Entity child)
		{
			const InternodeInfo childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			const float d = childNodeInfo.TotalDistanceToEnd + childNodeInfo.DistanceToParent;
			if (d > maxChildLength) {
				maxChildLength = d;
				maxChild = child;
			}
		}
	);
	EntityManager::ForEachChild(internode, [this, &internodeInfo, &maxChild](Entity child)
		{
			InternodeInfo childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			childNodeInfo.DistanceToRoot = internodeInfo.DistanceToRoot + childNodeInfo.DistanceToParent;
			if (childNodeInfo.Order != internodeInfo.Order) {
				childNodeInfo.BranchStartPosition = internodeInfo.GlobalTransform[3];
				childNodeInfo.DistanceToBranchStart = childNodeInfo.DistanceToParent;
			}
			else
			{
				childNodeInfo.BranchStartPosition = internodeInfo.BranchStartPosition;
				childNodeInfo.DistanceToBranchStart = internodeInfo.DistanceToBranchStart + childNodeInfo.DistanceToParent;
			}
			childNodeInfo.IsMaxChild = child.Index == maxChild.Index;
			EntityManager::SetComponentData(child, childNodeInfo);
			UpdateDistanceToBranchStart(child);

		}
	);
	EntityManager::SetComponentData(internode, internodeInfo);
}
void TreeUtilities::PlantSimulationSystem::UpdateLocalTransform(Entity& internode, TreeParameters& treeParameters, glm::mat4& parentLTW, glm::mat4& treeLTW)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	glm::quat actualLocalRotation = internodeInfo.DesiredLocalRotation;

	glm::vec3 scale;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::quat rotation;
	glm::vec3 translation;
	glm::decompose(parentLTW, scale, internodeInfo.ParentRotation, internodeInfo.ParentTranslation, skew, perspective);

#pragma region Apply force here.
	glm::decompose(treeLTW, scale, rotation, translation, skew, perspective);
	glm::quat newGlobalRotation = rotation * internodeInfo.ParentRotation * internodeInfo.DesiredLocalRotation;
	glm::vec3 front = newGlobalRotation * glm::vec3(0, 0, -1);
	glm::vec3 up = newGlobalRotation * glm::vec3(0, 1, 0);
	float gravityBending = treeParameters.GravityBendingStrength * internodeInfo.AccumulatedGravity;
	ApplyTropism(glm::vec3(0, -1, 0), gravityBending, front, up);
	newGlobalRotation = glm::quatLookAt(front, up);
	actualLocalRotation = glm::inverse(rotation) * glm::inverse(internodeInfo.ParentRotation) * newGlobalRotation;
#pragma endregion

	internodeInfo.LocalTransform = glm::translate(glm::mat4(1.0f), actualLocalRotation * glm::vec3(0, 0, -1)
		* internodeInfo.DistanceToParent) * glm::mat4_cast(actualLocalRotation)
		* glm::scale(glm::vec3(1.0f));

	internodeInfo.GlobalTransform = parentLTW * internodeInfo.LocalTransform;



	float mainChildThickness = 0;
	InternodeInfo mainChildInfo;
	unsigned mainChildEntityIndex = 0;
	EntityManager::ForEachChild(internode, [this, &mainChildInfo, &mainChildEntityIndex, &mainChildThickness, &treeParameters, &internodeInfo, &treeLTW](Entity child)
		{
			UpdateLocalTransform(child, treeParameters, internodeInfo.GlobalTransform, treeLTW);
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
	if (EntityManager::GetChildrenAmount(internode) == 0) internodeInfo.MainChildRotation = newGlobalRotation;
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
void TreeUtilities::PlantSimulationSystem::UpdateInternodeResource(Entity& internode, TreeParameters& treeParameters, TreeAge& treeAge, glm::mat4& treeTransform, std::vector<glm::mat4>& leafTransforms, bool isLeft)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	Illumination internodeIllumination = EntityManager::GetComponentData<Illumination>(internode);
	internodeInfo.AccumulatedLight = internodeIllumination.Value;
	internodeInfo.AccumulatedLength = internodeInfo.DistanceToParent;
	internodeInfo.AccumulatedActivatedBudsAmount = internodeInfo.ActivatedBudsAmount;

	auto internodeData = EntityManager::GetSharedComponent<InternodeData>(internode);
	internodeData->LeafLocalTransforms.clear();
	if (internodeIllumination.Value > treeParameters.LeafIlluminationLimit) {
		if (glm::linearRand(0.0f, 1.0f) >= internodeInfo.Inhibitor * treeParameters.LeafInhibitorFactor)
		{
			glm::vec3 translation;
			glm::quat rotation;
			glm::vec3 scale;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose(treeTransform * internodeInfo.GlobalTransform, scale, rotation, translation, skew, perspective);
			//x, �����ᣬy: ���ᣬz��roll
			glm::vec3 ls = glm::vec3(treeParameters.LeafSize.x, 1.0f, treeParameters.LeafSize.y);
			auto branchFront = rotation * glm::vec3(0, 0, -1);
			auto branchUp = rotation * glm::vec3(0, 1, 0);
			if (treeParameters.IsBothSide || isLeft)
			{
				for (int i = 0; i < treeParameters.SideLeafAmount; i++)
				{
					auto front = glm::rotate(branchFront,
						glm::radians(treeParameters.StartBendingAngle + i * treeParameters.BendingAngleIncrement), branchUp);
					auto up = branchUp;
					ApplyTropism(internodeIllumination.LightDir, treeParameters.LeafPhotoTropism, up, front);
					ApplyTropism(glm::vec3(0, -1, 0), treeParameters.LeafGravitropism, front, up);
					auto localPosition = treeParameters.LeafDistance * front;
					auto leafTransform = glm::translate(glm::mat4(1.0f), localPosition + translation) * glm::mat4_cast(glm::quatLookAt(front, up)) * glm::scale(ls);
					internodeData->LeafLocalTransforms.push_back(leafTransform);
					leafTransforms.push_back(leafTransform);
				}
			}
			if (treeParameters.IsBothSide || !isLeft)
			{
				for (int i = 0; i < treeParameters.SideLeafAmount; i++)
				{
					for (int i = 0; i < treeParameters.SideLeafAmount; i++)
					{
						auto front = glm::rotate(branchFront,
							glm::radians(-(treeParameters.StartBendingAngle + i * treeParameters.BendingAngleIncrement)), branchUp);
						auto up = branchUp;
						ApplyTropism(internodeIllumination.LightDir, treeParameters.LeafPhotoTropism, up, front);
						ApplyTropism(glm::vec3(0, -1, 0), treeParameters.LeafGravitropism, front, up);
						auto localPosition = treeParameters.LeafDistance * front;
						auto leafTransform = glm::translate(glm::mat4(1.0f), localPosition + translation) * glm::mat4_cast(glm::quatLookAt(front, up)) * glm::scale(ls);
						internodeData->LeafLocalTransforms.push_back(leafTransform);
						leafTransforms.push_back(leafTransform);
					}
				}
			}
			if(internodeInfo.Order == internodeInfo.MaxChildOrder)
			{
				auto front = branchFront;
				auto up = branchUp;
				ApplyTropism(internodeIllumination.LightDir, treeParameters.LeafPhotoTropism, up, front);
				ApplyTropism(glm::vec3(0, -1, 0), treeParameters.LeafGravitropism, front, up);
				auto localPosition = treeParameters.LeafDistance * front;
				auto leafTransform = glm::translate(glm::mat4(1.0f), localPosition + translation) * glm::mat4_cast(glm::quatLookAt(front, up)) * glm::scale(ls);
				internodeData->LeafLocalTransforms.push_back(leafTransform);
				leafTransforms.push_back(leafTransform);
			}
		}
	}

	EntityManager::ForEachChild(internode, [this, &internodeInfo, &treeParameters, &treeAge, &leafTransforms, &treeTransform, isLeft](Entity child)
		{
			UpdateInternodeResource(child, treeParameters, treeAge, treeTransform, leafTransforms, !isLeft);
			InternodeInfo childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			internodeInfo.AccumulatedLight += childNodeInfo.AccumulatedLight;
			internodeInfo.AccumulatedLength += childNodeInfo.AccumulatedLength;
			if (childNodeInfo.Pruned) return;
			internodeInfo.AccumulatedActivatedBudsAmount += childNodeInfo.AccumulatedActivatedBudsAmount;
		}
	);


	EntityManager::SetComponentData(internode, internodeInfo);
}
#pragma endregion



void PlantSimulationSystem::BuildHullForTree(Entity& tree)
{
	std::vector<LocalToWorld> internodeLTWs;
	std::vector<InternodeInfo> internodeInfos;
	const auto treeIndex = EntityManager::GetComponentData<TreeIndex>(tree);
	_InternodeQuery.ToComponentDataArray(treeIndex, internodeLTWs);
	_InternodeQuery.ToComponentDataArray(treeIndex, internodeInfos);
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	for (size_t i = 0; i < internodeLTWs.size(); i++)
	{
		if (internodeInfos[i].LongestDistanceToEnd == 0)
		{
			const auto transform = internodeInfos[i].GlobalTransform;
			glm::vec3 translation;
			glm::quat rotation;
			glm::vec3 scale;
			glm::vec3 skew;
			glm::vec4 perspective;
			glm::decompose(transform, scale, rotation, translation, skew, perspective);
			auto front = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
			positions.push_back(translation);
			normals.push_back(front);
		}
	}

	auto treeInfo = EntityManager::GetSharedComponent<TreeData>(tree);
	treeInfo->ConvexHull = std::make_shared<Mesh>();
	CrownSurfaceRecon psr;
	//psr.PoissonConstruct(positions, normals, treeInfo->ConvexHull);
	//psr.AdvancingFrontConstruct(positions, treeInfo->ConvexHull);
	psr.ScaleSpaceConstruct(positions, treeInfo->ConvexHull);


	/*
	quickhull::QuickHull<float> qh; // Could be double as well
	std::vector<quickhull::Vector3<float>> pointCloud;
	for (size_t i = 0; i < internodeLTWs.size(); i++)
	{
		if(internodeInfos[i].LongestDistanceToEnd == 0)
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
	*/
}

void PlantSimulationSystem::RefreshTrees()
{
	auto trees = std::vector<Entity>();
	_TreeQuery.ToEntityArray(trees);
	TreeManager::CalculateInternodeIllumination();
	for (auto& treeEntity : trees)
	{
		auto rootInternode = EntityManager::GetChildren(treeEntity).at(0);
		auto treeInfo = EntityManager::GetComponentData<TreeInfo>(treeEntity);
		auto treeData = EntityManager::GetSharedComponent<TreeData>(treeEntity);
		auto treeAge = EntityManager::GetComponentData<TreeAge>(treeEntity);
		auto treeParameters = EntityManager::GetComponentData<TreeParameters>(treeEntity);
		auto treeLocalToWorld = EntityManager::GetComponentData<LocalToWorld>(treeEntity);
		auto immc = EntityManager::GetSharedComponent<InstancedMeshRenderer>(treeEntity);
		immc->Matrices.clear();
		UpdateInternodeResource(rootInternode, treeParameters, treeAge, treeLocalToWorld.Value, immc->Matrices, true);
		immc->RecalculateBoundingBox();
		EvaluatePruning(rootInternode, treeParameters, treeAge, treeInfo);
		EvaluateRemoval(rootInternode, treeParameters);
		if (_EnableDirectionPruning) EvaluateDirectionPruning(rootInternode, glm::normalize(glm::vec3(treeLocalToWorld.Value[3])), _DirectionPruningLimitAngle);
	}
	CalculateCrownShyness(10.0f);
}

void TreeUtilities::PlantSimulationSystem::OnCreate()
{
	_InternodeSystem = TreeManager::GetInternodeSystem();
	_getcwd(_CurrentWorkingDir, 256);
	_TreeQuery = TreeManager::GetTreeQuery();
	_InternodeQuery = TreeManager::GetInternodeQuery();
	_Gravity = 0;
	for (int i = 0; i < 256; i++) {
		_TempImportFilePath[i] = 0;
	}

	_DefaultConvexHullSurfaceMaterial = std::make_shared<Material>();
	_DefaultConvexHullSurfaceMaterial->SetMaterialProperty("material.shininess", 32.0f);
	_DefaultConvexHullSurfaceMaterial->SetProgram(Default::GLPrograms::StandardProgram);
	_DefaultConvexHullSurfaceMaterial->SetTexture(Default::Textures::StandardTexture, TextureType::DIFFUSE);

	_DefaultTreeSurfaceTex1 = AssetManager::LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Bark_Pine_baseColor.jpg"));
	_DefaultTreeSurfaceNTex1 = AssetManager::LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Bark_Pine_normal.jpg"));
	_DefaultTreeSurfaceSTex1 = AssetManager::LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Aspen_bark_001_SPEC.jpg"));
	_DefaultTreeSurfaceTex2 = AssetManager::LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Aspen_bark_001_COLOR.jpg"));
	_DefaultTreeSurfaceNTex2 = AssetManager::LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Aspen_bark_001_NORM.jpg"));

	_DefaultTreeLeafMaterial1 = std::make_shared<Material>();
	_DefaultTreeLeafMaterial1->SetMaterialProperty("material.shininess", 32.0f);
	_DefaultTreeLeafMaterial1->SetProgram(Default::GLPrograms::StandardInstancedProgram);
	auto textureDiffuseLeaf1 = AssetManager::LoadTexture(FileIO::GetResourcePath("Textures/Leaf/PrunusAvium/A/level0.png"));
	//textureDiffuseLeaf1->LoadTexture(FileIO::GetResourcePath("Textures/green.png"), "");
	auto textureNormalLeaf1 = AssetManager::LoadTexture(FileIO::GetResourcePath("Textures/BarkMaterial/Aspen_bark_001_NORM.jpg"));
	_DefaultTreeLeafMaterial1->SetTexture(textureDiffuseLeaf1, TextureType::DIFFUSE);
	//_DefaultTreeLeafMaterial1->Textures2Ds()->push_back(textureNormalLeaf1, TextureType::NORMAL);

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
				RenderManager::DrawMesh(data->ConvexHull.get(), _DefaultConvexHullSurfaceMaterial.get(), ltws[i].Value, Application::GetMainCameraComponent()->get()->Value.get(), false);
			}
		}

	}
	if (_InternodeSystem->GetConfigFlags() & InternodeSystem_DrawInternodes || _InternodeSystem->GetConfigFlags() & InternodeSystem_DrawConnections)
	{
		std::vector<Entity> trees;
		_TreeQuery.ToEntityArray(trees);
		for (size_t i = 0; i < trees.size(); i++)
		{
			ApplyLocalTransform(trees[i]);
		}
		if (_InternodeSystem->GetConfigFlags() & InternodeSystem_DrawConnections)
		{
			_InternodeSystem->RefreshConnections();
		}
	}
	DrawGui();
}
void PlantSimulationSystem::CreateDefaultTree()
{
	TreeParameters tps;
	LoadDefaultTreeParameters(1, tps);
	CreateTree(_DefaultTreeLeafMaterial1, _DefaultTreeLeafMesh, tps, glm::vec3(0.0f), true);
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
		tps.ThicknessControlFactor = 0.65f;
		tps.SaggingForceBackPropagateFixedCoefficient = 0.5f;

		tps.CrownShynessBase = 1.0f;
		tps.CrownShynessFactor = 1.0f;
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
		tps.CrownShynessBase = 1.0f;
		tps.CrownShynessFactor = 1.0f;
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
		tps.CrownShynessBase = 1.0f;
		tps.CrownShynessFactor = 1.0f;
		break;
	case 4:
		//F6b
		tps.Seed = 1;
		tps.VarianceApicalAngle = 20;
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
		tps.CrownShynessBase = 1.0f;
		tps.CrownShynessFactor = 1.0f;
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
		tps.CrownShynessBase = 1.0f;
		tps.CrownShynessFactor = 1.0f;
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
		tps.CrownShynessBase = 1.0f;
		tps.CrownShynessFactor = 1.0f;
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
		tps.CrownShynessBase = 1.0f;
		tps.CrownShynessFactor = 1.0f;
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
		tps.GrowthRate = 2.2f;
		tps.InternodeLengthBase = 0.61f;
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
		tps.CrownShynessBase = 1.0f;
		tps.CrownShynessFactor = 1.0f;
		break;
	}
}
#pragma endregion
inline void TreeUtilities::PlantSimulationSystem::DrawGui()
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
						if (ImGui::BeginMenu("Examples"))
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
					ImGui::DragInt("Seed", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].Seed);

					ImGui::DragInt("Lateral Bud Number", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].LateralBudPerNode);

					ImGui::DragFloat("Apical Angle Var", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].VarianceApicalAngle, 0.01f);

					ImGui::DragFloat2("Branching Angle M/Var", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].BranchingAngleMean, 0.01f);

					ImGui::DragFloat2("Roll Angle M/Var", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].RollAngleMean, 0.01f);

					ImGui::DragFloat2("Extinction Prob A/L", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].ApicalBudKillProbability, 0.01f);

					ImGui::DragFloat3("AD Base/Dis/Age", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].ApicalDominanceBase, 0.01f);

					ImGui::DragFloat("Growth Rate", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].GrowthRate, 0.01f);

					ImGui::DragFloat2("Node Len Base/Age", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].InternodeLengthBase, 0.01f);

					ImGui::DragFloat4("AC Base/Age/Lvl/Dist", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].ApicalControlBase, 0.01f);

					ImGui::DragInt("Max Bud Age", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].MaxBudAge);

					ImGui::DragFloat("Phototropism", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].Phototropism, 0.01f);

					ImGui::DragFloat2("Gravitropism Base/Dist", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].GravitropismBase, 0.01f);

					ImGui::DragFloat("PruningFactor", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].PruningFactor, 0.01f);

					ImGui::DragFloat("LowBranchPruningFactor", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].LowBranchPruningFactor, 0.01f);

					ImGui::DragFloat("RemovalFactor Thickness", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].ThicknessRemovalFactor, 0.01f);

					ImGui::DragFloat("GravityBendingStrength", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].GravityBendingStrength, 0.01f);

					ImGui::DragFloat2("Lighting Factor A/L", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].ApicalBudLightingFactor, 0.01f);

					ImGui::DragFloat2("Gravity Base/BPCo", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].SaggingFactor, 0.01f);

					ImGui::DragFloat2("Thickness End/Fac", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].EndNodeThickness, 0.01f);

					ImGui::DragFloat2("Crown Shyness Base/Factor", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].CrownShynessBase, 0.01f);

					ImGui::DragFloat2("Leaf Size XY", (float*)(void*)&_NewTreeParameters[_CurrentFocusedNewTreeIndex].LeafSize, 0.01f);

					ImGui::DragFloat("LeafIlluminationLimit", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].LeafIlluminationLimit, 0.01f);

					ImGui::DragFloat("LeafInhibitorFactor", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].LeafInhibitorFactor, 0.01f);

					ImGui::Checkbox("IsBothSide", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].IsBothSide);

					ImGui::DragInt("SideLeafAmount", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].SideLeafAmount, 0.01f);

					ImGui::DragFloat("StartBendingAngle", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].StartBendingAngle, 0.01f);

					ImGui::DragFloat("BendingAngleIncrement", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].BendingAngleIncrement, 0.01f);

					ImGui::DragFloat("LeafPhotoTropism", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].LeafPhotoTropism, 0.01f);

					ImGui::DragFloat("LeafGravitropism", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].LeafGravitropism, 0.01f);
					
					ImGui::DragFloat("LeafDistance", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].LeafDistance, 0.01f);
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
						CreateTree(_DefaultTreeLeafMaterial1, _DefaultTreeLeafMesh, _NewTreeParameters[i], _NewTreePositions[i], true);
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
		if (ImGui::Button("Refresh all trees"))
		{
			RefreshTrees();
		}
		ImGui::InputFloat("Limit angle", &_DirectionPruningLimitAngle, 0.0f, 0.0f, "%.1f");
		ImGui::Checkbox("Enable direction pruning", &_EnableDirectionPruning);
		if (ImGui::DragFloat("Gravity", &_Gravity, 0.01f, 0.0f, 10.0f))
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
				BuildHullForTree(tree);
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