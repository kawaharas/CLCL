# CLCL (CAVELib Compatible Library for HMD)

***This is the CLCL for Oculus SDK. OpenVR version is [here](https://github.com/kawaharas/CLCL-OpenVR).***

CLCL is a C++ library for porting CAVE application software to HMDs. 
It emulates the function calls of CAVELib, which is a commercial library 
for developing application software executable on CAVEs, 
and it enables us to easily port CAVELib application software 
to HMDs with minor modification to the original source code.
Function calls commonly used in the CAVELib program are implemented 
in CLCL. Networking functions for multiple CAVEs have not yet been implemented.

## Required Hardware

&nbsp; One of the following hardware:

- Oculus Rift with Oculus Touch, Xbox controller or mouse
- Oculus Development Kit 2 with mouse

## Required Software / Libraries

**Minimum requirement:**

- Visual Studio
- Oculus SDK 1.26.0 (Contains code for old SDK versions (0.5.0.1 to 1.0), but they are not already maintained.)
- GLFW 3.2.1 *
- GLEW 2.1.0 *

&nbsp; \*  These libraries are needed to build with multi-threaded (/MT) option.

**Optional (experimental: for the external camera function):**

Following libraries are not used in the pre-built library. 
Recompilation is required to enable the external camera function. 
To toggle enable/disable the external camera, press "c" key.

- ZED SDK 2.7 *
- CUDA Toolkit 10 *
- GLM 0.9.9 *
- Ovrvision Pro SDK 1.90 **

&nbsp; \*  These libraries are needed to use the external camera (ZED Mini by Stereolabs).  
&nbsp; \** The library is needed to use the external camera (Ovrvision Pro by Wizapply).

## Building the Library

1) Open CLCL.sln.
2) Set include path and library path to Oculus SDK.
3) Build library.

## Using the Library

Source code modifications are needed to use CLCL.

1) Disable a line includes "glClear()" in the rendering routine by commenting out or using "USE_CLCL_OCULUS_SDK" directive.
2) Resolve conflicts between GLEW and other OpenGL header files if these are used in the target code.

Compilation of the code.

3) Change include path and library path from CAVELib's to CLCL's.
4) Change library file to link from "libcave_ogl_XX.lib" to "CLCL.lib".
5) Build with multi-threaded (/MT) option.
6) If compilation failed, modification of code or project settings are needed. -> go back to 5)

## Controller Inputs

| |CAVE_JOYSTICK_X<br>CAVE_JOYSTICK_Y |CAVE_BUTTON1 |CAVE_BUTTON2 |CAVE_BUTTON3 |
|---|---|---|---|---|
|Oculus Touch (Right) |Thumb stick |Button A |Trigger |Button B |
|Mouse |Wheel (CAVE_JOYSTICK_Y) |Left button |Middle button |Right button |

## About Trademarks

&nbsp; The CAVE is a registered trademark of the Board of Trustees of the University of Illinois at Chicago.  
&nbsp; CAVELib is a trademark of the University of Illinois Board of Trustees.  
&nbsp; Oculus, Rift, and Oculus Touch are trademarks or registered trademarks of Facebook Technologies, LLC.  
&nbsp; All other trademarks are property of their respective owners.
