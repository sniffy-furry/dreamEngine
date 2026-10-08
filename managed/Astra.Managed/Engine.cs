using System.Diagnostics;
using System.Numerics;
using System.Runtime.InteropServices;

namespace Astra;

public readonly record struct EntityId(ulong Value) { public static readonly EntityId Invalid = new(0); }

public abstract class Component { public EntityId Entity { get; internal set; } }

public sealed class Transform : Component {
    public Vector3 LocalPosition { get; set; }
    public Quaternion LocalRotation { get; set; } = Quaternion.Identity;
    public Vector3 LocalScale { get; set; } = Vector3.One;
    public EntityId Parent { get; private set; } = EntityId.Invalid;
    public Matrix4x4 LocalMatrix => Matrix4x4.CreateScale(LocalScale) * Matrix4x4.CreateFromQuaternion(LocalRotation) * Matrix4x4.CreateTranslation(LocalPosition);
    public Matrix4x4 WorldMatrix { get; internal set; } = Matrix4x4.Identity;
    public Vector3 WorldPosition => new(WorldMatrix.M41,WorldMatrix.M42,WorldMatrix.M43);
    public Vector3 EulerAnglesRadians { get { var q=LocalRotation; var sinr=2*(q.W*q.X+q.Y*q.Z); var cosr=1-2*(q.X*q.X+q.Y*q.Y); var roll=MathF.Atan2(sinr,cosr); var sinp=2*(q.W*q.Y-q.Z*q.X); var pitch=MathF.Abs(sinp)>=1?MathF.CopySign(MathF.PI/2,sinp):MathF.Asin(sinp); var siny=2*(q.W*q.Z+q.X*q.Y); var cosy=1-2*(q.Y*q.Y+q.Z*q.Z); return new(roll,pitch,MathF.Atan2(siny,cosy)); } }
    public void SetParent(EntityId parent) => Parent = parent;
    public Vector3 Forward => Vector3.Transform(Vector3.UnitZ, LocalRotation);
}

public abstract class Behaviour : Component {
    public bool Enabled { get; set; } = true;
    public virtual void OnInitialize() { }
    public virtual void OnEnable() { }
    public virtual void OnFixedUpdate(float fixedDeltaTime) { }
    public virtual void OnUpdate(float deltaTime) { }
    public virtual void OnLateUpdate(float deltaTime) { }
    public virtual void OnDisable() { }
    public virtual void OnDestroy() { }
}

public sealed class GameObject {
    readonly Dictionary<Type, Component> components = new();
    readonly List<GameObject> children = new();
    public EntityId Id { get; }
    public string Name { get; set; }
    public bool ActiveSelf { get; set; } = true;
    public GameObject? Parent { get; private set; }
    public Transform Transform { get; }
    public IReadOnlyList<GameObject> Children => children;
    internal GameObject(EntityId id, string name) { Id=id; Name=name; Transform=AddComponent<Transform>(); }
    public T AddComponent<T>() where T:Component,new(){var c=new T{Entity=Id}; components[typeof(T)]=c; return c;}
    public T? GetComponent<T>() where T:Component => components.TryGetValue(typeof(T), out var c) ? (T)c : null;
    public bool RemoveComponent<T>() where T:Component => components.Remove(typeof(T));
    public void SetParent(GameObject? parent){ Parent?.children.Remove(this); Parent=parent; parent?.children.Add(this); Transform.SetParent(parent?.Id??EntityId.Invalid); }
}

public sealed class Scene {
    readonly Dictionary<EntityId,GameObject> entities = new();
    ulong nextId;
    public string Name { get; }
    public IReadOnlyDictionary<EntityId,GameObject> Entities => entities;
    public Scene(string name="Untitled")=>Name=name;
    public GameObject Create(string name="GameObject",GameObject? parent=null){var go=new GameObject(new EntityId(++nextId),name);entities.Add(go.Id,go);go.SetParent(parent);return go;}
    public bool Destroy(GameObject go){foreach(var child in go.Children.ToArray())Destroy(child);go.SetParent(null);return entities.Remove(go.Id);}
    public void UpdateWorldTransforms(){foreach(var root in entities.Values.Where(e=>e.Parent is null)){UpdateWorldRecursive(root,Matrix4x4.Identity);}}
    void UpdateWorldRecursive(GameObject go,Matrix4x4 parent){go.Transform.WorldMatrix=go.Transform.LocalMatrix*parent;foreach(var c in go.Children)UpdateWorldRecursive(c,go.Transform.WorldMatrix);}
}

public sealed class Prefab {
    public required Guid Guid { get; init; }
    public required GameObjectTemplate Root { get; init; }
    public List<PrefabOverride> Overrides { get; } = new();
}
public sealed class GameObjectTemplate { public string Name { get; init; }="GameObject"; public Vector3 Position { get; init; } ; public List<GameObjectTemplate> Children { get; init; }=new(); }
public sealed record PrefabOverride(string PropertyPath,string JsonValue);

