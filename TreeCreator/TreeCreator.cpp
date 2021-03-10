#include <direct.h>
#include <iostream>

#include "UniEngine.h"
#include "CameraControlSystem.h"
#include "PlantSimulationSystem.h"
#include "TreeManager.h"
#include "DataCollectionSystem.h"
#include "TreeReconstructionSystem.h"
#include "TreeCollectionGenerationSystem.h"
#include "RealTreeReconstructionSystem.h"
#include "Bloom.h"
#include "SSAO.h"

#include "GreyScale.h"
using namespace UniEngine;
using namespace TreeUtilities;
Entity InitGround();
PlantSimulationSystem* InitPlantSimulationSystem();
DataCollectionSystem* InitImageCollectionSystem();
TreeReconstructionSystem* InitTreeReconstructionSystem();
TreeCollectionGenerationSystem* InitTreeCollectionGenerationSystem();
RealTreeReconstructionSystem* InitRealTreeReconstructionSystem();
void EngineSetup();
void main()
{
	EngineSetup();
	Transform transform;
	
#pragma region Lights
	RenderManager::GetInstance().m_lightSettings.m_ambientLight = (0.1f);
	float brightness = 5.0f;
	auto dlc = std::make_unique<DirectionalLight>();
	dlc->m_diffuse = glm::normalize(glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0));
	dlc->m_diffuseBrightness = brightness / 2.0f;
	dlc->m_bias = 0.3f;
	dlc->m_normalOffset = 0.01f;
	dlc->m_lightSize = 1.0;
	dlc->m_castShadow = true;
	auto dlc1 = std::make_unique<DirectionalLight>();
	dlc1->m_diffuse = glm::normalize(glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0));
	dlc1->m_diffuseBrightness = brightness / 3.0f;
	dlc1->m_bias = 0.3f;
	dlc1->m_normalOffset = 0.01f;
	dlc1->m_lightSize = 1.0;
	dlc->m_castShadow = true;
	auto dlc2 = std::make_unique<DirectionalLight>();
	dlc2->m_diffuse = glm::normalize(glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0));
	dlc2->m_diffuseBrightness = brightness / 3.0f;
	dlc2->m_bias = 0.3f;
	dlc2->m_normalOffset = 0.01f;
	dlc2->m_lightSize = 1.0;
	dlc->m_castShadow = true;
	auto dlc3 = std::make_unique<DirectionalLight>();
	dlc3->m_diffuse = glm::normalize(glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0));
	dlc3->m_diffuseBrightness = brightness / 8.0f;
	dlc3->m_bias = 0.3f;
	dlc3->m_normalOffset = 0.01f;
	dlc3->m_lightSize = 1.0;
	dlc->m_castShadow = true;
	
	float angle = 30;
	transform.SetEulerRotation(glm::radians(glm::vec3(150, angle, 0)));
	Entity dle = EntityManager::CreateEntity("Directional Light Main");
	Entity dle1 = EntityManager::CreateEntity("Directional Light Left");
	Entity dle2 = EntityManager::CreateEntity("Directional Light Right");
	Entity dle3 = EntityManager::CreateEntity("Directional Light Back");
	EntityManager::SetPrivateComponent(dle, std::move(dlc));
	EntityManager::SetPrivateComponent(dle1, std::move(dlc1));
	EntityManager::SetPrivateComponent(dle2, std::move(dlc2));
	EntityManager::SetPrivateComponent(dle3, std::move(dlc3));
	EntityManager::SetComponentData(dle, transform);
	transform.SetEulerRotation(glm::radians(glm::vec3(150, angle - 35, 0)));
	EntityManager::SetComponentData(dle1, transform);
	transform.SetEulerRotation(glm::radians(glm::vec3(150, angle + 35, 0)));
	EntityManager::SetComponentData(dle2, transform);
	transform.SetEulerRotation(glm::radians(glm::vec3(150, -angle, 0)));
	EntityManager::SetComponentData(dle3, transform);
	
