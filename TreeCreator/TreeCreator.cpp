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

float lightAngle0 = 60;
float lightAngle1 = 0;
float lightDiffuse = 1.0f;
float lightSpecular = 0.2f;
float lightSize = 1.2f;
float pcssScale = 1.0f;

int main()
{
#pragma region Global light settings
	RenderManager::SetEnableShadow(true);
	RenderManager::SetDirectionalLightResolution(2048);
	RenderManager::SetStableFit(true);
	RenderManager::SetMaxShadowDistance(100.0f);
	RenderManager::SetSeamFixRatio(0.05f);
	RenderManager::SetVSMMaxVariance(0.001f);
	RenderManager::SetEVSMExponent(80.0f);
	RenderManager::SetSplitRatio(0.2f, 0.4f, 0.6f, 1.0f);
#pragma endregion
	FileIO::SetResourcePath("../Submodules/UniEngine/Resources/");
	Application::Init();
#pragma region Lights
	EntityArchetype lightArchetype = EntityManager::CreateEntityArchetype("Light", Translation(), Rotation(), LocalToWorld());
	auto dlc = std::make_shared<DirectionalLightComponent>();
	Entity dle = EntityManager::CreateEntity(lightArchetype, "Directional Light");
	EntityManager::SetSharedComponent<DirectionalLightComponent>(dle, dlc);
	
#pragma endregion
#pragma region Preparations
	Application::SetTimeStep(0.016f);
	World* world = Application::GetWorld();
	WorldTime* time = world->Time();

	EntityArchetype archetype = EntityManager::CreateEntityArchetype("General", Translation(), Rotation(), Scale(), LocalToWorld());
	CameraControlSystem* ccs = world->CreateSystem<CameraControlSystem>(SystemGroup::SimulationSystemGroup);
	ccs->Enable();
	ccs->SetPosition(glm::vec3(0, 6, 20));
	ccs->SetVelocity(5.0f);
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
		Translation t1;
		Translation t2;
		Rotation r1;
		Rotation r2;
		t1.Value = glm::vec3(-2.5, 0.0, 0);
		t2.Value = glm::vec3(2.5, 0.0, 0);
		r1.Value = glm::quat(glm::vec3(glm::radians(-90.0f), 0, 0));
		r2.Value = glm::quat(glm::vec3(glm::radians(-90.0f), 0, 0));

		EntityManager::SetComponentData(plant1, t1);
		EntityManager::SetComponentData(plant1, r2);

		EntityManager::SetComponentData(plant2, t2);
		EntityManager::SetComponentData(plant2, r2);
		
		srSys->GenerateMeshForAllPlants();
		srSys->ExportPlant(plant1, "plant1");
		srSys->ExportPlant(plant2, "plant2");
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
	mat->Programs()->push_back(Default::GLPrograms::StandardInstancedProgram);
	auto texture = new Texture2D(TextureType::DIFFUSE);
	texture->LoadTexture("../Resources/Textures/grassland.jpg", "");
	mat->Textures2Ds()->push_back(texture);
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
	int radius = 10;
	float size = 1.0f;
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
	ImGui::Begin("Light Angle Controller");
	ImGui::SliderFloat("Angle", &lightAngle0, 0.0f, 89.0f);
	ImGui::SliderFloat("Circle", &lightAngle1, 0.0f, 360.0f);
	ImGui::SliderFloat("Diffuse", &lightDiffuse, 0.0f, 2.0f);
	ImGui::SliderFloat("Specular", &lightSpecular, 0.0f, 2.0f);
	ImGui::SliderFloat("PCSS Scale", &pcssScale, 0.0f, 3.0f);
	ImGui::SliderFloat("Light Size", &lightSize, 0.0f, 5.0f);
	ImGui::End();
}