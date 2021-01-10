#include "TreeCollectionGenerationSystem.h"
#include "rapidcsv/rapidcsv.h"
void TreeCollectionGenerationSystem::ImportCsv(const std::string& path)
{
	rapidcsv::Document doc(path, rapidcsv::LabelParams(-1, -1));
	std::vector<std::string> types = doc.GetRow<std::string>(0);
	std::vector<std::string> firstColumn = doc.GetColumn<std::string>(0);
	Debug::Log("Rows: " + std::to_string(types.size()));
	Debug::Log("Column: " + std::to_string(firstColumn.size()));
	int instanceAmount = firstColumn.size() - 1;
	for(int i = 1; i < 100; i++)
	{
		std::vector<float> values = doc.GetRow<float>(i);
		TreeParameters treeParameters;
		treeParameters.Seed = 0;
		treeParameters.BranchingAngleMean = values[0];
		treeParameters.BranchingAngleVariance = values[1];
		treeParameters.VarianceApicalAngle = values[2];
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
		treeParameters.ApicalDominanceDistanceFactor = 0.0f;
		treeParameters.PruningFactor = values[21];
		treeParameters.LowBranchPruningFactor = values[22];
		treeParameters.LateralBudPerNode = values[24];
		treeParameters.LateralBudKillProbability = values[25];
		treeParameters.ApicalBudKillProbability = values[26];
		treeParameters.GrowthRate = values[27] * 8.0f;
		treeParameters.ApicalBudLightingFactor = values[28];
		treeParameters.LateralBudLightingFactor = values[29];
		treeParameters.EndNodeThickness = values[30];
		treeParameters.ThicknessControlFactor = values[31];
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
			_DataCollectionSystem->SetCameraPose(_Position, glm::radians(_Rotation));
			auto treeParameters = _CreationQueue.front();
			_CreationQueue.pop();
			RenderManager::SetAmbientLight(0.3f);
			float brightness = glm::linearRand(5.0f, 7.0f);
			_DataCollectionSystem->_DirectionalLightEntity.GetPrivateComponent<DirectionalLight>()->diffuseBrightness = brightness / 2.0f;
			_DataCollectionSystem->_DirectionalLightEntity1.GetPrivateComponent<DirectionalLight>()->diffuseBrightness = brightness / 3.0f;
			_DataCollectionSystem->_DirectionalLightEntity2.GetPrivateComponent<DirectionalLight>()->diffuseBrightness = brightness / 3.0f;
			_DataCollectionSystem->_DirectionalLightEntity3.GetPrivateComponent<DirectionalLight>()->diffuseBrightness = brightness / 8.0f;
			_DataCollectionSystem->_DirectionalLightEntity1.GetPrivateComponent<DirectionalLight>()->CastShadow = false;
			_DataCollectionSystem->_DirectionalLightEntity2.GetPrivateComponent<DirectionalLight>()->CastShadow = false;
			_DataCollectionSystem->_DirectionalLightEntity3.GetPrivateComponent<DirectionalLight>()->CastShadow = false;

			glm::vec3 mainLightAngle = glm::vec3(150 + glm::linearRand(-30, 30), glm::linearRand(0, 360), 0);
			float lightFocus = 35;
			_DataCollectionSystem->_LightTransform.SetEulerRotation(glm::radians(mainLightAngle));
			_DataCollectionSystem->_LightTransform1.SetEulerRotation(glm::radians(mainLightAngle + glm::vec3(0, -lightFocus, 0)));
			_DataCollectionSystem->_LightTransform2.SetEulerRotation(glm::radians(mainLightAngle + glm::vec3(0, lightFocus, 0)));

			_DataCollectionSystem->_LightTransform2.SetEulerRotation(glm::radians(mainLightAngle + glm::vec3(0, -180, 0)));

			_DataCollectionSystem->_DirectionalLightEntity.SetComponentData(_DataCollectionSystem->_LightTransform);
			_DataCollectionSystem->_DirectionalLightEntity1.SetComponentData(_DataCollectionSystem->_LightTransform1);
			_DataCollectionSystem->_DirectionalLightEntity2.SetComponentData(_DataCollectionSystem->_LightTransform2);
			_DataCollectionSystem->_DirectionalLightEntity3.SetComponentData(_DataCollectionSystem->_LightTransform3);

			//_DataCollectionSystem->_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->ResizeResolution(_CaptureResolution, _CaptureResolution);
			//_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->ResizeResolution(_CaptureResolution, _CaptureResolution);

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

			_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->ResizeResolution(1280, 1280);
			
			_Status = TreeCollectionGenerationSystenStatus::Growing;
		}

		break;
	case TreeCollectionGenerationSystenStatus::Growing:
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
		_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
			_StorePath + "image/" +
			std::string(7 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter) + ".jpg"
			, 640, 640);
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
	//Enable();
}

void TreeCollectionGenerationSystem::SetDataCollectionSystem(DataCollectionSystem* value)
{
	_DataCollectionSystem = value;
}
