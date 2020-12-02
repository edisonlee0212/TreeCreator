#include "ImageCollectionSystem.h"

#include "AcaciaFoliageGenerator.h"
#include "CrownSurfaceRecon.h"
#include "MapleFoliageGenerator.h"
#include "PineFoliageGenerator.h"
#include "WillowFoliageGenerator.h"
#include "AppleFoliageGenerator.h"
#include "OakFoliageGenerator.h"
#include "BirchFoliageGenerator.h"

void ImageCollectionSystem::ResetCounter(int value, int startIndex, int endIndex)
{
	_Counter = value;
	_StartIndex = startIndex;
	_EndIndex = endIndex;
	_Timer = Application::EngineTime();
	_Status = ImageCollectionSystemStatus::Idle;
	_CurrentSelectedSequenceIndex = 0;
}

void ImageCollectionSystem::SetIsTrain(bool value)
{
	_IsTrain = value;
}

auto ImageCollectionSystem::IsExport() const -> bool
{
	return _Export;
}

void ImageCollectionSystem::PushImageCaptureSequence(ImageCaptureSequence sequence)
{
	_ImageCaptureSequences.push_back({ sequence, _PlantSimulationSystem->LoadParameters(sequence.ParamPath) });
	_Export = true;
}

void ImageCollectionSystem::SetCameraPose(glm::vec3 position, glm::vec3 rotation)
{
	_CameraPosition = position;
	_CameraEulerRotation = glm::radians(rotation);
	LocalToParent transform;
	transform.SetPosition(_CameraPosition);
	transform.SetEulerRotation(_CameraEulerRotation);
	_CameraEntity.SetComponentData(transform);
	_SemanticMaskCameraEntity.SetComponentData(transform);
}

void ImageCollectionSystem::OnCreate()
{
	EntityArchetype archetype = EntityManager::CreateEntityArchetype("General", LocalToWorld(), LocalToParent());
	_CameraEntity = EntityManager::CreateEntity(archetype);
	LocalToParent transform;
	transform.SetPosition(_CameraPosition);
	transform.SetEulerRotation(_CameraEulerRotation);
	_CameraEntity.SetComponentData(transform);
	auto cameraComponent = std::make_unique<CameraComponent>();
	cameraComponent->ResizeResolution(_CaptureResolution, _CaptureResolution);
	cameraComponent->DrawSkyBox = false;
	cameraComponent->ClearColor = glm::vec3(1.0f);
	_CameraEntity.SetName("ImageCap Camera");
	_CameraEntity.SetPrivateComponent(std::move(cameraComponent));

	
	_SemanticMaskCameraEntity = EntityManager::CreateEntity(archetype);
	transform.SetPosition(_CameraPosition);
	transform.SetEulerRotation(_CameraEulerRotation);
	_SemanticMaskCameraEntity.SetComponentData(transform);
	cameraComponent = std::make_unique<CameraComponent>();
	cameraComponent->ResizeResolution(_TargetResolution, _TargetResolution);
	cameraComponent->DrawSkyBox = false;
	cameraComponent->ClearColor = glm::vec3(1.0f);
	_SemanticMaskCameraEntity.SetName("Semantic Mask Camera");
	_SemanticMaskCameraEntity.SetPrivateComponent(std::move(cameraComponent));

	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/2236927059_a18cdd9196.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/2289428141_c758f436a1.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/2814264828_bb3f9d7ca9.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/3397325268_dc6135c432.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/69498568_e43c0e8520.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/1122838735_bc116c7a7c.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/1123280110_dda3037a69.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/3837561150_9f786dc7e5.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/st-andrewgate-2_300px.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/winecentre.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/calle-2.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/calle-3.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/calle+3.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/MainStreet_t.jpg"));
	_BackgroundTextures.push_back(AssetManager::LoadTexture("../Resources/Textures/Street/st-andrewgate-2_300px.jpg"));

	
	_BackgroundMaterial = std::make_shared<Material>();
	_BackgroundMaterial->SetMaterialProperty("material.shininess", 32.0f);
	std::string vertShaderCode = std::string("#version 460 core\n")
		+ *Default::ShaderIncludes::Uniform +
		+"\n"
		+ FileIO::LoadFileAsString(FileIO::GetResourcePath("Shaders/Vertex/Standard.vert"));
	std::string fragShaderCode = std::string("#version 460 core\n")
		+ *Default::ShaderIncludes::Uniform
		+ "\n"
		+ FileIO::LoadFileAsString("../Resources/Shaders/Fragment/Background.frag");

	std::unique_ptr<GLShader> standardvert = std::make_unique<GLShader>(ShaderType::Vertex);
	standardvert->SetCode(&vertShaderCode);
	std::unique_ptr<GLShader> standardfrag = std::make_unique<GLShader>(ShaderType::Fragment);
	standardfrag->SetCode(&fragShaderCode);
	auto program = std::make_shared<GLProgram>();
	program->Attach(ShaderType::Vertex, standardvert.get());
	program->Attach(ShaderType::Fragment, standardfrag.get());
	program->Link();
	
	
	_BackgroundMaterial->SetProgram(program);
	_Background = EntityManager::CreateEntity(archetype);

	vertShaderCode = std::string("#version 460 core\n") +
		FileIO::LoadFileAsString(FileIO::GetResourcePath("Shaders/Vertex/TexturePassThrough.vert"));
	fragShaderCode = std::string("#version 460 core\n") +
		FileIO::LoadFileAsString("../Resources/Shaders/Fragment/SmallBranch.frag");
	standardvert = std::make_unique<GLShader>(ShaderType::Vertex);
	standardvert->SetCode(&vertShaderCode);
	standardfrag = std::make_unique<GLShader>(ShaderType::Fragment);
	standardfrag->SetCode(&fragShaderCode);

	_SmallBranchProgram = std::make_unique<GLProgram>();
	_SmallBranchProgram->Attach(ShaderType::Vertex, standardvert.get());
	_SmallBranchProgram->Attach(ShaderType::Fragment, standardfrag.get());
	_SmallBranchProgram->Link();

	fragShaderCode = std::string("#version 460 core\n") +
		FileIO::LoadFileAsString("../Resources/Shaders/Fragment/SmallBranchCopy.frag");
	standardvert = std::make_unique<GLShader>(ShaderType::Vertex);
	standardvert->SetCode(&vertShaderCode);
	standardfrag = std::make_unique<GLShader>(ShaderType::Fragment);
	standardfrag->SetCode(&fragShaderCode);

	_SmallBranchCopyProgram = std::make_unique<GLProgram>();
	_SmallBranchCopyProgram->Attach(ShaderType::Vertex, standardvert.get());
	_SmallBranchCopyProgram->Attach(ShaderType::Fragment, standardfrag.get());
	_SmallBranchCopyProgram->Link();

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
	transform.SetPosition(glm::vec3(0, 17, -13));
	transform.SetEulerRotation(glm::radians(glm::vec3(75, -0, -180)));
	transform.SetScale(glm::vec3(30, 1, 30));
	_Background.SetComponentData(transform);
	_Background.SetPrivateComponent(std::move(mmr));
	_Background.SetName("Background");
	Enable();
}

