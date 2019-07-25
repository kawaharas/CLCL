////////////////////////////////////////////////////////////////////////////////
//
// oculus.cpp
//
//   CLCL: CAVELib Compatible Library
//
//     Copyright 2015-2019 Shintaro Kawahara(kawahara@jamstec.go.jp).
//     All rights reserved.
//
//   Please read the file "LICENCE.txt" before you use this software.
//
////////////////////////////////////////////////////////////////////////////////

#include "oculus.h"

#define FULL_SCREEN_MODE

bool m_InitializedGLFW = false;
float offset[3] = {0.0, 0.0, -5.0f};

Oculus::Oculus()
{
	m_HmdSession = nullptr;
#if ((OVR_PRODUCT_VERSION == 0) && (OVR_MAJOR_VERSION == 5))
	m_IsHMDDebug = false;  // for Oculus SDK 0.5.0.1
#endif
	m_HeadTranslation.x = 0.0f;
	m_HeadTranslation.y = 0.0f;
	m_HeadTranslation.z = 0.0f;
	for (int i = 0; i < 3; i++)
	{
		m_HeadVector[i].x = 0.0f;
		m_HeadVector[i].y = 0.0f;
		m_HeadVector[i].z = 0.0f;
	}
	m_HeadVector[VECTOR_FRONT].x = 0.0f;
	m_HeadVector[VECTOR_FRONT].y = 0.0f;
	m_HeadVector[VECTOR_FRONT].z = -1.0f;

	m_FrameBuffer = 0;
#if (OVR_PRODUCT_VERSION == 0)
	m_TextureBuffer = 0;    // for Oculus SDK 0.5.0.1
#endif
	m_DepthBuffer = 0;

	m_FrameIndex = 0;
	m_SnapNo = 0;

	m_CurrentEyeIndex = ovrEyeType::ovrEye_Left;
	m_NavigationMatrix.SetIdentity();
	m_ModelMatrix.SetIdentity();

#if (OVR_PRODUCT_VERSION == 1)
	m_TextureSwapChain = 0; // for Oculus SDK 1.10.1
	m_MirrorFBO = 0;        // for Oculus SDK 1.10.1

	m_CurrentControllerType = MOUSE;
	m_IsConnected[0] = true; // mouse
	for (int i = 1; i < ENUM_CONTROLLER_TYPE_SIZE; i++)
	{
		m_IsConnected[i] = false;
	}
#endif

	for (int i = 0; i < 4; i++)
	{
		m_ButtonState[i] = -1;
	}

	m_IsThreadRunning = true;
	m_IsInitializedGLFW.store(false);
	m_HMutex = nullptr;
	m_HRender = nullptr;
	p_InitFunction = nullptr;
	p_StopFunction = nullptr;
	p_DrawFunction = nullptr;
	p_IdleFunction = nullptr;
	m_MainThreadID = 0;
	m_DisplayThreadID = 0;

	m_IsInitFunctionExecuted = false;

	m_FPS = new float;
}

Oculus::~Oculus()
{
	delete m_FPS;
}

void Oculus::Init()
{
	// initialization of oculus
#if (OVR_PRODUCT_VERSION == 1)
	ovrResult result = ovr_Initialize(nullptr);
	if (OVR_FAILURE(result))
	{
		std::cout << "HMD Initialization : FAILED\n";
		exit(EXIT_FAILURE);
	}

	ovrGraphicsLuid luid;
	result = ovr_Create(&m_HmdSession, &luid);
	if (OVR_FAILURE(result))
	{
		std::cout << "HMD Initialization : FAILED2\n";
		ovr_Shutdown();
		exit(EXIT_FAILURE);
	}

	m_HmdDesc = ovr_GetHmdDesc(m_HmdSession);
	ovrSizei hmdResolution = m_HmdDesc.Resolution;
#ifdef FULL_SCREEN_MODE
	m_WindowSize = { 1920, 1080 };
#else
	m_WindowSize = { hmdResolution.w / 2, hmdResolution.h / 2 };
#endif

	ovr_SetTrackingOriginType(m_HmdSession, ovrTrackingOrigin::ovrTrackingOrigin_FloorLevel);

	// check connected controller(s)
	uint m_ConnectedController = ovr_GetConnectedControllerTypes(m_HmdSession);
	std::cout << "========== Check Connection of Controller(s) ==========\n";
	if ((m_ConnectedController & ovrControllerType_XBox) == ovrControllerType_XBox)
	{
		m_IsConnected[XBOX_CONTROLLER] = true;
		m_CurrentControllerType = XBOX_CONTROLLER;
		std::cout << "XBOX Controller     : ENABLE\n";
	}
	else
	{
		std::cout << "XBOX Controller     : DISABLE\n";
	}

	if ((m_ConnectedController & ovrControllerType_RTouch) == ovrControllerType_RTouch)
	{
		m_IsConnected[OCULUS_TOUCH_RIGHT] = true;
		m_CurrentControllerType = OCULUS_TOUCH_RIGHT;
		std::cout << "Oculus Touch (Right): ENABLE\n";
	}
	else
	{
		std::cout << "Oculus Touch (Right): DISABLE\n";
	}

#else
#if (OVR_MAJOR_VERSION > 6)
	ovrResult result = ovr_Initialize(nullptr);
	if (OVR_FAILURE(result))
	{
		exit(EXIT_FAILURE);
	}

	ovrGraphicsLuid luid;
	result = ovr_Create(&m_HmdSession, &luid);
	if (OVR_FAILURE(result))
	{
		ovr_Shutdown();
		exit(EXIT_FAILURE);
	}

	m_HmdDesc = ovr_GetHmdDesc(m_HmdSession);
	ovrSizei hmdResolution = m_HmdDesc.Resolution;
	m_WindowSize = { hmdResolution.w / 2, hmdResolution.h / 2 };
#elif (OVR_MAJOR_VERSION == 6)
	ovrResult result = ovr_Initialize(nullptr);
	if (OVR_FAILURE(result))
	{
		exit(EXIT_FAILURE);
	}

	result = ovrHmd_Create(0, &m_HmdSession);
	if (OVR_FAILURE(result))
	{
//		m_IsHMDDebug = true;
		ovrHmd_CreateDebug(ovrHmd_DK2, &m_HmdSession);
//		ovr_Shutdown();
//		exit(EXIT_FAILURE);
	}

	ovrSizei hmdResolution = m_HmdSession->Resolution;
	m_WindowSize = { hmdResolution.w / 2, hmdResolution.h / 2 };

	ovrHmd_ConfigureTracking(m_HmdSession,
		ovrTrackingCap_Orientation |
		ovrTrackingCap_MagYawCorrection |
		ovrTrackingCap_Position, 0);
#else
	if (ovr_Initialize(nullptr))
	{
		m_HmdSession = ovrHmd_Create(0);
		if (m_HmdSession)
		{
			ovrSizei resolution = m_HmdSession->Resolution;
		}
		else
		{
			m_IsHMDDebug = true;
			m_HmdSession = ovrHmd_CreateDebug(ovrHmd_DK2);
			std::cout << "ovrHmd_CreateDebug(ovrHmd_DK2)" << std::endl;
		}
	}

	ovrHmd_ConfigureTracking(m_HmdSession,
		ovrTrackingCap_Orientation |
		ovrTrackingCap_MagYawCorrection |
		ovrTrackingCap_Position, 0);
#endif
#endif
}

void Oculus::InitGL()
{
	glfwSetErrorCallback(this->ErrorCallback);
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}

#if (OVR_PRODUCT_VERSION == 1)
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	int width, height;
	glfwGetMonitorPhysicalSize(monitor, &width, &height);
	const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	std::cout << "display width  : " << mode->width << std::endl;
	std::cout << "display height : " << mode->height << std::endl;

