////////////////////////////////////////////////////////////////////////////////
//
// zedmini.cpp
//
//   CLCL: CAVELib Compatible Library
//
//     Copyright 2015-2019 Shintaro Kawahara(kawahara@jamstec.go.jp).
//     All rights reserved.
//
//   This file is based on the following code,
//    https://github.com/stereolabs/zed-oculus/blob/master/src/main.cpp
//     Copyright (c) 2018, STEREOLABS.
//
////////////////////////////////////////////////////////////////////////////////

#include "zedmini.h"

#ifdef USE_ZEDMINI

#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <Extras/OVR_Math.h>

#include <glm/glm.hpp>
#include <glm/vec4.hpp>

const GLchar* OVR_ZED_VS =
"#version 330 core\n \
			layout(location=0) in vec3 in_vertex;\n \
			layout(location=1) in vec2 in_texCoord;\n \
			uniform uint isLeft;\n \
			out vec2 b_coordTexture;\n \
			void main()\n \
			{\n \
				if (isLeft == 1U)\n \
				{\n \
					b_coordTexture = in_texCoord;\n \
					gl_Position = vec4(in_vertex.x, in_vertex.y, in_vertex.z, 1);\n \
				}\n \
				else\n \
				{\n \
					b_coordTexture = vec2(1.0 - in_texCoord.x, in_texCoord.y);\n \
					gl_Position = vec4(-in_vertex.x, in_vertex.y, in_vertex.z, 1);\n \
				}\n \
			}";

const GLchar* OVR_ZED_FS =
"#version 330 core\n \
			uniform sampler2D u_textureZED;\n \
			in vec2 b_coordTexture;\n \
			out vec4 out_color;\n \
			void main()\n \
			{\n \
				out_color = vec4(texture(u_textureZED, b_coordTexture).bgr, 1);\n \
				gl_FragDepth = 1.0;\n \
			}";

const GLchar* OVR_ZED_VS_FOR_DEPTH =
"#version 330 core\n \
			layout(location=0) in vec3 in_vertex;\n \
			layout(location=1) in vec2 in_texCoord;\n \
			uniform uint isLeft;\n \
			out vec2 b_coordTexture;\n \
			void main()\n \
			{\n \
				if (isLeft == 1U)\n \
				{\n \
					b_coordTexture = in_texCoord;\n \
					gl_Position = vec4(in_vertex.x, in_vertex.y, in_vertex.z, 1);\n \
				}\n \
				else\n \
				{\n \
					b_coordTexture = vec2(1.0 - in_texCoord.x, in_texCoord.y);\n \
					gl_Position = vec4(-in_vertex.x, in_vertex.y, in_vertex.z, 1);\n \
				}\n \
			}";

const GLchar* OVR_ZED_FS_FOR_DEPTH =
"#version 330 core\n \
			uniform sampler2D u_textureZED;\n \
			in vec2 b_coordTexture;\n \
			out vec4 out_color;\n \
			void main()\n \
			{\n \
				float n = -gl_DepthRange.near;\n \
				float f = -gl_DepthRange.far;\n \
				float z = texture(u_textureZED, b_coordTexture).r + gl_FragCoord.z / gl_FragCoord.w;\n \
				float z_ndc = z * -2.0 / (f - n) - (f + n) / (f - n);\n \
				float depth = (z_ndc + 1.0) / 2.0;\n \
				out_color = vec4(1, 1, 1, 1);\n \
				gl_FragDepth = depth;\n \
			}";

ZedMini::ZedMini()
{
	m_Width = m_Height = 0;
	m_IsOpen = m_CameraState = false;
}

ZedMini::~ZedMini()
{
}

