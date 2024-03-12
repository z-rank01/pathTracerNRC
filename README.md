# pathTracerNRC
a vulkan based path tracer featured in real-time NRC.

### Requirements
1. Vulkan 1.1
2. A GPU support extension of ray tracing

### Usage:

1. Use `cmake` to build up the project.
2. Please ensure Vulkan SDK has been installed in the computer and has been added to environment path.
3. If cmake does not find the Vulkan SDK from path, please designate the path in **CMakeLists.txt** or in **gui of CMake**.
4. In windows: 
    - generate solution after opening `pathTracerNRC.sln` in build folder.
5. Executable files will be in bin_{arch} folder.