#include "ImageCollectionSystem.h"

void ImageCollectionSystem::PushImageCaptureSequence(ImageCaptureSequence sequence)
{
	_ImageCaptureSequences.push(sequence);
	_Capturing = true;
}

void ImageCollectionSystem::SetCameraPose(glm::vec3 position, glm::vec3 rotation)
{
	_CameraPosition = position;
	_CameraEulerRotation = glm::radians(rotation);
	LocalToWorld ltw;
	ltw.SetPosition(_CameraPosition);
	ltw.SetEulerRotation(_CameraEulerRotation);
	_CameraEntity.SetComponentData(ltw);
}

void ImageCollectionSystem::OnCreate()
{
	EntityArchetype archetype = EntityManager::CreateEntityArchetype("General", LocalToWorld(), LocalToParent());
	_CameraEntity = EntityManager::CreateEntity(archetype);
	LocalToWorld ltw;
	ltw.SetPosition(_CameraPosition);
	ltw.SetEulerRotation(_CameraEulerRotation);
	_CameraEntity.SetComponentData(ltw);
	auto cameraComponent = std::make_unique<CameraComponent>();
	cameraComponent->ResizeResolution(960, 960);
	cameraComponent->DrawSkyBox = false;
	cameraComponent->ClearColor = glm::vec3(1.0f);

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
	
	_CameraEntity.SetName("ImageCap Camera");
	_CameraEntity.SetPrivateComponent(std::move(cameraComponent));
	_BackgroundMaterial = std::make_shared<Material>();
	_BackgroundMaterial->SetMaterialProperty("material.shininess", 32.0f);
	_BackgroundMaterial->SetProgram(Default::GLPrograms::StandardProgram);
	_Background = EntityManager::CreateEntity(archetype);
	auto mmr = std::make_unique<MeshRenderer>();
	mmr->Mesh = Default::Primitives::Quad;
	mmr->ForwardRendering = true;
	mmr->ReceiveShadow = false;
	mmr->CastShadow = false;
	mmr->Material = _BackgroundMaterial;
	ltw.SetPosition(glm::vec3(0, 17, -13));
	ltw.SetEulerRotation(glm::radians(glm::vec3(75, -0, -180)));
	ltw.SetScale(glm::vec3(30, 1, 30));
	_Background.SetComponentData(ltw);
	_Background.SetPrivateComponent(std::move(mmr));
	Enable();
}

void ImageCollectionSystem::AttachToPlantSimulationSystem(PlantSimulationSystem* value)
{
	_PlantSimulationSystem = value;
}

void ImageCollectionSystem::Update()
{
	if(_Capturing)
	{
		_Capturing = false;
		if (_Running) {
			if(_EnableSemanticMask)_CameraEntity.GetPrivateComponent<CameraComponent>()->get()->GetCamera()->StoreToPng(_StorePath + std::to_string(_RemainingAmount) + "_" + (_EnableRandomBackground ? "rand" : "") + (_EnableSemanticMask ? "sem" : "") + ".png");
			else _CameraEntity.GetPrivateComponent<CameraComponent>()->get()->GetCamera()->StoreToJpg(_StorePath + std::to_string(_RemainingAmount) + "_" + (_EnableRandomBackground ? "rand" : "") + (_EnableSemanticMask ? "sem" : "") + ".jpg", (_EnableSemanticMask ? -1 : 320), (_EnableSemanticMask ? -1 : 320));
			TreeManager::DeleteAllTrees();
			_RemainingAmount--;
		}
		if (_RemainingAmount != 0) {
			_CurrentTreeParameters.Seed = _RemainingAmount;
			_CurrentTree = _PlantSimulationSystem->CreateTree(_CurrentTreeParameters, glm::vec3(0.0f));
			if(_EnableSemanticMask)
			{
				TreeInfo treeInfo = _CurrentTree.GetComponentData<TreeInfo>();
				treeInfo.EnableSemanticOutput = true;
				_CurrentTree.SetComponentData(treeInfo);
				auto mmc = _CurrentTree.GetPrivateComponent<MeshRenderer>();
				mmc->get()->ForwardRendering = true;
				mmc->get()->Material = TreeManager::SemanticTreeBranchMaterial;
				auto p = _CurrentTree.GetPrivateComponent<Particles>();
				p->get()->ForwardRendering = true;
				p->get()->Material = TreeManager::SemanticTreeLeafMaterial;
			}
			if(_EnableRandomBackground)
			{
				size_t index = glm::linearRand((size_t)0, _BackgroundTextures.size() - 1);
				_BackgroundMaterial->SetTexture(_BackgroundTextures[index], TextureType::DIFFUSE);
				_Background.SetEnabled(true);
				
			}else
			{
				_Background.SetEnabled(false);
			}
		}
		else {
			if (!_ImageCaptureSequences.empty())
			{
				ImageCaptureSequence seq = _ImageCaptureSequences.front();
				_ImageCaptureSequences.pop();
				_RemainingAmount = seq.Amount;
				SetCameraPose(seq.CameraPos, seq.CameraEulerDegreeRot);
				_CurrentTreeParameters = _PlantSimulationSystem->LoadParameters(seq.ParamPath);
				_StorePath = seq.OutputPath;
				_CurrentTreeParameters.Seed = _RemainingAmount;
				_EnableSemanticMask = seq.EnableSemanticOutput;
				_EnableRandomBackground = seq.EnableRandomBackground;
				_CurrentTree = _PlantSimulationSystem->CreateTree(_CurrentTreeParameters, glm::vec3(0.0f));
				if (_EnableSemanticMask)
				{
					TreeInfo treeInfo = _CurrentTree.GetComponentData<TreeInfo>();
					treeInfo.EnableSemanticOutput = true;
					_CurrentTree.SetComponentData(treeInfo);
					auto mmc = _CurrentTree.GetPrivateComponent<MeshRenderer>();
					mmc->get()->ForwardRendering = true;
					mmc->get()->Material = TreeManager::SemanticTreeBranchMaterial;
					auto p = _CurrentTree.GetPrivateComponent<Particles>();
					p->get()->ForwardRendering = true;
					p->get()->Material = TreeManager::SemanticTreeLeafMaterial;
				}
				if (_EnableRandomBackground)
				{
					size_t index = glm::linearRand((size_t)0, _BackgroundTextures.size() - 1);
					std::cout << index << std::endl;
					_BackgroundMaterial->SetTexture(_BackgroundTextures[index], TextureType::DIFFUSE);
					_Background.SetEnabled(true);
				}
				else
				{
					_Background.SetEnabled(false);
				}
				_Running = true;
			}
			else
			{
				_Running = false;
			}
		}
	}
	else if(_RemainingAmount != 0)
	{
		if(!_PlantSimulationSystem->_Growing)
		{
			TreeManager::GenerateSimpleMeshForTree(_CurrentTree, 0.01f, 1.0);
			if(_EnableSemanticMask)
			{
				_CameraEntity.GetPrivateComponent<CameraComponent>()->get()->ResizeResolution(320, 320);
			}else
			{
				_CameraEntity.GetPrivateComponent<CameraComponent>()->get()->ResizeResolution(320, 320);
			}
			_Capturing = true;
		}
	}
}
