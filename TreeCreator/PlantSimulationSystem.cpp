#include "PlantSimulationSystem.h"
#include "TreeScene.h"

#include "quickhull/QuickHull.hpp"
#include <gtx/vector_angle.hpp>
#include <gtx/matrix_decompose.hpp>
#include <direct.h>

#include "pugixml/pugixml.hpp"


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
		if(_Growing)
		{
			_Growing = false;
		}
		
	}else
	{
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
			UpdateInternodeResource(rootInternode, treeParameters, treeAge, immc->Matrices);
			immc->RecalculateBoundingBox();
			EvaluatePruning(rootInternode, treeParameters, treeAge, treeInfo);
			EvaluateRemoval(rootInternode, treeParameters);
			if (_EnableDirectionPruning) EvaluateDirectionPruning(rootInternode, glm::normalize(glm::vec3(treeLocalToWorld.Value[3])), _DirectionPruningLimitAngle);
		}
		CalculateCrownShyness();
		TreeManager::GetInternodeSystem()->RefreshConnections();
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
	const bool growed = GrowShoots(rootInternode, treeData, treeAge, treeParameters, treeIndex);
	if (growed) {
		UpdateDistanceToBranchEnd(rootInternode, treeParameters);
		InternodeInfo tempBNInfo = EntityManager::GetComponentData<InternodeInfo>(rootInternode);
		tempBNInfo.Level = 0;
		tempBNInfo.MaxActivatedChildLevel = 0;
		tempBNInfo.DistanceToBranchStart = 0;
		EntityManager::SetComponentData(rootInternode, tempBNInfo);
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
TreeParameters PlantSimulationSystem::ImportTreeParameters(const std::string& path)
{
	TreeParameters ret;
	std::ifstream ifs;
	ifs.open((path + ".tps").c_str());
	if (ifs.is_open())
	{
		std::string temp;
		TreeParameterImportHelper(ifs, ret);
	}
	else
	{
		Debug::Error("Can't open file!");
	}
	return ret;
}
void PlantSimulationSystem::ExportTreeParameters(const std::string& path, TreeParameters& treeParameters)
{
	std::ofstream ofs;
	ofs.open((path + ".tps").c_str(), std::ofstream::out | std::ofstream::trunc);
	if (ofs.is_open())
	{
		TreeParameterExportHelper(ofs, treeParameters);
		ofs.close();
		Debug::Log("Tree parameters saved: " + path + ".tps");
	}
	else
	{
		Debug::Error("Can't open file!");
	}
}
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
	for(const auto& instanceNode : doc.child("TreeSettings"))
	{
		const auto& positionNode = instanceNode.child("Position");
		_NewTreePositions[i].x = std::atof(positionNode.child("x").first_child().value());
		_NewTreePositions[i].y = std::atof(positionNode.child("y").first_child().value());
		_NewTreePositions[i].z = std::atof(positionNode.child("z").first_child().value());
		const auto& treeParametersNode = instanceNode.child("TreeParameters");
		for(const auto& parameterNode : treeParametersNode.children())
		{
			std::string name = parameterNode.name();
			if(name.compare("Seed") == 0)
			{
				_NewTreeParameters[i].Seed = std::atoi(parameterNode.first_child().value());
			}else if(name.compare("LateralBudPerNode") == 0)
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
		}
		i++;
	}
}
void PlantSimulationSystem::TreeParameterImportHelper(std::ifstream& ifs, TreeParameters& treeParameters)
{
	std::string temp;
	ifs >> temp; ifs >> treeParameters.Seed;
#pragma region Geometric
	ifs >> temp; ifs >> treeParameters.LateralBudPerNode;

	ifs >> temp; ifs >> treeParameters.VarianceApicalAngle; // Training target

	ifs >> temp; ifs >> treeParameters.BranchingAngleMean; // Training target
	ifs >> temp; ifs >> treeParameters.BranchingAngleVariance; // Training target

	ifs >> temp; ifs >> treeParameters.RollAngleMean; // Training target
	ifs >> temp; ifs >> treeParameters.RollAngleVariance; // Training target
#pragma endregion
#pragma region Bud fate
	ifs >> temp; ifs >> treeParameters.ApicalBudKillProbability; // Useless.
	ifs >> temp; ifs >> treeParameters.LateralBudKillProbability; //Useless.

	ifs >> temp; ifs >> treeParameters.ApicalDominanceBase;
	ifs >> temp; ifs >> treeParameters.ApicalDominanceDistanceFactor; // Training target
	ifs >> temp; ifs >> treeParameters.ApicalDominanceAgeFactor; // Training target

	ifs >> temp; ifs >> treeParameters.GrowthRate;

	ifs >> temp; ifs >> treeParameters.InternodeLengthBase; //Fixed
	ifs >> temp; ifs >> treeParameters.InternodeLengthAgeFactor; // Training target


	ifs >> temp; ifs >> treeParameters.ApicalControlBase; // Training target
	ifs >> temp; ifs >> treeParameters.ApicalControlAgeFactor; // Training target
	ifs >> temp; ifs >> treeParameters.ApicalControlLevelFactor; // Training target
	ifs >> temp; ifs >> treeParameters.ApicalControlDistanceFactor; // Training target

	ifs >> temp; ifs >> treeParameters.MaxBudAge;
#pragma endregion
#pragma region Environmental
	ifs >> temp; ifs >> treeParameters.Phototropism; // Based on tree leaf properties.
	ifs >> temp; ifs >> treeParameters.GravitropismBase; //Based on tree material properties.
	ifs >> temp; ifs >> treeParameters.GravitropismLevelFactor;  //Based on tree material properties.

	ifs >> temp; ifs >> treeParameters.PruningFactor;
	ifs >> temp; ifs >> treeParameters.LowBranchPruningFactor;

	ifs >> temp; ifs >> treeParameters.ThicknessRemovalFactor;

	ifs >> temp; ifs >> treeParameters.GravityBendingStrength;

	ifs >> temp; ifs >> treeParameters.ApicalBudLightingFactor;
	ifs >> temp; ifs >> treeParameters.LateralBudLightingFactor;
#pragma endregion

#pragma region Sagging
	ifs >> temp; ifs >> treeParameters.SaggingFactor;
	ifs >> temp; ifs >> treeParameters.SaggingForceBackPropagateFixedCoefficient;
#pragma endregion
	ifs >> temp; ifs >> treeParameters.EndNodeThickness;
	ifs >> temp; ifs >> treeParameters.ThicknessControlFactor;

	ifs >> temp; ifs >> treeParameters.CrownShynessBase;
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
	output += "\t\t\t<InternodeNodeLengthBase>";  output += std::to_string(treeParameters.InternodeLengthBase) + "</InternodeLengthBase>\n";
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

	output += "\t\t</TreeParameters>\n";
	ofs.write(output.c_str(), output.size());
	ofs.flush();
}
#pragma endregion
#pragma region Growth
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
	UpdateLocalTransform(internode, treeParameters, transform, r.Value);

	EntityManager::SetComponentData(internode, internodeInfo);
	EntityManager::SetComponentData(treeEntity, treeParameters);
	EntityManager::SetComponentData(treeEntity, age);
	_Growing = true;
	return treeEntity;
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
					newApicalBud.EulerAngles = glm::vec3(glm::gaussRand(glm::vec2(0.0f), glm::vec2(glm::radians(treeParameters.VarianceApicalAngle))), 0.0f);
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
inline float TreeUtilities::PlantSimulationSystem::GetApicalControl(std::shared_ptr<TreeData>& treeInfo, InternodeInfo& internodeInfo, TreeParameters& treeParameters, TreeAge& treeAge, int level) const
{
	float apicalControl = treeParameters.ApicalControlBase * glm::pow(treeParameters.ApicalControlAgeFactor, treeAge.Value);
	if (treeInfo->ApicalControlTimeVal.at(treeAge.Value) < 1.0f) {
		const int reversedLevel = internodeInfo.MaxActivatedChildLevel - level + 1;
		return treeInfo->ApicalControlTimeLevelVal.at(treeAge.Value)[reversedLevel];
	}
	return treeInfo->ApicalControlTimeLevelVal.at(treeAge.Value)[level];
}
void TreeUtilities::PlantSimulationSystem::DeactivateBud(InternodeInfo& internodeInfo, Bud& bud)
{
	internodeInfo.ActivatedBudsAmount--;
	bud.IsActive = false;
	if (bud.IsApical) internodeInfo.ApicalBudExist = false;
}
#pragma endregion
#pragma region PostProcessing
#pragma region Pruning
void PlantSimulationSystem::CalculateCrownShyness()
{
	std::vector<LocalToWorld> internodesLTWs;
	std::vector<TreeIndex> internodesTreeIndices;
	std::vector<TreeIndex> treeIndices;
	std::vector<TreeParameters> treeParameters;
	_InternodeQuery.ToComponentDataArray(internodesLTWs);
	_InternodeQuery.ToComponentDataArray(internodesTreeIndices);

	_TreeQuery.ToComponentDataArray(treeIndices);
	_TreeQuery.ToComponentDataArray(treeParameters);
	EntityManager::ForEach<LocalToWorld, InternodeInfo, TreeIndex>(_InternodeQuery, [&treeIndices, &treeParameters, &internodesLTWs, &internodesTreeIndices, this](int i, Entity branchNode, LocalToWorld* ltw, InternodeInfo* info, TreeIndex* index)
		{
			if (info->Pruned) return;
			if (!info->IsActivatedEndNode) return;
			for(size_t ii = 0; ii < treeIndices.size(); ii++)
			{
				if(treeIndices[ii].Value == index->Value)
				{
					info->CrownShyness = treeParameters[ii].CrownShynessBase;
					break;
				}
			}
			if (info->CrownShyness <= 0) return;
			for (size_t bi = 0; bi < internodesLTWs.size(); bi++)
			{
				if (internodesTreeIndices[bi].Value != index->Value)
				{
					auto position1 = glm::vec3(ltw->Value[3].x, ltw->Value[3].y, ltw->Value[3].z);
					auto position2 = glm::vec3(internodesLTWs[bi].Value[3].x, internodesLTWs[bi].Value[3].y, internodesLTWs[bi].Value[3].z);

					if (glm::distance(position1, position2) < info->CrownShyness)
					{
						info->Pruned = true;
						info->PruneReason = 4;
						info->IsActivatedEndNode = false;
					}
				}
			}
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
	if (internodeInfo.Level == 0 && treeAge.Value < 3) return;
	if (internodeInfo.Level == 1 && !internodeInfo.IsApical) {
		float height = EntityManager::GetComponentData<LocalToWorld>(internode).Value[3].y;
		if (height < treeParameters.LowBranchPruningFactor && height < treeInfo.Height / 2) {
			PruneInternode(internode, &internodeInfo, 0);
			return;
		}
	}
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
	if (glm::angle(escapeDirection, internodeInfo.DesiredGlobalRotation * glm::vec3(0.0f, 0.0f, -1.0f)) < glm::radians(limitAngle))
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
void PlantSimulationSystem::EvaluateRemoval(Entity& internode, TreeParameters& treeParameters)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	EntityManager::ForEachChild(internode, [&internodeInfo, this, &treeParameters](Entity child)
		{
			InternodeInfo childInternodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			if (internodeInfo.Thickness - childInternodeInfo.Thickness > treeParameters.ThicknessRemovalFactor)
			{
				EntityManager::DeleteEntity(child);
			}
			EvaluateRemoval(child, treeParameters);
		}
	);
}
#pragma endregion
#pragma region Physics
void TreeUtilities::PlantSimulationSystem::CalculatePhysics(Entity tree)
{
		Rotation rotation = EntityManager::GetComponentData<Rotation>(tree);
		TreeParameters treeParameters = EntityManager::GetComponentData<TreeParameters>(tree);
		if (EntityManager::GetChildrenAmount(tree) == 0) return;
		Entity rootInternode = EntityManager::GetChildren(tree).at(0);
		CalculateDirectGravityForce(tree, _Gravity);
		BackPropagateForce(rootInternode, treeParameters.SaggingForceBackPropagateFixedCoefficient);
		glm::mat4 transform = glm::identity<glm::mat4>(); //glm::translate(glm::mat4(1.0f), glm::vec3(0.0f))* glm::mat4_cast(glm::quatLookAt(glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)))* glm::scale(glm::vec3(1.0f));
		UpdateLocalTransform(rootInternode, treeParameters, transform, rotation.Value);
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
void TreeUtilities::PlantSimulationSystem::UpdateDistanceToBranchEnd(Entity& internode, TreeParameters& treeParameters)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	internodeInfo.DistanceToBranchEnd = 0;
	internodeInfo.TotalDistanceToBranchEnd = 0;
	internodeInfo.BranchEndInternodeAmount = EntityManager::GetChildrenAmount(internode) == 0 ? 1 : 0;

	if (EntityManager::GetChildrenAmount(internode) == 0) internodeInfo.Thickness = treeParameters.EndNodeThickness;
	else {
		internodeInfo.Thickness = 0;
	}
	internodeInfo.MaxChildLevel = internodeInfo.Level;
	float mainChildThickness = 0.0f;
	EntityManager::ForEachChild(internode, [this, &internodeInfo, &mainChildThickness, &treeParameters](Entity child)
		{
			UpdateDistanceToBranchEnd(child, treeParameters);
			const InternodeInfo childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			float d = childNodeInfo.DistanceToBranchEnd + childNodeInfo.DistanceToParent;
			internodeInfo.TotalDistanceToBranchEnd += childNodeInfo.DistanceToParent + childNodeInfo.TotalDistanceToBranchEnd;
			if (d > internodeInfo.DistanceToBranchEnd) internodeInfo.DistanceToBranchEnd = d;

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
			UpdateDistanceToBranchStart(child);
			childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);
			if (childNodeInfo.MaxActivatedChildLevel > internodeInfo.MaxActivatedChildLevel) internodeInfo.MaxActivatedChildLevel = childNodeInfo.MaxActivatedChildLevel;
		}
	);
	EntityManager::SetComponentData(internode, internodeInfo);
}
void TreeUtilities::PlantSimulationSystem::UpdateLocalTransform(Entity& internode, TreeParameters& treeParameters, glm::mat4& parentLTW, glm::quat& treeRotation)
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
	float currentAngle = glm::abs(glm::angle(front, glm::vec3(0, -1, 0)) - glm::radians(90.0f));
	float gravityBending = treeParameters.GravityBendingStrength * internodeInfo.AccumulatedGravity;

	//glm::vec3 left = glm::cross(front, glm::vec3(0, 1, 0));
	//front = glm::rotate(front, gravityBending * currentAngle / glm::radians(90.0f), left);
	front += gravityBending * currentAngle / glm::radians(90.0f) * glm::vec3(0, -1, 0);
	front = glm::normalize(front);
	up = glm::cross(glm::cross(front, up), front);
	newGlobalRotation = glm::quatLookAt(front, up);
	actualLocalRotation = glm::inverse(internodeInfo.ParentRotation) * glm::inverse(treeRotation) * newGlobalRotation;
#pragma endregion

	internodeInfo.LocalTransform = glm::translate(glm::mat4(1.0f), actualLocalRotation * glm::vec3(0, 0, -1)
		* internodeInfo.DistanceToParent) * glm::mat4_cast(actualLocalRotation)
		* glm::scale(glm::vec3(1.0f));

	internodeInfo.GlobalTransform = parentLTW * internodeInfo.LocalTransform;

	

	float mainChildThickness = 0;
	InternodeInfo mainChildInfo;
	unsigned mainChildEntityIndex = 0;
	EntityManager::ForEachChild(internode, [this, &mainChildInfo, &mainChildEntityIndex, &mainChildThickness, &treeParameters, &internodeInfo, &treeRotation](Entity child)
		{
			UpdateLocalTransform(child, treeParameters, internodeInfo.GlobalTransform, treeRotation);
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
void TreeUtilities::PlantSimulationSystem::UpdateInternodeResource(Entity& internode, TreeParameters& treeParameters, TreeAge& treeAge, std::vector<glm::mat4>& leafTransforms)
{
	InternodeInfo internodeInfo = EntityManager::GetComponentData<InternodeInfo>(internode);
	Illumination internodeIllumination = EntityManager::GetComponentData<Illumination>(internode);
	internodeInfo.AccumulatedLight = internodeIllumination.Value;
	internodeInfo.AccumulatedLength = internodeInfo.DistanceToParent;
	internodeInfo.AccumulatedActivatedBudsAmount = internodeInfo.ActivatedBudsAmount;
	
	auto internodeData = EntityManager::GetSharedComponent<InternodeData>(internode);
	internodeData->LeafLocalTransforms.clear();
	float illumination = EntityManager::GetComponentData<Illumination>(internode).Value;
	if (illumination > 0.04f) {
		if (internodeInfo.Level > internodeInfo.MaxChildLevel - 6)
		{
			glm::vec3 lp = glm::vec3(0.0f);
			//x, �����ᣬy: ���ᣬz��roll
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

	EntityManager::ForEachChild(internode, [this, &internodeInfo, &treeParameters, &treeAge, &leafTransforms](Entity child)
		{
			UpdateInternodeResource(child, treeParameters, treeAge, leafTransforms);
			InternodeInfo childNodeInfo = EntityManager::GetComponentData<InternodeInfo>(child);

			if (internodeInfo.MaxChildLevel < childNodeInfo.MaxChildLevel) {
				internodeInfo.MaxChildLevel = childNodeInfo.MaxChildLevel;
			}
			else if (internodeInfo.MaxChildLevel < childNodeInfo.Level) {
				internodeInfo.MaxChildLevel = childNodeInfo.Level;
			}
			internodeInfo.AccumulatedLight += childNodeInfo.AccumulatedLight;
			internodeInfo.AccumulatedLength += childNodeInfo.AccumulatedLength;
			if (childNodeInfo.Pruned) return;
			internodeInfo.AccumulatedActivatedBudsAmount += childNodeInfo.AccumulatedActivatedBudsAmount;
		}
	);
	

	EntityManager::SetComponentData(internode, internodeInfo);
}
#pragma endregion
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
	DrawGui();
}
void PlantSimulationSystem::CreateDefaultTree()
{
	TreeParameters tps;
	LoadDefaultTreeParameters(1, tps);
	CreateTree(_DefaultTreeSurfaceMaterial1, _DefaultTreeLeafMaterial1, _DefaultTreeLeafMesh, tps, glm::vec3(0.0f), true);
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

					ImGui::InputFloat("Crown Shyness Base", &_NewTreeParameters[_CurrentFocusedNewTreeIndex].CrownShynessBase);
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