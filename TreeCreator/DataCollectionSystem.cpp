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
#include "Bloom.h"
#include "SSAO.h"
#include "GreyScale.h"
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

void DataCollectionSystem::CaptureSemantic(ImageCaptureSequence& imageCaptureSequence, int angle) const
{
	_SmallBranchFilter->AttachTexture(_SmallBranchBuffer.get(), GL_COLOR_ATTACHMENT0);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	_SmallBranchFilter->GetFrameBuffer()->DrawBuffer(GL_COLOR_ATTACHMENT0);
	_SmallBranchFilter->Clear();
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
	std::string path;
	std::string angleTxt = "_" + std::to_string(angle * 90);
	if (!_Reconstruction) {
		path = _StorePath + "mask_" + (_IsTrain ? "train/" : "val/") +
			std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
			+ "_" + imageCaptureSequence.Name + (_EnableMultipleAngles ? angleTxt : "") +
			+ ".png";
	}
	else
	{
		path = _ReconPath + (_EnableMultipleAngles ? angleTxt : "") + ".png";
	}
	_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToPng(
		path, _TargetResolution, _TargetResolution, false);
	
}

void DataCollectionSystem::ExportCakeTowerForRecon(int layer, int sector)
{
	auto& cakeTower = _CurrentTree.GetPrivateComponent<RBV>();
	cakeTower->LayerAmount = layer;
	cakeTower->SectorAmount = sector;
	cakeTower->CalculateVolume();
	std::string path = _ReconPath + "_" + std::to_string(layer) + "_" + std::to_string(sector) + ".ct";
	const std::string data = cakeTower->Save();
	std::ofstream ofs;
	ofs.open(path.c_str(), std::ofstream::out | std::ofstream::trunc);
	if (ofs.is_open())
	{
		ofs.write(data.c_str(), data.length());
		ofs.flush();
		ofs.close();
	}
	else
	{
		Debug::Error("Can't open file!");
	}
	/*
	ofs.open((_ReconPath + ".csv").c_str(), std::ofstream::trunc);
	if (ofs.is_open())
	{
		auto& kdop = _CurrentTree.GetPrivateComponent<KDop>()->DirectionalDistance;
		std::string output;
		for (int i = 0; i < kdop.size(); i++)
		{
			output += std::to_string(kdop[i]);
			if (i < kdop.size() - 1) output += ",";
		}
		output += "\n";
		ofs.write(output.c_str(), output.size());
		ofs.flush();

		ofs.close();
		Debug::Log("Tree group saved: " + _ReconPath + ".csv");
	}
	else
	{
		Debug::Error("Can't open file!");
	}
	*/
}

