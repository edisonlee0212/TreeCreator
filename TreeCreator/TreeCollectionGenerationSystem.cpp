#include "TreeCollectionGenerationSystem.h"
#include "rapidcsv/rapidcsv.h"

#include <filesystem>

using namespace UniEngine;
void TreeCollectionGenerationSystem::OnGui()
{
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Perception Tree Data Generation")) {
			if (ImGui::Button("Load csv...(Stava)"))
			{
				auto result = FileIO::OpenFile("Parameters list (*.csv)\0*.csv\0");
				if (result.has_value())
				{
					const std::string path = result.value();
					if (!path.empty())
					{
						ImportCsv(path);
					}
				}
			}
			if (ImGui::Button("Load csv..."))
			{
				auto result = FileIO::OpenFile("Parameters list (*.csv)\0*.csv\0");
				if (result.has_value())
				{
					const std::string path = result.value();
					if (!path.empty())
					{
						ImportCsv2(path);
					}
				}
			}
			if (ImGui::Button("Load multiple csv..."))
			{
				auto result = FileIO::OpenFile("Parameters list (*.csv)\0*.csv\0");
				if (result.has_value())
				{
					const std::string path = result.value();
                    for (const auto &file : std::filesystem::directory_iterator(std::filesystem::path{ path }.parent_path()))
                    {
						ImportCsv2(file.path().string());
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

	CreationQueueSettings settings{ };

	settings._StorePath = _BaseStorePath + std::filesystem::path(path).stem().string() + "/";
	std::filesystem::create_directory(std::filesystem::path{ settings._StorePath });
	std::filesystem::create_directory(std::filesystem::path{ settings._StorePath + "/graph/" });
	std::filesystem::create_directory(std::filesystem::path{ settings._StorePath + "/image/" });
	std::filesystem::create_directory(std::filesystem::path{ settings._StorePath + "/parameters/" });
	Debug::Log("Storing results in: " + settings._StorePath);

	const std::string checkPath{ settings._StorePath + "/graph/" };
	// Start from 1 because we need to ignore the first row that contains the label.
	int startIndex{ 1 };
	for (const auto &file : std::filesystem::directory_iterator(checkPath))
	{
	    try
	    {
			const auto fileIndex{ std::stoi(std::filesystem::path(file).stem()) };
			startIndex = std::max(startIndex, fileIndex + 1);
	    } catch (std::exception &e)
	    { /* Skip this file */ }
	}

	settings._Counter = startIndex - 1;
	Debug::Log("Starting with index: " + std::to_string(startIndex));

	for(int i = startIndex; i < firstColumn.size(); i++)
	{
		//TODO: Change the parsing here.
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
		//Once you push the parameter set into the queue, the system will try to load it and build the tree and save corresponding data
		settings._CreationQueue.push(treeParameters);
	}

	_QueueSettings.push(std::move(settings));
}

void TreeCollectionGenerationSystem::ImportCsv2(const std::string& path)
{
	rapidcsv::Document doc(path, rapidcsv::LabelParams(-1, -1));
	std::vector<std::string> firstColumn = doc.GetColumn<std::string>(0);
	Debug::Log("Count: " + std::to_string(firstColumn.size() - 1));

	CreationQueueSettings settings{ };

	settings._StorePath = _BaseStorePath + std::filesystem::path(path).stem().string() + "/";
	std::filesystem::create_directory(std::filesystem::path{ settings._StorePath });
	std::filesystem::create_directory(std::filesystem::path{ settings._StorePath + "/graph/" });
	std::filesystem::create_directory(std::filesystem::path{ settings._StorePath + "/image/" });
	std::filesystem::create_directory(std::filesystem::path{ settings._StorePath + "/parameters/" });
	Debug::Log("Storing results in: " + settings._StorePath);

	const std::string checkPath{ settings._StorePath + "/graph/" };
	// Start from 1 because we need to ignore the first row that contains the label.
	int startIndex{ 1 };
	for (const auto &file : std::filesystem::directory_iterator(checkPath))
	{
	    try
	    {
			const auto fileIndex{ std::stoi(std::filesystem::path(file).stem()) };
			startIndex = std::max(startIndex, fileIndex + 1);
	    } catch (std::exception &e)
	    { /* Skip this file */ }
	}

	settings._Counter = startIndex - 1;
	Debug::Log("Starting with index: " + std::to_string(startIndex));

	for (int i = startIndex; i < firstColumn.size(); i++)
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
		settings._CreationQueue.push(treeParameters);
	}

	_QueueSettings.push(std::move(settings));
}

void TreeUtilities::TreeCollectionGenerationSystem::LateUpdate()
{
	if (!_DataCollectionSystem) return;

	if (_QueueSettings.empty())
	{ OnGui(); return; }

    auto &_CreationQueue{ _QueueSettings.front()._CreationQueue };
    auto &_StorePath{ _QueueSettings.front()._StorePath };
    auto &_Counter{ _QueueSettings.front()._Counter };

	if (_CreationQueue.empty() && _Status == TreeCollectionGenerationSystenStatus::Idle)
	{ _QueueSettings.pop(); OnGui(); return; }

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

			glm::vec3 mainLightAngle = glm::vec3(150, -45, 0);
			float lightFocus = 35;
			_DataCollectionSystem->_LightTransform.SetEulerRotation(glm::radians(mainLightAngle));
			_DataCollectionSystem->_LightTransform1.SetEulerRotation(glm::radians(mainLightAngle + glm::vec3(0, -lightFocus, 0)));
			_DataCollectionSystem->_LightTransform2.SetEulerRotation(glm::radians(mainLightAngle + glm::vec3(0, lightFocus, 0)));

			_DataCollectionSystem->_LightTransform2.SetEulerRotation(glm::radians(mainLightAngle + glm::vec3(0, -180, 0)));

			_DataCollectionSystem->_DirectionalLightEntity.SetComponentData(_DataCollectionSystem->_LightTransform);
			_DataCollectionSystem->_DirectionalLightEntity1.SetComponentData(_DataCollectionSystem->_LightTransform1);
			_DataCollectionSystem->_DirectionalLightEntity2.SetComponentData(_DataCollectionSystem->_LightTransform2);
			_DataCollectionSystem->_DirectionalLightEntity3.SetComponentData(_DataCollectionSystem->_LightTransform3);
			
			/*
			_DataCollectionSystem->_CameraPosition = glm::vec3{ 0.0f, 2.0f, 25.0f };
			_DataCollectionSystem->_CameraEulerRotation = glm::vec3{ 0.0f, 0.0f, 0.0f };
            Transform transform;
            transform.SetPosition(_DataCollectionSystem->_CameraPosition);
            transform.SetEulerRotation(_DataCollectionSystem->_CameraEulerRotation);
			_DataCollectionSystem->_ImageCameraEntity.SetComponentData(transform);
			*/

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

			//_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->ResizeResolution(1280, 1280);
			_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->ResizeResolution(1024, 1024);
			
			_Status = TreeCollectionGenerationSystenStatus::Growing;
		}else
		{ OnGui(); }

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
			if (_Internodes.size() > 7500)
			{
				Debug::Log("Reached maximum internode limit: " + std::to_string(_Internodes.size()));
				_Status = TreeCollectionGenerationSystenStatus::Rendering;
				_DataCollectionSystem->_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
			}
		}
		break;
	case TreeCollectionGenerationSystenStatus::Rendering:
	{
        _Internodes.resize(0);
        const auto treeIndex = _DataCollectionSystem->_CurrentTree.GetComponentData<TreeIndex>();
        TreeManager::GetInternodeQuery().ToEntityArray(treeIndex, _Internodes);

		Debug::Log("Internodes: " + std::to_string(_Internodes.size()));

		glm::vec3 minPos{
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::max(),
		};
		glm::vec3 maxPos{
			std::numeric_limits<float>::min(),
			std::numeric_limits<float>::min(),
			std::numeric_limits<float>::min(),
		};

		for (const auto &internode : _Internodes)
		{
			if (!internode.HasComponentData<InternodeInfo>())
			{ continue; }

			const auto& internodeInfo{ internode.GetComponentData<InternodeInfo>() };

            minPos = glm::vec3{
                std::min(minPos.x, internodeInfo.BranchStartPosition.x),
                std::min(minPos.y, internodeInfo.BranchStartPosition.y),
                std::min(minPos.z, internodeInfo.BranchStartPosition.z),
            };
            maxPos = glm::vec3{
                std::max(maxPos.x, internodeInfo.BranchStartPosition.x),
                std::max(maxPos.y, internodeInfo.BranchStartPosition.y),
                std::max(maxPos.z, internodeInfo.BranchStartPosition.z),
            };

            minPos = glm::vec3{
                std::min(minPos.x, internodeInfo.BranchEndPosition.x),
                std::min(minPos.y, internodeInfo.BranchEndPosition.y),
                std::min(minPos.z, internodeInfo.BranchEndPosition.z),
            };
            maxPos = glm::vec3{
                std::max(maxPos.x, internodeInfo.BranchEndPosition.x),
                std::max(maxPos.y, internodeInfo.BranchEndPosition.y),
                std::max(maxPos.z, internodeInfo.BranchEndPosition.z),
            };
		}

		//Debug::Log("Min: " + std::to_string(minPos.x) + " " + std::to_string(minPos.y) + " " + std::to_string(minPos.z));
		//Debug::Log("Max: " + std::to_string(maxPos.x) + " " + std::to_string(maxPos.y) + " " + std::to_string(maxPos.z));

        const auto size{ maxPos - minPos };
		Debug::Log("Size: " + std::to_string(size.x) + " " + std::to_string(size.y) + " " + std::to_string(size.z));
		const auto maxSize{ std::max(size.x, std::max(size.y, size.z)) };

        glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
        if (size.y < 10.0f && size.y > 0.0f)
        { scale *= 10.0f / size.y; }
        else if (maxSize > 30.0f)
        { scale *= 30.0f / maxSize; }
		GlobalTransform transform{ _DataCollectionSystem->_CurrentTree.GetComponentData<GlobalTransform>() };
		transform.SetScale(scale);
        _DataCollectionSystem->_CurrentTree.SetComponentData<GlobalTransform>(transform);

        TreeManager::GenerateSimpleMeshForTree(_DataCollectionSystem->_CurrentTree, 
			PlantSimulationSystem::_MeshGenerationResolution, 
			PlantSimulationSystem::_MeshGenerationSubdivision);

		_Status = TreeCollectionGenerationSystenStatus::CollectData;
	}
		break;
	case TreeCollectionGenerationSystenStatus::CollectData:
		_DataCollectionSystem->_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
			_StorePath + "image/" +
			std::string(7 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter) + ".jpg"
			, 1024, 1024, true);
		TreeManager::SerializeTreeGraph(_StorePath + "graph/" +
			std::string(7 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
			, _DataCollectionSystem->_CurrentTree, "tree");

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
