using System.Collections.Generic;
using Unity.Burst;
using Unity.Collections;
using Unity.Entities;
using Unity.Jobs;
using Unity.Mathematics;
using Unity.Transforms;
using UnityEngine;
using UnityEngine.InputSystem;

namespace Universe
{
    public enum ControlMode
    {
        NoControl,
        StarCluster,
        PlanetarySystem,
        Planet
    }
    public class ControlSystem
    {
        #region Private
        #endregion

        #region Public
        private static ControlMode _ControlMode, _SavedControlMode;
        private static Controls _InputSystem;
        public static Controls InputSystem { get => _InputSystem; set => _InputSystem = value; }
        public static ControlMode ControlMode
        {
            get => _ControlMode;
            set
            {
                if (_InputSystem != null)
                {
                    switch (_ControlMode)
                    {
                        case ControlMode.StarCluster:
                            _InputSystem.StarCluster.Disable();
                            break;
                        case ControlMode.PlanetarySystem:
                            _InputSystem.PlanetarySystem.Disable();
                            break;
                        case ControlMode.Planet:
                            _InputSystem.Planet.Disable();
                            break;
                        default:
                            break;
                    }
                    switch (value)
                    {
                        case ControlMode.StarCluster:
                            _InputSystem.StarCluster.Enable();
                            break;
                        case ControlMode.PlanetarySystem:
                            _InputSystem.PlanetarySystem.Enable();
                            break;
                        case ControlMode.Planet:
                            _InputSystem.Planet.Enable();
                            break;
                        default:
                            break;
                    }
                }

                _ControlMode = value;
            }
        }

        public static ControlMode SavedControlMode { get => _SavedControlMode; set => _SavedControlMode = value; }
        #endregion

        #region Methods
        public static void Interrupt()
        {
            _SavedControlMode = _ControlMode;
            ControlMode = ControlMode.NoControl;
        }

        public static void Resume()
        {
            ControlMode = _SavedControlMode;
        }
        #endregion

        #region Managers
        public ControlSystem()
        {
            _InputSystem = new Controls();
            _InputSystem.StarCluster.Disable();
            _InputSystem.PlanetarySystem.Disable();
            _InputSystem.Planet.Disable();
            //Star Cluster
            _InputSystem.StarCluster.Cancel.performed += ctx => CentralSystem.OnStarClusterCancel(ctx);
            _InputSystem.StarCluster.BoxSelectionStart.performed += ctx => SelectionSystem.OnBoxSelectEnter(ctx);
            _InputSystem.StarCluster.BoxSelectionRelease.performed += ctx => SelectionSystem.OnBoxSelectRelease(ctx);
            _InputSystem.StarCluster.ToPlanetarySystem.performed += ctx => CentralSystem.StarClusterModeToPlanetarySystemMode(ctx);
            _InputSystem.StarCluster.CameraRotateStart.performed += ctx => CentralSystem.OnRotateCameraStart(ctx);
            _InputSystem.StarCluster.CameraRotateRelease.performed += ctx => CentralSystem.OnRotateCameraRelease(ctx);
            _InputSystem.StarCluster.FollowStar.performed += ctx => SelectionSystem.OnRaySelectEnter(ctx);
            //Planetary System
            _InputSystem.PlanetarySystem.Cancel.performed += ctx => CentralSystem.OnPlanetarySystemCancel(ctx);
            _InputSystem.PlanetarySystem.CameraRotateStart.performed += ctx => CentralSystem.OnRotateCameraStart(ctx);
            _InputSystem.PlanetarySystem.CameraRotateRelease.performed += ctx => CentralSystem.OnRotateCameraRelease(ctx);
            //Planet
        }
        #endregion
    }

    [UpdateInGroup(typeof(InitializationSystemGroup))]
    public class CentralSystem : JobComponentSystem
    {
        #region Resources
        private static RenderMeshResources m_RenderMeshResources;
        public static RenderMeshResources RenderMeshResources { get => m_RenderMeshResources; set => m_RenderMeshResources = value; }

        #endregion