void ImageCollectionSystem::SetPlantSimulationSystem(PlantSimulationSystem* value)
{
	_PlantSimulationSystem = value;
}

void ImageCollectionSystem::Update()
{
	std::string path;
	switch (_Status)
	{
		case ImageCollectionSystemStatus::Idle:
			if (!_ImageCaptureSequences.empty() && _StartIndex <= _EndIndex)
			{
				auto& imageCaptureSequence = _ImageCaptureSequences[_CurrentSelectedSequenceIndex].first;
				auto& treeParameters = _ImageCaptureSequences[_CurrentSelectedSequenceIndex].second;
				SetCameraPose(imageCaptureSequence.CameraPos, imageCaptureSequence.CameraEulerDegreeRot);
				treeParameters = _PlantSimulationSystem->LoadParameters(imageCaptureSequence.ParamPath);
				treeParameters.Seed = _StartIndex + (_IsTrain ? 0 : 9999);
				_CurrentTree = _PlantSimulationSystem->CreateTree(treeParameters, glm::vec3(0.0f));
				_TreeParametersOutputList.push_back(treeParameters);
				_Status = ImageCollectionSystemStatus::Growing;
			}else if(_Export)
			{
				_Export = false;
				_PlantSimulationSystem->ExportTreeParametersAsCsv(_StorePath + "params_" + (_IsTrain ? "train" : "val"), _TreeParametersOutputList);
				_TreeParametersOutputList.clear();
				const double spentTime = Application::EngineTime() - _Timer;
				Debug::Log("Generation Finished. Used time: " + std::to_string(spentTime));
			}
			break;
		case ImageCollectionSystemStatus::Growing:
			if (!_PlantSimulationSystem->_Growing)
			{
				TreeManager::GenerateSimpleMeshForTree(_CurrentTree, 0.1f, 1.0);
				_Status = ImageCollectionSystemStatus::Rendering;
				_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
			}
			break;
		case ImageCollectionSystemStatus::Rendering:
			_Status = ImageCollectionSystemStatus::CaptureOriginal;
			break;
		case ImageCollectionSystemStatus::CaptureOriginal:
			path = _StorePath + "white_" + (_IsTrain ? "train/" : "val/") +
				std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
				+ "_" + _ImageCaptureSequences[_CurrentSelectedSequenceIndex].first.Name
				+ "_" + std::to_string(_ImageCaptureSequences[_CurrentSelectedSequenceIndex].second.Seed)
				+ ".jpg";
			_CameraEntity.GetPrivateComponent<CameraComponent>()->GetCamera()->StoreToJpg(
				path, _TargetResolution, _TargetResolution);
		
			_Status = ImageCollectionSystemStatus::CaptureRandom;
			_BackgroundMaterial->SetTexture(_BackgroundTextures[glm::linearRand((size_t)0, _BackgroundTextures.size() - 1)], TextureType::DIFFUSE);
			_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(true);
			break;
		case ImageCollectionSystemStatus::CaptureRandom:
			path = _StorePath + "rgb_" + (_IsTrain ? "train/" : "val/") +
				std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
				+ "_" + _ImageCaptureSequences[_CurrentSelectedSequenceIndex].first.Name
				+ "_" + std::to_string(_ImageCaptureSequences[_CurrentSelectedSequenceIndex].second.Seed)
				+ ".jpg";
			_CameraEntity.GetPrivateComponent<CameraComponent>()->GetCamera()->StoreToJpg(
				path, _TargetResolution, _TargetResolution);
		
			_Status = ImageCollectionSystemStatus::CaptureSemantic;
			_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
			EnableSemantic();
			break;
		case ImageCollectionSystemStatus::CaptureSemantic:
			if (true) {
				_SmallBranchFilter->Bind();
				_SmallBranchProgram->Bind();
				_SmallBranchFilter->GetFrameBuffer()->DrawBuffer(GL_COLOR_ATTACHMENT0);
				_SmallBranchFilter->AttachTexture(_SmallBranchBuffer.get(), GL_COLOR_ATTACHMENT0);
				_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->GetCamera()->GetTexture()->Bind(0);
				_SmallBranchProgram->SetInt("InputTex", 0);
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				Default::GLPrograms::ScreenVAO->Bind();
				glDrawArrays(GL_TRIANGLES, 0, 6);

				_SmallBranchCopyProgram->Bind();
				_SmallBranchFilter->AttachTexture(_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->GetCamera()->GetTexture(), GL_COLOR_ATTACHMENT0);
				_SmallBranchBuffer->Bind(0);
				_SmallBranchCopyProgram->SetInt("InputTex", 0);
				Default::GLPrograms::ScreenVAO->Bind();
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
			path = _StorePath + "mask_" + (_IsTrain ? "train/" : "val/") +
				std::string(5 - std::to_string(_Counter).length(), '0') + std::to_string(_Counter)
				+ "_" + _ImageCaptureSequences[_CurrentSelectedSequenceIndex].first.Name
				+ "_" + std::to_string(_ImageCaptureSequences[_CurrentSelectedSequenceIndex].second.Seed)
				+ ".png";
			_SemanticMaskCameraEntity.GetPrivateComponent<CameraComponent>()->GetCamera()->StoreToPng(
				path);
			TreeManager::DeleteAllTrees();
			_CurrentSelectedSequenceIndex++;
			_CurrentSelectedSequenceIndex %= _ImageCaptureSequences.size();
			if (_CurrentSelectedSequenceIndex == 0) _StartIndex++;
			_Counter++;
			_Status = ImageCollectionSystemStatus::Idle;
			break;
	}
}

void ImageCollectionSystem::EnableSemantic()
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
	try {
		auto& branchletRenderer = foliageEntity.GetPrivateComponent<MeshRenderer>();
		branchletRenderer->ForwardRendering = true;
		branchletRenderer->Material = TreeManager::SemanticTreeBranchMaterial;
	}catch (int e){}
	
	auto& leavesRenderer = foliageEntity.GetPrivateComponent<Particles>();
	leavesRenderer->ForwardRendering = true;
	leavesRenderer->Material = TreeManager::SemanticTreeLeafMaterial;
	
	auto& branchRenderer = _CurrentTree.GetPrivateComponent<MeshRenderer>();
	branchRenderer->ForwardRendering = true;
	branchRenderer->Material = TreeManager::SemanticTreeBranchMaterial;
	
}