bool ZedMini::Init()
{
	glClearDepth(0.0); // for ZEDMini

#if (ZED_SDK_MAJOR_VERSION == 2)
	sl::InitParameters initParameters;
	initParameters.camera_resolution = sl::RESOLUTION_HD720;
//	initParameters.camera_resolution = sl::RESOLUTION_HD1080;
//	initParameters.camera_resolution = sl::RESOLUTION_HD2K;
//	initParameters.depth_mode = sl::DEPTH_MODE_NONE;
//	initParameters.depth_mode = sl::DEPTH_MODE_PERFORMANCE;
//	initParameters.depth_mode = sl::DEPTH_MODE_QUALITY;
	initParameters.depth_mode = sl::DEPTH_MODE_ULTRA;
	initParameters.coordinate_system = sl::COORDINATE_SYSTEM_RIGHT_HANDED_Y_UP; // OpenGL's coordinate system is right_handed
//	initParameters.coordinate_units = sl::UNIT_MILLIMETER;
//	initParameters.coordinate_units = sl::UNIT_CENTIMETER;
	initParameters.coordinate_units = sl::UNIT_METER;
#else
	sl::InitParameters initParameters;
	initParameters.camera_resolution = sl::RESOLUTION::HD720;
//	initParameters.camera_resolution = sl::RESOLUTION::HD1080;
//	initParameters.camera_resolution = sl::RESOLUTION::HD2K;
//	initParameters.depth_mode = sl::DEPTH_MODE::NONE;
//	initParameters.depth_mode = sl::DEPTH_MODE::PERFORMANCE;
//	initParameters.depth_mode = sl::DEPTH_MODE::QUALITY;
	initParameters.depth_mode = sl::DEPTH_MODE::ULTRA;
	initParameters.coordinate_system = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP; // OpenGL's coordinate system is right_handed
//	initParameters.coordinate_units = sl::UNIT::MILLIMETER;
//	initParameters.coordinate_units = sl::UNIT::CENTIMETER;
	initParameters.coordinate_units = sl::UNIT::METER;
	initParameters.depth_maximum_distance = 500.0f;
#endif
	initParameters.depth_minimum_distance = 0.100000f;
	initParameters.enable_right_side_measure = true;

	// Open the camera
	sl::ERROR_CODE err = m_Camera.open(initParameters);
#if (ZED_SDK_MAJOR_VERSION == 2)
	if (err != sl::SUCCESS)
#else
	if (err != sl::ERROR_CODE::SUCCESS)
#endif
	{
		std::cout << toString(err) << std::endl;
		m_Camera.close();
		return false; // Quit if an error occurred
	}

#if (ZED_SDK_MAJOR_VERSION == 2)
	m_Width  = (int)m_Camera.getResolution().width;
	m_Height = (int)m_Camera.getResolution().height;
#elif (ZED_SDK_MAJOR_VERSION == 3)
#if (ZED_SDK_MINOR_VERSION < 2)
	m_Width  = (int)m_Camera.getCameraInformation().camera_resolution.width;
	m_Height = (int)m_Camera.getCameraInformation().camera_resolution.height;
#else
	m_Width  = (int)m_Camera.getCameraInformation().camera_configuration.resolution.width;
	m_Height = (int)m_Camera.getCameraInformation().camera_configuration.resolution.height;
#endif // ZED_SDK_MINOR_VERSION
#else
	m_Width  = (int)(sl::getResolution(initParameters.camera_resolution).width);
	m_Height = (int)(sl::getResolution(initParameters.camera_resolution).height);
#endif
	m_Format = GL_BGRA;
	std::cout << "Width  : " << m_Width  << std::endl;
	std::cout << "Height : " << m_Height << std::endl;
	m_IsOpen = true;

#if (ZED_SDK_MAJOR_VERSION == 2)
	m_Camera.setDepthMaxRangeValue(500.0);
	std::cout << "\n[Init]" << std::endl;
	std::cout << "Resolution=" << sl::toString(initParameters.camera_resolution).c_str() << std::endl;
	std::cout << "FPS=" << m_Camera.getCameraFPS() << std::endl;
#else
//	m_Camera.setDepthMaxRangeValue(500.0); // move to initialization of camera
	std::cout << "\n[Init]" << std::endl;
	std::cout << "Resolution=" << sl::toString(initParameters.camera_resolution).c_str() << std::endl;
	std::cout << "FPS=" << m_Camera.getCameraInformation().camera_configuration.fps << std::endl;
#endif

#ifdef DEBUG
	std::cout << "bUseSVO=" << std::endl;
	if (initParameters.svo_real_time_mode)
	{
		std::cout << "bRealTime=True" << std::endl;
	}
	else
	{
		std::cout << "bRealTime=False" << std::endl;
	}
	std::cout << "DepthMode=" << (int)initParameters.depth_mode << std::endl;
	std::cout << "GPUID=" << initParameters.sdk_gpu_id << std::endl;
	std::cout << "DepthMinimumDistance=" << m_Camera.getDepthMinRangeValue() << std::endl;
	std::cout << "DepthMaximumDistance=" << m_Camera.getDepthMaxRangeValue() << std::endl;
	if (initParameters.sdk_verbose)
	{
		std::cout << "bVerbose=True" << std::endl;
	}
	else
	{
		std::cout << "bVerbose=False" << std::endl;
	}
	if (initParameters.camera_disable_self_calib)
	{
		std::cout << "bDisableSelfCalibration=True" << std::endl;
	}
	else
	{
		std::cout << "bDisableSelfCalibration=False" << std::endl;
	}
	if (initParameters.camera_image_flip)
	{
		std::cout << "bVerticalFlipImage=True" << std::endl;
	}
	else
	{
		std::cout << "bVerticalFlipImage=False" << std::endl;
	}
	std::cout << "SVOFilePath=" << std::endl;
	std::cout << "VerboseFilePath=" << std::endl;
	if (initParameters.depth_stabilization)
	{
		std::cout << "bEnableDepthStabilization=True" << std::endl;
	}
	else
	{
		std::cout << "bEnableDepthStabilization=False" << std::endl;
	}
	if (initParameters.enable_right_side_measure)
	{
		std::cout << "bEnableRightSideMeasure=True" << std::endl;
	}
	else
	{
		std::cout << "bEnableRightSideMeasure=False" << std::endl;
	}
/*
	// from ZED_SDK 3.0 : CAMERA_SETTINGS is renamed to VIDEO_SETTINGS
	m_Camera.setCameraSettings(sl::CAMERA_SETTINGS_AUTO_WHITEBALANCE, 0);
	m_Camera.setCameraSettings(sl::CAMERA_SETTINGS_WHITEBALANCE, 4700, false);
	m_Camera.setCameraSettings(sl::CAMERA_SETTINGS_GAIN, 56, false);
	m_Camera.setCameraSettings(sl::CAMERA_SETTINGS_EXPOSURE, 100, false);
*/
	std::cout << "\n[Camera]" << std::endl;
	std::cout << "Brightness=" << m_Camera.getCameraSettings(sl::CAMERA_SETTINGS_BRIGHTNESS) << std::endl;
	std::cout << "Contrast = " << m_Camera.getCameraSettings(sl::CAMERA_SETTINGS_CONTRAST) << std::endl;
	std::cout << "Hue = " << m_Camera.getCameraSettings(sl::CAMERA_SETTINGS_HUE) << std::endl;
	std::cout << "Saturation = " << m_Camera.getCameraSettings(sl::CAMERA_SETTINGS_SATURATION) << std::endl;
	std::cout << "WhiteBalance = " << m_Camera.getCameraSettings(sl::CAMERA_SETTINGS_WHITEBALANCE) << std::endl;
	std::cout << "Gain = " << m_Camera.getCameraSettings(sl::CAMERA_SETTINGS_GAIN) << std::endl;
	std::cout << "Exposure = " << m_Camera.getCameraSettings(sl::CAMERA_SETTINGS_EXPOSURE) << std::endl;
	std::cout << "bAutoWhiteBalance = " << std::endl;
	std::cout << "bAutoGainAndExposure = " << std::endl;
	std::cout << "bDefault = " << std::endl;
#endif // DEBUG

	sl::uchar4 dark_bckgrd(44, 44, 44, 255);
	glGenTextures(2, m_TexID);
	for (int eye = 0; eye < 2; eye++)
	{
		// Generate OpenGL texture for RGB Image
		glBindTexture(GL_TEXTURE_2D, m_TexID[eye]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// Set size and default value to texture
#if (ZED_SDK_MAJOR_VERSION == 2)
		m_Image[eye].alloc(m_Width, m_Height, sl::MAT_TYPE_8U_C4, sl::MEM_GPU);
		m_Image[eye].setTo(dark_bckgrd, sl::MEM_GPU);
#else
		m_Image[eye].alloc(m_Width, m_Height, sl::MAT_TYPE::U8_C4, sl::MEM::GPU);
		m_Image[eye].setTo(dark_bckgrd, sl::MEM::GPU);
#endif
	}

	glGenTextures(2, m_DepthID);
	for (int eye = 0; eye < 2; eye++)
	{
		// Generate OpenGL texture for depthmap
		glBindTexture(GL_TEXTURE_2D, m_DepthID[eye]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_Width, m_Height, 0, GL_RED, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// Set size and default value to texture
#if (ZED_SDK_MAJOR_VERSION == 2)
		m_Depth[eye].alloc(m_Width, m_Height, sl::MAT_TYPE_32F_C1, sl::MEM_GPU);
#else
		m_Depth[eye].alloc(m_Width, m_Height, sl::MAT_TYPE::F32_C1, sl::MEM::GPU);
#endif
	}

	// Register texture
	cudaError_t err1 = cudaGraphicsGLRegisterImage(&cimg_l, m_TexID[ovrEye_Left],  GL_TEXTURE_2D, cudaGraphicsRegisterFlagsWriteDiscard);
	cudaError_t err2 = cudaGraphicsGLRegisterImage(&cimg_r, m_TexID[ovrEye_Right], GL_TEXTURE_2D, cudaGraphicsRegisterFlagsWriteDiscard);
	cudaError_t err3 = cudaGraphicsGLRegisterImage(&cimg_ld, m_DepthID[ovrEye_Left], GL_TEXTURE_2D, cudaGraphicsRegisterFlagsWriteDiscard);
	cudaError_t err4 = cudaGraphicsGLRegisterImage(&cimg_rd, m_DepthID[ovrEye_Right], GL_TEXTURE_2D, cudaGraphicsRegisterFlagsWriteDiscard);
	if (err1 != cudaSuccess || err2 != cudaSuccess)
		std::cout << "ERROR: cannot create CUDA texture : " << err1 << std::endl;
	if (err3 != cudaSuccess)
		std::cout << "ERROR: cannot create CUDA texture : " << cudaGetErrorString(err3) << std::endl;
	if (err4 != cudaSuccess)
		std::cout << "ERROR: cannot create CUDA texture : " << cudaGetErrorString(err4) << std::endl;

	cuCtxSetCurrent(m_Camera.getCUDAContext());
	p_Shader = new Shader(OVR_ZED_VS, OVR_ZED_FS);
	p_ShaderDepth = new Shader(OVR_ZED_VS_FOR_DEPTH, OVR_ZED_FS_FOR_DEPTH);
	cudaGraphicsMapResources(1, &cimg_l, 0);
	cudaGraphicsMapResources(1, &cimg_r, 0);
	cudaGraphicsMapResources(1, &cimg_ld, 0);
	cudaGraphicsMapResources(1, &cimg_rd, 0);

	return true;
}

void ZedMini::Terminate()
{
	if (m_IsOpen)
	{
		cudaGraphicsUnmapResources(1, &cimg_l);
		cudaGraphicsUnmapResources(1, &cimg_r);
		cudaGraphicsUnmapResources(1, &cimg_ld);
		cudaGraphicsUnmapResources(1, &cimg_rd);

		for (int eye = 0; eye < 2; eye++)
		{
			m_Image[eye].free();
			m_Depth[eye].free();
		}
		m_Camera.close();

		delete p_Shader;
		delete p_ShaderDepth;
	}
}

void ZedMini::PreStore()
{
	if (m_IsOpen && m_CameraState)
	{
		sl::RuntimeParameters runtime_parameters;
#if (ZED_SDK_MAJOR_VERSION == 2)
//		runtime_parameters.sensing_mode = sl::SENSING_MODE_STANDARD; // 0
		runtime_parameters.sensing_mode = sl::SENSING_MODE_FILL;     // 1
		runtime_parameters.enable_depth = true;
		runtime_parameters.enable_point_cloud = false;
#elif (ZED_SDK_MAJOR_VERSION == 3)
//		runtime_parameters.sensing_mode = sl::SENSING_MODE::STANDARD; // 0
		runtime_parameters.sensing_mode = sl::SENSING_MODE::FILL;     // 1
		runtime_parameters.enable_depth = true;
#else
		runtime_parameters.enable_fill_mode = false;
		runtime_parameters.enable_fill_mode = true;
		runtime_parameters.enable_depth = true;
#endif

		// Set the ZED's CUDA context to this separate CPU thread
		cuCtxSetCurrent(m_Camera.getCUDAContext());
#if (ZED_SDK_MAJOR_VERSION == 2)
		if (m_Camera.grab(runtime_parameters) == sl::SUCCESS)
		{
			// copy both left and right images
			m_Camera.retrieveImage(m_Image[0], sl::VIEW_LEFT,  sl::MEM_GPU);
			m_Camera.retrieveImage(m_Image[1], sl::VIEW_RIGHT, sl::MEM_GPU);
			m_Camera.retrieveMeasure(m_Depth[0], sl::MEASURE_DEPTH, sl::MEM_GPU);
			m_Camera.retrieveMeasure(m_Depth[1], sl::MEASURE_DEPTH_RIGHT, sl::MEM_GPU);
		}
#else
		if (m_Camera.grab(runtime_parameters) == sl::ERROR_CODE::SUCCESS)
		{
			// copy both left and right images
			m_Camera.retrieveImage(m_Image[0], sl::VIEW::LEFT, sl::MEM::GPU);
			m_Camera.retrieveImage(m_Image[1], sl::VIEW::RIGHT, sl::MEM::GPU);
			m_Camera.retrieveMeasure(m_Depth[0], sl::MEASURE::DEPTH, sl::MEM::GPU);
			m_Camera.retrieveMeasure(m_Depth[1], sl::MEASURE::DEPTH_RIGHT, sl::MEM::GPU);
		}
#endif
		else
		{
			sl::sleep_ms(2);
		}
		glClearDepth(0.0f);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
	}
	else
	{
		glClearDepth(1.0f);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
	}
}

void ZedMini::DrawImage(int eyeIndex)
{
	if (m_IsOpen && m_CameraState)
	{
		glUseProgram(0);
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		DrawRGBImage(eyeIndex);
		DrawDepth(eyeIndex);
		glPopAttrib();
		glUseProgram(0);
	}
}

void ZedMini::DrawRGBImage(int eyeIndex)
{
	if (m_IsOpen && m_CameraState)
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);
		glDepthMask(GL_TRUE);

		// Enable the shader
		glUseProgram(p_Shader->getProgramId());

		// Bind the Vertex Buffer Objects of the rectangle that displays ZED images
		// vertices
		glEnableVertexAttribArray(Shader::ATTRIB_VERTICES_POS);
		glBindBuffer(GL_ARRAY_BUFFER, m_RectVBO[eyeIndex][0]);
		glVertexAttribPointer(Shader::ATTRIB_VERTICES_POS, 3, GL_FLOAT, GL_FALSE, 0, 0);
		// indices
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RectVBO[eyeIndex][2]);
		// texture coordinates
		glEnableVertexAttribArray(Shader::ATTRIB_TEXTURE2D_POS);
		glBindBuffer(GL_ARRAY_BUFFER, m_RectVBO[eyeIndex][1]);
		glVertexAttribPointer(Shader::ATTRIB_TEXTURE2D_POS, 2, GL_FLOAT, GL_FALSE, 0, 0);

		cudaArray_t arrIm;

		unsigned char* imagePtr = nullptr;
		if (eyeIndex == 0)
		{
			cudaGraphicsSubResourceGetMappedArray(&arrIm, cimg_l, 0, 0);
			cudaMemcpy2DToArray(arrIm, 0, 0,
#if (ZED_SDK_MAJOR_VERSION == 2)
				m_Image[ovrEye_Left].getPtr<sl::uchar1>(sl::MEM_GPU),
				m_Image[ovrEye_Left].getStepBytes(sl::MEM_GPU),
#else
				m_Image[ovrEye_Left].getPtr<sl::uchar1>(sl::MEM::GPU),
				m_Image[ovrEye_Left].getStepBytes(sl::MEM::GPU),
#endif
				m_Image[ovrEye_Left].getWidth() * 4,
				m_Image[ovrEye_Left].getHeight(), cudaMemcpyDeviceToDevice);
			glBindTexture(GL_TEXTURE_2D, m_TexID[ovrEye_Left]);
			glUniform1ui(glGetUniformLocation(p_Shader->getProgramId(), "isLeft"), 1U);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		else
		{
			cudaGraphicsSubResourceGetMappedArray(&arrIm, cimg_r, 0, 0);
			cudaMemcpy2DToArray(arrIm, 0, 0,
#if (ZED_SDK_MAJOR_VERSION == 2)
				m_Image[ovrEye_Right].getPtr<sl::uchar1>(sl::MEM_GPU),
				m_Image[ovrEye_Right].getStepBytes(sl::MEM_GPU),
#else
				m_Image[ovrEye_Right].getPtr<sl::uchar1>(sl::MEM::GPU),
				m_Image[ovrEye_Right].getStepBytes(sl::MEM::GPU),
#endif
				m_Image[ovrEye_Right].getWidth() * 4,
				m_Image[ovrEye_Right].getHeight(), cudaMemcpyDeviceToDevice);
			glBindTexture(GL_TEXTURE_2D, m_TexID[ovrEye_Right]);
			glUniform1ui(glGetUniformLocation(p_Shader->getProgramId(), "isLeft"), 1U);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);

		glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_TEST);
	}
}

