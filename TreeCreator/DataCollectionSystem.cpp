#include "DataCollectionSystem.h"

#include <direct.h>


#include "AcaciaFoliageGenerator.h"
#include "MapleFoliageGenerator.h"
#include "PineFoliageGenerator.h"
#include "WillowFoliageGenerator.h"
#include "AppleFoliageGenerator.h"
#include "OakFoliageGenerator.h"
#include "BirchFoliageGenerator.h"

#include "MaskProcessor.h"

void DataCollectionSystem::ResetCounter(int value, int startIndex, int endIndex, bool obj, bool graph)
{
	_Counter = value;
	_StartIndex = startIndex;
	_EndIndex = endIndex;
	_Timer = Application::EngineTime();
	_Status = DataCollectionSystemStatus::Idle;
	_CurrentSelectedSequenceIndex = 0;
	_NeedExport = true;
	_ExportOBJ = obj;
	_ExportGraph = graph;
}

void DataCollectionSystem::SetIsTrain(bool value)
{
	_IsTrain = value;
}

auto DataCollectionSystem::IsExport() const -> bool
{
	return _NeedExport;
}

void DataCollectionSystem::PushImageCaptureSequence(ImageCaptureSequence sequence)
{
	_ImageCaptureSequences.emplace_back(sequence, _PlantSimulationSystem->LoadParameters(sequence.ParamPath));
}

