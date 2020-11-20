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
	_CameraEntity.SetPrivateComponent(std::move(cameraComponent));
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
			_CameraEntity.GetPrivateComponent<CameraComponent>()->get()->GetCamera()->StoreToJpg(_StorePath + std::to_string(_RemainingAmount) + ".jpg", 320, 320);
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
			_Capturing = true;
		}
	}
}
