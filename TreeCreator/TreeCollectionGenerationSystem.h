#pragma once
#include "UniEngine.h"
#include "TreeManager.h"
#include "DataCollectionSystem.h"
using namespace UniEngine;

namespace TreeUtilities
{
	enum class TreeCollectionGenerationSystenStatus
	{
		Idle,
		Growing,
		Rendering,
		CollectData,
		CleanUp
	};
	
	class TreeCollectionGenerationSystem :
		public SystemBase
	{
		TreeCollectionGenerationSystenStatus _Status = TreeCollectionGenerationSystenStatus::Idle;
		static DataCollectionSystem* _DataCollectionSystem;
		double _Timer;
		bool _Generation;
		glm::vec3 _Position = glm::vec3(-15, 10, -50);
		glm::vec3 _Rotation = glm::vec3(-180, -20, -180);
		Entity _CameraEntity;
		Entity _GroundEntity;
		static int _CaptureResolution;
		float _CurrentDegrees = 0;
		float _DegreeIncrementation = 60;
		std::vector<Entity> _Internodes;
		std::queue<TreeParameters> _CreationQueue;
		static std::string _StorePath;
		int _Counter = 0;
		static bool _SceneReady;
		void PrepareScene();
		void OnGui();
	public:
		void ImportCsv(const std::string& path);
		void ImportCsv2(const std::string& path);
		void LateUpdate() override;
		void OnCreate() override;
		void SetDataCollectionSystem(DataCollectionSystem* value);
		void SetGroundEntity(Entity value);

#pragma region For Tomas
		static float PredictedScore;
		static void TakeSnapShot();
		
#pragma endregion

		
	};
}


