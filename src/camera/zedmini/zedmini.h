////////////////////////////////////////////////////////////////////////////////
//
// zedmini.h
//
//   CLCL: CAVELib Compatible Library
//
//     Copyright 2015-2019 Shintaro Kawahara(kawahara@jamstec.go.jp).
//     All rights reserved.
//
//   Please read the file "LICENCE.txt" before you use this software.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../../settings.h"

#ifdef USE_ZEDMINI

#include <GL/glew.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <cuda_gl_interop.h>
#if (USE_ZED_SDK_VERSION == 2)
#include <sl_zed/Camera.hpp>
#else
#include <sl/Camera.hpp>
#endif // USE_ZED_SDK_VERSION
#pragma comment(lib, "cuda.lib")
#pragma comment(lib, "cudart.lib")
#pragma comment(lib, "cudart_static.lib")
#if (ZED_SDK_MAJOR_VERSION == 2)
#pragma comment(lib, "sl_core64.lib")
#endif
#pragma comment(lib, "sl_zed64.lib")

#include "../../camera/zedmini/shader.h"

class ZedMini
{
	sl::Camera m_Camera;
	bool    m_IsOpen;
	bool    m_CameraState;
	int     m_Width;
	int     m_Height;
	sl::Mat m_Image[2];
	sl::Mat m_Depth[2];
	sl::Mat m_PointCloud[2];
	cudaGraphicsResource *cimg_l;  // RGB
	cudaGraphicsResource *cimg_r;  // RGB
	cudaGraphicsResource *cimg_ld; // Depth
	cudaGraphicsResource *cimg_rd; // Depth
	GLenum  m_Format;
	GLuint  m_TexID[2];
	GLuint  m_DepthID[2];
	Shader* p_Shader;
	Shader* p_ShaderDepth;
	GLuint  m_RectVBO[2][3];
	cudaArray_t m_ArrIm;

public:
	ZedMini();
	~ZedMini();

	bool   Init();
	void   Terminate();
	void   PreStore();
	void   DrawImage(int eyeIndex);
	void   DrawRGBImage(int eyeIndex);
	void   DrawDepth(int eyeIndex);
	void   DrawPointCloud(int eyeIndex);
	void   SetScreenCoord(int bufferWidth, int bufferHeight, float offsetLensCenterX, float offsetLensCenterY);
	bool   IsOpen() { return m_IsOpen; }
	bool   cameraState() { return m_CameraState; }
	void   toggleCameraState() { m_CameraState = !m_CameraState; }
	int    width() { return m_Width; }
	int    height() { return m_Height; }
};

#endif // USE_ZEDMINI
