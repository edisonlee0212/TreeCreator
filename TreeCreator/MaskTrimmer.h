#pragma once
#include "UniEngine.h"

using namespace UniEngine;
namespace TreeUtilities {
	class MaskTrimmer : public PrivateComponentBase
	{
		static std::unique_ptr<GLProgram> _FilterProgram;
		std::unique_ptr<RenderTarget> _Filter;
		std::unique_ptr<GLRenderBuffer> _DepthBuffer;
		std::unique_ptr<GLTexture2D> _Result;
	public:
		static void Init();
		MaskTrimmer();
		void Trim(Entity tree, Entity cameraEntity, std::shared_ptr<Texture2D> mask, unsigned amount = 1);
		void OnGui() override;
	};
}