#ifdef FULL_SCREEN_MODE
	m_Window = glfwCreateWindow(
		m_WindowSize.w, m_WindowSize.h, "CLCL", monitor, NULL);
#else
	m_Window = glfwCreateWindow(
		m_WindowSize.w, m_WindowSize.h, "vFive(GLFW)", NULL, NULL);
#endif

#else
#if (OVR_MAJOR_VERSION > 5)
	m_Window = glfwCreateWindow(
		m_WindowSize.w, m_WindowSize.h, "vFive(GLFW)", NULL, NULL);
#else
	// Oculus SDK 0.5.0.1 only
	GLFWmonitor* monitor = CheckOVR();

#ifdef DEBUG
	switch (m_WindowStyle)
	{
		case DIRECT_MODE:
			std::cout << "DIRECT_MODE" << std::endl;
			break;
		case EXTEND_MODE:
			std::cout << "EXTEND_MODE" << std::endl;
			break;
		case EXTEND_MODE_PORTRAIT:
			std::cout << "EXTEND_MODE_PORTRAIT" << std::endl;
			break;
	}
#endif // DEBUG

	// create render window
	glfwWindowHint(GLFW_DECORATED, GL_FALSE);
	if (m_WindowStyle != EXTEND_MODE_PORTRAIT)
	{
		m_Window = glfwCreateWindow(
			m_HmdSession->Resolution.w, m_HmdSession->Resolution.h, "VFIVE(GLFW)", monitor, NULL);
	}
	else
	{
		m_Window = glfwCreateWindow(
			m_HmdSession->Resolution.w, m_HmdSession->Resolution.h, "VFIVE(GLFW)", NULL, NULL);
	}
#endif
#endif

	if (!m_Window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetWindowUserPointer(m_Window, this); // technique for registering member functions as callback functions

	glfwMakeContextCurrent(m_Window);
	glfwSwapInterval(1);
	glfwSetKeyCallback(m_Window, KeyCallback);
	glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
	glfwSetCursorPosCallback(m_Window, MouseCursorPositionCallback);
	glfwSetScrollCallback(m_Window, MouseWheelCallback);
	glfwSetFramebufferSizeCallback(m_Window, ResizeCallback);
	glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

#if ((OVR_PRODUCT_VERSION == 0) && (OVR_MAJOR_VERSION == 5)) // Oculus SDK 0.5.0.1 only
#ifdef USE_MIRROR_WINDOW
	if (m_WindowStyle != DIRECT_MODE)
	{
		m_MirrorRGBImage = new uchar[m_HmdSession->Resolution.w * m_HmdSession->Resolution.h * 3];
		if (m_WindowStyle == EXTEND_MODE)
		{
			glfwWindowHint(GLFW_DECORATED, GL_TRUE);
			m_MirrorWindow = glfwCreateWindow(
				m_MirrorWindowWidth, m_MirrorWindowHeight,
				"VFIVE(GLFW, mirror)", NULL, NULL);
		}
		else // windowStyle == EXTEND_MODE_PORTRAIT
		{
			glfwWindowHint(GLFW_DECORATED, GL_FALSE);
			glfwWindowHint(GLFW_AUTO_ICONIFY, GL_FALSE);
			m_MirrorWindow = glfwCreateWindow(
				m_MirrorWindowWidth, m_MirrorWindowHeight,
				"VFIVE(GLFW, mirror)", monitor, NULL);
		}

		if (!m_MirrorWindow)
		{
			glfwDestroyWindow(m_Window);
			glfwTerminate();
			exit(EXIT_FAILURE);
		}
		glfwSetFramebufferSizeCallback(m_MirrorWindow, this->ResizeCallback);
	}
#endif // USE_MIRROR_WINDOW
#endif

	// initialize GLEW
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		std::cout << "glewInit failed, aborting." << std::endl;
		exit(EXIT_FAILURE);
	}

#ifdef USE_OVRVISION
	m_OVRVision.Init();
//	m_OVRVision.toggleCameraState(); // change value from "false" to "true" (default: false)
#endif // USE_OVRVISION

#ifdef USE_ZEDMINI
	if (m_ZedMini.Init())
	{
		std::cout << "initialization of zed mini succeeded." << std::endl;
//		glfwMakeContextCurrent(m_Window);
	}
#endif // USE_ZEDMINI
}

