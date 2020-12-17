#include <direct.h>
#include <iostream>

#include "UniEngine.h"
#include "CameraControlSystem.h"
#include "PlantSimulationSystem.h"
#include "TreeManager.h"
#include "SorghumReconstructionSystem.h"
#include "DataCollectionSystem.h"
using namespace UniEngine;
using namespace TreeUtilities;
using namespace SorghumReconstruction;
void InitGround();
PlantSimulationSystem* InitPlantSimulationSystem();
DataCollectionSystem* InitImageCollectionSystem();
SorghumReconstructionSystem* InitSorghumReconstructionSystem();
void EngineSetup();
void main()
{
	EngineSetup();

#pragma region Lights
	EntityArchetype lightArchetype = EntityManager::CreateEntityArchetype("Directional Light", DirectionalLight(), GlobalTransform(), Transform());
	DirectionalLight dlc;
	dlc.diffuse = glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0);
	dlc.diffuseBrightness = 1.0f;
	dlc.specularBrightness = 1.0f;
	dlc.bias = 0.1f;
	dlc.normalOffset = 0.001f;
	dlc.lightSize = 1.0;
	Transform transform;
	transform.SetEulerRotation(glm::radians(glm::vec3(150, 30, 0)));
	Entity dle = EntityManager::CreateEntity(lightArchetype, "Directional Light");
	EntityManager::SetComponentData(dle, dlc);
	EntityManager::SetComponentData(dle, transform);
#pragma endregion
	
	bool generateLearningData = false;
	bool generateSorghum = false;
	bool generateSorghumField = true;
	PlantSimulationSystem* pss = InitPlantSimulationSystem();
	if (generateLearningData) {
		RenderManager::SetAmbientLight(1.0f);
		dlc.diffuse = glm::vec3(253.0 / 256.0, 251.0 / 256.0, 211.0 / 256.0);
		dlc.diffuseBrightness = 0.5f;
		dlc.specularBrightness = 1.0f;
		dlc.bias = 0.1f;
		dlc.normalOffset = 0.001f;
		dlc.lightSize = 1.0;
		Transform lightTransform;
		lightTransform.SetEulerRotation(glm::radians(glm::vec3(150, 30, 0)));
		EntityManager::SetComponentData(dle, dlc);
		EntityManager::SetComponentData(dle, lightTransform);
		
		DataCollectionSystem* ics = InitImageCollectionSystem();
		ics->SetPlantSimulationSystem(pss);


		int counter = 0;
		int startIndex = 1;
		int endIndex = 100;

		ics->ResetCounter(counter, startIndex, endIndex, true);
		ics->SetIsTrain(true);
		bool eval = true;
		Application::RegisterUpdateFunction([&]()
			{
				if (eval && generateLearningData && !ics->IsExport())
				{
					ics->ResetCounter(counter, startIndex, endIndex, true);
					eval = false;
					ics->SetIsTrain(false);
				}
			}
		);
	}
	else
	{
		InitGround();

	}
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

SorghumReconstructionSystem* InitSorghumReconstructionSystem()
{
	return Application::GetCurrentWorld()->CreateSystem<SorghumReconstructionSystem>(SystemGroup::SimulationSystemGroup);
}

void EngineSetup()
{
#pragma region Engine Setup
#pragma region Global light settings
	RenderManager::SetPCSSScaleFactor(1.0f);
	RenderManager::SetSSAOKernelBias(0.08);
	RenderManager::SetSSAOKernelRadius(0.05f);
	RenderManager::SetSSAOSampleSize(32);
	RenderManager::SetAmbientLight(0.5f);
	RenderManager::SetSSAOFactor(10.0f);
	RenderManager::SetShadowMapResolution(4096);
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
	CameraControlSystem* ccs = world->CreateSystem<CameraControlSystem>(SystemGroup::SimulationSystemGroup);
	ccs->Enable();
	transform = Transform();
	transform.SetPosition(glm::vec3(0, 2, 35));
	transform.SetEulerRotation(glm::radians(glm::vec3(15, 0, 0)));
	auto mainCamera = RenderManager::GetMainCamera();
	if (mainCamera) {
		mainCamera->GetOwner().SetComponentData(transform);
		mainCamera->DrawSkyBox = false;
		mainCamera->ClearColor = glm::vec3(1.0f);
	}
	ccs->SetVelocity(15.0f);

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

void InitGround() {
	EntityArchetype archetype = EntityManager::CreateEntityArchetype("General", Transform(), GlobalTransform());
	auto entity = EntityManager::CreateEntity(archetype);
	entity.SetName("Ground");
	Transform transform;
	transform.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
	transform.SetScale(glm::vec3(40.0f, 20.0f, 20.0f));
	EntityManager::SetComponentData(entity, transform);

	auto mat = std::make_shared<Material>();
	mat->SetProgram(Default::GLPrograms::StandardProgram);
	auto textureD = ResourceManager::LoadTexture("../Resources/Textures/dirt_01_diffuse.jpg");
	mat->SetTexture(Default::Textures::StandardTexture, TextureType::DIFFUSE);
	auto textureN = ResourceManager::LoadTexture("../Resources/Textures/dirt_01_normal.jpg");
	//mat->SetTexture(textureN, TextureType::NORMAL);
	//auto textureH = AssetManager::LoadTexture("../Resources/Textures/dirt_01_height.jpg");
	//mat->SetTexture(textureH, TextureType::DISPLACEMENT);

	mat->Shininess = 32.0f;
	auto meshMaterial = std::make_unique<MeshRenderer>();
	meshMaterial->Mesh = Default::Primitives::Quad;
	meshMaterial->Material = mat;
	meshMaterial->ReceiveShadow = true;
	meshMaterial->ForwardRendering = false;
	EntityManager::SetPrivateComponent<MeshRenderer>(entity, std::move(meshMaterial));
}


#pragma endregion

