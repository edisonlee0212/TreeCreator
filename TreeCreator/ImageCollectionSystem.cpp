#include "ImageCollectionSystem.h"

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
	cameraComponent->ResizeResolution(640, 640);
	cameraComponent->DrawSkyBox = false;
	cameraComponent->ClearColor = glm::vec3(1.0f);
	_CameraEntity.SetPrivateComponent(std::move(cameraComponent));
	Enable();
}

void ImageCollectionSystem::AttachToPlantSimulationSystem(PlantSimulationSystem* value)
{
	_PlantSimulationSystem = value;
}

void ImageCollectionSystem::CreateCaptureSequence(TreeParameters& treeParameters, int amount, std::string path)
{
	if (_RemainingAmount != 0 || _PlantSimulationSystem == nullptr) return;
	_StorePath = path;
	_RemainingAmount = amount;
	_CurrentTreeParameters = treeParameters;
	TreeManager::DeleteAllTrees();
	_CurrentTreeParameters.Seed = _RemainingAmount;
	_CurrentTree = _PlantSimulationSystem->CreateTree(_CurrentTreeParameters, glm::vec3(0.0f));
}

void ImageCollectionSystem::Update()
{
	if(_Capturing)
	{
		_CameraEntity.GetPrivateComponent<CameraComponent>()->get()->GetCamera()->StoreToJpg(_StorePath + std::to_string(_RemainingAmount) + ".jpg");
		_Capturing = false;
		TreeManager::DeleteAllTrees();
		_RemainingAmount--;
		_CurrentTreeParameters.Seed = _RemainingAmount;
		_CurrentTree = _PlantSimulationSystem->CreateTree(_CurrentTreeParameters, glm::vec3(0.0f));
	}
	else if(_RemainingAmount != 0)
	{
		if(!_PlantSimulationSystem->_Growing)
		{
			TreeManager::GenerateSimpleMeshForTree(_CurrentTree, 0.05f, 1.0);
			_Capturing = true;
		}
	}
}