void Oculus::CreateBuffers()
{
#if (OVR_PRODUCT_VERSION == 1)
	m_ViewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
	memset(&m_LayerEyeFov, 0, sizeof(ovrLayerEyeFov));
	m_LayerEyeFov.Header.Type = ovrLayerType_EyeFov;
	m_LayerEyeFov.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
#else
#if (OVR_MAJOR_VERSION > 5)
	m_ViewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
	memset(&m_LayerEyeFov, 0, sizeof(ovrLayerEyeFov));
	m_LayerEyeFov.Header.Type = ovrLayerType_EyeFov;
	m_LayerEyeFov.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;
#else
	// Oculus SDK 0.5.0.1
	ovrGLConfig cfg;
	cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
	cfg.OGL.Header.BackBufferSize = OVR::Sizei(m_HmdSession->Resolution.w, m_HmdSession->Resolution.h);
	cfg.OGL.Header.Multisample = 1;

	ovrHmd_AttachToWindow(m_HmdSession, glfwGetWin32Window(m_Window), NULL, NULL);
	cfg.OGL.Window = glfwGetWin32Window(m_Window);	// unnecessary line?
	cfg.OGL.DC = GetDC(glfwGetWin32Window(m_Window));

	ovrHmd_ConfigureRendering(m_HmdSession, &cfg.Config,
		ovrDistortionCap_Vignette |
		ovrDistortionCap_TimeWarp |
		ovrDistortionCap_Overdrive,
		m_HmdSession->DefaultEyeFov, m_EyeRenderDesc);

	ovrHmd_SetEnabledCaps(m_HmdSession,
		ovrHmdCap_LowPersistence |
		ovrHmdCap_DynamicPrediction);
#endif
#endif

	ovrSizei recommendedTextureSize[2];
	for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
	{
#if (OVR_PRODUCT_VERSION == 1)
		ovrEyeRenderDesc eyeRenderDesc;
		eyeRenderDesc = ovr_GetRenderDesc(m_HmdSession, static_cast<ovrEyeType>(eyeIndex), m_HmdDesc.DefaultEyeFov[eyeIndex]);
		m_ProjectionMatrix[eyeIndex] = ovrMatrix4f_Projection(eyeRenderDesc.Fov, 0.3f, 1000.0f, ovrProjection_ClipRangeOpenGL);
#if (OVR_MINOR_VERSION >= 17)
		m_ViewScaleDesc.HmdToEyePose[eyeIndex] = eyeRenderDesc.HmdToEyePose;
#else
		m_ViewScaleDesc.HmdToEyeOffset[eyeIndex] = eyeRenderDesc.HmdToEyeOffset;
#endif
		m_LayerEyeFov.Fov[eyeIndex] = eyeRenderDesc.Fov;
		recommendedTextureSize[eyeIndex] =
			ovr_GetFovTextureSize(m_HmdSession, static_cast<ovrEyeType>(eyeIndex), m_HmdDesc.DefaultEyeFov[eyeIndex], 1.75f);
#else
#if (OVR_MAJOR_VERSION > 6)
		ovrEyeRenderDesc eyeRenderDesc;
		eyeRenderDesc = ovr_GetRenderDesc(m_HmdSession, static_cast<ovrEyeType>(eyeIndex), m_HmdDesc.DefaultEyeFov[eyeIndex]);
//		m_ProjectionMatrix[eyeIndex] = ovrMatrix4f_Projection(eyeRenderDesc.Fov, 0.01f, 10000.0f, ovrProjection_ClipRangeOpenGL);
//		m_ProjectionMatrix[eyeIndex] = ovrMatrix4f_Projection(m_LayerEyeFov.Fov[eyeIndex], 0.01f, 10000.0f, ovrProjection_RightHanded);
		m_ProjectionMatrix[eyeIndex] = ovrMatrix4f_Projection(eyeRenderDesc.Fov, 0.01f, 10000.0f, ovrProjection_RightHanded);
		m_ViewScaleDesc.HmdToEyeViewOffset[eyeIndex] = eyeRenderDesc.HmdToEyeViewOffset;
		m_LayerEyeFov.Fov[eyeIndex] = eyeRenderDesc.Fov;
		recommendedTextureSize[eyeIndex] =
			ovr_GetFovTextureSize(m_HmdSession, static_cast<ovrEyeType>(eyeIndex), m_HmdDesc.DefaultEyeFov[eyeIndex], 1.0f);
#elif (OVR_MAJOR_VERSION == 6)
		ovrEyeRenderDesc eyeRenderDesc;
		eyeRenderDesc = ovrHmd_GetRenderDesc(m_HmdSession, static_cast<ovrEyeType>(eyeIndex), m_HmdSession->DefaultEyeFov[eyeIndex]);
//		m_ProjectionMatrix[eyeIndex] = ovrMatrix4f_Projection(eyeRenderDesc.Fov, 0.01f, 10000.0f, ovrProjection_ClipRangeOpenGL);
		m_ProjectionMatrix[eyeIndex] = ovrMatrix4f_Projection(eyeRenderDesc.Fov, 0.01f, 10000.0f, ovrProjection_RightHanded);
		m_ViewScaleDesc.HmdToEyeViewOffset[eyeIndex] = eyeRenderDesc.HmdToEyeViewOffset;
		m_LayerEyeFov.Fov[eyeIndex] = eyeRenderDesc.Fov;
		recommendedTextureSize[eyeIndex] =
			ovrHmd_GetFovTextureSize(m_HmdSession, static_cast<ovrEyeType>(eyeIndex), m_HmdSession->DefaultEyeFov[eyeIndex], 1.0f);
#else
		m_ProjectionMatrix[eyeIndex] = ovrMatrix4f_Projection(m_EyeRenderDesc[eyeIndex].Fov, 0.01f, 10000.0f, true);
		m_HmdToEyeViewOffset[eyeIndex] = m_EyeRenderDesc[eyeIndex].HmdToEyeViewOffset;
		recommendedTextureSize[eyeIndex] =
			ovrHmd_GetFovTextureSize(m_HmdSession, static_cast<ovrEyeType>(eyeIndex), m_HmdSession->DefaultEyeFov[eyeIndex], 1.0f);
#endif
#endif
	}

	m_RenderTargetSize.w = recommendedTextureSize[0].w + recommendedTextureSize[1].w;
	m_RenderTargetSize.h = std::max(recommendedTextureSize[0].h, recommendedTextureSize[1].h);
	std::cout << "renderTargetSize: " << m_RenderTargetSize.w << ", " << m_RenderTargetSize.h << std::endl;
#if (OVR_PRODUCT_VERSION == 1)
	ovrTextureSwapChainDesc textureSwapChainDesc = {};
	textureSwapChainDesc.Type = ovrTexture_2D;
	textureSwapChainDesc.ArraySize = 1;
	textureSwapChainDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
	textureSwapChainDesc.Width  = m_RenderTargetSize.w;
	textureSwapChainDesc.Height = m_RenderTargetSize.h;
	textureSwapChainDesc.MipLevels = 1;
	textureSwapChainDesc.SampleCount = 1;
	textureSwapChainDesc.StaticImage = ovrFalse;

	ovrResult result = ovr_CreateTextureSwapChainGL(m_HmdSession, &textureSwapChainDesc, &m_TextureSwapChain);
	if (OVR_FAILURE(result))
	{
		std::cout << "ERROR: Cound not create ovrTextureSwapChain objects." << std::endl;
		exit(EXIT_FAILURE);
	}

	int textureSwapChainLength = 0;
	result = ovr_GetTextureSwapChainLength(m_HmdSession, m_TextureSwapChain, &textureSwapChainLength);
	if (OVR_FAILURE(result) || !textureSwapChainLength)
	{
		std::cout << "ERROR: Cound not get length of TextureSwapChain." << std::endl;
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < textureSwapChainLength; ++i)
	{
		GLuint swapChainTextureID;
		ovr_GetTextureSwapChainBufferGL(m_HmdSession, m_TextureSwapChain, i, &swapChainTextureID);
		glBindTexture(GL_TEXTURE_2D, swapChainTextureID);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	m_LayerEyeFov.ColorTexture[0] = m_TextureSwapChain;
	m_LayerEyeFov.ColorTexture[1] = m_TextureSwapChain;
	m_LayerEyeFov.Viewport[0] = OVR::Recti(0, 0, m_RenderTargetSize.w / 2, m_RenderTargetSize.h);
	m_LayerEyeFov.Viewport[1] = OVR::Recti(m_RenderTargetSize.w / 2, 0, m_RenderTargetSize.w / 2, m_RenderTargetSize.h);
#else
#if (OVR_MAJOR_VERSION > 5)
	ovrResult result;

#if (OVR_MAJOR_VERSION > 6)
	result = ovr_CreateSwapTextureSetGL(m_HmdSession, GL_SRGB8_ALPHA8, m_RenderTargetSize.w, m_RenderTargetSize.h, &p_SwapTextureSet);
#else
//	result = ovrHmd_CreateSwapTextureSetGL(m_HmdSession, GL_SRGB8_ALPHA8, m_RenderTargetSize.w, m_RenderTargetSize.h, &p_SwapTextureSet);
	result = ovrHmd_CreateSwapTextureSetGL(m_HmdSession, GL_RGBA, m_RenderTargetSize.w, m_RenderTargetSize.h, &p_SwapTextureSet);
#endif
	if (OVR_FAILURE(result))
	{
		std::cout << "ERROR: Cound not create ovrTextureSwapChain objects." << std::endl;
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < p_SwapTextureSet->TextureCount; i++)
	{
		ovrGLTexture* tex = (ovrGLTexture*)&p_SwapTextureSet->Textures[i];
		glBindTexture(GL_TEXTURE_2D, tex->OGL.TexId);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	m_LayerEyeFov.ColorTexture[0] = p_SwapTextureSet;
	m_LayerEyeFov.ColorTexture[1] = p_SwapTextureSet;
	m_LayerEyeFov.Viewport[0] = OVR::Recti(0, 0, m_RenderTargetSize.w / 2, m_RenderTargetSize.h);
	m_LayerEyeFov.Viewport[1] = OVR::Recti(m_RenderTargetSize.w / 2, 0, m_RenderTargetSize.w / 2, m_RenderTargetSize.h);
#else
	// Oculus SDK 0.5.0.1
	m_EyeRenderViewport[0] = OVR::Recti(0, 0, m_RenderTargetSize.w / 2, m_RenderTargetSize.h);
	m_EyeRenderViewport[1] = OVR::Recti(m_RenderTargetSize.w / 2, 0, m_RenderTargetSize.w / 2, m_RenderTargetSize.h);

	// create a color buffer
	glGenTextures(1, &m_TextureBuffer);
	glBindTexture(GL_TEXTURE_2D, m_TextureBuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
		m_RenderTargetSize.w, m_RenderTargetSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
#endif
#endif

	// create a depth buffer
	glGenRenderbuffers(1, &m_DepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBuffer);
#if (OVR_PRODUCT_VERSION == 1)
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, m_RenderTargetSize.w, m_RenderTargetSize.h);
#else
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_RenderTargetSize.w, m_RenderTargetSize.h);
#endif
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// create a framebuffer object and bind the depth buffer
	glGenFramebuffers(1, &m_FrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
#if ((OVR_PRODUCT_VERSION == 0) && (OVR_MAJOR_VERSION == 5))
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_TextureBuffer, 0);
#endif
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

#if (OVR_PRODUCT_VERSION == 1)
	// create a mirror texture
	ovrMirrorTextureDesc mirrorTextureDesc;
	memset(&mirrorTextureDesc, 0, sizeof(mirrorTextureDesc));
	mirrorTextureDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
#ifdef FULL_SCREEN_MODE
	mirrorTextureDesc.Width = m_WindowSize.w;
	mirrorTextureDesc.Height = m_WindowSize.h;
#else
	mirrorTextureDesc.Width  = m_WindowSize.w / 2;
	mirrorTextureDesc.Height = m_WindowSize.h / 2;
#endif
	result = ovr_CreateMirrorTextureGL(m_HmdSession, &mirrorTextureDesc, &m_MirrorTexture);
	if (OVR_FAILURE(result))
	{
		std::cout << "ERROR: Cound not create mirror texture." << std::endl;
		exit(EXIT_FAILURE);
	}
	glGenFramebuffers(1, &m_MirrorFBO);
#else
#if (OVR_MAJOR_VERSION > 5)
#if (OVR_MAJOR_VERSION > 6)
	result = ovr_CreateMirrorTextureGL(m_HmdSession, GL_SRGB8_ALPHA8, m_WindowSize.w, m_WindowSize.h, reinterpret_cast<ovrTexture **>(&mirrorTexture));
#else
	result = ovrHmd_CreateMirrorTextureGL(m_HmdSession, GL_SRGB8_ALPHA8, m_WindowSize.w, m_WindowSize.h, reinterpret_cast<ovrTexture **>(&mirrorTexture));
#endif
	if (OVR_SUCCESS(result))
	{
		glGenFramebuffers(1, &m_MirrorFBO);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_MirrorFBO);
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTexture->OGL.TexId, 0);
		glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}
	glGenFramebuffers(ovrEye_Count, oculusFbo);
	glEnable(GL_FRAMEBUFFER_SRGB);
