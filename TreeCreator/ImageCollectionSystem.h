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
		bool EnableSemanticOutput = false;
		bool EnableRandomBackground = false;
	};
	class ImageCollectionSystem : public SystemBase
	{
		TreeParameters _CurrentTreeParameters;
		std::string _StorePath;
		int _RemainingAmount = 0;
		bool _Capturing = false;
		Entity _CurrentTree;

		Entity _Background;
		std::shared_ptr<Material> _BackgroundMaterial;

		bool _Running = false;
		bool _EnableSemanticMask = false;
		bool _EnableRandomBackground = false;
		PlantSimulationSystem* _PlantSimulationSystem = nullptr;
		Entity _CameraEntity;
		glm::vec3 _CameraPosition = glm::vec3(0);
		glm::vec3 _CameraEulerRotation = glm::vec3(0);
		std::queue<ImageCaptureSequence> _ImageCaptureSequences;
		std::vector<std::shared_ptr<Texture2D>> _BackgroundTextures;
	public:
		void PushImageCaptureSequence(ImageCaptureSequence sequence);
		void SetCameraPose(glm::vec3 position, glm::vec3 rotation);
		void OnCreate() override;
		void AttachToPlantSimulationSystem(PlantSimulationSystem* value);
		void Update() override;
	};
}