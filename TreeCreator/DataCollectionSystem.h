#pragma once
#include "CakeTower.h"
#include "KDop.h"
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
		CaptureRandom,
		CaptureSemantic,
		CaptureBranch,
		CollectData,
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

	struct KDopOutput
	{
		int Index;
		std::string Name;
		std::vector<float> data;
		KDopOutput(int index, std::string& name, std::unique_ptr<KDop>& kdop)
		{
			Index = index;
			Name = name;
			data = kdop->DirectionalDistance;
		}
	};

	struct CakeTowerOutput
	{
		int Index;
		std::string Name;
		std::vector<std::vector<CakeSlice>> data;
		CakeTowerOutput(int index, std::string& name, std::unique_ptr<CakeTower>& cakeTower)
		{
			Index = index;
			Name = name;
			data = cakeTower->CakeTiers;
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
		std::vector<KDopOutput> _KDopsOutputList;
		std::vector<CakeTowerOutput> _CakeTowersOutputList;
		std::vector<std::shared_ptr<Texture2D>> _BackgroundTextures;

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
		
		void OnGui();
	public:
		void SetDirectionalLightEntity(Entity entity, Entity entity1, Entity entity2, Entity entity3);
		void ExportAllData();
		void ResetCounter(int value, int startIndex, int endIndex, bool obj, bool graph);
		void SetIsTrain(bool value);
		bool IsExport() const;
		void PushImageCaptureSequence(ImageCaptureSequence sequence);
		void ExportParams(const std::string& path) const;
		void ExportKDops(const std::string& path) const;
		void ExportCakeTower(const std::string& path) const;
		void SetCameraPose(glm::vec3 position, glm::vec3 rotation);
		void OnCreate() override;
		void SetPlantSimulationSystem(PlantSimulationSystem* value);
		void Update() override;
		void LateUpdate() override;
		void EnableSemantic() const;
		void HideFoliage() const;
	};
}