/*
	// create a mirror texture
	result = ovr_CreateMirrorTextureGL(m_HmdSession, GL_SRGB8_ALPHA8, m_WindowSize.w / 2, m_WindowSize.h / 2, &p_MirrorTexture);
	if (OVR_FAILURE(result))
	{
		std::cout << "ERROR: Cound not create mirror texture." << std::endl;
		exit(EXIT_FAILURE);
	}
	glGenFramebuffers(1, &m_MirrorFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_MirrorFBO);
//	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_MirrorTexture->OGL.TexId, 0);
	glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
*/
#endif
#endif

	// check buffers
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR: Could not initialize VR buffers -- aborting." << std::endl;
#if (OVR_PRODUCT_VERSION == 1)
		glDeleteFramebuffers(1, &m_FrameBuffer);
		glDeleteRenderbuffers(1, &m_DepthBuffer);
		glDeleteFramebuffers(1, &m_MirrorFBO);
#else
#if (OVR_MAJOR_VERSION > 5)
		glDeleteFramebuffers(1, &m_FrameBuffer);
		glDeleteRenderbuffers(1, &m_DepthBuffer);
		glDeleteFramebuffers(1, &m_MirrorFBO);
#else
		glDeleteFramebuffers(1, &m_FrameBuffer);
		glDeleteTextures(1, &m_TextureBuffer);
		glDeleteRenderbuffers(1, &m_DepthBuffer);
#endif
#endif
		exit(EXIT_FAILURE);
	}

#if ((OVR_PRODUCT_VERSION == 0) && (OVR_MAJOR_VERSION == 5)) // Oculus SDK 0.5.0.1 only
	// bind framebuffers to eyeTextures
	m_EyeTexture[0].OGL.Header.API = ovrRenderAPI_OpenGL;
	m_EyeTexture[0].OGL.Header.TextureSize = m_RenderTargetSize;
	m_EyeTexture[0].OGL.Header.RenderViewport = m_EyeRenderViewport[0];
	m_EyeTexture[0].OGL.TexId = m_TextureBuffer;
	m_EyeTexture[1] = m_EyeTexture[0];
	m_EyeTexture[1].OGL.Header.RenderViewport = m_EyeRenderViewport[1];
#endif

#ifdef USE_ZEDMINI
	// Compute the Horizontal Oculus' field of view with its parameters
	float ovrFovH = (atanf(m_HmdDesc.DefaultEyeFov[0].LeftTan) + atanf(m_HmdDesc.DefaultEyeFov[0].RightTan));
	// Compute the Vertical Oculus' field of view with its parameters
	float ovrFovV = (atanf(m_HmdDesc.DefaultEyeFov[0].UpTan) + atanf(m_HmdDesc.DefaultEyeFov[0].DownTan));

	// Compute the center of the optical lenses of the headset
	float offsetLensCenterX = ((atanf(m_HmdDesc.DefaultEyeFov[0].LeftTan)) / ovrFovH) * 2.0f - 1.0f;
	float offsetLensCenterY = ((atanf(m_HmdDesc.DefaultEyeFov[0].UpTan)) / ovrFovV) * 2.0f - 1.0f;

	m_ZedMini.SetScreenCoord(m_RenderTargetSize.w, m_RenderTargetSize.h, offsetLensCenterX, offsetLensCenterY);
#endif // USE_ZEDMINI

#ifdef STORE_LEFT_EYE_TEXTURE
	m_LeftEyeTextureData = (uchar *)malloc(sizeof(uchar) * m_RenderTargetSize.w / 2 * m_RenderTargetSize.h * 3);
/*
	glGenTextures(1, &m_LeftEyeTextureID);
	glBindTexture(GL_TEXTURE_2D, m_LeftEyeTextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_RenderTargetSize.w / 2, m_RenderTargetSize.h,
		0, GL_RGBA, GL_UNSIGNED_BYTE, m_LeftEyeTextureData);
	glBindTexture(GL_TEXTURE_2D, 0);
*/
#endif // STORE_LEFT_EYE_TEXTURE
}

void Oculus::Terminate()
{
	if (m_HmdSession != nullptr)
	{
#if (OVR_PRODUCT_VERSION == 1)
		glDeleteFramebuffers(1, &m_FrameBuffer);
		glDeleteRenderbuffers(1, &m_DepthBuffer);
		glDeleteFramebuffers(1, &m_MirrorFBO);
		ovr_DestroyTextureSwapChain(m_HmdSession, m_TextureSwapChain);
		ovr_DestroyMirrorTexture(m_HmdSession, m_MirrorTexture);
#else
#if (OVR_MAJOR_VERSION > 5)
		glDeleteFramebuffers(1, &m_FrameBuffer);
		glDeleteRenderbuffers(1, &m_DepthBuffer);
		glDeleteFramebuffers(1, &m_MirrorFBO);
#if (OVR_MAJOR_VERSION > 6)
		ovr_DestroySwapTextureSet(m_HmdSession, p_SwapTextureSet);
		ovr_DestroyMirrorTexture(m_HmdSession, p_MirrorTexture);
#else
		ovrHmd_DestroySwapTextureSet(m_HmdSession, p_SwapTextureSet);
		ovrHmd_DestroyMirrorTexture(m_HmdSession, p_MirrorTexture);
#endif
#else
		// Oculus SDK 0.5.0.1
		ovrHmd_ConfigureRendering(m_HmdSession, nullptr, 0, nullptr, nullptr);
		glDeleteFramebuffers(1, &m_FrameBuffer);
		glDeleteTextures(1, &m_TextureBuffer);
		glDeleteRenderbuffers(1, &m_DepthBuffer);
#endif
#endif
	}

#if ((OVR_PRODUCT_VERSION == 0) && (OVR_MAJOR_VERSION == 5))
#ifdef USE_MIRROR_WINDOW
	if (m_WindowStyle != DIRECT_MODE)
	{
		delete[] m_MirrorRGBImage;
		glfwDestroyWindow(m_MirrorWindow);
	}
#endif // USE_MIRROR_WINDOW
#endif

	glfwDestroyWindow(m_Window);
	glfwTerminate();

	if (m_HmdSession != nullptr)
	{
#if (OVR_PRODUCT_VERSION == 1)
		ovr_Destroy(m_HmdSession);
#else
#if (OVR_MAJOR_VERSION > 6)
		ovr_Destroy(m_HmdSession);
#else
		ovrHmd_Destroy(m_HmdSession); // Oculus SDK 0.5.0.1 and 0.6.0.1
#endif
#endif
		ovr_Shutdown();
	}

#ifdef USE_OVRVISION
	m_OVRVision.Terminate();
#endif USE_OVRVISION
#ifdef USE_ZEDMINI
	m_ZedMini.Terminate();
#endif // USE_ZEDMINI
}