        #region Subsystems
        #region Non-ECS
        private static ControlSystem m_ControlSystem;
        private static CameraModule m_CameraModule;
        private static DataSystem m_DataSystem;
        #endregion
        #region ECS
        private static WorldSystem m_WorldSystem;
        private static StarTransformSystem m_StarTransformSystem;
        private static SelectionSystem m_SelectionSystem;
        private static StarDisplayColorSystem m_StarDisplayColorSystem;
        private static StarRenderSystem m_RenderSystem;
        private static CopyStarColorSystem m_CopyStarColorSystem;
        private static PlanetarySystemSimulationSystem m_PlanetarySystemSimulationSystem;
        private static PlanetarySystemRenderSystem m_PlanetarySystemRenderSystem;
        #endregion
        #endregion

        #region Private
        private static RenderContent _CurrentStarRenderContent;
        #endregion

        #region Public
        public struct SwitchingModeInfo
        {
            public float TotalTime, Time;
            public float FromDistanceFactor, ToDistanceFactor;
            public float FromScaleFactor, ToScaleFactor;
            public float FromTimeSpeed, ToTimeSpeed;
            public ControlMode TargetControlMode;
            public Mesh TargetMesh;
            public Material TargetMaterial;
        }
        private static SwitchingModeInfo _SwitchingModeInfo;
        private static bool _IsRunning, _IsSwitching, _IsRotatingCamera;
        private static float _FixedUpdateTimer;
        private static float _SystemTime;
        private static int _TurnCount;
        public static float SystemTime { get => _SystemTime; set => _SystemTime = value; }
        public static SwitchingModeInfo SwitchingMode { get => _SwitchingModeInfo; set => _SwitchingModeInfo = value; }
        public static bool IsSwitching { get => _IsSwitching; set => _IsSwitching = value; }
        public static RenderContent CurrentStarRenderContent { get => _CurrentStarRenderContent; set => _CurrentStarRenderContent = value; }
        public static WorldSystem WorldSystem { get => m_WorldSystem; set => m_WorldSystem = value; }
        public static StarTransformSystem StarTransformSystem { get => m_StarTransformSystem; set => m_StarTransformSystem = value; }
        public static PlanetarySystemRenderSystem PlanetarySystemRenderSystem { get => m_PlanetarySystemRenderSystem; set => m_PlanetarySystemRenderSystem = value; }
        public static SelectionSystem SelectionSystem { get => m_SelectionSystem; set => m_SelectionSystem = value; }
        public static StarRenderSystem RenderSystem { get => m_RenderSystem; set => m_RenderSystem = value; }
        public static CopyStarColorSystem CopyStarColorSystem { get => m_CopyStarColorSystem; set => m_CopyStarColorSystem = value; }
        public static StarDisplayColorSystem StarDisplayColorSystem { get => m_StarDisplayColorSystem; set => m_StarDisplayColorSystem = value; }
        public static ControlSystem ControlSystem { get => m_ControlSystem; set => m_ControlSystem = value; }
        public static CameraModule CameraModule { get => m_CameraModule; set => m_CameraModule = value; }
        public static bool IsRotatingCamera { get => _IsRotatingCamera; set => _IsRotatingCamera = value; }
        public static bool IsRunning { get => _IsRunning; set => _IsRunning = value; }
        public static float FixedUpdateTimer { get => _FixedUpdateTimer; set => _FixedUpdateTimer = value; }
        public static PlanetarySystemSimulationSystem PlanetarySystemSimulationSystem { get => m_PlanetarySystemSimulationSystem; set => m_PlanetarySystemSimulationSystem = value; }
        public static DataSystem DataSystem { get => m_DataSystem; set => m_DataSystem = value; }
        public static int TurnCount { get => _TurnCount; set => _TurnCount = value; }
        #endregion

        #region Managers
        protected override void OnCreate()
        {
            Init();
        }

