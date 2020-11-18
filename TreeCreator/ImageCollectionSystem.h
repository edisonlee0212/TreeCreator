#pragma once
#include "PlantSimulationSystem.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace TreeUtilities {
	class ImageCollectionSystem : public SystemBase
	{
		TreeParameters _CurrentTreeParameters;
		std::string _StorePath;
		int _RemainingAmount = 0;
		bool _Capturing = false;
		Entity _CurrentTree;
		PlantSimulationSystem* _PlantSimulationSystem = nullptr;
		Entity _CameraEntity;
		glm::vec3 _CameraPosition = glm::vec3(0);
		glm::vec3 _CameraEulerRotation = glm::vec3(0);
	public:
		void SetCameraPose(glm::vec3 position, glm::vec3 rotation);
		void OnCreate() override;
		void AttachToPlantSimulationSystem(PlantSimulationSystem* value);
		void CreateCaptureSequence(TreeParameters& treeParameters, int amount, std::string path);
		void Update() override;
	};
}