#include <direct.h>
#include <iostream>

#include "UniEngine.h"
#include "CameraControlSystem.h"
#include "PlantSimulationSystem.h"
#include "TreeManager.h"
#include "SorghumReconstructionSystem.h"
#include "DataCollectionSystem.h"
#include "TreeReconstructionSystem.h"
#include "TreeCollectionGenerationSystem.h"
#include "Bloom.h"
#include "SSAO.h"

#include "GreyScale.h"
using namespace UniEngine;
using namespace TreeUtilities;
using namespace SorghumReconstruction;
Entity InitGround();
PlantSimulationSystem* InitPlantSimulationSystem();
DataCollectionSystem* InitImageCollectionSystem();
TreeReconstructionSystem* InitTreeReconstructionSystem();
SorghumReconstructionSystem* InitSorghumReconstructionSystem();
TreeCollectionGenerationSystem* InitTreeCollectionGenerationSystem();
void EngineSetup();
void main()
{
	EngineSetup();
	Transform transform;
	
#pragma region Lights
	RenderManager::SetAmbientLight(0.1f);
	float brightness = 5.0f;
	auto dlc = std::make_unique<DirectionalLight>();
	dlc->diffuse = glm::normalize(glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0));
	dlc->diffuseBrightness = brightness / 2.0f;
	dlc->bias = 0.3f;
	dlc->normalOffset = 0.01f;
	dlc->lightSize = 1.0;
	dlc->CastShadow = true;
	auto dlc1 = std::make_unique<DirectionalLight>();
	dlc1->diffuse = glm::normalize(glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0));
	dlc1->diffuseBrightness = brightness / 3.0f;
	dlc1->bias = 0.3f;
	dlc1->normalOffset = 0.01f;
	dlc1->lightSize = 1.0;
	dlc->CastShadow = true;
	auto dlc2 = std::make_unique<DirectionalLight>();
	dlc2->diffuse = glm::normalize(glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0));
	dlc2->diffuseBrightness = brightness / 3.0f;
	dlc2->bias = 0.3f;
	dlc2->normalOffset = 0.01f;
	dlc2->lightSize = 1.0;
	dlc->CastShadow = true;
	auto dlc3 = std::make_unique<DirectionalLight>();
	dlc3->diffuse = glm::normalize(glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0));
	dlc3->diffuseBrightness = brightness / 8.0f;
	dlc3->bias = 0.3f;
	dlc3->normalOffset = 0.01f;
	dlc3->lightSize = 1.0;
	dlc->CastShadow = true;
	
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
	bool generateSorghum = false;
	bool generateSorghumField = true;
	PlantSimulationSystem* pss = InitPlantSimulationSystem();
	DataCollectionSystem* dcs = InitImageCollectionSystem();
	TreeReconstructionSystem* trs = InitTreeReconstructionSystem();
	TreeCollectionGenerationSystem* tcgs = InitTreeCollectionGenerationSystem();
	dcs->SetPlantSimulationSystem(pss);
	trs->SetPlantSimulationSystem(pss);
	dcs->SetDirectionalLightEntity(dle, dle1, dle2, dle3);
	trs->SetDataCollectionSystem(dcs);
	tcgs->SetDataCollectionSystem(dcs);
	//tcgs->ImportCsv("./parameters.csv");
	Entity ground = InitGround();
	ground.SetEnabled(false);
	tcgs->SetGroundEntity(ground);
	if (generateSorghum) {
		auto srSys = InitSorghumReconstructionSystem();
		Entity plant1 = srSys->ImportPlant("skeleton_procedural_1.txt", 0.01f, "Sorghum 1");
		Entity plant2 = srSys->ImportPlant("skeleton_procedural_2.txt", 0.01f, "Sorghum 2");
		Entity plant3 = srSys->ImportPlant("skeleton_procedural_3.txt", 0.01f, "Sorghum 3");
		Entity plant4 = srSys->ImportPlant("skeleton_procedural_4.txt", 0.01f, "Sorghum 4");
		srSys->GenerateMeshForAllPlants(2, 3);
		srSys->ExportPlant(plant1, "plant1");
		srSys->ExportPlant(plant2, "plant2");
		srSys->ExportPlant(plant3, "plant3");
		srSys->ExportPlant(plant4, "plant4");
		if (generateSorghumField) {
			glm::vec2 radius = glm::vec2(53, 12);
			glm::vec2 size = glm::vec2(0.565f, 2.7f);
			std::vector<std::vector<glm::mat4>> matricesList;
			matricesList.resize(4);
			for (auto& i : matricesList)
			{
				i.clear();
			}
			float xStep = -radius.x * size.x * 2;

			for (int i = -radius.x; i <= radius.x; i++)
			{
				if (i % 18 == 0) xStep += 2.0f * glm::gaussRand(1.0f, 0.2f);
				xStep += size.x * 2 * glm::gaussRand(1.0f, 0.5f);
				float yStep = -radius.y * size.y * 2;
				for (int j = -radius.y; j <= radius.y; j++)
				{
					int index = glm::linearRand(0, (int)matricesList.size() - 1);
					glm::vec3 translation;
					glm::quat rotation;
					float angle = glm::gaussRand(35.0, 4.0);
					float change = 10.0f;
					switch (index)
					{
					case 0:
						angle = glm::gaussRand(42.0f, 5.0f) + glm::linearRand(-change, change);
						break;
					case 1:
						angle = glm::gaussRand(30.0f, 5.0f) + glm::linearRand(-change, change);
						break;
					case 2:
						angle = glm::gaussRand(40.0f, 5.0f) + glm::linearRand(-change, change);
						break;
					case 3:
						angle = glm::gaussRand(-110.0f, 5.0f) + glm::linearRand(-change, change);
						break;
					default:
						break;
					}
					float sway = 5.0f;
					rotation = glm::quat(glm::vec3(glm::radians(-90.0f + glm::linearRand(-sway, sway)), glm::radians(angle), glm::radians(glm::linearRand(-sway, sway))));
					translation = glm::vec3(xStep, 0.0f, yStep) + glm::gaussRand(glm::vec3(0.0f), glm::vec3(0.02f));
					yStep += size.y * 2 * glm::gaussRand(1.0f, 0.05f);
					glm::vec3 scale;
					scale = glm::vec3(1.0f, 1.0f, 1.0f) * glm::gaussRand(1.0f, 0.05f);
					glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(scale);
					matricesList[index].push_back(transform);
				}

			}

			Entity gridPlant1 = srSys->CreateGridPlant(plant1, matricesList[0]);
			gridPlant1.SetName("Grid 1");
			Entity gridPlant2 = srSys->CreateGridPlant(plant2, matricesList[1]);
			gridPlant2.SetName("Grid 2");
			Entity gridPlant3 = srSys->CreateGridPlant(plant3, matricesList[2]);
			gridPlant3.SetName("Grid 3");
			Entity gridPlant4 = srSys->CreateGridPlant(plant4, matricesList[3]);
			gridPlant4.SetName("Grid 4");
		}
		Transform t1;
		Transform t2;
		Transform t3;
		Transform t4;

		t1.SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
		t2.SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
		t3.SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
		t4.SetScale(glm::vec3(1.0f, 1.0f, 1.0f));

		t1.SetPosition(glm::vec3(-5.0f, 0.0, 0));
		t2.SetPosition(glm::vec3(0.0f, 0.0, 0));
		t3.SetPosition(glm::vec3(5.0f, 0.0, 0));
		t4.SetPosition(glm::vec3(10.0f, 0.0, 0));

		t1.SetEulerRotation(glm::vec3(glm::radians(-90.0f), glm::radians(42.0f), 0));
		t2.SetEulerRotation(glm::vec3(glm::radians(-90.0f), glm::radians(30.0f), 0));
		t3.SetEulerRotation(glm::vec3(glm::radians(-90.0f), glm::radians(40.0f), 0));
		t4.SetEulerRotation(glm::vec3(glm::radians(-90.0f), glm::radians(-110.0f), 0));
		EntityManager::SetComponentData(plant1, t1);

		EntityManager::SetComponentData(plant2, t2);

		EntityManager::SetComponentData(plant3, t3);

		EntityManager::SetComponentData(plant4, t4);
	}
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

