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
		DataCollectionSystem* _DataCollectionSystem = nullptr;
		double _Timer;
		glm::vec3 _Position = glm::vec3(-15, 10, -50);
		glm::vec3 _Rotation = glm::vec3(-180, -20, -180);
		Entity _CameraEntity;
		int _CaptureResolution = 320;
		float _CurrentDegrees = 0;
		float _DegreeIncrementation = 60;
		std::vector<Entity> _Internodes;
		std::queue<TreeParameters> _CreationQueue;
		std::string _StorePath = "./tree_perceptual/";
		int _Counter = 0;
		void OnGui();
	public:
		void ImportCsv(const std::string& path);
		void ImportCsv2(const std::string& path);
		void LateUpdate() override;
		void OnCreate() override;
		void SetDataCollectionSystem(DataCollectionSystem* value);
	};
}