void ZedMini::DrawDepth(int eyeIndex)
{
	if (m_IsOpen && m_CameraState)
	{
#if (ZED_SDK_MAJOR_VERSION == 2)
		sl::CalibrationParameters calib_params = m_Camera.getCameraInformation().calibration_parameters;
		const float z_near = m_Camera.getDepthMinRangeValue();
		const float z_far  = m_Camera.getDepthMaxRangeValue();
#elif (ZED_SDK_MAJOR_VERSION == 3)
#if (ZED_SDK_MINOR_VERSION < 2)
		sl::CalibrationParameters calib_params = m_Camera.getCameraInformation().calibration_parameters;
		const float z_near = m_Camera.getInitParameters().depth_minimum_distance;
		const float z_far  = m_Camera.getInitParameters().depth_maximum_distance;
#else
		sl::CalibrationParameters calib_params = m_Camera.getCameraInformation().camera_configuration.calibration_parameters;
		const float z_near = m_Camera.getInitParameters().depth_minimum_distance;
		const float z_far = m_Camera.getInitParameters().depth_maximum_distance;
#endif // ZED_SDK_MINOR_VERSION
#else
		sl::CalibrationParameters calib_params = m_Camera.getCameraInformation().camera_configuration.calibration_parameters;
		const float z_near = m_Camera.getInitParameters().depth_minimum_distance;
		const float z_far  = m_Camera.getInitParameters().depth_maximum_distance;
		//ERROR_CODE getCameraSettingsRange(VIDEO_SETTINGS settings, int& min, int& max);
#endif
		const float fov_x  = glm::radians(calib_params.left_cam.h_fov);
		const float fov_y  = glm::radians(calib_params.left_cam.v_fov);
		const float width  = static_cast<float>(calib_params.left_cam.image_size.width);
		const float height = static_cast<float>(calib_params.left_cam.image_size.height);

		glm::mat4 proj(1);

		proj[0][0] = 1.0f / std::tan(fov_x * 0.5f);
		proj[1][1] = 1.0f / std::tan(fov_y * 0.5f);
		proj[2][0] = 2.0f * ((width - 1.0f * calib_params.left_cam.cx) / width) - 1.0f;
		proj[2][1] = -(2.0f * ((height - 1.0f * calib_params.left_cam.cy) / height) - 1.0f);
		proj[2][2] = 0.0f;
		proj[2][3] = -1.0f;
		proj[3][2] = z_near;
		proj[3][3] = 0.0f;

		// Enable the shader
		glUseProgram(p_ShaderDepth->getProgramId());

		// Bind the Vertex Buffer Objects of the rectangle that displays ZED images
		// vertices
		glEnableVertexAttribArray(Shader::ATTRIB_VERTICES_POS);
		glBindBuffer(GL_ARRAY_BUFFER, m_RectVBO[eyeIndex][0]);
		glVertexAttribPointer(Shader::ATTRIB_VERTICES_POS, 3, GL_FLOAT, GL_FALSE, 0, 0);
		// indices
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RectVBO[eyeIndex][2]);
		// texture coordinates
		glEnableVertexAttribArray(Shader::ATTRIB_TEXTURE2D_POS);
		glBindBuffer(GL_ARRAY_BUFFER, m_RectVBO[eyeIndex][1]);
		glVertexAttribPointer(Shader::ATTRIB_TEXTURE2D_POS, 2, GL_FLOAT, GL_FALSE, 0, 0);

		cudaArray_t arrIm;

		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		if (eyeIndex == 0)
		{
			cudaGraphicsSubResourceGetMappedArray(&arrIm, cimg_ld, 0, 0);
			cudaMemcpy2DToArray(arrIm, 0, 0,
#if (ZED_SDK_MAJOR_VERSION == 2)
				m_Depth[ovrEye_Left].getPtr<sl::uchar1>(sl::MEM_GPU),
				m_Depth[ovrEye_Left].getStepBytes(sl::MEM_GPU),
#else
				m_Depth[ovrEye_Left].getPtr<sl::uchar1>(sl::MEM::GPU),
				m_Depth[ovrEye_Left].getStepBytes(sl::MEM::GPU),
#endif
				m_Depth[ovrEye_Left].getWidth() * 4,
				m_Depth[ovrEye_Left].getHeight(), cudaMemcpyDeviceToDevice);
			glBindTexture(GL_TEXTURE_2D, m_DepthID[ovrEye_Left]);
			glUniform1ui(glGetUniformLocation(p_ShaderDepth->getProgramId(), "isLeft"), 1U);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		else
		{
			cudaGraphicsSubResourceGetMappedArray(&arrIm, cimg_rd, 0, 0);
			cudaMemcpy2DToArray(arrIm, 0, 0,
#if (ZED_SDK_MAJOR_VERSION == 2)
				m_Depth[ovrEye_Right].getPtr<sl::uchar1>(sl::MEM_GPU),
				m_Depth[ovrEye_Right].getStepBytes(sl::MEM_GPU),
#else
				m_Depth[ovrEye_Right].getPtr<sl::uchar1>(sl::MEM::GPU),
				m_Depth[ovrEye_Right].getStepBytes(sl::MEM::GPU),
#endif
				m_Depth[ovrEye_Right].getWidth() * 4,
				m_Depth[ovrEye_Right].getHeight(), cudaMemcpyDeviceToDevice);
			glBindTexture(GL_TEXTURE_2D, m_DepthID[ovrEye_Right]);
			glUniform1ui(glGetUniformLocation(p_ShaderDepth->getProgramId(), "isLeft"), 1U);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);
	}
}