#pragma endregion
	PlantSimulationSystem* pss = InitPlantSimulationSystem();
	DataCollectionSystem* dcs = InitImageCollectionSystem();
	TreeReconstructionSystem* trs = InitTreeReconstructionSystem();
	TreeCollectionGenerationSystem* tcgs = InitTreeCollectionGenerationSystem();
	RealTreeReconstructionSystem* rtr = InitRealTreeReconstructionSystem();
	dcs->SetPlantSimulationSystem(pss);
	trs->SetPlantSimulationSystem(pss);
	dcs->SetDirectionalLightEntity(dle, dle1, dle2, dle3);
	trs->SetDataCollectionSystem(dcs);
	tcgs->SetDataCollectionSystem(dcs);
	rtr->AttachDataCollectionSystem(dcs, pss, trs);
	//tcgs->ImportCsv("./parameters.csv");
	Entity ground = InitGround();
	ground.SetEnabled(false);
	tcgs->SetGroundEntity(ground);
#pragma region Engine Loop
	Application::Run();
#pragma endregion
	Application::End();
}
#pragma region Helpers
PlantSimulationSystem* InitPlantSimulationSystem() {
	return Application::GetCurrentWorld()->CreateSystem<PlantSimulationSystem>(SystemGroup::SimulationSystemGroup);
}

DataCollectionSystem* InitImageCollectionSystem()
{
	return Application::GetCurrentWorld()->CreateSystem<DataCollectionSystem>(SystemGroup::SimulationSystemGroup);
}

TreeReconstructionSystem* InitTreeReconstructionSystem()
{
	return Application::GetCurrentWorld()->CreateSystem<TreeReconstructionSystem>(SystemGroup::SimulationSystemGroup);
}


TreeCollectionGenerationSystem* InitTreeCollectionGenerationSystem()
{
	return Application::GetCurrentWorld()->CreateSystem<TreeCollectionGenerationSystem>(SystemGroup::SimulationSystemGroup);
}

RealTreeReconstructionSystem* InitRealTreeReconstructionSystem()
{
	return Application::GetCurrentWorld()->CreateSystem<RealTreeReconstructionSystem>(SystemGroup::SimulationSystemGroup);
}

void EngineSetup()
{
	FileIO::SetProjectPath("../Resources/");
	FileIO::SetResourcePath("../Submodules/UniEngine/Resources/");
	Application::Init();
#pragma region Engine Setup
#pragma region Global light settings
	RenderManager::GetInstance().m_lightSettings.m_scaleFactor = 1.0f;
	RenderManager::SetShadowMapResolution(8192);
	RenderManager::GetInstance().m_stableFit = false;
	RenderManager::GetInstance().m_lightSettings.m_seamFixRatio = (0.05f);
	RenderManager::GetInstance().m_maxShadowDistance = (100);
	RenderManager::SetSplitRatio(0.15f, 0.3f, 0.5f, 1.0f);
#pragma endregion
	
	Transform transform;
	transform.SetEulerRotation(glm::radians(glm::vec3(150, 30, 0)));

#pragma region Preparations
	Application::SetTimeStep(0.016f);
	auto& world = Application::GetCurrentWorld();
	WorldTime* time = world->Time();

	EntityArchetype archetype = EntityManager::CreateEntityArchetype("General", GlobalTransform(), Transform());
	const bool enableCameraControl = true;
	if (enableCameraControl) {
		auto* ccs = world->CreateSystem<CameraControlSystem>(SystemGroup::SimulationSystemGroup);
		ccs->Enable();
		ccs->SetVelocity(15.0f);
	}
	transform = Transform();
	transform.SetPosition(glm::vec3(0, 2, 35));
	transform.SetEulerRotation(glm::radians(glm::vec3(15, 0, 0)));
	auto mainCamera = RenderManager::GetMainCamera();
	if (mainCamera) {
		mainCamera->GetOwner().SetComponentData(transform);
		mainCamera->m_drawSkyBox = false;
		mainCamera->m_clearColor = glm::vec3(1.0f);
		auto postProcessing = std::make_unique<PostProcessing>();
		
		postProcessing->PushLayer(std::make_unique<Bloom>());
		postProcessing->PushLayer(std::make_unique<SSAO>());
		
		mainCamera->GetOwner().SetPrivateComponent(std::move(postProcessing));
	}

#pragma endregion
	TreeManager::Init();
#pragma region Light estimator setup
	//The smaller the branch node's size, the more branching for tree.
	TreeManager::GetLightEstimator()->ResetResolution(512);
	TreeManager::GetLightEstimator()->ResetCenterDistance(60);
	TreeManager::GetLightEstimator()->ResetSnapShotWidth(30);
	//From top
	TreeManager::GetLightEstimator()->PushSnapShot(glm::vec3(0, -1, 0), 1.0f);

	//45
	float tilt = 0.2f;
	TreeManager::GetLightEstimator()->PushSnapShot(glm::vec3(0, -1, tilt), 0.9f);
	TreeManager::GetLightEstimator()->PushSnapShot(glm::vec3(0, -1, -tilt), 0.9f);
	TreeManager::GetLightEstimator()->PushSnapShot(glm::vec3(tilt, -1, 0), 0.9f);
	TreeManager::GetLightEstimator()->PushSnapShot(glm::vec3(-tilt, -1, 0), 0.9f);

	tilt = 1.0f;
	TreeManager::GetLightEstimator()->PushSnapShot(glm::vec3(0, -1, tilt), 0.5f);
	TreeManager::GetLightEstimator()->PushSnapShot(glm::vec3(0, -1, -tilt), 0.5f);
	TreeManager::GetLightEstimator()->PushSnapShot(glm::vec3(tilt, -1, 0), 0.5f);
	TreeManager::GetLightEstimator()->PushSnapShot(glm::vec3(-tilt, -1, 0), 0.5f);

	tilt = 10.0f;
	TreeManager::GetLightEstimator()->PushSnapShot(glm::vec3(0, -1, tilt), 0.1f);
	TreeManager::GetLightEstimator()->PushSnapShot(glm::vec3(0, -1, -tilt), 0.1f);
	TreeManager::GetLightEstimator()->PushSnapShot(glm::vec3(tilt, -1, 0), 0.1f);
	TreeManager::GetLightEstimator()->PushSnapShot(glm::vec3(-tilt, -1, 0), 0.1f);
#pragma endregion
#pragma endregion
}

