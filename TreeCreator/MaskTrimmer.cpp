#include "MaskTrimmer.h"
#include "TreeManager.h"
void TreeUtilities::MaskTrimmer::Init()
{
	
}

TreeUtilities::MaskTrimmer::MaskTrimmer()
{
	
}

void TreeUtilities::MaskTrimmer::Trim(Entity tree, Entity cameraEntity, std::shared_ptr<Texture2D> mask, unsigned amount)
{
	auto& cameraComponent = cameraEntity.GetPrivateComponent<CameraComponent>();
	
}

void TreeUtilities::MaskTrimmer::OnGui()
{
}