        public void Init()
        {
            _IsRunning = true;
            m_RenderMeshResources = Resources.Load<RenderMeshResources>("RenderMeshResources");
            m_CameraModule = Resources.Load<CameraModule>("Modules/Camera Module");
            m_CameraModule.Init();

            SelectionSystem.Camera = Camera.main;
            m_WorldSystem = World.Active.GetOrCreateSystem<WorldSystem>();
            m_StarTransformSystem = World.Active.GetOrCreateSystem<StarTransformSystem>();
            m_SelectionSystem = World.Active.GetOrCreateSystem<SelectionSystem>();
            m_RenderSystem = World.Active.GetOrCreateSystem<StarRenderSystem>();
            m_StarDisplayColorSystem = World.Active.GetOrCreateSystem<StarDisplayColorSystem>();
            m_CopyStarColorSystem = World.Active.GetOrCreateSystem<CopyStarColorSystem>();
            m_PlanetarySystemRenderSystem = World.Active.GetOrCreateSystem<PlanetarySystemRenderSystem>();
            m_PlanetarySystemSimulationSystem = World.Active.GetOrCreateSystem<PlanetarySystemSimulationSystem>();
            m_DataSystem = new DataSystem();
            m_WorldSystem.Init();
            m_StarTransformSystem.Init();
            m_SelectionSystem.Init();
            m_StarDisplayColorSystem.Init();
            m_CopyStarColorSystem.Init();
            m_RenderSystem.Init();
            m_PlanetarySystemSimulationSystem.Init();
            m_PlanetarySystemRenderSystem.Init();
            m_ControlSystem = new ControlSystem();

            ControlSystem.ControlMode = ControlMode.StarCluster;

            StarClusterPattern pattern = new StarClusterPattern
            {
                Index = new StarClusterIndex { Value = 0 },
                YSpread = 0.05D,
                XZSpread = 0.015D,
                DiskAB = 3000D,
                DiskEccentricity = 0.5D,
                CoreProportion = 0.4D,
                CoreEccentricity = 0.8D,
                CenterAB = 10D,
                CenterEccentricity = 0.5D,
                DiskSpeed = 1,
                CoreSpeed = 5,
                CenterSpeed = 10,
                DiskTiltX = 0,
                DiskTiltZ = 0,
                CoreTiltX = 0,
                CoreTiltZ = 0,
                CenterTiltX = 0,
                CenterTiltZ = 0,
                DiskColor = Color.blue,
                CoreColor = Color.yellow,
                CenterColor = Color.white,
                Rotation = 360,
                CenterPosition = new double3(0)
            };
            pattern.SetAB();
            WorldSystem.AddStarClusterPattern(pattern);

            _CurrentStarRenderContent = new RenderContent { MeshMaterial = new MeshMaterial() };
            _CurrentStarRenderContent.MeshMaterial.Material = m_RenderMeshResources.Materials[0];
            _CurrentStarRenderContent.MeshMaterial.Mesh = m_RenderMeshResources.Meshes[0];

            for (int i = 0; i < 6000; i++)
            {
                var value = Random.Next();
                var proportion = new StarOrbitProportion { Value = System.Math.Sqrt(value) };
                WorldSystem.AddStar(pattern.Index, proportion, _CurrentStarRenderContent, new StarSeed { Value = Random.Next() });
            }
        }

        public void ShutDown()
        {
            m_WorldSystem.ShutDown();
            m_StarTransformSystem.ShutDown();
            m_SelectionSystem.ShutDown();
            m_RenderSystem.ShutDown();
            m_CameraModule.ShutDown();
        }
        protected override void OnDestroy()
        {
            if (_IsRunning) ShutDown();
        }
        #endregion

        #region Methods
        public static void StarClusterModeToPlanetarySystemMode(InputAction.CallbackContext ctx)
        {
            if (_IsSwitching || StarTransformSystem.FollowingEntity == Entity.Null) return;
            ControlSystem.Interrupt();
            //TODO: Switch to planetary system.
            _SwitchingModeInfo.TargetControlMode = ControlMode.PlanetarySystem;
            _SwitchingModeInfo.TotalTime = 2f;
            _SwitchingModeInfo.Time = 0;
            _SwitchingModeInfo.FromDistanceFactor = 0;
            _SwitchingModeInfo.ToDistanceFactor = 1;
            _SwitchingModeInfo.FromScaleFactor = StarTransformSystem.ScaleFactor;
            _SwitchingModeInfo.ToScaleFactor = 1;
            _SwitchingModeInfo.FromTimeSpeed = StarTransformSystem.TimeSpeed;
            _SwitchingModeInfo.ToTimeSpeed = 0;
            _SwitchingModeInfo.TargetMaterial = m_RenderMeshResources.Materials[1];
            _SwitchingModeInfo.TargetMesh = m_RenderMeshResources.Meshes[1];
            _IsSwitching = true;
        }

        public static void OnStarClusterCancel(InputAction.CallbackContext ctx)
        {
            if (StarTransformSystem.FollowingEntity != Entity.Null) StarTransformSystem.UnFollow();
            else SelectionSystem.OnSelectionStatusReset();
        }