void ZedMini::DrawPointCloud(int eyeIndex)
{
	if (m_IsOpen && m_CameraState)
	{
		glEnable(GL_DEPTH_TEST);
		glUseProgram(0);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		sl::float4 point3D;
		glBegin(GL_POINTS);
		for (int j = 0; j < m_Height; j++)
		{
			for (int i = 0; i < m_Width; i++)
			{
				m_PointCloud[eyeIndex].getValue(i, j, &point3D);
				glVertex3f(point3D.x, point3D.y, point3D.z);
			}
		}
		glEnd();
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}
}

void ZedMini::SetScreenCoord(int bufferWidth, int bufferHeight, float offsetLensCenterX, float offsetLensCenterY)
{
	if (m_IsOpen)
	{
		// Compute the useful part of the ZED image
		unsigned int widthFinal = bufferWidth / 2;
		float heightGL = 1.0f;
		float widthGL  = 1.0f;
		if (m_Width > 0.0f)
		{
			unsigned int heightFinal = m_Height * widthFinal / (float)m_Width;
			// Convert this size to OpenGL viewport's frame's coordinates
			heightGL = (heightFinal) / (float)(bufferHeight);
			widthGL  = ((m_Width * (heightFinal / (float)m_Height)) / (float)widthFinal);
		}
		else
		{
			std::cout << "WARNING: ZED parameters got wrong values."
				"Default vertical and horizontal FOV are used.\n"
				"Check your calibration file or check if your ZED is not too close to a surface or an object."
				<< std::endl;
		}
		float up    = heightGL + offsetLensCenterY;
		float down  = heightGL - offsetLensCenterY;
		float right = widthGL  + offsetLensCenterX;
		float left  = widthGL  - offsetLensCenterX;
		for (int eyeIndex = 0; eyeIndex < 2; eyeIndex++)
		{
			float rectVertices0[12] = { -left, -up, 0, right, -up, 0, right, down, 0, -left, down, 0 };
			float rectVertices1[12] = { -right, -up, 0, left, -up, 0, left, down, 0, -right, down, 0 };
			glGenBuffers(1, &m_RectVBO[eyeIndex][0]);
			glBindBuffer(GL_ARRAY_BUFFER, m_RectVBO[eyeIndex][0]);
			if (eyeIndex == 0)
			{
				glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices0), rectVertices0, GL_STATIC_DRAW);
			}
			else
			{
				glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices0), rectVertices1, GL_STATIC_DRAW);
			}

			float rectTexCoord[8] = { 0, 1, 1, 1, 1, 0, 0, 0 };
			glGenBuffers(1, &m_RectVBO[eyeIndex][1]);
			glBindBuffer(GL_ARRAY_BUFFER, m_RectVBO[eyeIndex][1]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(rectTexCoord), rectTexCoord, GL_STATIC_DRAW);

			unsigned int rectIndices[6] = { 0, 1, 2, 0, 2, 3 };
			glGenBuffers(1, &m_RectVBO[eyeIndex][2]);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RectVBO[eyeIndex][2]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectIndices), rectIndices, GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}
}

#endif // USE_ZEDMINI
