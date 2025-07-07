# ReSTIR Raytracer
A Simple CPU ReSTIR DI raytracer application based on the paper by [Bitterli et. al.](https://benedikt-bitterli.me/restir/bitterli20restir.pdf) built for a few courses at Utrecht University.

The program is built in C++ in an early version of TheCherno's [Hazel Engine](https://github.com/TheCherno/Hazel) and uses Jacco Bikker's [tinybvh](https://github.com/jbikker/tinybvh).

![GIF of ReSTIR Raytracer](./Images/ReSTIR_Engine.gif)

Features:
- ReSTIR DI
- Reprojection for camera and object animations
<!---- OBJ mesh loading-->

Build Instructions:
1. Clone Repository
2. Set ```GenerateProject.bat``` to your version of Visual Studio (```vs2022``` or ```vs2019``` reccomended)
3. Run ```GenerateProject.bat```
4. Open ```hazel.sln``` and run program

Requirements:
- Windows 10 or higher
- Visual Studio 2019 or higher
- C++17
