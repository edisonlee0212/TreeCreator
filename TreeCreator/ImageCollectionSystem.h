#pragma once
#include "PlantSimulationSystem.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace TreeUtilities {
	struct ImageCaptureSequence
	{
		glm::vec3 CameraPos;
		glm::vec3 CameraEulerDegreeRot;
		std::string ParamPath;
		int Amount;
		std::string OutputPath;
	};
	class ImageCollectionSystem : public SystemBase
	{
		TreeParameters _CurrentTreeParameters;
		std::string _StorePath;
		int _RemainingAmount = 0;
		bool _Capturing = false;
		Entity _CurrentTree;
		bool _Running = false;
		PlantSimulationSystem* _PlantSimulationSystem = nullptr;
		Entity _CameraEntity;
		glm::vec3 _CameraPosition = glm::vec3(0);
		glm::vec3 _CameraEulerRotation = glm::vec3(0);
		std::queue<ImageCaptureSequence> _ImageCaptureSequences;
	public:
		void PushImageCaptureSequence(ImageCaptureSequence sequence);
		void SetCameraPose(glm::vec3 position, glm::vec3 rotation);
		void OnCreate() override;
		void AttachToPlantSimulationSystem(PlantSimulationSystem* value);
		void CreateCaptureSequence(TreeParameters& treeParameters, int amount, std::string path);
		void Update() override;
	};
}