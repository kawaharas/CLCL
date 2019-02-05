////////////////////////////////////////////////////////////////////////////////
//
// oculus.h
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

//#define TEXTURE_COPY_TEST

#define _CRT_SECURE_NO_WARNINGS
#include "../../settings.h"

const float FEET_PER_METER = 3.280840f;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX // to use "std::max()"
#include <windows.h>
#endif // _WIN32

#define _USE_MATH_DEFINES
#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <atomic>
#include <thread>
#include <process.h>

#define GLEW_STATIC
#include <GL/glew.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_INCLUDE_GLU
#define OVR_OS_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
//#pragma comment(lib, "legacy_stdio_definitions.lib")
//#pragma comment(lib, "GLFW3.lib")
//#pragma comment(lib, "GLEW32s.lib")
//#pragma comment(lib, "opengl32.lib")
//#pragma comment(lib, "glu32.lib")
//#pragma comment(lib, "winmm.lib")

#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <Extras/OVR_Math.h>
//#pragma comment(lib, "libovr.lib")

#ifdef USE_OVRVISION
#include "../../camera/ovrvision/ovrvision.h"
#endif // USE_OVRVISION

#ifdef USE_ZEDMINI
#include "../../camera/zedmini/zedmini.h"
#endif // USE_ZEDMINI

typedef void(*OVRCALLBACK)();
typedef void(*OVRCALLBACK1)(void*);
typedef void(*OVRCALLBACK2)(void*, void*);
typedef void(*OVRCALLBACK3)(void*, void*, void*);
typedef void(*OVRCALLBACK4)(void*, void*, void*, void*);
typedef void(*OVRCALLBACK5)(void*, void*, void*, void*, void*);

#if (OVR_PRODUCT_VERSION == 0)
#define USE_MIRROR_WINDOW // Oculus SDK 0.5.0.1
#endif

typedef enum {
	VECTOR_UP = 0,
	VECTOR_FRONT,
	VECTOR_RIGHT
} VECTOR_TYPE;

class Oculus {
public:
	Oculus();
	~Oculus();

	GLFWwindow* window() { return m_Window; }

#if (OVR_PRODUCT_VERSION == 1)
	ovrSession hmdSession() { return m_HmdSession; } // for Oculus SDK 1.10.1
#endif

	void Init();
	void InitGL();
	void CreateBuffers();
	void Terminate();
	void UpdateTrackingData();
	void PreProcess();
	void PostProcess();
	void SetMatrix(int eyeIndex);
	void Translate(float x, float y, float z);
	void Rotate(float angle_degree, char axis);
	void Scale(float x, float y, float z);
	void WorldTranslate(float x, float y, float z);
	void WorldRotate(float angle, char axis);
	void WorldScale(float x, float y, float z);
	OVR::Matrix4f GetNavigationMatrix();
	void LoadNavigationMatrix(OVR::Matrix4f matrix);
	void SetNavigationMatrixIdentity();
	void SetNavigationMatrix();
	void SetNavigationInverseMatrix();
	void MultiNavigationMatrix(float matrix[4][4]);
	void PreMultiNavigationMatrix(float matrix[4][4]);
	void StoreNavigationMatrix() { m_NavigationMatrix_Backup = m_NavigationMatrix; }
	void RestoreNavigationMatrix() { m_NavigationMatrix = m_NavigationMatrix_Backup; }
	int  ShouldClose() const { return glfwWindowShouldClose(m_Window); }
	void PollEvents() { glfwPollEvents(); }

	OVR::Matrix4f projectionMatrix(int eyeIndex) { return m_ProjectionMatrix[eyeIndex]; }
	OVR::Vector3f bodyTranslation() { return m_BodyTranslation; }
	OVR::Vector3f headTranslation() { return m_HeadTranslation; }
	OVR::Vector3f headOrientation() { return m_HeadOrientation; }
	OVR::Vector3f headVector(VECTOR_TYPE type) { return m_HeadVector[type]; }
	OVR::Vector3f headVectorNav(VECTOR_TYPE type) { return m_HeadVectorNav[type]; }
	OVR::Vector3f headTranslationNav() { return m_HeadTranslationNav; }
	OVR::Vector3f headOrientationNav() { return m_HeadOrientationNav; }