public abstract class ScriptableObject { public Guid AssetGuid { get; init; }=Guid.NewGuid(); }

public sealed record AssetRecord(Guid Guid,string Path,string Importer,string MetaPath,IReadOnlyList<Guid> Dependencies);
public interface IAssetImporter { string FileExtension { get; } object Import(string path); }
public sealed class AssetDatabase {
    readonly Dictionary<Guid,AssetRecord> byGuid=new(); readonly Dictionary<string,Guid> byPath=new(StringComparer.OrdinalIgnoreCase);
    public IEnumerable<AssetRecord> Assets=>byGuid.Values;
    public void Register(AssetRecord asset){byGuid[asset.Guid]=asset;byPath[Path.GetFullPath(asset.Path)]=asset.Guid;}
    public AssetRecord? Find(Guid guid)=>byGuid.GetValueOrDefault(guid);
    public AssetRecord? Find(string path)=>byPath.TryGetValue(Path.GetFullPath(path),out var id)?Find(id):null;
}

public sealed record PackageManifest(string Name,string Version,string Source,IReadOnlyDictionary<string,string> Dependencies);
public sealed class PackageManager { readonly Dictionary<string,PackageManifest> packages=new(); public void Add(PackageManifest p)=>packages[p.Name]=p; public PackageManifest? Get(string n)=>packages.GetValueOrDefault(n); public IReadOnlyCollection<PackageManifest> All=>packages.Values; }
public sealed record AssemblyDefinition(string Name,IReadOnlyList<string> References,IReadOnlyList<string> Platforms,bool AllowUnsafe);

public sealed class PlayerLoop {
    public double TimeSeconds { get; private set; }
    public double FixedStep { get; set; }=1.0/60.0;
    double accumulator;
    public void Tick(double delta,Action fixedUpdate,Action update,Action lateUpdate){delta=Math.Clamp(delta,0,0.25);accumulator+=delta;TimeSeconds+=delta;while(accumulator>=FixedStep){fixedUpdate();accumulator-=FixedStep;}update();lateUpdate();}
}

public interface IAwaitableScheduler { ValueTask Yield(); }
public sealed class Coroutine { public required IEnumerator<TimeSpan> Enumerator { get; init; } public TimeSpan Remaining { get; set; } }

public interface IJob { void Execute(); }
public readonly record struct JobHandle(Task Task) { public void Complete()=>Task.GetAwaiter().GetResult(); }
public sealed class JobSystem { public JobHandle Schedule(Action action,IReadOnlyList<JobHandle>? dependencies=null){var deps=(dependencies??[]).Select(x=>x.Task);return new(Task.Run(async()=>{await Task.WhenAll(deps);action();}));} }

public sealed class NativeArray<T> : IDisposable where T:unmanaged { T[] data; public int Length=>data.Length; public NativeArray(int length)=>data=new T[length]; public Span<T> Span=>data; public void Dispose(){data=[];} }

public readonly record struct ArchetypeKey(params Type[] Components);
public sealed class EcsWorld { readonly Dictionary<ArchetypeKey,List<EntityId>> chunks=new(); public ArchetypeKey CreateArchetype(params Type[] components){var key=new ArchetypeKey(components);chunks.TryAdd(key,[]);return key;} public void Add(ArchetypeKey key,EntityId e)=>chunks[key].Add(e); public IEnumerable<EntityId> Query(ArchetypeKey key)=>chunks.GetValueOrDefault(key)??[]; }

public abstract class SystemBase { public virtual void OnCreate(){} public abstract void OnUpdate(float dt); public virtual void OnDestroy(){} }

public static class Input {
    static readonly Dictionary<string,float> actions=new(StringComparer.OrdinalIgnoreCase);
    public static void Set(string action,float value)=>actions[action]=value;
    public static float ReadValue(string action)=>actions.GetValueOrDefault(action);
    public static bool IsPressed(string action)=>ReadValue(action)>0.5f;
}
public enum DeviceKind { KeyboardMouse, Gamepad, Touch, XR }
public sealed record InputBinding(DeviceKind Device,string Control,float Scale=1f);
public sealed class InputActionMap { readonly Dictionary<string,List<InputBinding>> map=new(); public void Bind(string action,params InputBinding[] bindings)=>map[action]=bindings.ToList(); }