        public static void OnPlanetarySystemCancel(InputAction.CallbackContext ctx)
        {
            if (_IsSwitching) return;

            ControlSystem.Interrupt();
            //TODO: Switch to star cluster.
            _SwitchingModeInfo.TargetControlMode = ControlMode.StarCluster;
            _SwitchingModeInfo.TotalTime = 2f;
            _SwitchingModeInfo.Time = 0;
            _SwitchingModeInfo.FromDistanceFactor = 1;
            _SwitchingModeInfo.ToDistanceFactor = 0;
            _SwitchingModeInfo.FromScaleFactor = StarTransformSystem.ScaleFactor;
            _SwitchingModeInfo.ToScaleFactor = 1;
            _SwitchingModeInfo.FromTimeSpeed = 0;
            _SwitchingModeInfo.ToTimeSpeed = .1f;
            _SwitchingModeInfo.TargetMaterial = m_RenderMeshResources.Materials[0];
            _SwitchingModeInfo.TargetMesh = m_RenderMeshResources.Meshes[0];
            _CurrentStarRenderContent.MeshMaterial.Mesh = _SwitchingModeInfo.TargetMesh;
            _CurrentStarRenderContent.MeshMaterial.Material = _SwitchingModeInfo.TargetMaterial;
            _IsSwitching = true;
        }

        public static void OnPlanetCancel(InputAction.CallbackContext ctx)
        {
        }

        public static void OnRotateCameraStart(InputAction.CallbackContext ctx)
        {
            _IsRotatingCamera = true;
        }

        public static void OnRotateCameraRelease(InputAction.CallbackContext ctx)
        {
            _IsRotatingCamera = false;
        }
        #endregion

        protected JobHandle OnFixedUpdate(JobHandle inputDeps)
        {
            inputDeps = m_PlanetarySystemRenderSystem.OnFixedUpdate(inputDeps);
            

            return inputDeps;
        }

        protected JobHandle OnEnterNextTurn(JobHandle inputDeps)
        {
            _TurnCount++;
            inputDeps = m_DataSystem.OnEnterNextTurn(this, inputDeps, _TurnCount);
            return inputDeps;
        }

        protected JobHandle OnExitNextTurn(JobHandle inputDeps)
        {
            inputDeps = m_DataSystem.OnExitNextTurn(this, inputDeps, _TurnCount);
            return inputDeps;
        }