	typedef enum {
		MOUSE = 0,
		XBOX_CONTROLLER,
		OCULUS_TOUCH_RIGHT,
		ENUM_CONTROLLER_TYPE_SIZE
	} ControllerType;

#if (OVR_PRODUCT_VERSION == 1)
	// for Oculus Touch
	ControllerType controllerType() { return static_cast<ControllerType>(m_CurrentControllerType); }
	void SwitchControllerType();
	bool IsConnected(ControllerType type) { return m_IsConnected[type]; }
	OVR::Vector3f handTranslation(ovrHandType handType) { return m_HandTranslation[handType]; }
	OVR::Vector3f handVector(ovrHandType handType, VECTOR_TYPE vectorType) { return m_HandVector[handType][vectorType]; }
	OVR::Vector3f handTranslationNav(ovrHandType handType) { return m_HandTranslationNav[handType]; }
	OVR::Vector3f handVectorNav(ovrHandType handType, VECTOR_TYPE vectorType) { return m_HandVectorNav[handType][vectorType]; }
#endif

#if (OVR_PRODUCT_VERSION == 1)
	ovrSizei windowSize() { return m_WindowSize; } // Oculus SDK 1.10.1
#endif
	ovrSizei renderTargetSize() { return m_RenderTargetSize; }
	ULONG64  frameIndex() { return m_FrameIndex; }

	void StartThread();
	void StopThread();

	void SetInitFunction(OVRCALLBACK callback, std::vector<void*> arg_list)
	{
		p_InitFunction = callback;
		m_InitFunctionArgs = arg_list;
	}
	void SetStopFunction(OVRCALLBACK callback, std::vector<void*> arg_list)
	{
		p_StopFunction = callback;
		m_StopFunctionArgs = arg_list;
	}
	void SetDrawFunction(OVRCALLBACK callback, std::vector<void*> arg_list)
	{
		p_DrawFunction = callback;
		m_DrawFunctionArgs = arg_list;
	}
	void SetIdleFunction(OVRCALLBACK callback, std::vector<void*> arg_list)
	{
		p_IdleFunction = callback;
		m_IdleFunctionArgs = arg_list;
	}

	bool GetKey(int);
	int  GetMouseButton(int);

//	bool UseMirrorWindow() { return m_UseMirrorWindow; }

private:
#if (OVR_PRODUCT_VERSION == 1)
	ovrHmdDesc          m_HmdDesc;               // Oculus SDK 1.10.1
	ovrSession          m_HmdSession;            // Oculus SDK 1.10.1
	ovrViewScaleDesc    m_ViewScaleDesc;         // Oculus SDK 1.10.1
	ovrLayerEyeFov      m_LayerEyeFov;           // Oculus SDK 1.10.1
	ovrTextureSwapChain m_TextureSwapChain;      // Oculus SDK 1.10.1 (this must set to zero in initialization)
	ovrMirrorTexture    m_MirrorTexture;         // Oculus SDK 1.10.1
	GLuint              m_MirrorFBO;             // Oculus SDK 1.10.1 (this must set to zero in initialization)
	int                 m_ButtonState[4];
#else
#if (OVR_MAJOR_VERSION > 7)
	ovrSession          m_HmdSession;            // Oculus SDK 1.10.1
#else
	ovrHmd              m_HmdSession;            // Oculus SDK 0.5.0.1
#endif

#if (OVR_MAJOR_VERSION > 5)
	ovrHmdDesc          m_HmdDesc;               // Oculus SDK 0.8.0
	ovrViewScaleDesc    m_ViewScaleDesc;         // Oculus SDK 1.10.1
	ovrLayerEyeFov      m_LayerEyeFov;           // Oculus SDK 1.10.1
//	ovrTextureSwapChain m_TextureSwapChain;      // Oculus SDK 1.10.1 (this must set to zero in initialization)
	ovrLayer_Union      m_LayerUnion;            // Oculus SDK 0.8.0
	ovrGLTexture       *mirrorTexture;           // Oculus SDK 0.8.0 (TEST)
	ovrTexture         *p_MirrorTexture;         // Oculus SDK 0.8.0
	ovrSwapTextureSet  *p_SwapTextureSet;        // Oculus SDK 0.8.0
	GLuint              m_MirrorFBO;             // Oculus SDK 1.10.1 (this must set to zero in initialization)
	GLuint              oculusFbo[ovrEye_Count]; // Oculus SDK 0.8.0 (TEST)

