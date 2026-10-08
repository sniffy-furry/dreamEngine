using Astra;
using System.Numerics;
var runtime = new GameRuntime();
var root = runtime.ActiveScene.Create("Player");
root.Transform.LocalPosition = new Vector3(0, 1, 0);
Console.WriteLine($"AstraForge managed sample: {runtime.ActiveScene.Name}, entity={root.Id.Value}");
for (var i=0; i<60; i++) runtime.Tick(1.0/60.0);
Console.WriteLine($"simulation time={runtime.Loop.TimeSeconds:F3}s");
