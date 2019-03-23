# How to build VFIVE using CLCL

VFIVE is an interactive visualization software for CAVE-type VR systems.  
It was develeped by Akira Kageyama (Kobe University) and Nobuaki Ohno (University of Hyogo).  
For details of VFIVE, please see the following paper;

&nbsp; A. Kageyama, Y. Tamura, T. Sato:  
&nbsp; Visualization of Vector Field by Virtual Reality,  
&nbsp; Pro-gress of Theoretical Physics Supplement, 138 (2000), 665-673.


## Download Source code

&nbsp; Download the source code of VFIVE and a sample dataset from the following URL:

&nbsp; &nbsp;  [https://www.jamstec.go.jp/ceist/aeird/avcrg/vfive.ja.html](https://www.jamstec.go.jp/ceist/aeird/avcrg/vfive.ja.html)

&nbsp; &nbsp; vfive3.72Amt.zip  
&nbsp; &nbsp; tex_maker.zip *  
&nbsp; &nbsp; sample_little.tar.gz

&nbsp; \*  **tex_maker** is a program to generate texture images for menu panels of VFIVE. To build **tex_maker**, GLUT is needed.

## Create project

&nbsp; Create new project on Visual Studio and add source code of VFIVE (\*.h and \*.cpp) to the project.

## Project settings

&nbsp; Add (PATH_TO_CLCL)/include to include paths and (PATH_TO_CLCL)/lib/x64 to library paths.  

&nbsp; \[C/C++\]-\[Pre-processor\]-\[Difinition of pre-processor\]  
&nbsp; &nbsp; WIN32;\_CRT_SECURE_NO_WARNINGS;

&nbsp; \[C/C++\]-\[Code generation\]-\[Runtime library\]  
&nbsp; &nbsp; Multi-threaded (/MT)

&nbsp; \[Linker\]-\[Input\]-\[Additional Library\]  
&nbsp; &nbsp; CLCL.lib

## Source code modifications

&nbsp; **vfive.h**

&nbsp; For resolving a conflict between glext.h and GLEW.
```diff
  12 #ifdef WIN32
+    #ifndef USE_CLCL
  13 #include <GL/glext.h>
+    #endif
  14 #endif
```

&nbsp; Sample dataset is double precision.
```diff
- 56 typedef float ffloat_;     
- 56 //typedef double ffloat_;  
+ 56 //typedef float ffloat_;     
+ 57 typedef double ffloat_;  
```

&nbsp; **volren.cpp**

&nbsp; For resolving conflicts between glext.h and GLEW.
```diff
  11 #ifdef WIN32
+    #ifndef USE_CLCL
  12 PFNGLTEXIMAGE3DPROC glTexImage3D;
+    #endif
  13 #endif
```

```diff
  264 #ifdef WIN32
+     #ifndef USE_CLCL
  265   glTexImage3D =
  266     (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");
+     #endif
  267 #endif
```

&nbsp; **panel.cpp**
```diff
  331 #ifdef WIN32
- 332   sprintf(command,"tex_maker %s 0x%x 0x%x %s",
- 333     label_temp,bgc,fgc,file_name);
+ 332   sprintf(command,"%s\\tex_maker %s 0x%x 0x%x %s",
+ 333     dir, label_temp,bgc,fgc,file_name);
  334 #else
```

&nbsp; **main.cpp**

&nbsp; In Oculus SDK, left-eye image and right-image is drawn on same render buffer.
```diff
+     #ifndef USE_CLCL_OCULUS_SDK
  337     glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
+     #endif
```

## Execution

&nbsp; To launch VFIVE, type following command.  

```diff
  > vfive.exe dynamo.v5 -l dynamolines
```
