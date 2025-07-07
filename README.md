# ReSTIR Raytracer
A Simple CPU ReSTIR DI raytracer application based on the paper by [Bitterli et. al.](https://benedikt-bitterli.me/restir/bitterli20restir.pdf) built for a few courses at Utrecht University.

The program is built in C++ in an early version of TheCherno's [Hazel Engine](https://github.com/TheCherno/Hazel) and uses Jacco Bikker's [tinybvh](https://github.com/jbikker/tinybvh).

![GIF of ReSTIR Raytracer](./Images/ReSTIR_Engine.gif)

Features:
- ReSTIR DI
- Rigid Animations
- OBJ Mesh Importing
- Scalable, runs at 4fps on an R5 5600X in a scene with 32 million vertices with animations

**Warning:** The build system uses the ```.\vendor\bin\premake\premake5.exe``` executable included in the repository to build a visual studio solution.  
Download the executable from the [official site](https://premake.github.io/download) if you want to be sure its safe!

Build Instructions:
1. Clone Repository.
2. Edit ```GenerateProject.bat``` to your version of Visual Studio (```vs2022``` or ```vs2019```).
3. Run ```GenerateProject.bat```.
4. Open ```hazel.sln``` and run program.

Requirements:
- Windows 10 or higher.
- Visual Studio 2019 or higher.
- C++17
