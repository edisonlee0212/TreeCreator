// SponzaTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "UniEngine.h"
#include "CameraControlSystem.h"
#include "PlantSimulationSystem.h"
#include "TreeManager.h"
#include "EntityEditorSystem.h"

#include "EntityEditorSystem.h"

using namespace UniEngine;
using namespace TreeUtilities;
void InitGround();
void InitPlantSimulationSystem();
void LightSettingMenu();

float lightAngle0 = 60;
float lightAngle1 = 0;
float lightDiffuse = 1.0f;
float lightSpecular = 0.2f;
float lightSize = 1.2f;
float pcssScale = 1.0f;

int main()
{
#pragma region Global light settings
	LightingManager::SetDirectionalLightResolution(1024);
	LightingManager::SetStableFit(true);
	LightingManager::SetMaxShadowDistance(50.0f);
	LightingManager::SetSeamFixRatio(0.05f);
	LightingManager::SetVSMMaxVariance(0.001f);
	LightingManager::SetEVSMExponent(80.0f);
	LightingManager::SetSplitRatio(0.2f, 0.4f, 0.7f, 1.0f);
#pragma endregion
	FileIO::SetResourcePath("../Submodules/UniEngine/Resources/");
	Application::Init();
#pragma region Lights
	EntityArchetype lightArchetype = EntityManager::CreateEntityArchetype("Light", Translation(), Rotation(), Scale(), LocalToWorld());
	DirectionalLightComponent* dlc = new DirectionalLightComponent();
	Entity dle = EntityManager::CreateEntity(lightArchetype);
	EntityManager::SetSharedComponent<DirectionalLightComponent>(dle, dlc);
	
#pragma endregion
#pragma region Preparations
	Application::SetTimeStep(0.016f);
	World* world = Application::GetWorld();
	WorldTime* time = world->Time();
	EntityEditorSystem* editorSystem = world->CreateSystem<EntityEditorSystem>(SystemGroup::PresentationSystemGroup);

	EntityArchetype archetype = EntityManager::CreateEntityArchetype("General", Translation(), Rotation(), Scale(), LocalToWorld());
	CameraControlSystem* ccs = world->CreateSystem<CameraControlSystem>(SystemGroup::SimulationSystemGroup);
	ccs->Enable();
	ccs->SetPosition(glm::vec3(0, 6, 20));
	ccs->SetVelocity(5.0f);
	InitGround();
#pragma endregion
	TreeManager::Init();
	//The smaller the branch node's size, the more branching for tree.
	TreeManager::GetLightEstimator()->SetBranchNodeSize(0.7f);
#pragma region Light estimator setup
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
	InitPlantSimulationSystem();
#pragma region Engine Loop
	bool loopable = true;
	//Start engine. Here since we need to inject procedures to the main engine loop we need to manually loop by our self.
	//Another way to run engine is to simply execute:
	//Application.Run();
	while (loopable) {
		Application::PreUpdate();
		LightSettingMenu();
#pragma region Apply lights
		Rotation r;
		r.Value = glm::quatLookAt(
			glm::normalize(glm::vec3(
				glm::cos(glm::radians(lightAngle0)) * glm::sin(glm::radians(lightAngle1)),
				glm::sin(glm::radians(lightAngle0)),
				glm::cos(glm::radians(lightAngle0)) * glm::cos(glm::radians(lightAngle1))))
			, glm::vec3(0, 1, 0));
		EntityManager::SetComponentData<Rotation>(dle, r);
		dlc->specular = glm::vec3(255, 255, 255) * lightSpecular / 256.0f;
		dlc->diffuse = glm::vec3(255, 255, 251) * lightDiffuse / 256.0f;
		dlc->lightSize = lightSize;
		LightingManager::SetPCSSScaleFactor(pcssScale);
#pragma endregion

		ImGui::ShowDemoWindow();
		Application::Update();
		loopable = Application::LateUpdate();
	}
#pragma endregion
	Application::End();
	return 0; 
}
void InitPlantSimulationSystem() {
	auto psSys = Application::GetWorld()->CreateSystem<PlantSimulationSystem>(SystemGroup::SimulationSystemGroup);
}
void InitGround() {
	EntityArchetype archetype = EntityManager::CreateEntityArchetype("General", Translation(), Rotation(), Scale(), LocalToWorld());
	auto entity = EntityManager::CreateEntity(archetype);
	Translation translation = Translation();
	translation.Value = glm::vec3(0.0f, 0.0f, 0.0f);
	Scale scale = Scale();
	scale.Value = glm::vec3(50.0f);
	EntityManager::SetComponentData<Translation>(entity, translation);
	EntityManager::SetComponentData<Scale>(entity, scale);


	auto mat = new Material();
	mat->Programs()->push_back(Default::GLPrograms::StandardProgram);
	auto texture = new Texture2D(TextureType::DIFFUSE);
	texture->LoadTexture(FileIO::GetResourcePath("Textures/white.png"), "");
	mat->Textures2Ds()->push_back(texture);
	mat->SetMaterialProperty("material.shininess", 32.0f);
	MeshMaterialComponent* meshMaterial = new MeshMaterialComponent();
	meshMaterial->_Mesh = Default::Primitives::Quad;
	meshMaterial->_Material = mat;
	EntityManager::SetSharedComponent<MeshMaterialComponent>(entity, meshMaterial);

}
void LightSettingMenu() {
	ImGui::Begin("Light Angle Controller");
	ImGui::SliderFloat("Angle", &lightAngle0, 0.0f, 89.0f);
	ImGui::SliderFloat("Circle", &lightAngle1, 0.0f, 360.0f);
	ImGui::SliderFloat("Diffuse", &lightDiffuse, 0.0f, 2.0f);
	ImGui::SliderFloat("Specular", &lightSpecular, 0.0f, 2.0f);
	ImGui::SliderFloat("PCSS Scale", &pcssScale, 0.0f, 3.0f);
	ImGui::SliderFloat("Light Size", &lightSize, 0.0f, 5.0f);
	ImGui::End();
}