void Oculus::UpdateTrackingData()
{
	m_FrameIndex++;

#if (OVR_PRODUCT_VERSION == 1)
	double frameTiming = ovr_GetPredictedDisplayTime(m_HmdSession, m_FrameIndex); // m_FrameIndex = 0 : Auto
	ovrTrackingState trackingState = ovr_GetTrackingState(m_HmdSession, frameTiming, ovrTrue);
#if (OVR_MINOR_VERSION >= 17)
	ovr_CalcEyePoses(trackingState.HeadPose.ThePose, m_ViewScaleDesc.HmdToEyePose, m_LayerEyeFov.RenderPose);
#else
	ovr_CalcEyePoses(trackingState.HeadPose.ThePose, m_ViewScaleDesc.HmdToEyeOffset, m_LayerEyeFov.RenderPose);
#endif
#else
#if (OVR_MAJOR_VERSION == 8)
	double frameTiming = ovr_GetPredictedDisplayTime(m_HmdSession, m_FrameIndex); // m_FrameIndex = 0 : Auto
	ovrTrackingState trackingState = ovr_GetTrackingState(m_HmdSession, frameTiming, ovrTrue);
	ovr_CalcEyePoses(trackingState.HeadPose.ThePose, m_ViewScaleDesc.HmdToEyeViewOffset, m_LayerEyeFov.RenderPose);
#elif (OVR_MAJOR_VERSION == 7)
	ovrTrackingState trackingState = ovr_GetTrackingState(m_HmdSession, ovr_GetTimeInSeconds());
	ovr_CalcEyePoses(trackingState.HeadPose.ThePose, m_ViewScaleDesc.HmdToEyeViewOffset, m_LayerEyeFov.RenderPose);
#elif (OVR_MAJOR_VERSION == 6)
	ovrTrackingState trackingState = ovrHmd_GetTrackingState(m_HmdSession, ovr_GetTimeInSeconds());
	ovr_CalcEyePoses(trackingState.HeadPose.ThePose, m_ViewScaleDesc.HmdToEyeViewOffset, m_LayerEyeFov.RenderPose);
#else
	ovrTrackingState trackingState = ovrHmd_GetTrackingState(m_HmdSession, ovr_GetTimeInSeconds()); // Oculus SDK 0.5.0.1
	ovrHmd_GetEyePoses(m_HmdSession, 0, m_HmdToEyeViewOffset, m_EyePose, NULL);
#endif
#endif

	if (trackingState.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
	{
		// get position and vector of devices in real world
		OVR::Posef pose = trackingState.HeadPose.ThePose;
		m_HeadTranslation = OVR::Vector3f(
			ovrPosef(pose).Position.x * 10.0f / FEET_PER_METER,
			ovrPosef(pose).Position.y * 10.0f / FEET_PER_METER,
			ovrPosef(pose).Position.z * 10.0f / FEET_PER_METER);
		OVR::Matrix4f rollPitchYaw = OVR::Matrix4f::RotationY(0.0f);
		OVR::Matrix4f finalRollPitchYaw =
			rollPitchYaw * OVR::Matrix4f(ovrPosef(pose).Orientation);
		m_HeadVector[VECTOR_UP]    = finalRollPitchYaw.Transform(OVR::Vector3f(0.0f, 1.0f,  0.0f));
		m_HeadVector[VECTOR_FRONT] = finalRollPitchYaw.Transform(OVR::Vector3f(0.0f, 0.0f, -1.0f));
		m_HeadVector[VECTOR_RIGHT] = finalRollPitchYaw.Transform(OVR::Vector3f(1.0f, 0.0f,  0.0f));
		float angle_x, angle_y, angle_z;
		finalRollPitchYaw.ToEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z, OVR::Rotate_CCW, OVR::Handed_R>(&angle_x, &angle_y, &angle_z);
		m_HeadOrientation = OVR::Vector3f(angle_x, angle_y, angle_z);

		// get position and vector of devices in navigated coordinate
		OVR::Matrix4f poseTmp = OVR::Matrix4f(pose);
		OVR::Matrix4f newMatrix = OVR::Matrix4f(
			poseTmp.M[0][0], poseTmp.M[0][1], poseTmp.M[0][2], poseTmp.M[0][3] * 10.0f / FEET_PER_METER,
			poseTmp.M[1][0], poseTmp.M[1][1], poseTmp.M[1][2], poseTmp.M[1][3] * 10.0f / FEET_PER_METER,
			poseTmp.M[2][0], poseTmp.M[2][1], poseTmp.M[2][2], poseTmp.M[2][3] * 10.0f / FEET_PER_METER,
			poseTmp.M[3][0], poseTmp.M[3][1], poseTmp.M[3][2], poseTmp.M[3][3]);
		finalRollPitchYaw = m_NavigationMatrix.Inverted() * newMatrix;
		m_HeadTranslationNav = OVR::Vector3f(
			finalRollPitchYaw.M[0][3],
			finalRollPitchYaw.M[1][3],
			finalRollPitchYaw.M[2][3]);
		m_HeadVectorNav[VECTOR_RIGHT] = OVR::Vector3f( finalRollPitchYaw.M[0][0],  finalRollPitchYaw.M[1][0],  finalRollPitchYaw.M[2][0]);
		m_HeadVectorNav[VECTOR_UP   ] = OVR::Vector3f( finalRollPitchYaw.M[0][1],  finalRollPitchYaw.M[1][1],  finalRollPitchYaw.M[2][1]);
		m_HeadVectorNav[VECTOR_FRONT] = OVR::Vector3f(-finalRollPitchYaw.M[0][2], -finalRollPitchYaw.M[1][2], -finalRollPitchYaw.M[2][2]); // VECTOR_BACK * -1.0f
		finalRollPitchYaw.ToEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z, OVR::Rotate_CCW, OVR::Handed_R>(&angle_x, &angle_y, &angle_z);
		m_HeadOrientationNav = OVR::Vector3f(angle_x, angle_y, angle_z);

#if (OVR_PRODUCT_VERSION == 1)
		if (m_CurrentControllerType == OCULUS_TOUCH_RIGHT)
		{
			ovrPosef         handPoses[2];
			ovrInputState    inputState;
			handPoses[ovrHand_Left]  = trackingState.HandPoses[ovrHand_Left].ThePose;
			handPoses[ovrHand_Right] = trackingState.HandPoses[ovrHand_Right].ThePose;
			OVR::Matrix4f finalRollPitchYaw_Hand[2];
			for (int i = 0; i < ovrHand_Count; i++)
			{
				// get position and vector of devices in real world
				m_HandTranslation[i] = OVR::Vector3f(
					ovrPosef(handPoses[i]).Position.x * 10.0f / FEET_PER_METER,
					ovrPosef(handPoses[i]).Position.y * 10.0f / FEET_PER_METER,
					ovrPosef(handPoses[i]).Position.z * 10.0f / FEET_PER_METER);
				finalRollPitchYaw_Hand[i] = rollPitchYaw * OVR::Matrix4f(handPoses[i].Orientation);
				m_HandVector[i][VECTOR_UP]    = finalRollPitchYaw_Hand[i].Transform(OVR::Vector3f(0.0f, 1.0f,  0.0f));
				m_HandVector[i][VECTOR_FRONT] = finalRollPitchYaw_Hand[i].Transform(OVR::Vector3f(0.0f, 0.0f, -1.0f));
				m_HandVector[i][VECTOR_RIGHT] = finalRollPitchYaw_Hand[i].Transform(OVR::Vector3f(1.0f, 0.0f,  0.0f));

				// get position and vector of devices in navigated coordinate
				OVR::Matrix4f handPoseTmp = OVR::Matrix4f(handPoses[i]);
				OVR::Matrix4f newMatrix = OVR::Matrix4f(
					handPoseTmp.M[0][0], handPoseTmp.M[0][1], handPoseTmp.M[0][2], handPoseTmp.M[0][3] * 10.0f / FEET_PER_METER,
					handPoseTmp.M[1][0], handPoseTmp.M[1][1], handPoseTmp.M[1][2], handPoseTmp.M[1][3] * 10.0f / FEET_PER_METER,
					handPoseTmp.M[2][0], handPoseTmp.M[2][1], handPoseTmp.M[2][2], handPoseTmp.M[2][3] * 10.0f / FEET_PER_METER,
					handPoseTmp.M[3][0], handPoseTmp.M[3][1], handPoseTmp.M[3][2], handPoseTmp.M[3][3]);
				finalRollPitchYaw = m_NavigationMatrix.Inverted() * newMatrix;
				m_HandTranslationNav[i] = OVR::Vector3f(
					finalRollPitchYaw.M[0][3],
					finalRollPitchYaw.M[1][3],
					finalRollPitchYaw.M[2][3]);
				m_HandVectorNav[i][VECTOR_RIGHT] = OVR::Vector3f( finalRollPitchYaw.M[0][0],  finalRollPitchYaw.M[1][0],  finalRollPitchYaw.M[2][0]);
				m_HandVectorNav[i][VECTOR_UP   ] = OVR::Vector3f( finalRollPitchYaw.M[0][1],  finalRollPitchYaw.M[1][1],  finalRollPitchYaw.M[2][1]);
				m_HandVectorNav[i][VECTOR_FRONT] = OVR::Vector3f(-finalRollPitchYaw.M[0][2], -finalRollPitchYaw.M[1][2], -finalRollPitchYaw.M[2][2]);
			}
		}
#endif
	}
}

void Oculus::PreProcess()
{
#if (OVR_PRODUCT_VERSION == 1)
	int currentIndex = 0;
	ovr_GetTextureSwapChainCurrentIndex(m_HmdSession, m_TextureSwapChain, &currentIndex);
	GLuint currentTextureID = 0;
	ovr_GetTextureSwapChainBufferGL(m_HmdSession, m_TextureSwapChain, currentIndex, &currentTextureID);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, currentTextureID, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);

#ifdef USE_ZEDMINI
	// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
	ovrPosef eyeRenderPose[2];
	ovrPosef hmdToEyeOffset[2];
	// Get the Oculus view scale description
	double sensorSampleTime;
	hmdToEyeOffset[ovrEye_Left]  = ovr_GetRenderDesc(m_HmdSession, ovrEye_Left, m_HmdDesc.DefaultEyeFov[ovrEye_Left]).HmdToEyePose;
	hmdToEyeOffset[ovrEye_Right] = ovr_GetRenderDesc(m_HmdSession, ovrEye_Right, m_HmdDesc.DefaultEyeFov[ovrEye_Right]).HmdToEyePose;

	// Get eye poses, feeding in correct IPD offset
	ovr_GetEyePoses2(m_HmdSession, m_FrameIndex, ovrTrue, hmdToEyeOffset, eyeRenderPose, &sensorSampleTime);
	m_LayerEyeFov.SensorSampleTime = sensorSampleTime;
#endif // USE_ZEDMINI

#else
#if (OVR_MAJOR_VERSION > 5)
	p_SwapTextureSet->CurrentIndex = (p_SwapTextureSet->CurrentIndex + 1) % p_SwapTextureSet->TextureCount;
	glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
	ovrGLTexture *glTexture = reinterpret_cast<ovrGLTexture *>(&p_SwapTextureSet->Textures[p_SwapTextureSet->CurrentIndex]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glTexture->OGL.TexId, 0);
//	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_DepthBuffer);
#else
	glfwMakeContextCurrent(m_Window);
	ovrHmd_BeginFrame(m_HmdSession, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
#endif
#endif

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef USE_OVRVISION
	m_OVRVision.PreStore();
#endif // USE_OVRVISION
#ifdef USE_ZEDMINI
	m_ZedMini.PreStore();
#endif // USE_ZEDMINI
}

void Oculus::PostProcess()
{
#if (OVR_PRODUCT_VERSION == 1)
#ifdef STORE_LEFT_EYE_TEXTURE
	const auto& vp0 = m_LayerEyeFov.Viewport[0];
//	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vp0.Size.w, vp0.Size.h, GL_RGB, GL_UNSIGNED_BYTE, m_LeftEyeTextureData);
	glDrawPixels(vp0.Size.w, vp0.Size.h, GL_RGB, GL_UNSIGNED_BYTE, m_LeftEyeTextureData);
#endif // STORE_LEFT_EYE_TEXTURE

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ovr_CommitTextureSwapChain(m_HmdSession, m_TextureSwapChain);
#else
#if (OVR_MAJOR_VERSION > 5)
//	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
//	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0); // TEST
//	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#else
	// Oculus SDK 0.5.0.1
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ovrHmd_EndFrame(m_HmdSession, m_EyePose, &m_EyeTexture[0].Texture);
#endif
#endif

#if (OVR_PRODUCT_VERSION == 1)
	ovrLayerHeader* layerHeader = &m_LayerEyeFov.Header;
//	ovr_SubmitFrame(m_HmdSession, 0, nullptr, &layerHeader, 1); // based on Developers Guide
	ovr_SubmitFrame(m_HmdSession, 0, &m_ViewScaleDesc, &layerHeader, 1);

	// render to mirror window
	GLuint mirrorTextureID;
	ovr_GetMirrorTextureBufferGL(m_HmdSession, m_MirrorTexture, &mirrorTextureID);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_MirrorFBO);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTextureID, 0);
