#pragma once
#include "UniEngine.h"
using namespace UniEngine;
namespace TreeUtilities {
    enum BranchNodeSystemConfigFlags {
        BranchNodeSystem_None = 0,
        BranchNodeSystem_DrawBranchNodes = 1 << 0,
        BranchNodeSystem_DrawConnections = 1 << 1

    };
    class Connection;
    class BranchNodeSystem :
        public SystemBase
    {
        unsigned int _ConfigFlags = 0;
        EntityQuery _BranchNodeQuery;

        float _ConnectionWidth = 1.0f;

        Entity _RaySelectedEntity;
    	
        std::vector<LocalToWorld> _BranchNodeLTWList;
        std::vector<Connection> _ConnectionList;
        void DrawGui();
        void RaySelection();
    public:
        void OnCreate() override;
        void OnDestroy() override;
        void Update() override;
        void RefreshConnections() const;
    };
}