void DataCollectionSystem::OnGui()
{
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Learning Data Generation")) {
			if (ImGui::Button("Create new data set...")) {
				ImGui::OpenPopup("Data set wizard");
			}
			const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Data set wizard", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Export options:");
				ImGui::DragInt("Export Resolution", &_TargetResolution, 1, 1, _CaptureResolution);
				ImGui::DragInt("Camera Resolution", &_CaptureResolution, 1, _TargetResolution, 5120);
				ImGui::Checkbox("Generate OBJ", &_ExportOBJ);
				ImGui::Checkbox("Generate graph", &_ExportGraph);

				ImGui::Text("Data set options:");
				ImGui::Checkbox("Batched", &_Batched);
				ImGui::Separator();
				ImGui::Text("Training data");
				if (_Batched) {
					ImGui::DragInt("Start Index##Train", &_StartIndex, 1, 0, _EndIndex);
					ImGui::DragInt("End Index##Train", &_EndIndex, 1, _StartIndex, 5000);
				}
				else
				{
					ImGui::DragInt("Amount##Train", &_EndIndex, 1, 1, 5000);
				}
				ImGui::Separator();
				ImGui::Text("Evaluation data");
				if (_Batched) {
					ImGui::DragInt("Start Index##Eval", &_EvalStartIndex, 1, 0, _EvalEndIndex);
					ImGui::DragInt("End Index##Eval", &_EvalEndIndex, 1, _EvalStartIndex, 5000);
				}
				else
				{
					ImGui::DragInt("Amount##Eval", &_EvalEndIndex, 1, 1, 5000);
				}


				if (ImGui::Button("Start Generation"))
				{
					if (!_Batched)
					{
						_StartIndex = 1;
						_EvalStartIndex = 1;

						std::filesystem::remove_all("tree_data");
						std::filesystem::create_directory("tree_data");
						std::filesystem::create_directory("tree_data/branch_train");
						std::filesystem::create_directory("tree_data/graph_train");
						std::filesystem::create_directory("tree_data/mask_train");
						std::filesystem::create_directory("tree_data/obj_train");
						std::filesystem::create_directory("tree_data/rgb_train");
						std::filesystem::create_directory("tree_data/white_train");

						std::filesystem::create_directory("tree_data/branch_val");
						std::filesystem::create_directory("tree_data/graph_val");
						std::filesystem::create_directory("tree_data/mask_val");
						std::filesystem::create_directory("tree_data/obj_val");
						std::filesystem::create_directory("tree_data/rgb_val");
						std::filesystem::create_directory("tree_data/white_val");
					}
					ResetCounter((_StartIndex - 1) * 7, _StartIndex, _EndIndex, _ExportOBJ, _ExportGraph);
					_NeedEval = true;
					_IsTrain = true;
					_NeedExport = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void DataCollectionSystem::ExportAllData()
{
	ExportParams(_StorePath + "params_" + (_IsTrain ? "train" : "val"));
	ExportKDops(_StorePath + "kdops_" + (_IsTrain ? "train" : "val"));
	ExportCakeTower(_StorePath + "cakes_" + (_IsTrain ? "train" : "val"));
	_TreeParametersOutputList.clear();
	_CakeTowersOutputList.clear();
	_KDopsOutputList.clear();
	const double spentTime = Application::EngineTime() - _Timer;
	Debug::Log("Generation Finished. Used time: " + std::to_string(spentTime));
}

void DataCollectionSystem::ExportParams(const std::string& path) const
{
	std::ofstream ofs;
	ofs.open((path + ".csv").c_str(), std::ofstream::out | (_Batched ? std::ofstream::app : std::ofstream::trunc));
	if (ofs.is_open())
	{
		for (auto& instance : _TreeParametersOutputList) {
			auto treeParameters = instance.Parameters;
			std::string output = "";
			output += std::to_string(instance.Index) + ",";
			output += instance.Name + ",";
			output += std::to_string(treeParameters.Seed) + ",";
#pragma region Geometric
			output += std::to_string(treeParameters.Age) + ",";
			output += std::to_string(treeParameters.LateralBudPerNode) + ",";
			output += std::to_string(treeParameters.VarianceApicalAngle) + ",";
			output += std::to_string(treeParameters.BranchingAngleMean) + ",";
			output += std::to_string(treeParameters.BranchingAngleVariance) + ",";
			output += std::to_string(treeParameters.RollAngleMean) + ",";
			output += std::to_string(treeParameters.RollAngleVariance) + ",";
#pragma endregion
#pragma region Bud fate
			output += std::to_string(treeParameters.ApicalBudKillProbability) + ",";
			output += std::to_string(treeParameters.LateralBudKillProbability) + ",";
			output += std::to_string(treeParameters.ApicalDominanceBase) + ",";
			output += std::to_string(treeParameters.ApicalDominanceDistanceFactor) + ",";
			output += std::to_string(treeParameters.ApicalDominanceAgeFactor) + ",";
			output += std::to_string(treeParameters.GrowthRate) + ",";
			output += std::to_string(treeParameters.InternodeLengthBase) + ",";
			output += std::to_string(treeParameters.InternodeLengthAgeFactor) + ",";
			output += std::to_string(treeParameters.ApicalControlBase) + ",";
			output += std::to_string(treeParameters.ApicalControlAgeFactor) + ",";
			output += std::to_string(treeParameters.ApicalControlLevelFactor) + ",";
			output += std::to_string(treeParameters.ApicalControlDistanceFactor) + ",";
			output += std::to_string(treeParameters.MaxBudAge) + ",";
#pragma endregion
#pragma region Environmental
			output += std::to_string(treeParameters.InternodeSize) + ",";
			output += std::to_string(treeParameters.Phototropism) + ",";
			output += std::to_string(treeParameters.GravitropismBase) + ",";
			output += std::to_string(treeParameters.GravitropismLevelFactor) + ",";
			output += std::to_string(treeParameters.PruningFactor) + ",";
			output += std::to_string(treeParameters.LowBranchPruningFactor) + ",";
			output += std::to_string(treeParameters.ThicknessRemovalFactor) + ",";
			output += std::to_string(treeParameters.GravityBendingStrength) + ",";
			output += std::to_string(treeParameters.GravityBendingAngleFactor) + ",";
			output += std::to_string(treeParameters.ApicalBudLightingFactor) + ",";
			output += std::to_string(treeParameters.LateralBudLightingFactor) + ",";
#pragma endregion
			output += std::to_string(treeParameters.EndNodeThickness) + ",";
			output += std::to_string(treeParameters.ThicknessControlFactor) + ",";

			output += std::to_string(treeParameters.CrownShynessBase) + ",";
			output += std::to_string(treeParameters.CrownShynessFactor);


			output += "\n";
			ofs.write(output.c_str(), output.size());
			ofs.flush();
		}
		ofs.close();
		Debug::Log("Tree group saved: " + path + ".csv");
	}
	else
	{
		Debug::Error("Can't open file!");
	}
}

void DataCollectionSystem::ExportKDops(const std::string& path) const
{
	if(_KDopsOutputList.empty()) return;
	std::ofstream ofs;
	ofs.open((path + ".csv").c_str(), std::ofstream::out | (_Batched ? std::ofstream::app : std::ofstream::trunc));
	if (ofs.is_open())
	{
		for (auto& instance : _KDopsOutputList) {
			auto& kdop = instance.data;
			std::string output = "";
			output += std::to_string(instance.Index) + ",";
			output += instance.Name;
			for(auto& i : kdop)
			{
				output += "," + std::to_string(i);
			}
			output += "\n";
			ofs.write(output.c_str(), output.size());
			ofs.flush();
		}
		ofs.close();
		Debug::Log("Tree group saved: " + path + ".csv");
	}
	else
	{
		Debug::Error("Can't open file!");
	}
}

void TreeUtilities::DataCollectionSystem::ExportCakeTower(const std::string& path) const
{
	std::ofstream ofs;
	ofs.open((path + ".csv").c_str(), std::ofstream::out | (_Batched ? std::ofstream::app : std::ofstream::trunc));
	if (ofs.is_open())
	{
		for (auto& instance : _CakeTowersOutputList) {
			auto cakeTower = instance.data;
			std::string output = "";
			output += std::to_string(instance.Index) + ",";
			output += instance.Name;
			for (auto& tier : cakeTower)
			{
				for(auto& slice : tier)
				{
					output += "," + std::to_string(slice.MaxDistance);
				}
			}
			output += "\n";
			ofs.write(output.c_str(), output.size());
			ofs.flush();
		}
		ofs.close();
		Debug::Log("Tree group saved: " + path + ".csv");
	}
	else
	{
		Debug::Error("Can't open file!");
	}
}

void DataCollectionSystem::SetCameraPose(glm::vec3 position, glm::vec3 rotation)
{
	_CameraPosition = position;
	_CameraEulerRotation = glm::radians(rotation);
	Transform transform;
	transform.SetPosition(_CameraPosition);
	transform.SetEulerRotation(_CameraEulerRotation);
	_ImageCameraEntity.SetComponentData(transform);
	_SemanticMaskCameraEntity.SetComponentData(transform);
}

void DataCollectionSystem::OnCreate()
{
	EntityArchetype archetype = EntityManager::CreateEntityArchetype("General", GlobalTransform(), Transform());
	_ImageCameraEntity = EntityManager::CreateEntity(archetype);
	TreeManager::GetInternodeSystem()->_CameraEntity = _ImageCameraEntity;
	Transform transform;
	transform.SetPosition(_CameraPosition);
	transform.SetEulerRotation(_CameraEulerRotation);
	_ImageCameraEntity.SetComponentData(transform);
	auto cameraComponent = std::make_unique<CameraComponent>();
	cameraComponent->ResizeResolution(_CaptureResolution, _CaptureResolution);
	cameraComponent->DrawSkyBox = false;
	cameraComponent->AllowAutoResize = false;
	cameraComponent->ClearColor = glm::vec3(1.0f);
	_ImageCameraEntity.SetName("ImageCap Camera");
	_ImageCameraEntity.SetPrivateComponent(std::move(cameraComponent));


	_SemanticMaskCameraEntity = EntityManager::CreateEntity(archetype);
	transform.SetPosition(_CameraPosition);
	transform.SetEulerRotation(_CameraEulerRotation);
	_SemanticMaskCameraEntity.SetComponentData(transform);
	cameraComponent = std::make_unique<CameraComponent>();
	cameraComponent->ResizeResolution(_TargetResolution, _TargetResolution);
	cameraComponent->DrawSkyBox = false;
	cameraComponent->AllowAutoResize = false;
	cameraComponent->ClearColor = glm::vec3(1.0f);
	_SemanticMaskCameraEntity.SetName("Semantic Mask Camera");
	_SemanticMaskCameraEntity.SetPrivateComponent(std::move(cameraComponent));

	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/2236927059_a18cdd9196.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/2289428141_c758f436a1.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/2814264828_bb3f9d7ca9.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/3397325268_dc6135c432.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/69498568_e43c0e8520.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/1122838735_bc116c7a7c.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/1123280110_dda3037a69.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/3837561150_9f786dc7e5.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/st-andrewgate-2_300px.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/winecentre.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/calle-2.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/calle-3.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/calle+3.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/MainStreet_t.jpg"));
	_BackgroundTextures.push_back(ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/Street/st-andrewgate-2_300px.jpg"));


	_BackgroundMaterial = std::make_shared<Material>();
	_BackgroundMaterial->Shininess = 32.0f;
	std::string vertShaderCode = std::string("#version 460 core\n")
		+ *Default::ShaderIncludes::Uniform +
		+"\n"
		+ FileIO::LoadFileAsString(FileIO::GetResourcePath("Shaders/Vertex/Standard.vert"));
	std::string fragShaderCode = std::string("#version 460 core\n")
		+ *Default::ShaderIncludes::Uniform
		+ "\n"
		+ FileIO::LoadFileAsString(FileIO::GetAssetFolderPath() + "Shaders/Fragment/Background.frag");

	auto standardvert = std::make_shared<GLShader>(ShaderType::Vertex);
	standardvert->Compile(vertShaderCode);
	auto standardfrag = std::make_shared<GLShader>(ShaderType::Fragment);
	standardfrag->Compile(fragShaderCode);
	auto program = std::make_shared<GLProgram>(standardvert, standardfrag);
	


	_BackgroundMaterial->SetProgram(program);
	

	vertShaderCode = std::string("#version 460 core\n") +
		FileIO::LoadFileAsString(FileIO::GetResourcePath("Shaders/Vertex/TexturePassThrough.vert"));
	fragShaderCode = std::string("#version 460 core\n") +
		FileIO::LoadFileAsString(FileIO::GetAssetFolderPath() + "Shaders/Fragment/SmallBranch.frag");
	standardvert = std::make_shared<GLShader>(ShaderType::Vertex);
	standardvert->Compile(vertShaderCode);
	standardfrag = std::make_shared<GLShader>(ShaderType::Fragment);
	standardfrag->Compile(fragShaderCode);

	_SmallBranchProgram = std::make_unique<GLProgram>(standardvert, standardfrag);

	fragShaderCode = std::string("#version 460 core\n") +
		FileIO::LoadFileAsString(FileIO::GetAssetFolderPath() + "Shaders/Fragment/SmallBranchCopy.frag");
	standardvert = std::make_shared<GLShader>(ShaderType::Vertex);
	standardvert->Compile(vertShaderCode);
	standardfrag = std::make_shared<GLShader>(ShaderType::Fragment);
	standardfrag->Compile(fragShaderCode);

	_SmallBranchCopyProgram = std::make_unique<GLProgram>(standardvert, standardfrag);
	
	_SmallBranchFilter = std::make_unique<RenderTarget>(_TargetResolution, _TargetResolution);
	_SmallBranchBuffer = std::make_unique<GLTexture2D>(1, GL_RGB32F, _TargetResolution, _TargetResolution, true);
	_SmallBranchBuffer->SetInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	_SmallBranchBuffer->SetInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	_SmallBranchBuffer->SetInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	_SmallBranchBuffer->SetInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	auto mmr = std::make_unique<MeshRenderer>();
	mmr->Mesh = Default::Primitives::Quad;
	mmr->ForwardRendering = true;
	mmr->ReceiveShadow = false;
	mmr->CastShadow = false;
	mmr->Material = _BackgroundMaterial;
	_BackgroundMaterial->CullingMode = MaterialCullingMode::OFF;
	transform.SetPosition(glm::vec3(0, 17, -13));
	transform.SetEulerRotation(glm::radians(glm::vec3(75, -0, -180)));
	transform.SetScale(glm::vec3(30, 1, 30));


	_Background = EntityManager::CreateEntity(archetype);
	_Background.SetComponentData(transform);
	_Background.SetPrivateComponent(std::move(mmr));
	_Background.SetName("Background");
	MaskProcessor::_Background = _Background;
#pragma region Load parameters
	ImageCaptureSequence sequence;
	char dir[256] = {};
	_getcwd(dir, 256);

	sequence.CameraPos = glm::vec3(0, 2, 30);
	sequence.CameraEulerDegreeRot = glm::vec3(15, 0, 0);
	sequence.ParamPath = std::string(dir) + "\\acacia";
	sequence.Name = "acacia";
	PushImageCaptureSequence(sequence);
	sequence.CameraPos = glm::vec3(0, 2, 25);
	sequence.CameraEulerDegreeRot = glm::vec3(15, 0, 0);
	sequence.ParamPath = std::string(dir) + "\\apple";
	sequence.Name = "apple";
	PushImageCaptureSequence(sequence);
	sequence.CameraPos = glm::vec3(0, 2, 35);
	sequence.CameraEulerDegreeRot = glm::vec3(15, 0, 0);
	sequence.ParamPath = std::string(dir) + "\\willow";
	sequence.Name = "willow";
	PushImageCaptureSequence(sequence);
	sequence.CameraPos = glm::vec3(0, 2, 45);
	sequence.CameraEulerDegreeRot = glm::vec3(15, 0, 0);
	sequence.ParamPath = std::string(dir) + "\\maple";
	sequence.Name = "maple";
	PushImageCaptureSequence(sequence);
	sequence.CameraPos = glm::vec3(0, 2, 45);
	sequence.CameraEulerDegreeRot = glm::vec3(15, 0, 0);
	sequence.ParamPath = std::string(dir) + "\\birch";
	sequence.Name = "birch";
	PushImageCaptureSequence(sequence);
	sequence.CameraPos = glm::vec3(0, 2, 40);
	sequence.CameraEulerDegreeRot = glm::vec3(13, 0, 0);
	sequence.ParamPath = std::string(dir) + "\\oak";
	sequence.Name = "oak";
	PushImageCaptureSequence(sequence);
	sequence.CameraPos = glm::vec3(0, 2, 20);
	sequence.CameraEulerDegreeRot = glm::vec3(15, 0, 0);
	sequence.ParamPath = std::string(dir) + "\\pine";
	sequence.Name = "pine";
	PushImageCaptureSequence(sequence);
#pragma endregion

#pragma region MaskTrimmer
	MaskProcessor::_ResolutionX = _TargetResolution;
	MaskProcessor::_ResolutionY = _TargetResolution;
	MaskProcessor::_CameraEntity = _SemanticMaskCameraEntity;
	MaskProcessor::_Filter = std::make_unique<RenderTarget>(_TargetResolution, _TargetResolution);
	MaskProcessor::_DepthStencilBuffer = std::make_unique<GLRenderBuffer>();
	MaskProcessor::_DepthStencilBuffer->AllocateStorage(GL_DEPTH24_STENCIL8, _TargetResolution, _TargetResolution);
	MaskProcessor::_Filter->AttachRenderBuffer(MaskProcessor::_DepthStencilBuffer.get(), GL_DEPTH_STENCIL_ATTACHMENT);

	vertShaderCode = std::string("#version 460 core\n") +
		FileIO::LoadFileAsString(FileIO::GetAssetFolderPath() + "Shaders/Vertex/LightSnapShot.vert");
	fragShaderCode = std::string("#version 460 core\n") +
		FileIO::LoadFileAsString(FileIO::GetAssetFolderPath() + "Shaders/Fragment/LightSnapShot.frag");
	auto vertShader = std::make_shared<GLShader>(ShaderType::Vertex, vertShaderCode);
	auto fragShader = std::make_shared<GLShader>(ShaderType::Fragment, fragShaderCode);
	MaskProcessor::_InternodeCaptureProgram = std::make_unique<GLProgram>(vertShader, fragShader);

	
	vertShaderCode = std::string("#version 460 core\n") +
		FileIO::LoadFileAsString(FileIO::GetResourcePath("Shaders/Vertex/TexturePassThrough.vert"));
	fragShaderCode = std::string("#version 460 core\n") +
		FileIO::LoadFileAsString(FileIO::GetAssetFolderPath() + "Shaders/Fragment/MaskFilter.frag");
	vertShader = std::make_shared<GLShader>(ShaderType::Vertex, vertShaderCode);
	fragShader = std::make_shared<GLShader>(ShaderType::Fragment, fragShaderCode);
	MaskProcessor::_FilterProgram = std::make_unique<GLProgram>(vertShader, fragShader);

	fragShaderCode = std::string("#version 460 core\n") +
		FileIO::LoadFileAsString(FileIO::GetAssetFolderPath() + "Shaders/Fragment/MaskPreprocess.frag");
	fragShader = std::make_shared<GLShader>(ShaderType::Fragment, fragShaderCode);
	MaskProcessor::_MaskPreprocessProgram = std::make_unique<GLProgram>(vertShader, fragShader);
	
#pragma endregion

	Enable();
}

void DataCollectionSystem::SetPlantSimulationSystem(PlantSimulationSystem* value)
{
	_PlantSimulationSystem = value;
}

void DataCollectionSystem::Update()
{
	if (_ImageCaptureSequences.empty() || _CurrentSelectedSequenceIndex < 0) return;
	std::string path;
	auto& imageCaptureSequence = _Reconstruction ? _ImageCaptureSequences[_Index].first : _ImageCaptureSequences[_CurrentSelectedSequenceIndex].first;
	auto& treeParameters = _Reconstruction ? _ImageCaptureSequences[_Index].second : _ImageCaptureSequences[_CurrentSelectedSequenceIndex].second;
	switch (_Status)
	{
	case DataCollectionSystemStatus::Idle:
		if (_NeedExport) {
			if (_StartIndex <= _EndIndex)
			{
				SetCameraPose(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot);
				treeParameters = _PlantSimulationSystem->LoadParameters(imageCaptureSequence.ParamPath);
				treeParameters.Seed = _StartIndex + (_IsTrain ? 0 : 9999);
				_CurrentTree = _PlantSimulationSystem->CreateTree(treeParameters, glm::vec3(0.0f));
				_PlantSimulationSystem->ResumeGrowth();
				_Status = DataCollectionSystemStatus::Growing;
			}
			else if (_NeedEval)
			{
				ExportAllData();
				_StartIndex = _EvalStartIndex;
				_EndIndex = _EvalEndIndex;
				ResetCounter((_StartIndex - 1) * 7, _StartIndex, _EndIndex, _ExportOBJ, _ExportGraph);
				_NeedEval = false;
				_IsTrain = false;
			}
			else {
				ExportAllData();
				_NeedExport = false;
			}
		}
		else if (_Reconstruction)
		{
			SetCameraPose(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot);
			treeParameters = _PlantSimulationSystem->LoadParameters(imageCaptureSequence.ParamPath);
			treeParameters.Seed = _Seed;
			_CurrentTree = _PlantSimulationSystem->CreateTree(treeParameters, glm::vec3(0.0f));
			_PlantSimulationSystem->ResumeGrowth();
			_Status = DataCollectionSystemStatus::Growing;
		}
		else {
			OnGui();
		}
		break;
	case DataCollectionSystemStatus::Growing:
		if (!_PlantSimulationSystem->_Growing)
		{
			_Status = DataCollectionSystemStatus::Rendering;
			_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
		}
		break;
	case DataCollectionSystemStatus::Rendering:
		_Status = DataCollectionSystemStatus::CaptureOriginal;
		break;
	case DataCollectionSystemStatus::CaptureOriginal:
		if (!_Reconstruction) {
			path = _StorePath + "white_" + (_IsTrain ? "train/" : "val/") +
				std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
				+ "_" + imageCaptureSequence.Name
				+ ".jpg";
		}
		else
		{
			path = _ReconPath + ".jpg";
		}
		_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
			path, _TargetResolution, _TargetResolution);

		_Status = DataCollectionSystemStatus::CaptureRandom;
		_BackgroundMaterial->SetTexture(_BackgroundTextures[glm::linearRand((size_t)0, _BackgroundTextures.size() - 1)]);
		_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(true);
		break;
	case DataCollectionSystemStatus::CaptureRandom:
		path = _StorePath + "rgb_" + (_IsTrain ? "train/" : "val/") +
			std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
			+ "_" + imageCaptureSequence.Name
			+ ".jpg";
		if (!_Reconstruction)
		{
			_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				path, _TargetResolution, _TargetResolution);
		}
		_Status = DataCollectionSystemStatus::CaptureSemantic;
		_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
		EnableSemantic();
		break;
	case DataCollectionSystemStatus::CaptureSemantic:
		_SmallBranchFilter->AttachTexture(_SmallBranchBuffer.get(), GL_COLOR_ATTACHMENT0);
		_SmallBranchFilter->Clear();
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		_SmallBranchFilter->GetFrameBuffer()->DrawBuffer(GL_COLOR_ATTACHMENT0);
		_SmallBranchFilter->Bind();
		_SmallBranchProgram->Bind();
		_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->GetTexture()->Texture()->Bind(0);
		_SmallBranchProgram->SetInt("InputTex", 0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		Default::GLPrograms::ScreenVAO->Bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);

		_SmallBranchCopyProgram->Bind();
		_SmallBranchFilter->AttachTexture(_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->GetTexture()->Texture().get(), GL_COLOR_ATTACHMENT0);
		_SmallBranchBuffer->Bind(0);
		_SmallBranchCopyProgram->SetInt("InputTex", 0);
		Default::GLPrograms::ScreenVAO->Bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);
		if (!_Reconstruction) {
			path = _StorePath + "mask_" + (_IsTrain ? "train/" : "val/") +
				std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
				+ "_" + imageCaptureSequence.Name
				+ ".png";
		}
		else
		{
			path = _ReconPath + ".png";
		}
		_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToPng(
			path);
		HideFoliage();
		_Status = DataCollectionSystemStatus::CaptureBranch;
		break;
	case DataCollectionSystemStatus::CaptureBranch:
		path = _StorePath + "branch_" + (_IsTrain ? "train/" : "val/") +
			std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
			+ "_" + imageCaptureSequence.Name
			+ ".png";
		if(!_Reconstruction) _SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToPng(
			path);
		_Status = DataCollectionSystemStatus::CollectData;
		break;
	case DataCollectionSystemStatus::CollectData:
		if (_ExportGraph || _Reconstruction) {
			TreeManager::SerializeTreeGraph(_Reconstruction ? _ReconPath : _StorePath + "graph_" + (_IsTrain ? "train/ " : "val/ ") +
				std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
				+ "_" + imageCaptureSequence.Name, _CurrentTree);
		}
		if (_ExportOBJ || _Reconstruction) {
			TreeManager::ExportTreeAsModel(_CurrentTree, _Reconstruction ? _ReconPath : _StorePath + "obj_" + (_IsTrain ? "train/ " : "val/ ") +
				std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
				+ "_" + imageCaptureSequence.Name, true);
		}
		_TreeParametersOutputList.emplace_back(_Counter, imageCaptureSequence.Name, treeParameters);
		if (_CurrentTree.HasPrivateComponent<KDop>()) {
			_CurrentTree.GetPrivateComponent<KDop>()->CalculateVolume();
			_KDopsOutputList.emplace_back(_Counter, imageCaptureSequence.Name, _CurrentTree.GetPrivateComponent<KDop>());
		}
		_CurrentTree.GetPrivateComponent<CakeTower>()->CalculateVolume();
		if (!_Reconstruction) {
			_CakeTowersOutputList.emplace_back(_Counter, imageCaptureSequence.Name, _CurrentTree.GetPrivateComponent<CakeTower>());
		}
		else
		{
			path = _ReconPath + ".ct";
			const std::string data = _CurrentTree.GetPrivateComponent<CakeTower>()->Save();
			std::ofstream ofs;
			ofs.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
			ofs.write(data.c_str(), data.length());
			ofs.flush();
			ofs.close();
		}
		
		_Status = DataCollectionSystemStatus::CleanUp;
		break;
	case DataCollectionSystemStatus::CleanUp:
		
		TreeManager::DeleteAllTrees();
		if (!_Reconstruction) {
			_CurrentSelectedSequenceIndex++;
			_CurrentSelectedSequenceIndex %= _ImageCaptureSequences.size();
			if (_CurrentSelectedSequenceIndex == 0) _StartIndex++;
			_Counter++;
		}
		_Reconstruction = false;
		_Status = DataCollectionSystemStatus::Idle;
		break;
	}
}