        protected override JobHandle OnUpdate(JobHandle inputDeps)
        {
            _SystemTime += Time.deltaTime;
            _FixedUpdateTimer += Time.deltaTime;
            if(_FixedUpdateTimer > 0.1)
            {
                inputDeps = OnFixedUpdate(inputDeps);
                inputDeps = OnEnterNextTurn(inputDeps);
                inputDeps = OnExitNextTurn(inputDeps);
                _FixedUpdateTimer = 0;
            }

            

            if (_IsRotatingCamera)
            {
                m_CameraModule.Update();
            }
            if (_IsSwitching)
            {
                switch (_SwitchingModeInfo.TargetControlMode)
                {
                    case ControlMode.PlanetarySystem:
                        StarTransformSystem.DistanceFactor = Mathf.Lerp(_SwitchingModeInfo.FromDistanceFactor, _SwitchingModeInfo.ToDistanceFactor, (_SwitchingModeInfo.Time / _SwitchingModeInfo.TotalTime));
                        StarTransformSystem.TimeSpeed = Mathf.Lerp(_SwitchingModeInfo.FromTimeSpeed, _SwitchingModeInfo.ToTimeSpeed, (_SwitchingModeInfo.Time / _SwitchingModeInfo.TotalTime));
                        StarTransformSystem.ScaleFactor = Mathf.Lerp(_SwitchingModeInfo.FromScaleFactor, _SwitchingModeInfo.ToScaleFactor, (_SwitchingModeInfo.Time / _SwitchingModeInfo.TotalTime));
                        StarTransformSystem.OnTransition(this, inputDeps);
                        break;
                    case ControlMode.StarCluster:
                        StarTransformSystem.DistanceFactor = Mathf.Lerp(_SwitchingModeInfo.FromDistanceFactor, _SwitchingModeInfo.ToDistanceFactor, (_SwitchingModeInfo.Time / _SwitchingModeInfo.TotalTime));
                        StarTransformSystem.TimeSpeed = Mathf.Lerp(_SwitchingModeInfo.FromTimeSpeed, _SwitchingModeInfo.ToTimeSpeed, (_SwitchingModeInfo.Time / _SwitchingModeInfo.TotalTime));
                        StarTransformSystem.ScaleFactor = Mathf.Lerp(_SwitchingModeInfo.FromScaleFactor, _SwitchingModeInfo.ToScaleFactor, (_SwitchingModeInfo.Time / _SwitchingModeInfo.TotalTime));
                        StarTransformSystem.OnTransition(this, inputDeps);
                        break;
                    default:
                        break;
                }                
                _SwitchingModeInfo.Time += Time.deltaTime;
                if (_SwitchingModeInfo.Time >= _SwitchingModeInfo.TotalTime)
                {
                    _IsSwitching = false;
                    ControlSystem.ControlMode = _SwitchingModeInfo.TargetControlMode;
                    switch (_SwitchingModeInfo.TargetControlMode)
                    {
                        case ControlMode.PlanetarySystem:
                            StarTransformSystem.DistanceFactor = _SwitchingModeInfo.ToDistanceFactor;
                            _CurrentStarRenderContent.MeshMaterial.Mesh = _SwitchingModeInfo.TargetMesh;
                            _CurrentStarRenderContent.MeshMaterial.Material = _SwitchingModeInfo.TargetMaterial;
                            SelectionSystem.OnSelectionStatusReset();

                            PlanetarySystemSimulationSystem.LoadPlanetarySystem();
                            StarTransformSystem.Enabled = false;
                            break;
                        case ControlMode.StarCluster:
                            StarTransformSystem.DistanceFactor = _SwitchingModeInfo.ToDistanceFactor;
                            _CurrentStarRenderContent.MeshMaterial.Mesh = _SwitchingModeInfo.TargetMesh;
                            _CurrentStarRenderContent.MeshMaterial.Material = _SwitchingModeInfo.TargetMaterial;
                            SelectionSystem.OnSelectionStatusReset();

                            PlanetarySystemSimulationSystem.Clear();
                            StarTransformSystem.Enabled = true;
                            break;
                        default: break;
                    }
                    
                }
            }
            return inputDeps;
        }
    }

    [UpdateInGroup(typeof(InitializationSystemGroup))]
    public class WorldSystem : JobComponentSystem, ISubSystem
    {
        #region Archetypes
        private static EntityArchetype _StarEntityArchetype;
        #endregion

        #region Private
        private static int _LastStarClusterIndex;
        private static EntityManager m_EntityManager;
        private struct StarInfo
        {
            public RenderContent RenderMesh;
            public StarClusterIndex StarClusterIndex;
            public StarOrbitProportion StarOrbitProportion;
            public StarSeed StarSeed;
        }

        private static Queue<StarInfo> _StarCreationQueue;
        private static Queue<Entity> _StarDestructionQueue;

        #endregion

        #region Public

        private static List<StarClusterPattern> _StarClusterPatterns;
        private static int _StarAmount;
        public static List<StarClusterPattern> StarClusterPatterns { get => _StarClusterPatterns; set => _StarClusterPatterns = value; }
        public static int StarAmount { get => _StarAmount; set => _StarAmount = value; }

        #endregion

        #region Managers
        protected override void OnCreate()
        {
            Enabled = false;
            _LastStarClusterIndex = 0;
            m_EntityManager = World.Active.EntityManager;
            _StarClusterPatterns = new List<StarClusterPattern>();
            _StarCreationQueue = new Queue<StarInfo>();
            _StarDestructionQueue = new Queue<Entity>();
            _StarEntityArchetype = EntityManager.CreateArchetype(
                typeof(Position),
                typeof(Translation),
                typeof(Rotation),
                typeof(Scale),
                typeof(LocalToWorld),
                typeof(Index),
                typeof(StarSeed),
                typeof(StarOrbitProportion),
                typeof(OriginalColor),
                typeof(SurfaceColor),
                typeof(DisplayColor),
                typeof(SelectionStatus),
                typeof(StarOrbit),
                typeof(StarClusterIndex),
                typeof(RenderContent),
                typeof(EnergyData)
                );
        }
        public void Init()
        {
            ShutDown();

            Enabled = true;
        }

        public void ShutDown()
        {
            Enabled = false;
        }
        protected override void OnDestroy()
        {
            ShutDown();
        }

        #endregion