	ovrGLTexture        m_EyeTexture[2];         // Oculus SDK 0.5.0.1

#else
//	ovrHmd              m_HmdSession;            // Oculus SDK 0.5.0.1
	bool                m_IsHMDDebug;            // Oculus SDK 0.5.0.1
	ovrVector3f         m_HmdToEyeViewOffset[2]; // Oculus SDK 0.5.0.1
	ovrRecti            m_EyeRenderViewport[2];  // Oculus SDK 0.5.0.1
	ovrGLTexture        m_EyeTexture[2];         // Oculus SDK 0.5.0.1
#endif
#endif
	ovrSizei            m_RenderTargetSize;
	ovrPosef            m_EyePose[2];
	ovrEyeRenderDesc    m_EyeRenderDesc[2];
	OVR::Matrix4f       m_ProjectionMatrix[2];
	OVR::Vector3f       m_HeadTranslation;
	OVR::Vector3f       m_HeadOrientation;
	OVR::Vector3f       m_HeadVector[3];
	OVR::Vector3f       m_HeadTranslationNav;
	OVR::Vector3f       m_HeadOrientationNav;
	OVR::Vector3f       m_HeadVectorNav[3];
	OVR::Vector3f       m_BodyTranslation;
	OVR::Vector3f       m_BodyRotation[3];

	int m_CurrentEyeIndex;
	OVR::Matrix4f       m_NavigationMatrix;
	OVR::Matrix4f       m_NavigationMatrix_Backup;
	OVR::Matrix4f       m_ModelMatrix;

#if (OVR_PRODUCT_VERSION == 1)
	bool m_IsConnected[ENUM_CONTROLLER_TYPE_SIZE];
	uint m_CurrentControllerType;                // Oculus Touch
	OVR::Vector3f       m_HandTranslation[2];    // Oculus Touch
	OVR::Vector3f       m_HandVector[2][3];      // Oculus Touch
	OVR::Vector3f       m_HandTranslationNav[2];    // Oculus Touch
	OVR::Vector3f       m_HandVectorNav[2][3];      // Oculus Touch
#endif

	GLFWwindow*         m_Window;
#if (OVR_PRODUCT_VERSION == 1)
	ovrSizei            m_WindowSize;            // Oculus SDK 1.10.1
#else
#if (OVR_MAJOR_VERSION > 5)
	ovrSizei            m_WindowSize;            // Oculus SDK 0.8.0
#endif
#endif
	GLuint              m_FrameBuffer;
#if (OVR_PRODUCT_VERSION == 0)
	GLuint              m_TextureBuffer;         // Oculus SDK 0.5.0.1
#endif
	GLuint              m_DepthBuffer;
	ULONG64             m_FrameIndex;

	int                 m_SnapNo;
	bool                m_UseMirrorWindow;

	bool                m_IsInitFunctionExecuted;
	OVRCALLBACK         p_InitFunction;
	OVRCALLBACK         p_StopFunction;
	OVRCALLBACK         p_DrawFunction;
	OVRCALLBACK         p_IdleFunction;
	std::vector<void*>  m_InitFunctionArgs;
	std::vector<void*>  m_StopFunctionArgs;
	std::vector<void*>  m_DrawFunctionArgs;
	std::vector<void*>  m_IdleFunctionArgs;

	HANDLE m_HMutex;
	HANDLE m_HRender;
	bool   m_IsThreadRunning; // flag to stop the thread
//	bool   m_IsInitializedGLFW;
	std::atomic<bool>   m_IsInitializedGLFW;

#ifdef USE_OVRVISION
	OVRVision           m_OVRVision;
#endif // USE_OVRVISION

#ifdef USE_ZEDMINI
	ZedMini             m_ZedMini;
#endif // USE_ZEDMINI

#ifdef TEXTURE_COPY_TEST
	uchar* m_LeftEyeTextureData;
	GLuint m_LeftEyeTextureID;
#endif // TEXTURE_COPY_BUFFER

/*
#ifdef USE_MIRROR_WINDOW
	typedef enum {
		DIRECT_MODE = 0,
		EXTEND_MODE,
		EXTEND_MODE_PORTRAIT,
	} WindowStyle;

	WindowStyle m_WindowStyle = DIRECT_MODE;
	GLFWwindow* m_MirrorWindow;
	uint        m_MirrorWindowWidth;
	uint        m_MirrorWindowHeight;
	uchar*      m_MirrorRGBImage;
#endif // USE_MIRROR_WINDOW

#if (OVR_PRODUCT_VERSION == 0)
	GLFWmonitor* CheckOVR();
#endif
*/

#ifdef USE_MIRROR_WINDOW
	typedef enum {
		DIRECT_MODE = 0,
		EXTEND_MODE,
		EXTEND_MODE_PORTRAIT,
	} WindowStyle;

