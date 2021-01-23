#pragma once
#include "CakeTower.h"
#include "PlantSimulationSystem.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace TreeUtilities {
	enum class DataCollectionSystemStatus
	{
		Idle,
		Growing,
		Rendering,
		CaptureOriginal,
		CaptureOriginalBranch,
		CaptureRandom,
		CaptureRandomBranch,
		CaptureSemantic,
		CaptureSemantic0,
		CaptureSemantic1,
		CaptureSemantic2,
		CaptureSemantic3,
		CaptureSemantic4,
		CaptureBranchMask,
		CollectData,
		CaptureCakeTower,
		CleanUp
	};

	struct ParamsOutput
	{
		int Index;
		std::string Name;
		TreeParameters Parameters;
		ParamsOutput(int index, std::string& name, TreeParameters& params)
		{
			Index = index;
			Name = name;
			Parameters = params;
		}
	};

	struct CakeTowerOutput
	{
		int Index;
		std::string Name;
		float _MaxRadius;
		float _MaxHeight;
		std::vector<std::vector<CakeSlice>> data;
		CakeTowerOutput(int index, std::string& name, std::unique_ptr<CakeTower>& cakeTower)
		{
			Index = index;
			Name = name;
			data = cakeTower->CakeTiers;
			_MaxHeight = cakeTower->MaxHeight;
			_MaxRadius = cakeTower->MaxRadius;
		}
	};
	
	struct ImageCaptureSequence
	{
		glm::vec3 CameraPos;
		glm::vec3 CameraEulerDegreeRot;
		std::string ParamPath;
		std::string Name;
	};
	class DataCollectionSystem : public SystemBase
	{
		friend class TreeReconstructionSystem;
		friend class TreeCollectionGenerationSystem;
		
		DataCollectionSystemStatus _Status = DataCollectionSystemStatus::Idle;
		int _CurrentSelectedSequenceIndex = 0;
		int _Counter = 0;
		int _EndIndex = 0;
		int _StartIndex = 0;
		int _EvalStartIndex = 0;
		int _EvalEndIndex = 0;
		std::string _StorePath = "./tree_data/";
		bool _IsTrain = true;
		Entity _CurrentTree;
		Entity _Background;
		
		bool _NeedExport = false;
		int _TargetResolution = 1280;
		int _CaptureResolution = 1280;
		double _Timer;
		bool _NeedEval = true;
		bool _ExportOBJ = false;
		bool _ExportGraph = false;
		bool _ExportImages = true;
		bool _ExportKDop = false;
		bool _ExportCakeTower = true;
		bool _EnableMultipleAngles = false;
		bool _ExportBranchOnly = false;
		std::unique_ptr<RenderTarget> _SmallBranchFilter;
		std::unique_ptr<GLProgram> _SmallBranchProgram;
		std::unique_ptr<GLTexture2D> _SmallBranchBuffer;
		std::unique_ptr<GLProgram> _SmallBranchCopyProgram;
		bool _Batched = false;
		std::shared_ptr<Material> _BackgroundMaterial;
		PlantSimulationSystem* _PlantSimulationSystem = nullptr;
		Entity _ImageCameraEntity;
		Entity _SemanticMaskCameraEntity;
		glm::vec3 _CameraPosition = glm::vec3(0, 2, 25);
		glm::vec3 _CameraEulerRotation = glm::radians(glm::vec3(15, 0, 0));
		std::vector<std::pair<ImageCaptureSequence, TreeParameters>> _ImageCaptureSequences;
		std::vector<ParamsOutput> _TreeParametersOutputList;
		std::vector<std::pair<std::pair<int, int>, std::vector<CakeTowerOutput>>> _GeneralCakeTowersOutputList;
		std::vector<std::pair<std::pair<int, int>, std::vector<CakeTowerOutput>>> _PerSpeciesCakeTowersOutputList;
		std::vector<std::pair<std::pair<int, int>, std::vector<CakeTowerOutput>>> _CakeTowersOutputList;
		std::vector<std::shared_ptr<Texture2D>> _BackgroundTextures;
		std::vector<std::shared_ptr<Texture2D>> _BranchBarkTextures;
		Entity _DirectionalLightEntity;
		Entity _DirectionalLightEntity1;
		Entity _DirectionalLightEntity2;
		Entity _DirectionalLightEntity3;
		Transform _LightTransform;
		Transform _LightTransform1;
		Transform _LightTransform2;
		Transform _LightTransform3;
		
		bool _Reconstruction;
		int _Index;
		int _Seed;
		std::string _ReconPath;
		void CaptureSemantic(ImageCaptureSequence& imageCaptureSequence, int angle) const;
		void ExportCakeTowerForRecon(int layer, int sector);
		void OnGui();
	public:
		void SetDirectionalLightEntity(Entity entity, Entity entity1, Entity entity2, Entity entity3);
		void ExportAllData();
		void ResetCounter(int value, int startIndex, int endIndex, bool obj, bool graph);
		void SetIsTrain(bool value);
		bool IsExport() const;
		void PushImageCaptureSequence(ImageCaptureSequence sequence);
		void ExportParams(const std::string& path) const;
		void ExportCakeTower(const std::string& path, bool isTrain) const;
		void ExportCakeTowerPerSpecies(const std::string& path, bool isTrain) const;
		void ExportCakeTowerGeneral(const std::string& path, bool isTrain) const;
		void SetCameraPose(glm::vec3 position, glm::vec3 rotation, bool random = false);
		void SetCameraPoseMulti(glm::vec3 position, glm::vec3 rotation, int angle);
		void OnCreate() override;
		void SetPlantSimulationSystem(PlantSimulationSystem* value);
		void Update() override;
		void LateUpdate() override;
		void EnableSemantic() const;
		void SetEnableFoliage(bool enabled) const;
	};
}