Entity InitGround() {
	const auto entity = EntityManager::CreateEntity();
	entity.SetName("Ground");
	Transform transform;
	transform.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
	transform.SetScale(glm::vec3(40.0f, 40.0f, 40.0f));
	EntityManager::SetComponentData(entity, transform);

	auto mat = std::make_shared<Material>();
	mat->SetProgram(Default::GLPrograms::StandardProgram);
	const auto textureD = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/leafy-grass2-bl/leafy-grass2-albedo.png", TextureType::Albedo);
	//mat->SetTexture(Default::Textures::StandardTexture);
	const auto textureN = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/leafy-grass2-bl/leafy-grass2-normal-ogl.png", TextureType::Normal);
	//mat->SetTexture(textureN);
	const auto textureH = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/leafy-grass2-bl/leafy-grass2-height.png", TextureType::Displacement);
	//mat->SetTexture(textureH);
	const auto textureA = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/leafy-grass2-bl/leafy-grass2-ao.png", TextureType::Ao);
	//mat->SetTexture(textureA);
	const auto textureM = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/leafy-grass2-bl/leafy-grass2-metallic.png", TextureType::Metallic);
	//mat->SetTexture(textureM);
	const auto textureR = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/leafy-grass2-bl/leafy-grass2-roughness.png", TextureType::Roughness);
	//mat->SetTexture(textureR);
	mat->m_shininess = 32.0f;
	auto meshMaterial = std::make_unique<MeshRenderer>();
	meshMaterial->m_mesh = Default::Primitives::Quad;
	meshMaterial->m_material = mat;
	meshMaterial->m_receiveShadow = true;
	meshMaterial->m_forwardRendering = false;
	meshMaterial->m_material->m_displacementMapScale = -0.02f;
	meshMaterial->m_material->m_metallic = 0.0f;
	meshMaterial->m_material->m_roughness = 0.0f;
	meshMaterial->m_material->m_ambientOcclusion = 2.0f;
	EntityManager::SetPrivateComponent<MeshRenderer>(entity, std::move(meshMaterial));
	return entity;
}


#pragma endregion

