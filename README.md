# ReSTIR DI Raytracer
A simple CPU ReSTIR DI raytracer application based on the paper by [Bitterli et. al.](https://benedikt-bitterli.me/restir/bitterli20restir.pdf) built during a few courses at Utrecht University.

The program is built in C++ in an early version of TheCherno's [Hazel Engine](https://github.com/TheCherno/Hazel) and uses Jacco Bikker's [tinybvh](https://github.com/jbikker/tinybvh).\
It is currently capable of running a 32M triangle scene at 5.1 fps in on a Ryzen 5 5600X vs a 400K triangles scene at 6 fps.

<img src="./Images/ReSTIR_Engine_Zoom.gif" width="640" height="320">
<img src="./Images/ReSTIR_Engine_Static.gif" width="640" height="320">

Features:
- ReSTIR Direct Illumination
- Rigid Animations
- OBJ Mesh Importing
- Mesh Instancing (Full Support Coming Soon)
- Fast BVH Traversal

**Warning:** The build system uses the ```.\vendor\bin\premake\premake5.exe``` executable included in the repository to build a visual studio solution.  
Download and replace it with the executable from the [official site](https://premake.github.io/download) if you want to be sure its safe!

Build Instructions:
1. Clone Repository.
2. Edit ```GenerateProject.bat``` to generate a solution for your version of Visual Studio (```vs2022``` or ```vs2019```).
3. Run ```GenerateProject.bat```.
4. Open ```hazel.sln``` and set to ```Release``` configuration
5. Run program.

Requirements:
- Windows 10 or higher.
- Visual Studio 2019 or higher.
- C++17