#ifdef FULL_SCREEN_MODE
	glBlitFramebuffer(0, 0, m_WindowSize.w, m_WindowSize.h, 0, m_WindowSize.h, m_WindowSize.w, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
#else
	glBlitFramebuffer(0, 0, m_WindowSize.w / 2, m_WindowSize.h / 2, 0, m_WindowSize.h, m_WindowSize.w, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
#endif
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	glfwSwapBuffers(m_Window);
#else
#if (OVR_MAJOR_VERSION > 5)
	ovrLayerHeader *layerHeader = &m_LayerEyeFov.Header;

#if (OVR_MAJOR_VERSION > 6)
//	ovr_SubmitFrame(m_HmdSession, 0, &m_ViewScaleDesc, &layerHeader, 1);
	ovr_SubmitFrame(m_HmdSession, 0, nullptr, &layerHeader, 1);
#else
//	ovrHmd_SubmitFrame(m_HmdSession, 0, &m_ViewScaleDesc, &layerHeader, 1);
	ovrHmd_SubmitFrame(m_HmdSession, 0, nullptr, &layerHeader, 1);
#endif

	// render to mirror window
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_MirrorFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, m_WindowSize.w, m_WindowSize.h, 0, m_WindowSize.h, m_WindowSize.w, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	glfwSwapBuffers(m_Window);

#else // Oculus SDK 0.5.0.1 only
#ifdef USE_MIRROR_WINDOW
	////////////////////////////////////////////////////////////
	// 2nd pass: render to mirror window
	////////////////////////////////////////////////////////////
	if (m_WindowStyle != DIRECT_MODE)
	{
		// store current image of main window to memory
		glReadBuffer(GL_FRONT);
		glReadPixels(0, 0, m_HmdSession->Resolution.w, m_HmdSession->Resolution.h,
			GL_RGB, GL_UNSIGNED_BYTE, m_MirrorRGBImage);

		// change render target to mirror window
		glfwMakeContextCurrent(m_MirrorWindow);
		glfwSwapInterval(1);

		// create texture image
		GLuint mirrorTexID;
		glGenTextures(1, &mirrorTexID);
		glBindTexture(GL_TEXTURE_2D, mirrorTexID);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_HmdSession->Resolution.w, m_HmdSession->Resolution.h,
			0, GL_RGB, GL_UNSIGNED_BYTE, m_MirrorRGBImage);
		glBindTexture(GL_TEXTURE_2D, 0);

		// render texture image to mirror window
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glViewport(0, 0, m_MirrorWindowWidth, m_MirrorWindowHeight);
		gluOrtho2D(0, m_MirrorWindowWidth, 0, m_MirrorWindowHeight);

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, mirrorTexID);
		glBegin(GL_QUADS);

		if (m_WindowStyle == EXTEND_MODE)
		{
			glTexCoord2i(0, 0); glVertex2i(0, 0);
			glTexCoord2i(1, 0); glVertex2i(m_MirrorWindowWidth, 0);
			glTexCoord2i(1, 1); glVertex2i(m_MirrorWindowWidth, m_MirrorWindowHeight);
			glTexCoord2i(0, 1); glVertex2i(0, m_MirrorWindowHeight);
		}
		else // windowStyle == EXTEND_MODE_PORTRAIT
		{
			glTexCoord2i(0, 0); glVertex2i(m_MirrorWindowWidth, 0);
			glTexCoord2i(1, 0); glVertex2i(m_MirrorWindowWidth, m_MirrorWindowHeight);
			glTexCoord2i(1, 1); glVertex2i(0, m_MirrorWindowHeight);
			glTexCoord2i(0, 1); glVertex2i(0, 0);
		}

		glEnd();
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		glDeleteTextures(1, &mirrorTexID);

		glfwSwapBuffers(m_MirrorWindow);

		// change render target to main window
		glfwMakeContextCurrent(m_Window);
	}