public enum NetworkRole { Server,Client,Host,Authority }
public sealed record Rpc(byte[] Payload, uint ObjectId,uint MethodId);
public interface INetworkTransport : IDisposable { void Start(int port); void Poll(); void Send(ReadOnlySpan<byte> payload); }
public sealed class LoopbackTransport : INetworkTransport { readonly Queue<byte[]> queue=new(); public int Pending=>queue.Count; public void Start(int port){} public void Poll(){} public void Send(ReadOnlySpan<byte> payload)=>queue.Enqueue(payload.ToArray()); public void Dispose()=>queue.Clear(); }

public readonly record struct RenderResourceId(uint Value);
public sealed class RenderGraph {
    sealed record Pass(string Name,List<RenderResourceId> Reads,List<RenderResourceId> Writes,Action Execute);
    uint next; readonly List<Pass> passes=[];
    public RenderResourceId CreateResource(string name,bool transient=true)=>new(++next);
    public void AddPass(string name,IEnumerable<RenderResourceId> reads,IEnumerable<RenderResourceId> writes,Action execute)=>passes.Add(new(name,reads.ToList(),writes.ToList(),execute));
    public void Execute(){foreach(var p in passes)p.Execute();}
}
public enum GraphicsBackend { Null, OpenGL, OpenGLES, Vulkan, DirectX11, DirectX12, Metal, WebGPU }
public sealed record RenderSettings(GraphicsBackend Backend,bool Hdr,bool VSync,int MsaaSamples);
public interface IRenderPipeline { void Render(Scene scene,RenderGraph graph,RenderSettings settings); }
public sealed record MaterialDescriptor(Vector4 BaseColor,float Metallic,float Roughness,Guid BaseColorTexture);
public sealed record LightDescriptor(string Type,Vector3 Position,Vector3 Color,float Intensity,float Range);

public sealed record PhysicsBody3D(Vector3 Position,Vector3 Velocity,Vector3 HalfExtents,float Mass,bool Kinematic=false);
public sealed record RaycastHit3D(bool Hit,float Distance,Vector3 Point,Vector3 Normal,EntityId Entity);
public interface IPhysics3D { void Step(float dt); RaycastHit3D Raycast(Vector3 origin,Vector3 direction,float maxDistance); }
public interface IPhysics2D { void Step(float dt); }

public sealed record AudioClipAsset(Guid Guid,string Path,float LengthSeconds);
public sealed record AudioEmitter(Vector3 Position,float Volume=1,float Pitch=1,bool Looping=false);
public interface IAudioBackend { void SetListener(Vector3 position,Vector3 forward,Vector3 up); void Play(AudioClipAsset clip,AudioEmitter emitter); }

public sealed record AnimationClip(string Name,float LengthSeconds);
public sealed class Animator { public AnimationClip? Current {get;private set;} public float Time {get;private set;} public void Play(AnimationClip clip){Current=clip;Time=0;} public void Update(float dt)=>Time+=dt; }

public sealed record UiRect(Vector2 Position,Vector2 Size,Vector2 AnchorMin,Vector2 AnchorMax);
public abstract class UiElement { public UiRect Rect{get;set;} public bool Interactable{get;set;}=true; public List<UiElement> Children{get;}=[]; }
public sealed class UiCanvas:UiElement { public float ScaleFactor{get;set;}=1; }
public sealed class UiText:UiElement { public string Text{get;set;}=""; public bool RichText{get;set;}=true; }
public sealed class UiButton:UiElement { public event Action? Clicked; public void Click()=>Clicked?.Invoke(); }

public interface IXrProvider { bool IsAvailable {get;} void BeginFrame(); void EndFrame(); }
public interface INativePlugin { nint Handle {get;} }

public sealed class Profiler {
    readonly Dictionary<string,Stopwatch> active=new(); readonly Dictionary<string,(long ticks,long calls)> data=new();
    public IDisposable Scope(string name){var sw=Stopwatch.StartNew();active[name]=sw;return new ScopeToken(()=>{sw.Stop();var old=data.GetValueOrDefault(name);data[name]=(old.ticks+sw.ElapsedTicks,old.calls+1);active.Remove(name);});}
    public (double Ms,long Calls) Get(string name){var d=data.GetValueOrDefault(name);return(d.ticks*1000.0/Stopwatch.Frequency,d.calls);}
    sealed class ScopeToken(Action end):IDisposable{public void Dispose()=>end();}
}

[AttributeUsage(AttributeTargets.Class|AttributeTargets.Struct)] public sealed class CustomEditorAttribute(Type target):Attribute { public Type Target=>target; }
[AttributeUsage(AttributeTargets.Class|AttributeTargets.Struct)] public sealed class PropertyDrawerAttribute(Type target):Attribute { public Type Target=>target; }
[AttributeUsage(AttributeTargets.Method)] public sealed class MenuItemAttribute(string path):Attribute { public string Path=>path; }
[AttributeUsage(AttributeTargets.Method)] public sealed class DrawGizmoAttribute:Attribute { }
[AttributeUsage(AttributeTargets.Method)] public sealed class BuildCallbackAttribute:Attribute { }