void DataCollectionSystem::OnGui()
{
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Learning Data Generation")) {
			if (ImGui::Button("Load Textures"))
			{
				_BackgroundTextures.clear();
				int j = 1;
				int amount = 950;
				while(j < amount)
				{
					int index = j * 8;
					auto texture = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() +
						"Textures/StreetView/" + std::string(6 - std::to_string(index).length(), '0') + std::to_string(index) + "_2.jpg");
					if(texture)_BackgroundTextures.push_back(texture);
					else
					{
						amount++;
					}
					j++;
				}
				_BranchBarkTextures.clear();
				for(int i = 0; i < 23; i++)
				{
					auto texture = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() +
						"Textures/Bark/" + std::to_string(i) + ".jpg");
					if (texture)_BranchBarkTextures.push_back(texture);
				}
			}
			if (ImGui::Button("Create new data set...")) {
				ImGui::OpenPopup("Data set wizard");
			}
			const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			if (ImGui::BeginPopupModal("Data set wizard", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::Text("Export options:");
				ImGui::DragInt("Export Resolution", &_TargetResolution, 1, 1, _CaptureResolution);
				ImGui::DragInt("Camera Resolution", &_CaptureResolution, 1, _TargetResolution, 5120);
				ImGui::Separator();
				ImGui::Checkbox("Generate OBJ", &_ExportOBJ);
				ImGui::Checkbox("Generate graph", &_ExportGraph);
				ImGui::Checkbox("Generate Images", &_ExportImages);
				if(_ExportImages)
				{
					ImGui::Checkbox("Generate Branch only Images", &_ExportBranchOnly);
				}
				ImGui::Checkbox("Generate KDop", &_ExportKDop);
				ImGui::Checkbox("Generate RBV", &_ExportCakeTower);
				ImGui::Checkbox("Enable multiple angles", &_EnableMultipleAngles);
				ImGui::Separator();
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
						std::filesystem::create_directory("tree_data/rgb_branch_train");
						std::filesystem::create_directory("tree_data/white_branch_train");

						std::filesystem::create_directory("tree_data/branch_val");
						std::filesystem::create_directory("tree_data/graph_val");
						std::filesystem::create_directory("tree_data/mask_val");
						std::filesystem::create_directory("tree_data/obj_val");
						std::filesystem::create_directory("tree_data/rgb_val");
						std::filesystem::create_directory("tree_data/white_val");
						std::filesystem::create_directory("tree_data/rgb_branch_val");
						std::filesystem::create_directory("tree_data/white_branch_val");
						
						std::filesystem::create_directory("tree_data/skeleton_recon");
						std::filesystem::create_directory("tree_data/image_recon");
						std::filesystem::create_directory("tree_data/graph_recon");
						std::filesystem::create_directory("tree_data/obj_recon");
						std::filesystem::create_directory("tree_data/volume_recon");
					}
					ResetCounter((_StartIndex - 1) * 7, _StartIndex, _EndIndex, _ExportOBJ, _ExportGraph);
					_NeedEval = true;
					_IsTrain = true;
					_NeedExport = true;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}

void DataCollectionSystem::SetDirectionalLightEntity(Entity entity, Entity entity1, Entity entity2, Entity entity3)
{
	_DirectionalLightEntity = entity;
	_DirectionalLightEntity1 = entity1;
	_DirectionalLightEntity2 = entity2;
	_DirectionalLightEntity3 = entity3;
}

