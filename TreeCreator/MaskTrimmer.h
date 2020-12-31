#pragma once
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	class MaskTrimmer : public PrivateComponentBase
	{
		friend class DataCollectionSystem;
		static Entity _CameraEntity;
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
		std::unique_ptr<GLTexture2D> _ProcessedMask;
		std::vector<float> _Data;
		float _InternodeSize = 0.075f;
		float _IgnoreMaxHeight = 0.3f;
		float _IgnoreWidth = 0.1f;
		float _TrimFactor = 0.0f;
		int _MainBranchOrderProtection = 6;
		void Trim(int& totalChild, int& trimmedChild, std::map<int, Entity>& map, Entity internode);
	public:
		void PreprocessMask() const;
		void ShotInternodes() const;
		void Filter() const;
		MaskTrimmer();
		void Trim();
		void OnGui() override;
	};
}