	WindowStyle m_WindowStyle = DIRECT_MODE;
	GLFWwindow* m_MirrorWindow;
	uint        m_MirrorWindowWidth;
	uint        m_MirrorWindowHeight;
	uchar*      m_MirrorRGBImage;
#endif // USE_MIRROR_WINDOW

#if (OVR_PRODUCT_VERSION == 0)
	GLFWmonitor* CheckOVR();
#endif

	void ExecInitCallback()
	{
		if (m_IsInitFunctionExecuted) return;

		if (p_InitFunction != nullptr)
		{
			std::vector<void*> args = m_InitFunctionArgs;
			switch (args.size())
			{
				case 0:
					p_InitFunction();
					break;
				case 1:
					((OVRCALLBACK1)p_InitFunction)(args[0]);
					break;
				case 2:
					((OVRCALLBACK2)p_InitFunction)(args[0], args[1]);
					break;
				case 3:
					((OVRCALLBACK3)p_InitFunction)(args[0], args[1], args[2]);
					break;
				case 4:
					((OVRCALLBACK4)p_InitFunction)(args[0], args[1], args[2], args[3]);
					break;
				case 5:
					((OVRCALLBACK5)p_InitFunction)(args[0], args[1], args[2], args[3], args[4]);
					break;
				default:
					break;
			}
			m_IsInitFunctionExecuted = true;
		}
	}

	void ExecStopCallback()
	{
		if (p_StopFunction != nullptr)
		{
			std::vector<void*> args = m_StopFunctionArgs;
			switch (args.size())
			{
				case 0:
					p_StopFunction();
					break;
				case 1:
					((OVRCALLBACK1)p_StopFunction)(args[0]);
					break;
				case 2:
					((OVRCALLBACK2)p_StopFunction)(args[0], args[1]);
					break;
				case 3:
					((OVRCALLBACK3)p_StopFunction)(args[0], args[1], args[2]);
					break;
				case 4:
					((OVRCALLBACK4)p_StopFunction)(args[0], args[1], args[2], args[3]);
					break;
				case 5:
					((OVRCALLBACK5)p_StopFunction)(args[0], args[1], args[2], args[3], args[4]);
					break;
				default:
					break;
			}
		}
	}

	void ExecDrawCallback()
	{
		if (!m_IsInitFunctionExecuted) return;

		if (p_DrawFunction != nullptr)
		{
			std::vector<void*> args = m_DrawFunctionArgs;
			switch (args.size())
			{
				case 0:
					p_DrawFunction();
					break;
				case 1:
					((OVRCALLBACK1)p_DrawFunction)(args[0]);
					break;
				case 2:
					((OVRCALLBACK2)p_DrawFunction)(args[0], args[1]);
					break;
				case 3:
					((OVRCALLBACK3)p_DrawFunction)(args[0], args[1], args[2]);
					break;
				case 4:
					((OVRCALLBACK4)p_DrawFunction)(args[0], args[1], args[2], args[3]);
					break;
				case 5:
					((OVRCALLBACK5)p_DrawFunction)(args[0], args[1], args[2], args[3], args[4]);
					break;
				default:
					break;
			}
		}
	}

	void ExecIdleCallback()
	{
		if (!m_IsInitFunctionExecuted) return;

		if (p_IdleFunction != nullptr)
		{
			std::vector<void*> args = m_IdleFunctionArgs;
			switch (args.size())
			{
				case 0:
					p_IdleFunction();
					break;
				case 1:
					((OVRCALLBACK1)p_IdleFunction)(args[0]);
					break;
				case 2:
					((OVRCALLBACK2)p_IdleFunction)(args[0], args[1]);
					break;
				case 3:
					((OVRCALLBACK3)p_IdleFunction)(args[0], args[1], args[2]);
					break;
				case 4:
					((OVRCALLBACK4)p_IdleFunction)(args[0], args[1], args[2], args[3]);
					break;
				case 5:
					((OVRCALLBACK5)p_IdleFunction)(args[0], args[1], args[2], args[3], args[4]);
					break;
				default:
					break;
			}
		}
	}

