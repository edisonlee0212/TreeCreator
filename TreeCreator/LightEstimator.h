#pragma once
#include "UniEngine.h"
using namespace UniEngine;
namespace TreeUtilities {
	class LightSnapShot {
		std::unique_ptr<GLTexture2D> _SnapShotTexture;
		std::vector<float> _SRC;
		glm::vec3 _Direction;
		float _CenterDistance;
		float _Width;
		float _Weight;
		float _Score;
		size_t _Resolution;
		friend class LightEstimator;
		glm::vec3 _CenterPosition;
	public:
		std::unique_ptr<GLTexture2D>& SnapShotTexture() { return _SnapShotTexture; }
		std::vector<float>& GetSRC() { return _SRC; }
		LightSnapShot(size_t resolution, glm::vec3 centerPosition, glm::vec3 direction, float centerDistance, float width, float weight);
		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetLightSpaceMatrix() const;
		glm::vec3 GetDirection() const;
		float CalculateScore();
		float CenterDistance() const;
		float Width() const;
		float Weight() const;
		float Resolution() const;
		unsigned GetEntityIndex(size_t x, size_t y);
	};

	class LightEstimator
	{
		RenderTarget* _RenderTarget = nullptr;
		size_t _Resolution = 2048;
		std::vector<LightSnapShot*> _SnapShots;
		std::unique_ptr<GLProgram> _SnapShotProgram = nullptr;

		GLRenderBuffer* _DepthBuffer = nullptr;
		float _LightEstimationScore = 0;
		friend class TreeSystem;
		float _SnapShotWidth = 50.0f;
		float _CenterDistance = 200.0f;

		float _MaxIllumination;
		glm::vec3 _CenterPositon = glm::vec3(0.0f);
		friend class TreeManager;
		void SetMaxIllumination(float value);
	public:
		glm::vec3 GetCenterPosition() const;
		LightEstimator(size_t resolution = 512, float centerDistance = 100.0f);
		void ResetResolution(size_t value);
		void ResetCenterPosition(glm::vec3 position);
		void ResetCenterDistance(float distance);
		void ResetSnapShotWidth(float width);
		void PushSnapShot(glm::vec3 direction, float weight);
		void Clear();
		void TakeSnapShot(bool storeSnapShot);
		float GetMaxIllumination();
		float CalculateScore();
		std::vector<LightSnapShot*>* GetSnapShots();
	};

} 