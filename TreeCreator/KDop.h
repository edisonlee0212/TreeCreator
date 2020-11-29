#pragma once
#include "UniEngine.h"
#include "TreeManager.h"
#include "FoliageGeneratorBase.h"
using namespace UniEngine;
using namespace TreeUtilities;
namespace TreeUtilities {
	class KDop : public PrivateComponentBase
	{
	public:
		float DirectionalDistance[26];
		void Reset();
	};
}