void DataCollectionSystem::EnableSemantic() const
{
	Entity foliageEntity;
	EntityManager::ForEachChild(_CurrentTree, [&foliageEntity](Entity child)
		{
			if (child.HasComponentData<WillowFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<AppleFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<AcaciaFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<BirchFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<OakFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<MapleFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<DefaultFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<PineFoliageInfo>())
			{
				foliageEntity = child;
			}
		}
	);
	if (foliageEntity.HasPrivateComponent<MeshRenderer>())
	{
		auto& branchletRenderer = foliageEntity.GetPrivateComponent<MeshRenderer>();
		branchletRenderer->ForwardRendering = true;
		branchletRenderer->Material = TreeManager::SemanticTreeBranchMaterial;
	}
	if (foliageEntity.HasPrivateComponent<Particles>())
	{
		auto& leavesRenderer = foliageEntity.GetPrivateComponent<Particles>();
		leavesRenderer->ForwardRendering = true;
		leavesRenderer->Material = TreeManager::SemanticTreeLeafMaterial;
	}
	auto& branchRenderer = _CurrentTree.GetPrivateComponent<MeshRenderer>();
	branchRenderer->ForwardRendering = true;
	branchRenderer->Material = TreeManager::SemanticTreeBranchMaterial;

}

void DataCollectionSystem::HideFoliage() const
{
	Entity foliageEntity;
	EntityManager::ForEachChild(_CurrentTree, [&foliageEntity](Entity child)
		{
			if (child.HasComponentData<WillowFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<AppleFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<AcaciaFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<BirchFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<OakFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<MapleFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<DefaultFoliageInfo>())
			{
				foliageEntity = child;
			}
			else if (child.HasComponentData<PineFoliageInfo>())
			{
				foliageEntity = child;
			}
		}
	);
	if (foliageEntity.HasPrivateComponent<MeshRenderer>())
	{
		auto& branchletRenderer = foliageEntity.GetPrivateComponent<MeshRenderer>();
		branchletRenderer->SetEnabled(false);
	}
	if (foliageEntity.HasPrivateComponent<Particles>())
	{
		auto& leavesRenderer = foliageEntity.GetPrivateComponent<Particles>();
		leavesRenderer->SetEnabled(false);
	}
}