        #region Method
        public static void AddStarClusterPattern(StarClusterPattern starClusterPattern)
        {
            _StarClusterPatterns.Add(starClusterPattern);
            _LastStarClusterIndex++;
        }

        public static void AddStar(StarClusterIndex starClusterIndex, StarOrbitProportion proportion, RenderContent renderMesh, StarSeed starSeed)
        {
            _StarCreationQueue.Enqueue(new StarInfo
            {
                StarSeed = starSeed,
                RenderMesh = renderMesh,
                StarClusterIndex = starClusterIndex,
                StarOrbitProportion = proportion,
            });
        }

        public static void RemoveStar(Entity starEntity)
        {
            _StarDestructionQueue.Enqueue(starEntity);
        }
        #endregion

        protected override JobHandle OnUpdate(JobHandle inputDeps)
        {

            if (_StarCreationQueue.Count != 0)
            {
                int count = _StarCreationQueue.Count;
                for (int i = 0; i < 100 && i < count; i++)
                {
                    var starInfo = _StarCreationQueue.Dequeue();
                    CreateStar(ref inputDeps, starInfo);
                }
            }
            else if (_StarDestructionQueue.Count != 0)
            {
                int count = _StarDestructionQueue.Count;
                for (int i = 0; i < 100 && i < count; i++)
                {
                    var starEntity = _StarDestructionQueue.Dequeue();
                    DestroyStar(ref inputDeps, starEntity);
                }
            }
            return inputDeps;
        }

        private void CreateStar(ref JobHandle inputDeps, StarInfo starInfo)
        {
            Entity instance = EntityManager.CreateEntity(_StarEntityArchetype);
            var orbitOffset = _StarClusterPatterns[starInfo.StarClusterIndex.Value].GetOrbitOffset(starInfo.StarOrbitProportion.Value);
            //Debug.Log("Proportion: " + starInfo.StarOrbitProportion.Value);
            EntityManager.SetSharedComponentData(instance, starInfo.RenderMesh);
            EntityManager.SetComponentData(instance, starInfo.StarClusterIndex);
            StarOrbit orbit = (StarOrbit)_StarClusterPatterns[starInfo.StarClusterIndex.Value].GetOrbit(starInfo.StarOrbitProportion.Value, ref orbitOffset);
            EntityManager.SetComponentData(instance, orbit);
            //Debug.Log(orbit.A + " + " + orbit.B);
            EntityManager.SetComponentData(instance, starInfo.StarOrbitProportion);
            EntityManager.SetComponentData(instance, starInfo.StarSeed);
            EntityManager.SetComponentData(instance, new DisplayColor { Value = Color.red });
            OriginalColor originalColor = default;
            originalColor.Value = new Color(Random.Next(), Random.Next(), Random.Next(), 1);
            EntityManager.SetComponentData(instance, originalColor);
            EntityManager.SetComponentData(instance, new EnergyData { TotalEnergy = 600000000000, ReleaseSpeed = 1 });
            _StarAmount++;
        }

        private void DestroyStar(ref JobHandle inputDeps, Entity starEntity)
        {
            EntityManager.DestroyEntity(starEntity);
            _StarAmount--;
        }
    }

    [UpdateInGroup(typeof(LateSimulationSystemGroup))]
    public class SelectionSystem : JobComponentSystem, ISubSystem
    {
        #region Private
        private static NativeQueue<Entity> _RayCastResultEntities;
        private static NativeQueue<float> _Distances;
        private static Vector3 _SavedMousePosition;

        #endregion

        #region Public
        private static Camera m_Camera;
        private static float _MaxRayCastDistance;
        private static Entity m_RayCastSelectedEntity;
        private static bool _EnableRaySelection, _EnableBoxSelection, _Reset;
        public static Camera Camera { get => m_Camera; set => m_Camera = value; }
        public static float MaxRayCastDistance { get => _MaxRayCastDistance; set => _MaxRayCastDistance = value; }
        public static Entity RayCastSelectedEntity { get => m_RayCastSelectedEntity; set => m_RayCastSelectedEntity = value; }
        public static bool EnableRaySelection { get => _EnableRaySelection; }
        public static bool EnableBoxSelection { get => _EnableBoxSelection; }
        public static bool Reset { get => _Reset; }
        #endregion