void DataCollectionSystem::ExportAllData()
{
	ExportParams(_StorePath + "params_" + (_IsTrain ? "train" : "val"));
	if (_ExportCakeTower) {
		ExportCakeTower(_StorePath + "rbv_", _IsTrain);
		ExportCakeTowerPerSpecies(_StorePath + "rbv_species_", _IsTrain);
		ExportCakeTowerGeneral(_StorePath + "rbv_general_", _IsTrain);
	}
	for (auto& i : _GeneralCakeTowersOutputList)
	{
		i.second.clear();
	}
	for (auto& i : _PerSpeciesCakeTowersOutputList)
	{
		i.second.clear();
	}
	for (auto& i : _CakeTowersOutputList)
	{
		i.second.clear();
	}
	_TreeParametersOutputList.clear();
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
			output += std::to_string(0.0) + ",";
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


void TreeUtilities::DataCollectionSystem::ExportCakeTower(const std::string& path, bool isTrain) const
{
	for (auto& lists : _CakeTowersOutputList) {
		std::ofstream ofs;
		ofs.open((path + std::to_string(lists.first.first) + "_" + std::to_string(lists.first.second) + (isTrain ? "_train" : "_val") + ".csv").c_str(), std::ofstream::out | (_Batched ? std::ofstream::app : std::ofstream::trunc));
		if (ofs.is_open())
		{
			for (auto& instance : lists.second) {
				auto cakeTower = instance.data;
				std::string output = "";
				output += std::to_string(instance.Index) + ",";
				output += instance.Name;
				output += "," + std::to_string(instance._MaxHeight);
				output += "," + std::to_string(instance._MaxRadius);
				for (auto& tier : cakeTower)
				{
					for (auto& slice : tier)
					{
						output += "," + std::to_string(slice.MaxDistance / instance._MaxRadius);
					}
				}
				output += "\n";
				ofs.write(output.c_str(), output.size());
			}
			ofs.flush();
			ofs.close();
			Debug::Log("Tree group saved: " + path + ".csv");
		}
		else
		{
			Debug::Error("Can't open file!");
		}
	}
}

void DataCollectionSystem::ExportCakeTowerPerSpecies(const std::string& path, bool isTrain) const
{
	for (auto& lists : _PerSpeciesCakeTowersOutputList) {
		std::ofstream ofs;
		ofs.open((path + std::to_string(lists.first.first) + "_" + std::to_string(lists.first.second) + (isTrain ? "_train" : "_val") + ".csv").c_str(), std::ofstream::out | (_Batched ? std::ofstream::app : std::ofstream::trunc));
		if (ofs.is_open())
		{
			for (auto& instance : lists.second) {
				auto cakeTower = instance.data;
				std::string output = "";
				output += std::to_string(instance.Index) + ",";
				output += instance.Name;
				for (auto& tier : cakeTower)
				{
					for (auto& slice : tier)
					{
						output += "," + std::to_string(slice.MaxDistance / 30.0f);
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
}

void DataCollectionSystem::ExportCakeTowerGeneral(const std::string& path, bool isTrain) const
{
	for (auto& lists : _GeneralCakeTowersOutputList) {
		std::ofstream ofs;
		ofs.open((path + std::to_string(lists.first.first) + "_" + std::to_string(lists.first.second) + (isTrain ? "_train" : "_val") + ".csv").c_str(), std::ofstream::out | (_Batched ? std::ofstream::app : std::ofstream::trunc));
		if (ofs.is_open())
		{
			for (auto& instance : lists.second) {
				auto cakeTower = instance.data;
				std::string output = "";
				output += std::to_string(instance.Index) + ",";
				output += instance.Name;
				for (auto& tier : cakeTower)
				{
					for (auto& slice : tier)
					{
						output += "," + std::to_string(slice.MaxDistance / 30.0f);
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
}

void DataCollectionSystem::SetCameraPose(glm::vec3 position, glm::vec3 rotation, bool random)
{
	_CameraPosition = position;
	_CameraEulerRotation = glm::radians(random ? rotation + glm::linearRand(glm::vec3(-5, -5, 0), glm::vec3(5, 5, 0)) : rotation);
	if(random)
	{
		float fov = glm::linearRand(90, 100);
		_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->m_fov = fov;
		_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->m_fov = fov;
	}else{
		_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->m_fov = 90;
		_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->m_fov = 90;
	}
	Transform transform;
	transform.SetPosition(_CameraPosition);
	transform.SetEulerRotation(_CameraEulerRotation);
	_ImageCameraEntity.SetComponentData(transform);
	_SemanticMaskCameraEntity.SetComponentData(transform);
}

void DataCollectionSystem::SetCameraPoseMulti(glm::vec3 position, glm::vec3 rotation, int angle)
{
	glm::vec3 actualPosition;
	glm::vec3 actualRotation;
	switch (angle)
	{
	case 0:
		actualPosition = position;
		actualRotation = rotation;
		break;
	case 1:
		actualPosition = glm::vec3(position.z, position.y, position.x);
		actualRotation = glm::vec3(rotation.x, 90.0f, 0.0f);
		break;
	case 2:
		actualPosition = glm::vec3(-position.x, position.y, -position.z);
		actualRotation = glm::vec3(rotation.x, 180.0f, 0.0f);
		break;
	case 3:
		actualPosition = glm::vec3(-position.z, position.y, -position.x);
		actualRotation = glm::vec3(rotation.x, 270.0f, 0.0f);
		break;
	}
	SetCameraPose(actualPosition, actualRotation, false);
}

void DataCollectionSystem::OnCreate()
{
	_LightTransform = Transform();
	_LightTransform.SetEulerRotation(glm::radians(glm::vec3(150, 30, 0)));

	EntityArchetype archetype = EntityManager::CreateEntityArchetype("General", GlobalTransform(), Transform());
	_ImageCameraEntity = EntityManager::CreateEntity(archetype);
	TreeManager::GetInternodeSystem()->_CameraEntity = _ImageCameraEntity;
	Transform transform;
	transform.SetPosition(_CameraPosition);
	transform.SetEulerRotation(_CameraEulerRotation);
	_ImageCameraEntity.SetComponentData(transform);
	auto cameraComponent = std::make_unique<CameraComponent>();
	cameraComponent->m_drawSkyBox = false;

	cameraComponent->m_clearColor = glm::vec3(1.0f);
	_ImageCameraEntity.SetName("ImageCap Camera");
	_ImageCameraEntity.SetPrivateComponent(std::move(cameraComponent));
	auto postProcessing = std::make_unique<PostProcessing>();
	postProcessing->PushLayer(std::make_unique<Bloom>());
	postProcessing->PushLayer(std::make_unique<SSAO>());
	postProcessing->PushLayer(std::make_unique<GreyScale>());
	_ImageCameraEntity.SetPrivateComponent(std::move(postProcessing));
	_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->ResizeResolution(_CaptureResolution, _CaptureResolution);
	_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->m_allowAutoResize = false;

	_SemanticMaskCameraEntity = EntityManager::CreateEntity(archetype);
	transform.SetPosition(_CameraPosition);
	transform.SetEulerRotation(_CameraEulerRotation);
	_SemanticMaskCameraEntity.SetComponentData(transform);
	cameraComponent = std::make_unique<CameraComponent>();
	cameraComponent->ResizeResolution(_TargetResolution, _TargetResolution);
	cameraComponent->m_drawSkyBox = false;
	cameraComponent->m_allowAutoResize = false;
	cameraComponent->m_clearColor = glm::vec3(1.0f);
	_SemanticMaskCameraEntity.SetName("Semantic Mask Camera");
	_SemanticMaskCameraEntity.SetPrivateComponent(std::move(cameraComponent));

	_BackgroundMaterial = std::make_shared<Material>();
	_BackgroundMaterial->m_shininess = 32.0f;
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
	_SmallBranchBuffer = std::make_unique<GLTexture2D>(0, GL_RGB32F, _TargetResolution, _TargetResolution, false);
	_SmallBranchBuffer->SetInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	_SmallBranchBuffer->SetInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	_SmallBranchBuffer->SetInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	_SmallBranchBuffer->SetInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	auto mmr = std::make_unique<MeshRenderer>();
	mmr->m_mesh = Default::Primitives::Quad;
	mmr->m_forwardRendering = true;
	mmr->m_receiveShadow = false;
	mmr->m_castShadow = false;
	mmr->m_material = _BackgroundMaterial;
	_BackgroundMaterial->m_cullingMode = MaterialCullingMode::Off;
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
	MaskProcessor::_CameraEntity = _ImageCameraEntity;
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
	for (int i = 1; i < 32; i *= 2)
	{
		_GeneralCakeTowersOutputList.push_back(std::move(std::make_pair(std::make_pair(i, i), std::vector<CakeTowerOutput>())));
		_PerSpeciesCakeTowersOutputList.push_back(std::move(std::make_pair(std::make_pair(i, i), std::vector<CakeTowerOutput>())));
		_CakeTowersOutputList.push_back(std::move(std::make_pair(std::make_pair(i, i), std::vector<CakeTowerOutput>())));
	}
	Enable();
}

void DataCollectionSystem::SetPlantSimulationSystem(PlantSimulationSystem* value)
{
	_PlantSimulationSystem = value;
}

void DataCollectionSystem::Update()
{
	if (!_NeedExport) OnGui();
}

void DataCollectionSystem::LateUpdate()
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
				if (_ExportImages || _EnableMultipleAngles) {
					//SetCameraPoseMulti(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot, 3);
					SetCameraPose(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot, false);

					RenderManager::GetInstance().m_lightSettings.m_ambientLight = (0.3f);
					float brightness = glm::linearRand(5.0f, 7.0f);
					_DirectionalLightEntity.GetPrivateComponent<DirectionalLight>()->m_diffuseBrightness = brightness / 2.0f;
					_DirectionalLightEntity.GetPrivateComponent<DirectionalLight>()->m_lightSize = 1.0f;
					_DirectionalLightEntity.GetPrivateComponent<DirectionalLight>()->m_bias = 0.3f;
					_DirectionalLightEntity1.GetPrivateComponent<DirectionalLight>()->m_diffuseBrightness = brightness / 3.0f;
					_DirectionalLightEntity2.GetPrivateComponent<DirectionalLight>()->m_diffuseBrightness = brightness / 3.0f;
					_DirectionalLightEntity3.GetPrivateComponent<DirectionalLight>()->m_diffuseBrightness = brightness / 8.0f;
					_DirectionalLightEntity.GetPrivateComponent<DirectionalLight>()->m_diffuse = glm::normalize(glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0));
					_DirectionalLightEntity1.GetPrivateComponent<DirectionalLight>()->m_diffuse = glm::normalize(glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0));
					_DirectionalLightEntity2.GetPrivateComponent<DirectionalLight>()->m_diffuse = glm::normalize(glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0));
					_DirectionalLightEntity3.GetPrivateComponent<DirectionalLight>()->m_diffuse = glm::normalize(glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0));
					_DirectionalLightEntity.GetPrivateComponent<DirectionalLight>()->m_castShadow = true;
					_DirectionalLightEntity1.GetPrivateComponent<DirectionalLight>()->m_castShadow = true;
					_DirectionalLightEntity2.GetPrivateComponent<DirectionalLight>()->m_castShadow = true;
					_DirectionalLightEntity3.GetPrivateComponent<DirectionalLight>()->m_castShadow = true;

					glm::vec3 mainLightAngle = glm::vec3(150 + glm::linearRand(-30, 30), glm::linearRand(0, 360), 0);
					float lightFocus = 35;
					_LightTransform.SetEulerRotation(glm::radians(mainLightAngle));
					_LightTransform1.SetEulerRotation(glm::radians(mainLightAngle + glm::vec3(0, -lightFocus, 0)));
					_LightTransform2.SetEulerRotation(glm::radians(mainLightAngle + glm::vec3(0, lightFocus, 0)));

					_LightTransform2.SetEulerRotation(glm::radians(mainLightAngle + glm::vec3(0, -180, 0)));

					_DirectionalLightEntity.SetComponentData(_LightTransform);
					_DirectionalLightEntity1.SetComponentData(_LightTransform1);
					_DirectionalLightEntity2.SetComponentData(_LightTransform2);
					_DirectionalLightEntity3.SetComponentData(_LightTransform3);


					_SmallBranchBuffer->ReSize(0, GL_RGB32F, GL_RGB, GL_FLOAT, 0, _TargetResolution, _TargetResolution);
					_SmallBranchFilter->SetResolution(_TargetResolution, _TargetResolution);
					_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->ResizeResolution(_TargetResolution, _TargetResolution);
					_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->ResizeResolution(_CaptureResolution, _CaptureResolution);
				}
				treeParameters.Seed = _StartIndex + (_IsTrain ? 0 : 9999);
				TreeParameters tps = treeParameters;
				/*
				 				if (treeParameters.FoliageType == 3)
					{
						//Pine
						tps.Age += glm::linearRand(-1, 1);
					}
					else if (treeParameters.FoliageType == 4)
					{
						//Pine
						tps.Age += glm::linearRand(-1, 1);
					}
					else if (treeParameters.FoliageType == 7)
					{
						//Pine
						tps.Age += glm::linearRand(-1, 1);
					}
				*/
				//tps.EndNodeThickness = tps.EndNodeThickness + tps.EndNodeThickness * glm::linearRand(-0.5f, 0.5f);
				if(!_BranchBarkTextures.empty()){
					auto mat = std::make_shared<Material>();
					mat->SetProgram(Default::GLPrograms::StandardProgram);
					mat->m_shininess = 32.0f;
					mat->m_metallic = 0.0f;
					mat->SetTexture(_BranchBarkTextures [glm::linearRand((size_t)0, _BranchBarkTextures.size() - 1)]);
					_CurrentTree = _PlantSimulationSystem->CreateTree(mat, tps, glm::vec3(0.0f), true);
				}else
				{
					_CurrentTree = _PlantSimulationSystem->CreateTree(tps, glm::vec3(0.0f));
				}
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
			_SmallBranchBuffer->ReSize(0, GL_RGB32F, GL_RGB, GL_FLOAT, 0, _TargetResolution, _TargetResolution);
			_SmallBranchFilter->SetResolution(_TargetResolution, _TargetResolution);
			SetCameraPose(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot, false);
			treeParameters = _PlantSimulationSystem->LoadParameters(imageCaptureSequence.ParamPath);
			treeParameters.Seed = _Seed;
			_CurrentTree = _PlantSimulationSystem->CreateTree(treeParameters, glm::vec3(0.0f));
			_PlantSimulationSystem->ResumeGrowth();
			_Status = DataCollectionSystemStatus::Growing;
		}
		break;
	case DataCollectionSystemStatus::Growing:
		if (!_PlantSimulationSystem->_Growing)
		{
			if (_Reconstruction) {
				Transform cameraTransform;
				cameraTransform.SetPosition(glm::vec3(0, imageCaptureSequence.CameraPos.z * 1.5f, 0));
				cameraTransform.SetEulerRotation(glm::radians(glm::vec3(-90, 0, 0)));
				_SemanticMaskCameraEntity.SetComponentData(cameraTransform);
			}
			_Status = DataCollectionSystemStatus::Rendering;
			_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
		}
		break;
	case DataCollectionSystemStatus::Rendering:
		if (_ExportImages) {
			
			_Status = DataCollectionSystemStatus::CaptureOriginal;
		}else
		{
			if(_EnableMultipleAngles)
			{
				SetCameraPoseMulti(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot, 0);
				_Status = DataCollectionSystemStatus::CaptureSemantic0;
				_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
				EnableSemantic();
			}
			else _Status = DataCollectionSystemStatus::CollectData;
		}
		break;
	case DataCollectionSystemStatus::CaptureOriginal:
		if (!_Reconstruction) {
			path = _StorePath + "white_" + (_IsTrain ? "train/" : "val/") +
				std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
				+ "_" + imageCaptureSequence.Name
				+ "_270.png";
		}
		else
		{
			path = _ReconPath + "_main.jpg";
		}
		if (!_Reconstruction) {
			_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToPng(
				path, _TargetResolution, _TargetResolution, true);
		}else
		{
			_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				path, _TargetResolution, _TargetResolution);
			_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				_ReconPath + "_main_top.jpg", _TargetResolution, _TargetResolution);
			SetCameraPose(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot, false);
		}
		if (_ExportBranchOnly) {
			_Status = DataCollectionSystemStatus::CaptureOriginalBranch;
			SetEnableFoliage(false);
		}else
		{
			if (!_BackgroundTextures.empty()) {
				_BackgroundMaterial->SetTexture(_BackgroundTextures[glm::linearRand((size_t)0, _BackgroundTextures.size() - 1)]);
			}
			_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(true);
			_Status = DataCollectionSystemStatus::CaptureRandom;
		}
		break;
	case DataCollectionSystemStatus::CaptureOriginalBranch:
		if (!_Reconstruction) {
			path = _StorePath + "white_branch_" + (_IsTrain ? "train/" : "val/") +
				std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
				+ "_" + imageCaptureSequence.Name
				+ ".png";
			_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToPng(
				path, _TargetResolution, _TargetResolution, true);
		}else
		{
			path = _ReconPath + "_branch.jpg";
			_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				path, _TargetResolution, _TargetResolution);
		}
		
		if (!_BackgroundTextures.empty()) {
			_BackgroundMaterial->SetTexture(_BackgroundTextures[glm::linearRand((size_t)0, _BackgroundTextures.size() - 1)]);
		}
		_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(true);
		SetEnableFoliage(true);
		_Status = DataCollectionSystemStatus::CaptureRandom;
		break;
	case DataCollectionSystemStatus::CaptureRandom:
		if (!_Reconstruction)
		{
			path = _StorePath + "rgb_" + (_IsTrain ? "train/" : "val/") +
				std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
				+ "_" + imageCaptureSequence.Name
				+ ".jpg";
			_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				path, _TargetResolution, _TargetResolution);
		}
		if (_ExportBranchOnly) {
			SetEnableFoliage(false);
			_Status = DataCollectionSystemStatus::CaptureRandomBranch;
		}else
		{
			if (_EnableMultipleAngles)
			{
				SetCameraPoseMulti(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot, 0);
				_Status = DataCollectionSystemStatus::CaptureSemantic0;
				_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
				EnableSemantic();
			}
			else {
				_Status = DataCollectionSystemStatus::CaptureSemantic;
				_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
				EnableSemantic();
			}
		}
		break;
	case DataCollectionSystemStatus::CaptureRandomBranch:
		if (!_Reconstruction) {
			path = _StorePath + "rgb_branch_" + (_IsTrain ? "train/" : "val/") +
				std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
				+ "_" + imageCaptureSequence.Name
				+ ".jpg";

			_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				path, _TargetResolution, _TargetResolution);
		}
		SetEnableFoliage(true);
		if(_EnableMultipleAngles)
		{
			SetCameraPoseMulti(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot, 0);
			_Status = DataCollectionSystemStatus::CaptureSemantic0;
		}
		else _Status = DataCollectionSystemStatus::CaptureSemantic;
		_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
		EnableSemantic();
		break;
	case DataCollectionSystemStatus::CaptureSemantic:
		CaptureSemantic(imageCaptureSequence, 0);
		if (_ExportBranchOnly) {
			SetEnableFoliage(false);
			_Status = DataCollectionSystemStatus::CaptureBranchMask;
		}else
		{
			_Status = DataCollectionSystemStatus::CollectData;
		}
		break;
	case DataCollectionSystemStatus::CaptureSemantic0:
		SetCameraPoseMulti(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot, 1);
		_Status = DataCollectionSystemStatus::CaptureSemantic1;
		break;
	case DataCollectionSystemStatus::CaptureSemantic1:
		CaptureSemantic(imageCaptureSequence, 0);
		SetCameraPoseMulti(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot, 2);
		_Status = DataCollectionSystemStatus::CaptureSemantic2;
		break;
	case DataCollectionSystemStatus::CaptureSemantic2:
		CaptureSemantic(imageCaptureSequence, 1);
		SetCameraPoseMulti(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot, 3);
		_Status = DataCollectionSystemStatus::CaptureSemantic3;
		break;
	case DataCollectionSystemStatus::CaptureSemantic3:
		CaptureSemantic(imageCaptureSequence, 2);
		SetCameraPoseMulti(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot, 0);
		_Status = DataCollectionSystemStatus::CaptureSemantic4;
		break;
	case DataCollectionSystemStatus::CaptureSemantic4:
		CaptureSemantic(imageCaptureSequence, 3);
		SetEnableFoliage(false);
		if (_ExportImages) {
			SetEnableFoliage(false);
			_Status = DataCollectionSystemStatus::CaptureBranchMask;
		}
		else
		{
			_Status = DataCollectionSystemStatus::CollectData;
		}
		break;
	case DataCollectionSystemStatus::CaptureBranchMask:
		path = _StorePath + "branch_" + (_IsTrain ? "train/" : "val/") +
			std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
			+ "_" + imageCaptureSequence.Name
			+ ".png";
		if (!_Reconstruction) _SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToPng(
			path, _TargetResolution, _TargetResolution);
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

		if (_CurrentTree.HasPrivateComponent<RBV>()) {
			if (_ExportCakeTower && !_Reconstruction) {
				for (auto& i : _GeneralCakeTowersOutputList) {
					_CurrentTree.GetPrivateComponent<RBV>()->LayerAmount = i.first.first;
					_CurrentTree.GetPrivateComponent<RBV>()->SectorAmount = i.first.second;
					_CurrentTree.GetPrivateComponent<RBV>()->CalculateVolume(36);
					i.second.emplace_back(_Counter, imageCaptureSequence.Name, _CurrentTree.GetPrivateComponent<RBV>());
				}
			}
			if (_ExportCakeTower && !_Reconstruction) {
				float height = 0;
				switch (_CurrentSelectedSequenceIndex)
				{
				case 0:
					height = 15;
					break;
				case 1:
					height = 12.5;
					break;
				case 2:
					height = 19;
					break;
				case 3:
					height = 32;
					break;
				case 4:
					height = 31;
					break;
				case 5:
					height = 18.5;
					break;
				case 6:
					height = 15;
					break;
				}
				for (auto& i : _PerSpeciesCakeTowersOutputList) {
					_CurrentTree.GetPrivateComponent<RBV>()->LayerAmount = i.first.first;
					_CurrentTree.GetPrivateComponent<RBV>()->SectorAmount = i.first.second;
					_CurrentTree.GetPrivateComponent<RBV>()->CalculateVolume(height);
					i.second.emplace_back(_Counter, imageCaptureSequence.Name, _CurrentTree.GetPrivateComponent<RBV>());
				}
			}
			if (_ExportCakeTower && !_Reconstruction) {
				for (auto& i : _CakeTowersOutputList) {
					_CurrentTree.GetPrivateComponent<RBV>()->LayerAmount = i.first.first;
					_CurrentTree.GetPrivateComponent<RBV>()->SectorAmount = i.first.second;
					_CurrentTree.GetPrivateComponent<RBV>()->CalculateVolume();
					i.second.emplace_back(_Counter, imageCaptureSequence.Name, _CurrentTree.GetPrivateComponent<RBV>());
				}
			}
			else if(_Reconstruction)
			{
				ExportCakeTowerForRecon(1, 1);
				ExportCakeTowerForRecon(2, 2);
				ExportCakeTowerForRecon(4, 4);
				ExportCakeTowerForRecon(8, 8);
				ExportCakeTowerForRecon(16, 16);
			}
		}
		if (_Reconstruction) {
			Transform cameraTransform;
			cameraTransform.SetPosition(glm::vec3(0, imageCaptureSequence.CameraPos.z * 1.5f, 0));
			cameraTransform.SetEulerRotation(glm::radians(glm::vec3(-90, 0, 0)));
			_SemanticMaskCameraEntity.SetComponentData(cameraTransform);
			
			cameraTransform.SetPosition(glm::vec3(imageCaptureSequence.CameraPos.x, _CurrentTree.GetPrivateComponent<RBV>()->MaxHeight / 2.0f, imageCaptureSequence.CameraPos.z * 1.5f));
			cameraTransform.SetEulerRotation(glm::radians(glm::vec3(0, 0, 0)));
			_ImageCameraEntity.SetComponentData(cameraTransform);
			
			_CurrentTree.GetPrivateComponent<RBV>()->FormEntity();
			_CurrentTree.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
			_Status = DataCollectionSystemStatus::CaptureCakeTower;
		}
		else _Status = DataCollectionSystemStatus::CleanUp;
		break;
	case DataCollectionSystemStatus::CaptureCakeTower:
		_Status = DataCollectionSystemStatus::CleanUp;
		break;
	case DataCollectionSystemStatus::CleanUp:
		if (_Reconstruction) {
			path = _ReconPath + "_rbv_main_8_8.jpg";
			_ImageCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				path, _TargetResolution, _TargetResolution);
			_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->StoreToJpg(
				_ReconPath + "_rbv_main_8_8_top.jpg", _TargetResolution, _TargetResolution);
		}
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
		branchletRenderer->m_forwardRendering = true;
		branchletRenderer->m_material = TreeManager::SemanticTreeBranchMaterial;
	}
	if (foliageEntity.HasPrivateComponent<Particles>())
	{
		auto& leavesRenderer = foliageEntity.GetPrivateComponent<Particles>();
		leavesRenderer->m_forwardRendering = true;
		leavesRenderer->m_material = TreeManager::SemanticTreeLeafMaterial;
	}
	auto& branchRenderer = _CurrentTree.GetPrivateComponent<MeshRenderer>();
	branchRenderer->m_forwardRendering = true;
	branchRenderer->m_material = TreeManager::SemanticTreeBranchMaterial;

}

void DataCollectionSystem::SetEnableFoliage(bool enabled) const
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
		branchletRenderer->SetEnabled(enabled);
	}
	if (foliageEntity.HasPrivateComponent<Particles>())
	{
		auto& leavesRenderer = foliageEntity.GetPrivateComponent<Particles>();
		leavesRenderer->SetEnabled(enabled);
	}
}
