# CLCL (CAVELib Compatible Library for HMD)

***This is the Oculus SDK version of CLCL.***

CLCL is a C++ library for porting CAVE application software to HMDs. 
It emulates the function calls of CAVELib, which is a commercial library 
for developing application software executable on CAVEs, 
and it enables us to easily port CAVELib application software 
to HMDs with minor modification to the original source code.

## Required Software / Libraries

&nbsp; Minimum requirement:

- Visual Studio 2017
- Oculus SDK 1.26.0
- GLFW 3.2.1
- GLEW 2.1.0

&nbsp; Optional:

- ZED SDK 2.7 *
- CUDA Toolkit 10 *
- GLM 0.9.9 *
- Ovrvision Pro SDK 1.90 **

&nbsp; \*  These libraries are needed to use the external camera (ZED Mini by Stereolabs).  
&nbsp; \** The library is needed to use the external camera (Ovrvision Pro by Wizapply).

## Building the Library

1) Open CLCL.sln.
2) Set include path and library path of Oculus SDK.
3) Build library.

## Using the Library

Source code modifications are needed to use CLCL.

1) Comment out a line includes "glClear()" in the rendering routine.
2) Resolve conflicts between GLEW and other OpenGL header files if these are used in the target code.

Compilation of the code.

3) Change include path and library path from CAVELib's to CLCL's.
4) Change library file to link from "libcave_ogl_XX.lib" to "CLCL.lib".
5) Build.
6) If compilation failed, modification of codes is needed. -> go back to 5)

## About Trademarks

###### &nbsp; The CAVE is a registered trademark of the Board of Trustees of the University of Illinois at Chicago.
###### &nbsp; CAVELib is a trademark of the University of Illinois Board of Trustees.
