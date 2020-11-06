#pragma once
#include "UniEngine.h"
using namespace UniEngine;
namespace TreeUtilities {
    enum TreeSystemConfigFlags {
        TreeSystem_None = 0,
        TreeSystem_DrawTrees = 1 << 0,
        TreeSystem_DrawTreeMeshes = 1 << 1
    };
    class TreeSystem :
        public SystemBase
    {
#pragma region GUI related
        float _MeshGenerationResolution = 0.01f;
        float _MeshGenerationSubdivision = 1.0f;
        char _MeshOBJFileName[256] = {};
#pragma endregion


        unsigned int _ConfigFlags = 0;
        EntityQuery _LeafQuery;
        EntityQuery _TreeQuery;
        void DrawGUI();
    public:
        void OnCreate() override;
        void OnDestroy() override;
        void Update() override;
        void FixedUpdate() override;
    };
}
