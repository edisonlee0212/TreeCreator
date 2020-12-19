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
		static std::unique_ptr<GLProgram> _FilterProgram;
		static std::unique_ptr<RenderTarget> _Filter;
		static std::unique_ptr<GLRenderBuffer> _DepthStencilBuffer;
		std::unique_ptr<GLTexture2D> _Result;
		std::shared_ptr<Texture2D> _Mask;
		float _InternodeSize = 0.075f;
	public:
		void ShotInternodes() const;
		void Filter();
		MaskTrimmer();
		void Trim();
		void OnGui() override;
	};
}