        #region Managers
        protected override void OnCreate()
        {
            Enabled = false;
            m_Camera = Camera.main;
        }
        public void Init()
        {
            ShutDown();
            _RayCastResultEntities = new NativeQueue<Entity>(Allocator.Persistent);
            _Distances = new NativeQueue<float>(Allocator.Persistent);
            _MaxRayCastDistance = 10000;
            _EnableRaySelection = true;
            _EnableBoxSelection = false;
            Enabled = true;
        }

        public void ShutDown()
        {
            Enabled = false;
            if (_RayCastResultEntities.IsCreated) _RayCastResultEntities.Dispose();
            if (_Distances.IsCreated) _Distances.Dispose();
        }
        protected override void OnDestroy()
        {
            ShutDown();
        }

        #endregion

        #region Methods
        public static void OnRaySelectEnter(InputAction.CallbackContext ctx)
        {
            if(m_RayCastSelectedEntity != Entity.Null)
            {
                StarTransformSystem.FollowEntity(m_RayCastSelectedEntity);
            }
        }

        public static void OnBoxSelectEnter(InputAction.CallbackContext ctx)
        {
            _EnableRaySelection = false;
            _SavedMousePosition = Input.mousePosition;
            _EnableBoxSelection = true;
        }

        public static void OnBoxSelectRelease(InputAction.CallbackContext ctx)
        {
            _EnableBoxSelection = false;
            _EnableRaySelection = true;
        }

        public static void OnSelectionStatusReset()
        {
            m_RayCastSelectedEntity = Entity.Null;
            _Reset = true;
        }
        #endregion

        #region Jobs
        [BurstCompile]
        struct RaySelectionJob : IJobForEachWithEntity<Scale, LocalToWorld>
        {
            [ReadOnly] public Vector3 start;
            [ReadOnly] public Vector3 end;
            [ReadOnly] public float rayCastDistance;
            [WriteOnly] public NativeQueue<Entity>.ParallelWriter rayCastResultEntities;
            [WriteOnly] public NativeQueue<float>.ParallelWriter rayCastDistances;

            public void Execute([ReadOnly] Entity entity, [ReadOnly] int index, [ReadOnly] ref Scale c1, [ReadOnly] ref LocalToWorld c3)
            {
                float d;
                float scale = c1.Value;
                float3 position = c3.Position;
                if (Vector3.Distance(position, start) <= rayCastDistance && Vector3.Angle(end - start, position - (float3)start) < 80)
                {
                    d = Vector3.Dot((position - (float3)start), (end - start)) / rayCastDistance;
                    float ap = Vector3.Distance(position, (float3)start);
                    if ((ap + d) * (ap - d) < scale * scale)
                    {
                        rayCastResultEntities.Enqueue(entity);
                        rayCastDistances.Enqueue(ap);
                    }
                }
                d = Vector3.Distance(Vector3.zero, position);
            }
        }

        [BurstCompile]
        public struct BoxSelectionJob : IJobForEach<LocalToWorld, SelectionStatus>
        {
            [ReadOnly] public float4 p0, p1, p2, p3, p4;
            [ReadOnly] public int mode;
            public void Execute([ReadOnly] ref LocalToWorld c0, [WriteOnly] ref SelectionStatus c1)
            {

                float3 pos = c0.Position;
                float radius = c0.Value.c0.x;
                bool selected = true;
                if (p0.x * pos.x + p0.y * pos.y + p0.z * pos.z + p0.w <= radius) selected = false;
                if (p1.x * pos.x + p1.y * pos.y + p1.z * pos.z + p1.w <= radius) selected = false;
                if (p2.x * pos.x + p2.y * pos.y + p2.z * pos.z + p2.w <= radius) selected = false;
                if (p3.x * pos.x + p3.y * pos.y + p3.z * pos.z + p3.w <= radius) selected = false;
                if (p4.x * pos.x + p4.y * pos.y + p4.z * pos.z + p4.w <= radius) selected = false;
                if (selected) c1.Value = 2;
                else c1.Value = 0;
            }
        }

        [BurstCompile]
        public struct ResetSelectionStatus : IJobForEach<SelectionStatus>
        {
            public void Execute([WriteOnly] ref SelectionStatus c0)
            {
                c0.Value = 0;
            }
        }
        #endregion

