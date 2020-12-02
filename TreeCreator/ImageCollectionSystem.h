#pragma once
#include "PlantSimulationSystem.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace TreeUtilities {
	enum class ImageCollectionSystemStatus
	{
		Idle,
		Growing,
		Rendering,
		CaptureOriginal,
		CaptureRandom,
		CaptureSemantic
	};
	
	struct ImageCaptureSequence
	{
		glm::vec3 CameraPos;
		glm::vec3 CameraEulerDegreeRot;
		std::string ParamPath;
		std::string Name;
	};
	class ImageCollectionSystem : public SystemBase
	{
		ImageCollectionSystemStatus _Status = ImageCollectionSystemStatus::Idle;
		int _CurrentSelectedSequenceIndex = -1;
		int _Counter = 0;
		int _EndIndex = 0;
		int _StartIndex = 0;
		std::string _StorePath = "./tree_data/";
		bool _IsTrain = true;
		Entity _CurrentTree;
		Entity _Background;
		bool _Export = false;
		size_t _TargetResolution = 320;
		size_t _CaptureResolution = 960;
		double _Timer;
		std::unique_ptr<RenderTarget> _SmallBranchFilter;
		std::unique_ptr<GLProgram> _SmallBranchProgram;
		std::unique_ptr<GLTexture2D> _SmallBranchBuffer;
		std::unique_ptr<GLProgram> _SmallBranchCopyProgram;
		
		std::shared_ptr<Material> _BackgroundMaterial;
		PlantSimulationSystem* _PlantSimulationSystem = nullptr;
		Entity _CameraEntity;
		Entity _SemanticMaskCameraEntity;
		glm::vec3 _CameraPosition = glm::vec3(0);
		glm::vec3 _CameraEulerRotation = glm::vec3(0);
		std::vector<std::pair<ImageCaptureSequence, TreeParameters>> _ImageCaptureSequences;
		std::vector<TreeParameters> _TreeParametersOutputList;
		std::vector<std::shared_ptr<Texture2D>> _BackgroundTextures;
	public:
		void ResetCounter(int value, int startIndex, int endIndex);
		void SetIsTrain(bool value);
		bool IsExport() const;
		void PushImageCaptureSequence(ImageCaptureSequence sequence);
		void SetCameraPose(glm::vec3 position, glm::vec3 rotation);
		void OnCreate() override;
		void SetPlantSimulationSystem(PlantSimulationSystem* value);
		void Update() override;
		void EnableSemantic();
	};
}