public sealed record BuildSettings(string Name,string OutputDirectory,bool Development,bool StripManagedCode,GraphicsBackend GraphicsApi);
public enum TargetPlatform { WindowsX64,LinuxX64,LinuxArm64,AndroidArm64 }
public sealed class BuildPipeline { public Task BuildAsync(BuildSettings settings,TargetPlatform target,CancellationToken ct=default){Directory.CreateDirectory(settings.OutputDirectory);File.WriteAllText(Path.Combine(settings.OutputDirectory,"build-manifest.json"),$"{{\"target\":\"{target}\",\"backend\":\"{settings.GraphicsApi}\"}}");return Task.CompletedTask;} }

public static class SceneSerializer {
    public static void SaveJson(Scene scene,string path){var payload=new {format=1,scene=scene.Name,entities=scene.Entities.Values.Select(e=>new {id=e.Id.Value,name=e.Name,parent=e.Parent?.Id.Value??0,active=e.ActiveSelf,position=new[]{e.Transform.LocalPosition.X,e.Transform.LocalPosition.Y,e.Transform.LocalPosition.Z}})};File.WriteAllText(path,System.Text.Json.JsonSerializer.Serialize(payload,new System.Text.Json.JsonSerializerOptions{WriteIndented=true}));}
    public static void SaveYaml(Scene scene,string path){using var w=new StreamWriter(path);w.WriteLine("format: 1");w.WriteLine($"scene: \"{scene.Name.Replace("\"","\\\"")}\"");w.WriteLine("entities:");foreach(var e in scene.Entities.Values){w.WriteLine($"  - id: {e.Id.Value}");w.WriteLine($"    name: \"{e.Name.Replace("\"","\\\"")}\"");w.WriteLine($"    parent: {e.Parent?.Id.Value??0}");w.WriteLine($"    active: {e.ActiveSelf.ToString().ToLowerInvariant()}");}}
}

internal static class Native {
    const string Library="Astra.Native";
    [DllImport(Library,CallingConvention=CallingConvention.Cdecl)] internal static extern nint astra_create();
    [DllImport(Library,CallingConvention=CallingConvention.Cdecl)] internal static extern void astra_destroy(nint h);
    [DllImport(Library,CallingConvention=CallingConvention.Cdecl)] internal static extern ulong astra_create_entity(nint h,string name);
    [DllImport(Library,CallingConvention=CallingConvention.Cdecl)] internal static extern void astra_set_position(nint h,ulong e,float x,float y,float z);
    [DllImport(Library,CallingConvention=CallingConvention.Cdecl)] internal static extern void astra_step(nint h,double dt);
    [DllImport(Library,CallingConvention=CallingConvention.Cdecl)] internal static extern ulong astra_frame_count(nint h);
    [DllImport(Library,CallingConvention=CallingConvention.Cdecl)] internal static extern int astra_save_scene(nint h,string path);
}
public sealed class NativeEngine : IDisposable {
    nint handle;
    public NativeEngine()=>handle=Native.astra_create();
    public EntityId CreateEntity(string name)=>new(Native.astra_create_entity(handle,name));
    public void SetPosition(EntityId e,Vector3 p)=>Native.astra_set_position(handle,e.Value,p.X,p.Y,p.Z);
    public void Tick(double dt)=>Native.astra_step(handle,dt);
    public ulong FrameCount=>Native.astra_frame_count(handle);
    public bool SaveScene(string path)=>Native.astra_save_scene(handle,path)!=0;
    public void Dispose(){if(handle!=0){Native.astra_destroy(handle);handle=0;}GC.SuppressFinalize(this);} ~NativeEngine()=>Dispose();
}

public sealed class GameRuntime {
    public Scene ActiveScene {get;set;}=new("Main");
    public PlayerLoop Loop {get;}=new();
    public JobSystem Jobs {get;}=new();
    public AssetDatabase Assets {get;}=new();
    public PackageManager Packages {get;}=new();
    public RenderGraph RenderGraph {get;}=new();
    public Profiler Profiler {get;}=new();
    readonly List<Behaviour> behaviours=[];
    public void Register(Behaviour behaviour){behaviours.Add(behaviour);behaviour.OnInitialize();if(behaviour.Enabled)behaviour.OnEnable();}
    public void Tick(double dt){Loop.Tick(dt,()=>{foreach(var b in behaviours)if(b.Enabled)b.OnFixedUpdate((float)Loop.FixedStep);},()=>{foreach(var b in behaviours)if(b.Enabled)b.OnUpdate((float)dt);},()=>{foreach(var b in behaviours)if(b.Enabled)b.OnLateUpdate((float)dt);});}
}