        protected override JobHandle OnUpdate(JobHandle inputDeps)
        {
            if (_EnableRaySelection)
            {
                UnityEngine.Ray ray = m_Camera.ScreenPointToRay(Input.mousePosition);
                //If we are in galaxy view, then scan for stars. Else scan for planets.
                inputDeps = new RaySelectionJob
                {
                    start = ray.origin,
                    end = ray.origin + ray.direction * _MaxRayCastDistance,
                    rayCastDistance = _MaxRayCastDistance,
                    rayCastResultEntities = _RayCastResultEntities.AsParallelWriter(),
                    rayCastDistances = _Distances.AsParallelWriter(),
                }.Schedule(this, inputDeps);
                inputDeps.Complete();
                float min = MaxRayCastDistance;
                while (_Distances.Count > 0)
                {
                    Entity e = _RayCastResultEntities.Dequeue();
                    float f = _Distances.Dequeue();
                    if (f < min)
                    {
                        min = f;
                        m_RayCastSelectedEntity = e;
                        EntityManager.SetComponentData(e, new SelectionStatus { Value = 1 });
                    }
                }
            }
            else if (_EnableBoxSelection)
            {
                float4[] clippingPlanes = new float4[5];
                CalculateClippingPlanes(ref clippingPlanes, -1);
                inputDeps = new BoxSelectionJob
                {
                    p0 = clippingPlanes[0],
                    p1 = clippingPlanes[1],
                    p2 = clippingPlanes[2],
                    p3 = clippingPlanes[3],
                    p4 = clippingPlanes[4]
                }.Schedule(this, inputDeps);
                inputDeps.Complete();
            }
            if (_Reset)
            {
                _Reset = false;
                
                inputDeps = new ResetSelectionStatus { }.Schedule(this, inputDeps);
                inputDeps.Complete();
            }
            return inputDeps;
        }

        private static void CalculateClippingPlanes(ref float4[] clippingPlanes, float farDistance = -1)
        {
            m_Camera = Camera.main;

            if (farDistance == -1) farDistance = m_Camera.farClipPlane;
            UnityEngine.Plane plane = default;
            Vector3 currentMousePosition = Input.mousePosition;
            Vector3[] points = new Vector3[4];
            if ((currentMousePosition.x - _SavedMousePosition.x) * (currentMousePosition.y - _SavedMousePosition.y) < 0)
            {
                points[0] = m_Camera.ScreenToWorldPoint(new Vector3(_SavedMousePosition.x, _SavedMousePosition.y, farDistance));
                points[1] = m_Camera.ScreenToWorldPoint(new Vector3(_SavedMousePosition.x, currentMousePosition.y, farDistance));
                points[2] = m_Camera.ScreenToWorldPoint(new Vector3(currentMousePosition.x, currentMousePosition.y, farDistance));
                points[3] = m_Camera.ScreenToWorldPoint(new Vector3(currentMousePosition.x, _SavedMousePosition.y, farDistance));
            }
            else
            {
                points[3] = m_Camera.ScreenToWorldPoint(new Vector3(_SavedMousePosition.x, _SavedMousePosition.y, farDistance));
                points[2] = m_Camera.ScreenToWorldPoint(new Vector3(_SavedMousePosition.x, currentMousePosition.y, farDistance));
                points[1] = m_Camera.ScreenToWorldPoint(new Vector3(currentMousePosition.x, currentMousePosition.y, farDistance));
                points[0] = m_Camera.ScreenToWorldPoint(new Vector3(currentMousePosition.x, _SavedMousePosition.y, farDistance));
            }
            plane.Set3Points(m_Camera.transform.position, points[0], points[1]);
            clippingPlanes[0] = ToFloat4(ref plane);
            plane.Set3Points(m_Camera.transform.position, points[1], points[2]);
            clippingPlanes[1] = ToFloat4(ref plane);
            plane.Set3Points(m_Camera.transform.position, points[2], points[3]);
            clippingPlanes[2] = ToFloat4(ref plane);
            plane.Set3Points(m_Camera.transform.position, points[3], points[0]);
            clippingPlanes[3] = ToFloat4(ref plane);
            plane.Set3Points(points[2], points[1], points[0]);
            clippingPlanes[4] = ToFloat4(ref plane);
        }

        private static float4 ToFloat4(ref UnityEngine.Plane plane)
        {
            float4 ret = default;
            ret.x = plane.normal.x;
            ret.y = plane.normal.y;
            ret.z = plane.normal.z;
            ret.w = plane.distance;
            return ret;
        }
    }

}
