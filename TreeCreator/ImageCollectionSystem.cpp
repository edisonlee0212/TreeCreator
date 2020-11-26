#include "ImageCollectionSystem.h"

#include "AcaciaFoliageGenerator.h"
#include "CrownSurfaceRecon.h"
#include "MapleFoliageGenerator.h"
#include "PineFoliageGenerator.h"
#include "WillowFoliageGenerator.h"
#include "AppleFoliageGenerator.h"
#include "OakFoliageGenerator.h"
#include "BirchFoliageGenerator.h"
void ImageCollectionSystem::PushImageCaptureSequence(ImageCaptureSequence sequence)
{
	_ImageCaptureSequences.push(sequence);
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
	_Background.SetName("Background");
	Enable();
}

void ImageCollectionSystem::AttachToPlantSimulationSystem(PlantSimulationSystem* value)
{
	_PlantSimulationSystem = value;
}

void ImageCollectionSystem::Update()
{
	switch (_Status)
	{
		case ImageCollectionSystemStatus::Idle:
			if (!_ImageCaptureSequences.empty())
			{
				ImageCaptureSequence seq = _ImageCaptureSequences.front();
				_ImageCaptureSequences.pop();
				_RemainingAmount = seq.Amount;
				SetCameraPose(seq.CameraPos, seq.CameraEulerDegreeRot);
				_CurrentTreeParameters = _PlantSimulationSystem->LoadParameters(seq.ParamPath);
				_StorePath = seq.OutputPath;
				_CurrentTreeParameters.Seed = _RemainingAmount;
				_CurrentTree = _PlantSimulationSystem->CreateTree(_CurrentTreeParameters, glm::vec3(0.0f));
				_Status = ImageCollectionSystemStatus::Growing;
			}
			break;
		case ImageCollectionSystemStatus::Growing:
			if (!_PlantSimulationSystem->_Growing)
			{
				TreeManager::GenerateSimpleMeshForTree(_CurrentTree, 0.01f, 1.0);
				_CameraEntity.GetPrivateComponent<CameraComponent>()->ResizeResolution(960, 960);
				
				_Status = ImageCollectionSystemStatus::Rendering;
				_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
			}
			break;
		case ImageCollectionSystemStatus::Rendering:
			_Status = ImageCollectionSystemStatus::CaptureOriginal;
			break;
		case ImageCollectionSystemStatus::CaptureOriginal:
			_CameraEntity.GetPrivateComponent<CameraComponent>()->GetCamera()->StoreToJpg(
				_StorePath + 
				std::to_string(_CurrentTree.GetComponentData<TreeParameters>().FoliageType)
				+ "_"
				+ std::to_string(_RemainingAmount) + "_white" + ".jpg", 320, 320);
		
			_Status = ImageCollectionSystemStatus::CaptureRandom;
			_BackgroundMaterial->SetTexture(_BackgroundTextures[glm::linearRand((size_t)0, _BackgroundTextures.size() - 1)], TextureType::DIFFUSE);
			_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(true);
			break;
		case ImageCollectionSystemStatus::CaptureRandom:
			_CameraEntity.GetPrivateComponent<CameraComponent>()->GetCamera()->StoreToJpg(
				_StorePath +
				std::to_string(_CurrentTree.GetComponentData<TreeParameters>().FoliageType)
				+ "_"
				+ std::to_string(_RemainingAmount) + "_random" + ".jpg", 320, 320);
		
			_Status = ImageCollectionSystemStatus::CaptureSemantic;
			_Background.GetPrivateComponent<MeshRenderer>()->SetEnabled(false);
			_CameraEntity.GetPrivateComponent<CameraComponent>()->ResizeResolution(320, 320);
			EnableSemantic();
			break;
		case ImageCollectionSystemStatus::CaptureSemantic:
			_CameraEntity.GetPrivateComponent<CameraComponent>()->GetCamera()->StoreToPng(_StorePath +
				std::to_string(_CurrentTree.GetComponentData<TreeParameters>().FoliageType)
				+ "_"
				+ std::to_string(_RemainingAmount) + "_mask" + ".png");
			TreeManager::DeleteAllTrees();
			_RemainingAmount--;
			if (_RemainingAmount != 0) {
				_CurrentTreeParameters.Seed = _RemainingAmount;
				_CurrentTree = _PlantSimulationSystem->CreateTree(_CurrentTreeParameters, glm::vec3(0.0f));
				_Status = ImageCollectionSystemStatus::Growing;
			}else
			{
				_Status = ImageCollectionSystemStatus::Idle;
			}
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
	leavesRenderer->BackCulling = false;
	leavesRenderer->Material = TreeManager::SemanticTreeLeafMaterial;
	
	auto& branchRenderer = _CurrentTree.GetPrivateComponent<MeshRenderer>();
	branchRenderer->ForwardRendering = true;
	branchRenderer->Material = TreeManager::SemanticTreeBranchMaterial;
	
}