SorghumReconstructionSystem* InitSorghumReconstructionSystem()
{
	return Application::GetCurrentWorld()->CreateSystem<SorghumReconstructionSystem>(SystemGroup::SimulationSystemGroup);
}

TreeCollectionGenerationSystem* InitTreeCollectionGenerationSystem()
{
	return Application::GetCurrentWorld()->CreateSystem<TreeCollectionGenerationSystem>(SystemGroup::SimulationSystemGroup);
}

void EngineSetup()
{
#pragma region Engine Setup
#pragma region Global light settings
	RenderManager::SetPCSSScaleFactor(1.0f);
	RenderManager::SetAmbientLight(0.4f);
	RenderManager::SetShadowMapResolution(8192);
	RenderManager::SetStableFit(false);
	RenderManager::SetSeamFixRatio(0.05f);
	RenderManager::SetMaxShadowDistance(100);
	RenderManager::SetSplitRatio(0.15f, 0.3f, 0.5f, 1.0f);
#pragma endregion
	FileIO::SetProjectPath("../Resources/");
	FileIO::SetResourcePath("../Submodules/UniEngine/Resources/");
	Application::Init();
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
		mainCamera->DrawSkyBox = false;
		mainCamera->ClearColor = glm::vec3(1.0f);
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
	const auto textureD = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/leafy-grass2-bl/leafy-grass2-albedo.png", TextureType::ALBEDO);
	//mat->SetTexture(Default::Textures::StandardTexture);
	const auto textureN = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/leafy-grass2-bl/leafy-grass2-normal-ogl.png", TextureType::NORMAL);
	//mat->SetTexture(textureN);
	const auto textureH = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/leafy-grass2-bl/leafy-grass2-height.png", TextureType::DISPLACEMENT);
	//mat->SetTexture(textureH);
	const auto textureA = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/leafy-grass2-bl/leafy-grass2-ao.png", TextureType::AO);
	//mat->SetTexture(textureA);
	const auto textureM = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/leafy-grass2-bl/leafy-grass2-metallic.png", TextureType::METALLIC);
	//mat->SetTexture(textureM);
	const auto textureR = ResourceManager::LoadTexture(false, FileIO::GetAssetFolderPath() + "Textures/leafy-grass2-bl/leafy-grass2-roughness.png", TextureType::ROUGHNESS);
	//mat->SetTexture(textureR);
	mat->Shininess = 32.0f;
	auto meshMaterial = std::make_unique<MeshRenderer>();
	meshMaterial->Mesh = Default::Primitives::Quad;
	meshMaterial->Material = mat;
	meshMaterial->ReceiveShadow = true;
	meshMaterial->ForwardRendering = false;
	meshMaterial->Material->DisplacementMapScale = -0.02f;
	meshMaterial->Material->Metallic = 0.0f;
	meshMaterial->Material->Roughness = 0.0f;
	meshMaterial->Material->AmbientOcclusion = 2.0f;
	EntityManager::SetPrivateComponent<MeshRenderer>(entity, std::move(meshMaterial));
	return entity;
}


#pragma endregion