#endif // USE_MIRROR_WINDOW
#endif
#endif

	glfwPollEvents();
}

void Oculus::SetMatrix(int eyeIndex)
{
	m_CurrentEyeIndex = eyeIndex;

#if ((OVR_PRODUCT_VERSION == 0) && (OVR_MAJOR_VERSION == 5))
	const auto& vp = m_EyeRenderViewport[eyeIndex];
	glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
#else
	const auto& vp = m_LayerEyeFov.Viewport[eyeIndex];
	glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);

#ifdef USE_OVRVISION
	m_OVRVision.DrawImege(eyeIndex);
#endif // USE_OVRVISION
#ifdef USE_ZEDMINI
	m_ZedMini.DrawImage(eyeIndex);
#endif // USE_ZEDMINI

#ifdef STORE_LEFT_EYE_TEXTURE
	if (eyeIndex == 1)
	{
		const auto& vp0 = m_LayerEyeFov.Viewport[0];
		glReadBuffer(GL_FRONT);
		glReadPixels(0, 0, vp0.Size.w, vp0.Size.h, GL_RGB, GL_UNSIGNED_BYTE, m_LeftEyeTextureData);
//		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vp0.Pos.x, vp0.Pos.y, vp0.Size.w, vp0.Size.h);
//		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LeftEyeTextureData);
}
#endif // STORE_LEFT_EYE_TEXTURE
#endif

	OVR::Matrix4f rollPitchYaw = OVR::Matrix4f::RotationY(0.0f);
#if ((OVR_PRODUCT_VERSION == 0) && (OVR_MAJOR_VERSION == 5))
	OVR::Matrix4f finalRollPitchYaw =
		rollPitchYaw * OVR::Matrix4f(m_EyePose[eyeIndex].Orientation);
	OVR::Vector3f finalUp = finalRollPitchYaw.Transform(OVR::Vector3f(0, 1, 0));
	OVR::Vector3f finalForward = finalRollPitchYaw.Transform(OVR::Vector3f(0, 0, -1));
	OVR::Vector3f shiftedEyePos =
		rollPitchYaw.Transform(m_EyePose[eyeIndex].Position) * 10.0f;
#else
	OVR::Matrix4f finalRollPitchYaw =
		rollPitchYaw * OVR::Matrix4f(m_LayerEyeFov.RenderPose[eyeIndex].Orientation);
	OVR::Vector3f finalUp = finalRollPitchYaw.Transform(OVR::Vector3f(0, 1, 0));
	OVR::Vector3f finalForward = finalRollPitchYaw.Transform(OVR::Vector3f(0, 0, -1));
	OVR::Vector3f shiftedEyePos =
		rollPitchYaw.Transform(m_LayerEyeFov.RenderPose[eyeIndex].Position) * 10.0f;
#endif
	OVR::Matrix4f viewMatrix = OVR::Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
//	OVR::Matrix4f modelViewMatrix = viewMatrix;

	glEnable(GL_DEPTH_TEST);

	glUseProgram(0);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&(m_ProjectionMatrix[eyeIndex].Transposed().M[0][0]));
	glMatrixMode(GL_MODELVIEW);
//	glLoadMatrixf(&(modelViewMatrix.Transposed().M[0][0]));
	glLoadMatrixf(&(viewMatrix.Transposed().M[0][0]));
}

void Oculus::Translate(float x, float y, float z)
{
	OVR::Matrix4f currentMatrix = m_NavigationMatrix;
	m_NavigationMatrix = OVR::Matrix4f::Translation(-x, -y, -z) * currentMatrix;
}

void Oculus::Rotate(float angle_degree, char axis)
{
	float angle_radian = -angle_degree * (float)M_PI / 180.0f;
	OVR::Matrix4f currentMatrix = m_NavigationMatrix;
	switch (tolower(axis))
	{
		case 'x':
			m_NavigationMatrix = OVR::Matrix4f::RotationX(angle_radian) * currentMatrix;
			break;
		case 'y':
			m_NavigationMatrix = OVR::Matrix4f::RotationY(angle_radian) * currentMatrix;
			break;
		case 'z':
			m_NavigationMatrix = OVR::Matrix4f::RotationZ(angle_radian) * currentMatrix;
			break;
		default:
			break;
	}
}

void Oculus::Scale(float x, float y, float z)
{
	OVR::Matrix4f currentMatrix = m_NavigationMatrix;
	m_NavigationMatrix = OVR::Matrix4f::Scaling(1.0f / x, 1.0f / y, 1.0f / z) * currentMatrix;
}

void Oculus::WorldTranslate(float x, float y, float z)
{
	m_NavigationMatrix *= OVR::Matrix4f::Translation(-x, -y, -z);
}

void Oculus::WorldRotate(float angle_degree, char axis)
{
	float angle_radian = -angle_degree * M_PI / 180.0f;
	switch (tolower(axis))
	{
		case 'x':
			m_NavigationMatrix *= OVR::Matrix4f::RotationX(angle_radian);
			break;
		case 'y':
			m_NavigationMatrix *= OVR::Matrix4f::RotationY(angle_radian);
			break;
		case 'z':
			m_NavigationMatrix *= OVR::Matrix4f::RotationZ(angle_radian);
			break;
		default:
			break;
	}
}