	void MainThreadEX();
//	int  MainThread();
//	void MainThreadLauncher(void*);
//	static void MainThreadLauncher(void *obj) { reinterpret_cast<Oculus *>(obj)->MainThread(); }
	static unsigned __stdcall MainThreadLauncherEX(void *obj);

	void GetSnap();

	static void ErrorCallback(int err, const char* description)
	{
		std::cerr << description << std::endl;
	}

	static void MouseCursorPositionCallback
		(GLFWwindow* window, double xpos, double ypos)
	{
		Oculus* instance = reinterpret_cast<Oculus*>(glfwGetWindowUserPointer(window));
		if (instance != nullptr)
		{
		}
	}

	static void MouseButtonCallback
		(GLFWwindow* window, int button, int action, int mods)
	{
		Oculus* instance = reinterpret_cast<Oculus*>(glfwGetWindowUserPointer(window));
		if (instance != nullptr)
		{
		}
	}

	static void KeyCallback
		(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		Oculus* instance = reinterpret_cast<Oculus*>(glfwGetWindowUserPointer(window));
		if (instance != nullptr)
		{
#if ((OVR_PRODUCT_VERSION == 0) && (OVR_MAJOR_VERSION == 5)) // Oculus SDK 0.5.0.1 only
			ovrHSWDisplayState hswDisplayState;
			ovrHmd_GetHSWDisplayState(instance->m_HmdSession, &hswDisplayState);
			if (hswDisplayState.Displayed)
			{
				ovrHmd_DismissHSWDisplay(instance->m_HmdSession);
			}
#endif
			if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			{
				glfwSetWindowShouldClose(window, GL_TRUE);
			}

			if (key == GLFW_KEY_R && action == GLFW_PRESS)
			{
#if (OVR_PRODUCT_VERSION == 1)
				ovr_RecenterTrackingOrigin(instance->m_HmdSession); // Oculus SDK 1.10.1
#elif (OVR_MAJOR_VERSION > 6)
				ovr_RecenterPose(instance->m_HmdSession);           // Oculus SDK 0.7.0 and 0.8.0
#else
				ovrHmd_RecenterPose(instance->m_HmdSession);        // Oculus SDK 0.5.0.1 and 0.6.0.1
#endif
			}

#if (OVR_PRODUCT_VERSION == 1)
			if (key == GLFW_KEY_T && action == GLFW_PRESS)
			{
				instance->SwitchControllerType();
			}
#endif

			if (key == GLFW_KEY_C && action == GLFW_PRESS)
			{
#ifdef USE_OVRVISION
				instance->m_OVRVision.toggleCameraState();
#endif // USE_OVRVISION
#ifdef USE_ZEDMINI
				instance->m_ZedMini.toggleCameraState();
#endif
			}
			if (key == GLFW_KEY_S && action == GLFW_PRESS)
			{
				instance->GetSnap();
			}
		}
	}

	static void MouseWheelCallback
		(GLFWwindow* window, double xpos, double ypos)
	{
		Oculus* instance = reinterpret_cast<Oculus*>(glfwGetWindowUserPointer(window));
		if (instance != nullptr)
		{
			float SPEED  = 0.2f;
			float delta  = (float)ypos * SPEED;
			float xtrans = delta * instance->m_HeadVector[VECTOR_FRONT].x;
			float ytrans = delta * instance->m_HeadVector[VECTOR_FRONT].y;
			float ztrans = delta * instance->m_HeadVector[VECTOR_FRONT].z;
			instance->Translate(xtrans, ytrans, ztrans);

			static float prevtime = 0;

			float t = static_cast<float>(glfwGetTime());
			float dt = t - prevtime;
			if (fabs(xpos) > 0.2)
				instance->Rotate((float)(-xpos) * 90.0f * dt, 'y');

			prevtime = t;
		}
	}

	static void ResizeCallback
		(GLFWwindow* window, int width, int height)
	{
		Oculus* instance = reinterpret_cast<Oculus*>(glfwGetWindowUserPointer(window));
		if (instance != nullptr)
		{
			glViewport(0, 0, width, height);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluPerspective(30.0, (GLdouble)width / (GLdouble)height, 0.001, 10000.0);
			glMatrixMode(GL_MODELVIEW);
		}
	}
};
