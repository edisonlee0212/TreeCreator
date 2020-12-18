#pragma once
#include "UniEngine.h"
using namespace UniEngine;
namespace TreeUtilities {
    enum InternodeSystemConfigFlags {
        InternodeSystem_None = 0,
        InternodeSystem_DrawInternodes = 1 << 0,
        InternodeSystem_DrawConnections = 1 << 1,
        InternodeSystem_DrawCameraRays = 1 << 2

    };
    class Connection;
    class InternodeSystem :
        public SystemBase
    {
        friend class PlantSimulationSystem;
        friend class DataCollectionSystem;
        unsigned int _ConfigFlags = 0;
        EntityQuery _InternodeQuery;

        float _ConnectionWidth = 1.0f;
        glm::vec4 _ConnectionColor = glm::vec4(0.6f, 0.3f, 0, 0.8f);
        glm::vec4 _RayColor = glm::vec4(0.0f, 1.0f, 0.0f, 0.3f);
        Entity _RaySelectedEntity;
        Entity _CameraEntity;
        std::vector<GlobalTransform> _InternodeLTWList;
        std::vector<Connection> _ConnectionList;
        std::vector<Ray> _RayList;
        void DrawGui();
        void RaySelection();
    public:
    	unsigned GetConfigFlags() const
        {
            return _ConfigFlags;
    	}
        void OnCreate() override;
        void OnDestroy() override;
        void Update() override;
        void RefreshConnections() const;
    };
}