void Oculus::WorldScale(float x, float y, float z)
{
	m_NavigationMatrix *= OVR::Matrix4f::Scaling(1.0f / x, 1.0f / y, 1.0f / z);
}

OVR::Matrix4f Oculus::GetNavigationMatrix()
{
	return m_NavigationMatrix;
}

void Oculus::LoadNavigationMatrix(OVR::Matrix4f matrix)
{
	m_NavigationMatrix = matrix;
}

void Oculus::SetNavigationMatrix()
{
//	glMatrixMode(GL_MODELVIEW_MATRIX);
	glMatrixMode(GL_MODELVIEW);
	glMultMatrixf(&(m_NavigationMatrix.Transposed().M[0][0]));
}

void Oculus::SetNavigationInverseMatrix()
{
//	glMatrixMode(GL_MODELVIEW_MATRIX);
	glMatrixMode(GL_MODELVIEW);
	glMultMatrixf(&((m_NavigationMatrix.Transposed()).Inverted().M[0][0]));
}

void Oculus::SetNavigationMatrixIdentity()
{
//	glMatrixMode(GL_MODELVIEW_MATRIX);
	m_NavigationMatrix.SetIdentity();
}

void Oculus::MultiNavigationMatrix(float matrix[4][4])
{
	OVR::Matrix4f mat4(
		matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
		matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
		matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2],
		matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3]);
	OVR::Matrix4f currentMatrix = m_NavigationMatrix;
	m_NavigationMatrix = mat4 * currentMatrix;
}

void Oculus::PreMultiNavigationMatrix(float matrix[4][4])
{
	OVR::Matrix4f mat4 = OVR::Matrix4f(
		matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
		matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
		matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2],
		matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3]);
	m_NavigationMatrix *= mat4;
}

#if (OVR_PRODUCT_VERSION == 1)
void Oculus::SwitchControllerType()
{
	m_CurrentControllerType++;
	m_CurrentControllerType %= static_cast<int>(ENUM_CONTROLLER_TYPE_SIZE);
	if ((m_CurrentControllerType == XBOX_CONTROLLER) && (m_IsConnected[XBOX_CONTROLLER] == false))
	{
		m_CurrentControllerType = OCULUS_TOUCH_RIGHT;
	}
	if ((m_CurrentControllerType == OCULUS_TOUCH_RIGHT) && (m_IsConnected[OCULUS_TOUCH_RIGHT] == false))
	{
		m_CurrentControllerType = MOUSE;
	}
	std::cout << "Switch ControllerType to: ";
	switch (m_CurrentControllerType)
	{
		case MOUSE:
			std::cout << "MOUSE\n";
			break;
		case XBOX_CONTROLLER:
			std::cout << "XBOX CONTROLLER\n";
			break;
		case OCULUS_TOUCH_RIGHT:
			std::cout << "OCULUS TOUCH RIGHT\n";
			break;
		default:
			break;
	}
}
#endif

void Oculus::GetSnap()
{
	char filename[256];
	sprintf(filename, "snapshot%04d.ppm", m_SnapNo);
	m_SnapNo++;
	uchar* buf = (uchar*)malloc(sizeof(uchar) * 1920 * 1080 * 3);
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, 1920, 1080, GL_RGB, GL_UNSIGNED_BYTE, buf);

	FILE *fp = fopen(filename, "wb");
	fprintf(fp, "P6\n1920 1080\n255\n");
	fwrite(buf, sizeof(uchar), 1920 * 1080 * 3, fp);
	fclose(fp);
	free(buf);
}

bool Oculus::GetKey(int key)
{
	Oculus* instance = reinterpret_cast<Oculus*>(glfwGetWindowUserPointer(m_Window));
	bool result = false;
	if (instance != nullptr)
	{
		result = glfwGetKey(m_Window, key);
	}
	return result;
}

int Oculus::GetMouseButton(int button)
{
	Oculus* instance = reinterpret_cast<Oculus*>(glfwGetWindowUserPointer(m_Window));
	int result = GLFW_RELEASE;
	if (instance != nullptr)
	{
		result = glfwGetMouseButton(m_Window, button);
	}
	return result;
}

void Oculus::StartThread()
{
	DWORD m_MainThreadID = GetCurrentThreadId();

	m_HMutex = CreateMutex(NULL, FALSE, NULL);
	m_HRender = (HANDLE)_beginthreadex(0, 0, MainThreadLauncherEX, reinterpret_cast<void*>(this), 0, 0);

	while(!m_IsInitializedGLFW.load()); // waiting the initialization of GLFW
}

void Oculus::StopThread()
{
	CloseHandle(m_HRender);
	CloseHandle(m_HMutex);
}

void Oculus::MainThreadEX()
{
	DWORD m_DisplayThreadID = GetCurrentThreadId();

	Init();
	InitGL();
	CreateBuffers();

	m_IsInitializedGLFW.store(true);

	int frameCounter = 0;
	double t, t0;
	t0 = glfwGetTime();
	while (m_IsThreadRunning)
	{
		ExecInitCallback();
		UpdateTrackingData();
		ExecIdleCallback();
		PreProcess();
		for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
		{
			SetMatrix(eyeIndex);
			glPushMatrix();
			glScalef(FEET_PER_METER, FEET_PER_METER, FEET_PER_METER);
			ExecDrawCallback();
			glPopMatrix();
		}
		PostProcess();

		t = glfwGetTime();
		if ((t - t0) > 1.0 || frameCounter == 0)
		{
			*m_FPS = (float)((double)(frameCounter) / (t - t0));
			t0 = t;
			frameCounter = 0;
		}
		frameCounter++;
	}

	ExecStopCallback();
	Terminate();
}

unsigned __stdcall Oculus::MainThreadLauncherEX(void *obj)
{
	reinterpret_cast<Oculus*>(obj)->MainThreadEX();
	_endthreadex(0);
	return 0;
}

bool Oculus::IsMainThread()
{
	if (GetCurrentThreadId() == m_MainThreadID)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Oculus::IsDisplayThread()
{
	if (GetCurrentThreadId() == m_DisplayThreadID)
	{
		return true;
	}
	else
	{
		return false;
	}
}

#if ((OVR_PRODUCT_VERSION == 0) && (OVR_MAJOR_VERSION == 5)) // Oculus SDK 0.5.0.1 only
GLFWmonitor* Oculus::CheckOVR()
{
	// another way to check mode whether "direct" or "extended desktop":
	// bool isDirectMode = (hmd->HmdCaps & ovrHmdCap_ExtendDesktop) ? false : true;

	int count;
	GLFWmonitor** monitors = glfwGetMonitors(&count);
	for (int i = 0; i < count; i++)
	{
		if (strcmp(glfwGetWin32Monitor(monitors[i]),
			m_HmdSession->DisplayDeviceName) == 0)
		{
			if (i != 0)
			{
				const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
				if ((m_HmdSession->Resolution.w == mode->height) &&
					(m_HmdSession->Resolution.h == mode->width))
				{
					m_WindowStyle = EXTEND_MODE_PORTRAIT;
					m_MirrorWindowWidth = m_HmdSession->Resolution.h;
					m_MirrorWindowHeight = m_HmdSession->Resolution.w;
				}
				else
				{
					if (m_IsHMDDebug)
					{
						m_WindowStyle = DIRECT_MODE;
					}
					else
					{
						m_WindowStyle = EXTEND_MODE;
						m_MirrorWindowWidth = m_HmdSession->Resolution.w / 2;
						m_MirrorWindowHeight = m_HmdSession->Resolution.h / 2;
					}
				}
			}
			else
			{
				m_WindowStyle = DIRECT_MODE;
			}

			return monitors[i];
		}
	}
	return NULL;
}
#endif
