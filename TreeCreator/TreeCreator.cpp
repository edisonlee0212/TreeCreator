#include <iostream>

#include "UniEngine.h"
#include "CameraControlSystem.h"
#include "PlantSimulationSystem.h"
#include "TreeManager.h"
#include "SorghumReconstructionSystem.h"

using namespace UniEngine;
using namespace TreeUtilities;
using namespace SorghumReconstruction;
void InitGround();
PlantSimulationSystem* InitPlantSimulationSystem();
SorghumReconstructionSystem* InitSorghumReconstructionSystem();
void LightSettingMenu();

float pcssScale = 0.03f;
float ambientLight = 0.1f;
float ssaobias = 0.08f;
float ssaoradius = 0.05f;
float ssaofactor = 10.0f;
int ssaoSample = 32;
int main()
{

#pragma region Global light settings
	RenderManager::SetEnableShadow(true);
	RenderManager::SetDirectionalLightResolution(8192);
	RenderManager::SetStableFit(true);
	RenderManager::SetMaxShadowDistance(100.0f);
	RenderManager::SetSeamFixRatio(0.05f);
	RenderManager::SetSplitRatio(0.2f, 0.4f, 0.6f, 1.0f);
#pragma endregion
	FileIO::SetResourcePath("../Submodules/UniEngine/Resources/");
	Application::Init();
#pragma region Lights
	EntityArchetype lightArchetype = EntityManager::CreateEntityArchetype("Directional Light", EulerRotation(), Rotation(), DirectionalLightComponent());
	DirectionalLightComponent dlc;
	dlc.diffuseBrightness = 1.1f;
	dlc.depthBias = 0.001;
	dlc.normalOffset = 0.001;
	dlc.lightSize = 5.2;
	EulerRotation er;
	er.Value = glm::vec3(60, 0, 0);
	Entity dle = EntityManager::CreateEntity(lightArchetype, "Directional Light");
	EntityManager::SetComponentData(dle, dlc);
	EntityManager::SetComponentData(dle, er);
#pragma endregion
#pragma region Preparations
	Application::SetTimeStep(0.016f);
	World* world = Application::GetWorld();
	WorldTime* time = world->Time();

	EntityArchetype archetype = EntityManager::CreateEntityArchetype("General", Translation(), Rotation(), Scale(), LocalToWorld());
	CameraControlSystem* ccs = world->CreateSystem<CameraControlSystem>(SystemGroup::SimulationSystemGroup);
	ccs->Enable();
	ccs->SetPosition(glm::vec3(0, 6, 20));
	ccs->SetVelocity(10.0f);
	InitGround();
#pragma endregion
	TreeManager::Init();
	
#pragma region Light estimator setup
	//The smaller the branch node's size, the more branching for tree.
	TreeManager::GetLightEstimator()->ResetResolution(512);
	TreeManager::GetLightEstimator()->SetBranchNodeSize(0.3f);
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
	auto pss = InitPlantSimulationSystem();

	const bool enableSorghumRecon = true;
	if (enableSorghumRecon) {
		auto srSys = InitSorghumReconstructionSystem();
		Entity plant1 = srSys->ImportPlant("skeleton_procedural_1.txt", 0.01f, "Sorghum 1");
		Entity plant2 = srSys->ImportPlant("skeleton_procedural_2.txt", 0.01f, "Sorghum 2");
		Entity plant3 = srSys->ImportPlant("skeleton_procedural_3.txt", 0.01f, "Sorghum 3");
		Entity plant4 = srSys->ImportPlant("skeleton_procedural_4.txt", 0.01f, "Sorghum 4");
		srSys->GenerateMeshForAllPlants();
		srSys->ExportPlant(plant1, "plant1");
		srSys->ExportPlant(plant2, "plant2");
		srSys->ExportPlant(plant3, "plant3");
		srSys->ExportPlant(plant4, "plant4");
		
		glm::vec2 radius = glm::vec2(45,5);
		glm::vec2 size = glm::vec2(0.565f, 2.7f);
		std::vector<std::vector<glm::mat4>> matricesList;
		matricesList.resize(4);
		for(auto& i : matricesList)
		{
			i.clear();
		}
		float xStep = -radius.x * size.x * 2;
		
		for (int i = -radius.x; i <= radius.x; i++)
		{
			if (i % 18 == 0) xStep += 2.0f * glm::gaussRand(1.0f, 0.5f);
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
		Translation t1;
		Translation t2;
		Translation t3;
		Translation t4;
		Rotation r1;
		Rotation r2;
		Rotation r3;
		Rotation r4;
		Scale s1;
		Scale s2;
		Scale s3;
		Scale s4;
		s1.Value = glm::vec3(1.0f, 1.0f, 1.0f);
		s2.Value = glm::vec3(1.0f, 1.0f, 1.0f);
		s3.Value = glm::vec3(1.0f, 1.0f, 1.0f);
		s4.Value = glm::vec3(1.0f, 1.0f, 1.0f);
		
		t1.Value = glm::vec3(-5.0f, 0.0, 0);
		t2.Value = glm::vec3(0.0f, 0.0, 0);
		t3.Value = glm::vec3(5.0f, 0.0, 0);
		t4.Value = glm::vec3(10.0f, 0.0, 0);
		
		r1.Value = glm::quat(glm::vec3(glm::radians(-90.0f), glm::radians(42.0f), 0));
		r2.Value = glm::quat(glm::vec3(glm::radians(-90.0f), glm::radians(30.0f), 0));
		r3.Value = glm::quat(glm::vec3(glm::radians(-90.0f), glm::radians(40.0f), 0));
		r4.Value = glm::quat(glm::vec3(glm::radians(-90.0f), glm::radians(-110.0f), 0));
		EntityManager::SetComponentData(plant1, t1);
		EntityManager::SetComponentData(plant1, r1);
		EntityManager::SetComponentData(plant1, s1);
		
		EntityManager::SetComponentData(plant2, t2);
		EntityManager::SetComponentData(plant2, r2);
		EntityManager::SetComponentData(plant2, s2);
		
		EntityManager::SetComponentData(plant3, t3);
		EntityManager::SetComponentData(plant3, r3);
		EntityManager::SetComponentData(plant3, s3);
		
		EntityManager::SetComponentData(plant4, t4);
		EntityManager::SetComponentData(plant4, r4);
		EntityManager::SetComponentData(plant4, s4);
	}
	
#pragma region Engine Loop
	bool loopable = true;
	//Start engine. Here since we need to inject procedures to the main engine loop we need to manually loop by our self.
	//Another way to run engine is to simply execute:
	//Application.Run();
	while (loopable) {
		Application::PreUpdate();
		LightSettingMenu();
#pragma region Apply lights
		RenderManager::SetPCSSScaleFactor(pcssScale);
#pragma endregion
		ImGui::Begin("WireFrame");
		static bool enableWireFrame = false;
		ImGui::Checkbox("Enable wire-frame mode", &enableWireFrame);
		ImGui::End();
		if(enableWireFrame)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}else
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		ImGui::Begin("SSAO");
		ImGui::SliderFloat("Radius", &ssaoradius, 0.0f, 2.0f);
		ImGui::SliderFloat("Bias", &ssaobias, 0.0f, 1.0f);
		ImGui::SliderFloat("Factor", &ssaofactor, 1.0f, 32.0f);
		ImGui::SliderInt("Sample amount", &ssaoSample, 0, 64);
		ImGui::End();
		RenderManager::SetSSAOKernelRadius(ssaoradius);
		RenderManager::SetSSAOKernelBias(ssaobias);
		RenderManager::SetSSAOFactor(ssaofactor);
		RenderManager::SetSSAOSampleSize(ssaoSample);
		RenderManager::SetAmbientLight(ambientLight);
		//ImGui::ShowDemoWindow();
		Application::Update();
		loopable = Application::LateUpdate();
	}
#pragma endregion
	Application::End();
	return 0; 
}
PlantSimulationSystem* InitPlantSimulationSystem() {
	return Application::GetWorld()->CreateSystem<PlantSimulationSystem>(SystemGroup::SimulationSystemGroup);
}
SorghumReconstructionSystem* InitSorghumReconstructionSystem()
{
	return Application::GetWorld()->CreateSystem<SorghumReconstructionSystem>(SystemGroup::SimulationSystemGroup);
}
void InitGround() {
	EntityArchetype archetype = EntityManager::CreateEntityArchetype("General", Translation(), Rotation(), Scale(), LocalToWorld());

	auto mat = std::make_shared<Material>();
	mat->SetProgram(Default::GLPrograms::StandardInstancedProgram);
	
	auto textureDiffuse = std::make_shared<Texture2D>(TextureType::DIFFUSE);
	textureDiffuse->LoadTexture("../Resources/Textures/dirt_01_diffuse.jpg", "");
	mat->SetTexture(textureDiffuse);
	auto textureNormal = new Texture2D(TextureType::NORMAL);
	textureNormal->LoadTexture("../Resources/Textures/dirt_01_normal.jpg", "");
	//mat->Textures2Ds()->push_back(textureNormal);
	
	mat->SetMaterialProperty("material.shininess", 32.0f);
	auto instancedMeshRenderer = std::make_shared<InstancedMeshRenderer>();
	instancedMeshRenderer->Mesh = Default::Primitives::Quad;
	instancedMeshRenderer->Material = mat;
	Translation translation = Translation();
	Scale scale = Scale();
	auto baseEntity = EntityManager::CreateEntity(archetype, "Ground");
	translation.Value = glm::vec3(0.0f);
	scale.Value = glm::vec3(1.0f);
	baseEntity.SetComponentData(translation);
	baseEntity.SetComponentData(scale);
	int radius = 4;
	float size = 10.0f;
	for(int i = -radius; i <= radius; i++)
	{
		for(int j = -radius; j <= radius; j++)
		{
			auto position = glm::vec3(size * 2 * i, 0.0f, size * 2 * j);
			auto scale = glm::vec3(size * 1.0f);
			instancedMeshRenderer->Matrices.emplace_back(glm::translate(glm::identity<glm::mat4>(), position) * glm::scale(glm::identity<glm::mat4>(), scale));
		}
	}
	instancedMeshRenderer->RecalculateBoundingBox();
	baseEntity.SetSharedComponent<InstancedMeshRenderer>(instancedMeshRenderer);

}
void LightSettingMenu() {
	ImGui::Begin("Light Settings");
	ImGui::SliderFloat("Ambient Light", &ambientLight, 0.0f, 1.0f);
	ImGui::SliderFloat("PCSS Scale", &pcssScale, 0.0f, 3.0f);
	ImGui::End();
}

