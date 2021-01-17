#include "TreeCollectionGenerationSystem.h"
#include "rapidcsv/rapidcsv.h"
using namespace UniEngine;
void TreeCollectionGenerationSystem::OnGui()
{
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Perception Tree Data Generation")) {
			ImGui::DragInt("Resolution", &_CaptureResolution, 1, 1, 2560);
			if (ImGui::Button("Load csv..."))
			{
				auto result = FileIO::OpenFile("Parameters list (*.csv)\0*.csv\0");
				if (result.has_value())
				{
					const std::string path = result.value();
					if (!path.empty())
					{
						ImportCsv2(path);
						_Timer = Application::EngineTime();
						_Generation = true;
					}
				}
			}
			if (ImGui::Button("Load csv (Old version - Stava)..."))
			{
				auto result = FileIO::OpenFile("Parameters list (*.csv)\0*.csv\0");
				if (result.has_value())
				{
					const std::string path = result.value();
					if (!path.empty())
					{
						ImportCsv(path);
						_Timer = Application::EngineTime();
						_Generation = true;
					}
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void TreeCollectionGenerationSystem::ImportCsv(const std::string& path)
{
	rapidcsv::Document doc(path, rapidcsv::LabelParams(-1, -1));
	std::vector<std::string> firstColumn = doc.GetColumn<std::string>(0);
	Debug::Log("Count: " + std::to_string(firstColumn.size() - 1));
	for(int i = 1; i < firstColumn.size(); i++)
	{
		std::vector<float> values = doc.GetRow<float>(i);
		TreeParameters treeParameters;
		treeParameters.Seed = 0;
		treeParameters.BranchingAngleMean = values[0];
		treeParameters.BranchingAngleVariance = values[1];
		treeParameters.VarianceApicalAngle = values[2] + values[3];
		treeParameters.RollAngleMean = values[4];
		treeParameters.RollAngleVariance = values[5];
		treeParameters.InternodeLengthBase = values[6];
		treeParameters.InternodeLengthAgeFactor = values[7];
		treeParameters.Age = values[8];
		treeParameters.Phototropism = values[9];
		treeParameters.GravitropismBase = values[10];
		treeParameters.GravitropismLevelFactor = values[11];
		treeParameters.GravityBendingStrength = values[12];
		treeParameters.GravityBendingAngleFactor = values[13];
		treeParameters.ApicalDominanceBase = values[14];
		treeParameters.ApicalDominanceAgeFactor = values[15];
		treeParameters.ApicalDominanceDistanceFactor = values[16];
		treeParameters.ApicalControlBase = values[17];
		treeParameters.ApicalControlAgeFactor = values[18];
		treeParameters.ApicalControlLevelFactor = values[19];
		treeParameters.PruningFactor = values[21];
		treeParameters.LowBranchPruningFactor = values[22];
		treeParameters.InternodeSize = values[23];
		treeParameters.LateralBudPerNode = values[24];
		treeParameters.LateralBudKillProbability = values[25];
		treeParameters.ApicalBudKillProbability = values[26];
		treeParameters.GrowthRate = values[27];
		treeParameters.ApicalBudLightingFactor = values[28];
		treeParameters.LateralBudLightingFactor = values[29];
		treeParameters.EndNodeThickness = values[30];
		treeParameters.ThicknessControlFactor = values[31];
		_CreationQueue.push(treeParameters);
	}
}

void TreeCollectionGenerationSystem::ImportCsv2(const std::string& path)
{
	rapidcsv::Document doc(path, rapidcsv::LabelParams(-1, -1), rapidcsv::SeparatorParams(';'));
	std::vector<std::string> firstColumn = doc.GetColumn<std::string>(0);
	Debug::Log("Count: " + std::to_string(firstColumn.size() - 1));
	//i start from 1 because we need to ignore the first row that contains the label.
	for (int i = 1; i < firstColumn.size(); i++)
	{
		//TODO: Change the parsing here.
		std::vector<float> values = doc.GetRow<float>(i);
		TreeParameters treeParameters;
		treeParameters.Seed = values[0];
		treeParameters.Age = values[1];

#pragma region Geometric
		treeParameters.LateralBudPerNode = values[2];

		treeParameters.VarianceApicalAngle = values[3];

		treeParameters.BranchingAngleMean = values[4];
		treeParameters.BranchingAngleVariance = values[5];

		treeParameters.RollAngleMean = values[6];
		treeParameters.RollAngleVariance = values[7];
#pragma endregion
#pragma region Bud fate
		treeParameters.ApicalBudKillProbability = values[8];
		treeParameters.LateralBudKillProbability = values[9];

		treeParameters.ApicalDominanceBase = values[10];
		treeParameters.ApicalDominanceDistanceFactor = values[11];
		treeParameters.ApicalDominanceAgeFactor = values[12];

		treeParameters.GrowthRate = values[13];

		treeParameters.InternodeLengthBase = values[14];
		treeParameters.InternodeLengthAgeFactor = values[15];


		treeParameters.ApicalControlBase = values[16];
		treeParameters.ApicalControlAgeFactor = values[17];
		treeParameters.ApicalControlLevelFactor = values[18];

		treeParameters.MaxBudAge = values[20];
#pragma endregion
#pragma region Environmental
		treeParameters.InternodeSize = values[21];

		treeParameters.Phototropism = values[22];
		treeParameters.GravitropismBase = values[23];
		treeParameters.GravitropismLevelFactor = values[24];

		treeParameters.PruningFactor = values[25];
		treeParameters.LowBranchPruningFactor = values[26];

		treeParameters.ThicknessRemovalFactor = values[27];

		treeParameters.GravityBendingStrength = values[28];
		treeParameters.GravityBendingAngleFactor = values[29];

		treeParameters.ApicalBudLightingFactor = values[30];
		treeParameters.LateralBudLightingFactor = values[31];
#pragma endregion

		treeParameters.EndNodeThickness = values[32];
		treeParameters.ThicknessControlFactor = values[33];

#pragma region CrownShyness
		treeParameters.CrownShynessBase = values[34];
		treeParameters.CrownShynessFactor = values[35];
#pragma endregion

#pragma region Organs
		treeParameters.FoliageType = values[36];
#pragma endregion
		//Once you push the parameter set into the queue, the system will try to load it and build the tree and save corresponding data
		_CreationQueue.push(treeParameters);
	}
}


void TreeUtilities::TreeCollectionGenerationSystem::LateUpdate()
{
	if (!_DataCollectionSystem) return;
	switch (_Status)
	{
	case TreeCollectionGenerationSystenStatus::Idle:
		if (!_CreationQueue.empty())
		{
			_CurrentDegrees = 0;
			_DataCollectionSystem->SetCameraPose(_Position, _Rotation);
			auto treeParameters = _CreationQueue.front();
			_CreationQueue.pop();
			RenderManager::SetAmbientLight(0.3f);
			float brightness = 6.0;
			_DataCollectionSystem->_DirectionalLightEntity.GetPrivateComponent<DirectionalLight>()->diffuseBrightness = brightness / 2.0f;
			_DataCollectionSystem->_DirectionalLightEntity.GetPrivateComponent<DirectionalLight>()->lightSize = 0.4f;
			_DataCollectionSystem->_DirectionalLightEntity.GetPrivateComponent<DirectionalLight>()->bias = 0.1f;
			_DataCollectionSystem->_DirectionalLightEntity1.GetPrivateComponent<DirectionalLight>()->diffuseBrightness = 0;
			_DataCollectionSystem->_DirectionalLightEntity2.GetPrivateComponent<DirectionalLight>()->diffuseBrightness = 0;
			_DataCollectionSystem->_DirectionalLightEntity3.GetPrivateComponent<DirectionalLight>()->diffuseBrightness = brightness / 2.0f;
			_DataCollectionSystem->_DirectionalLightEntity.GetPrivateComponent<DirectionalLight>()->diffuse = glm::vec3(1.0f);
			_DataCollectionSystem->_DirectionalLightEntity1.GetPrivateComponent<DirectionalLight>()->diffuse = glm::vec3(1.0f);
			_DataCollectionSystem->_DirectionalLightEntity2.GetPrivateComponent<DirectionalLight>()->diffuse = glm::vec3(1.0f);
			_DataCollectionSystem->_DirectionalLightEntity3.GetPrivateComponent<DirectionalLight>()->diffuse = glm::vec3(1.0f);
			_DataCollectionSystem->_DirectionalLightEntity1.GetPrivateComponent<DirectionalLight>()->CastShadow = false;
			_DataCollectionSystem->_DirectionalLightEntity2.GetPrivateComponent<DirectionalLight>()->CastShadow = false;
			_DataCollectionSystem->_DirectionalLightEntity3.GetPrivateComponent<DirectionalLight>()->CastShadow = false;

			glm::vec3 mainLightAngle = glm::vec3(150, 0, 0);
			float lightFocus = 35;
			_DataCollectionSystem->_LightTransform.SetEulerRotation(glm::radians(mainLightAngle));
			_DataCollectionSystem->_LightTransform1.SetEulerRotation(glm::radians(mainLightAngle + glm::vec3(0, -lightFocus, 0)));
			_DataCollectionSystem->_LightTransform2.SetEulerRotation(glm::radians(mainLightAngle + glm::vec3(0, lightFocus, 0)));

			_DataCollectionSystem->_LightTransform2.SetEulerRotation(glm::radians(mainLightAngle + glm::vec3(0, -180, 0)));

			_DataCollectionSystem->_DirectionalLightEntity.SetComponentData(_DataCollectionSystem->_LightTransform);
			_DataCollectionSystem->_DirectionalLightEntity1.SetComponentData(_DataCollectionSystem->_LightTransform1);
			_DataCollectionSystem->_DirectionalLightEntity2.SetComponentData(_DataCollectionSystem->_LightTransform2);
			_DataCollectionSystem->_DirectionalLightEntity3.SetComponentData(_DataCollectionSystem->_LightTransform3);
			_GroundEntity.SetEnabled(true);
			
			treeParameters.Seed = 0;
			_DataCollectionSystem->_CurrentTree = _DataCollectionSystem->_PlantSimulationSystem->CreateTree(treeParameters, glm::vec3(0.0f));
			_DataCollectionSystem->_PlantSimulationSystem->ResumeGrowth();
			std::ofstream ofs;
			ofs.open((_StorePath + "parameters/" + std::string(7 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter) + ".xml").c_str(), std::ofstream::out | std::ofstream::trunc);
			if (ofs.is_open())
			{
				_DataCollectionSystem->_PlantSimulationSystem->TreeParameterExportHelper(ofs, treeParameters);
			}
			else
			{
				Debug::Error("Can't open file!");
			}

			_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->ResizeResolution(_CaptureResolution, _CaptureResolution);
			_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<PostProcessing>()->SetEnableLayer("GreyScale", true);
			_Status = TreeCollectionGenerationSystenStatus::Growing;
		}else
		{
			if(_Generation)
			{
				_Generation = false;
				const double spentTime = Application::EngineTime() - _Timer;
				Debug::Log("Generation Finished. Used time: " + std::to_string(spentTime));
			}
			OnGui();
		}

		break;
	case TreeCollectionGenerationSystenStatus::Growing:
		_DataCollectionSystem->_PlantSimulationSystem->_AutoGenerateLeaves = false;
		if (!_DataCollectionSystem->_PlantSimulationSystem->_Growing)
		{
			_Status = TreeCollectionGenerationSystenStatus::Rendering;
			_DataCollectionSystem->_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
		}
		else
		{
			_Internodes.resize(0);
			const auto treeIndex = _DataCollectionSystem->_CurrentTree.GetComponentData<TreeIndex>();
			TreeManager::GetInternodeQuery().ToEntityArray(treeIndex, _Internodes);
			if (_Internodes.size() > 10000)
			{
				
				_Status = TreeCollectionGenerationSystenStatus::Rendering;
				_DataCollectionSystem->_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
			}
		}
		break;
	case TreeCollectionGenerationSystenStatus::Rendering:
	{
		_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToPng(
			_StorePath + "image/" +
			std::string(7 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter) + ".jpg"
			, _CaptureResolution, _CaptureResolution, true);
		_Status = TreeCollectionGenerationSystenStatus::CollectData;
	}
		break;
	case TreeCollectionGenerationSystenStatus::CollectData:
		TreeManager::SerializeTreeGraph(_StorePath + "graph/" +
			std::string(7 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
			, _DataCollectionSystem->_CurrentTree);
		_Status = TreeCollectionGenerationSystenStatus::CleanUp;
		break;
	case TreeCollectionGenerationSystenStatus::CleanUp:
		TreeManager::DeleteAllTrees();
		_Counter++;
		_Status = TreeCollectionGenerationSystenStatus::Idle;
		break;
	}
}

void TreeCollectionGenerationSystem::OnCreate()
{
	Enable();
}

void TreeCollectionGenerationSystem::SetDataCollectionSystem(DataCollectionSystem* value)
{
	_DataCollectionSystem = value;
}

void TreeCollectionGenerationSystem::SetGroundEntity(Entity value)
{
	_GroundEntity = value;
}
