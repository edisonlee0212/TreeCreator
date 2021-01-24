#pragma once
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	class MaskProcessor : public PrivateComponentBase
	{
		friend class PlantSimulationSystem;
		int _AttractionPointsCount = 2000;
		bool _Display = true;
		
		friend class DataCollectionSystem;
		friend class TreeReconstructionSystem;
		friend class RealTreeReconstructionSystem;
		static Entity _CameraEntity;
		static Entity _Background;
		static unsigned _ResolutionX;
		static unsigned _ResolutionY;
		static std::unique_ptr<GLProgram> _InternodeCaptureProgram;
		static std::unique_ptr<GLProgram> _FilterProgram;
		static std::unique_ptr<GLProgram> _MaskPreprocessProgram;
		static std::unique_ptr<RenderTarget> _Filter;
		static std::unique_ptr<GLRenderBuffer> _DepthStencilBuffer;
		std::unique_ptr<GLTexture2D> _InternodeCaptureResult;
		std::unique_ptr<GLTexture2D> _FilteredResult;
		std::shared_ptr<Texture2D> _Mask;
		std::shared_ptr<Texture2D> _Skeleton;
		std::unique_ptr<GLTexture2D> _ProcessedMask;
		std::vector<float> _Data;
		std::vector<float> _SkeletonData;
		std::vector<glm::vec3> _MaskData;
		
		float _InternodeSize = 0.075f;
		float _IgnoreMaxHeight = 0.3f;
		float _IgnoreWidth = 0.1f;
		float _TrimFactor = 0.0f;
		int _MainBranchOrderProtection = 6;
		void Trim(int& totalChild, int& trimmedChild, std::map<int, Entity>& map, Entity internode);
	public:
		float RemovalDistance = 0.7f;
		float AttractionDistance = 1.0f;
		void ClearAttractionPoints() const;
		void PlaceAttractionPoints();
		void PreprocessMask() const;
		void ShotInternodes() const;
		void Filter() const;
		MaskProcessor();
		void Trim();
		void OnGui() override;
	};
}