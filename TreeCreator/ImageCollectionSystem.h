#pragma once
#include "PlantSimulationSystem.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace TreeUtilities {
	enum class ImageCollectionSystemStatus
	{
		Idle,
		Growing,
		CaptureOriginal,
		CaptureRandom,
		CaptureSemantic
	};
	
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
		ImageCollectionSystemStatus _Status = ImageCollectionSystemStatus::Idle;
		TreeParameters _CurrentTreeParameters = TreeParameters();
		std::string _StorePath;
		int _RemainingAmount = 0;
		Entity _CurrentTree;
		Entity _Background;
		std::shared_ptr<Material> _BackgroundMaterial;
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
		void EnableSemantic();
	};
}