#pragma once
#include "UniEngine.h"
using namespace UniEngine;
namespace TreeUtilities {
    enum InternodeSystemConfigFlags {
        InternodeSystem_None = 0,
        InternodeSystem_DrawInternodes = 1 << 0,
        InternodeSystem_DrawConnections = 1 << 1

    };
    class Connection;
    class InternodeSystem :
        public SystemBase
    {
        unsigned int _ConfigFlags = 0;
        EntityQuery _InternodeQuery;

        float _ConnectionWidth = 1.0f;

        Entity _RaySelectedEntity;
    	
        std::vector<LocalToWorld> _InternodeLTWList;
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
