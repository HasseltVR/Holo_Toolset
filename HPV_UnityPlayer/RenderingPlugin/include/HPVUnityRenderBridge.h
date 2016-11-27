/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
#pragma once

#include <string>
#include <map>
#include <stdint.h>
#include <queue>

#include "RenderingPlugin.h"
#include "Unity/IUnityGraphics.h"
#include "Log.h"
#include "HPVManager.h"

/* D3D11 */
#if SUPPORT_D3D11
#	include <d3d11.h>
#	include "Unity/IUnityGraphicsD3D11.h"
#endif

/* OpenGL */
#if SUPPORT_OPENGL_LEGACY
#if UNITY_WIN || UNITY_LINUX
#include "GL/glew.h"
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif

#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#else
#include <OpenGL/gl.h>
#endif
#endif
#if SUPPORT_OPENGL_UNIFIED
#	if UNITY_IPHONE
#		include <OpenGLES/ES2/gl.h>
#	elif UNITY_ANDROID
#		include <GLES2/gl2.h>
#	else
#		include "GL/glew.h"
#	endif
#endif

#ifdef SUPPORT_OPENGLES
static int   g_TexWidth = 0;
static int   g_TexHeight = 0;
#endif

void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

namespace HPV {

	/*
	* HPVRenderType enum specifies a render backend for the engine
	*/
	enum class HPVRendererType : std::uint8_t
	{
		RENDERER_NONE,
		RENDERER_DIRECT3D11,
		RENDERER_OPENGLCORE,
	};

	/*
	* D3D11TexData: the necessary data to use Direct3D 11 resources
	*/
	struct D3D11TexData
	{
		ID3D11Texture2D* tex = NULL;
		ID3D11ShaderResourceView * tex_view = NULL;
		UINT tex_update_height = 0;
		UINT buffer_stride = 0;
		UINT runtime_stride = 0;
	};

	/*
	* OpenGLTexData: the necessary data to use OpenGL resources
	*/
	struct OpenGLTexData
	{
		/* OpenGL texture handle */
		GLuint tex = 0;

		/* OpenGL Pixel Buffer Object handles (double-buffered) */
		GLuint pboIds[2] = { 0 };

		/* The gl pixel format for this file */
		GLenum gl_format;

		/* The current fill index (in case of using PBO) */
		uint8_t tex_fill_index = 0;
	};

	/*
	* OpenGLTexData: the necessary data to use OpenGL resources
	*/
	struct HPVRenderStats
	{
		uint64_t before_upload = 0;
		uint64_t after_upload = 0;
	};

	/*
	* 	HPVRenderData: allocted for each player, containing pointers 
	*	to all GPU resources
	*/
	struct HPVRenderData
	{
		/* Pointer to the player */
		HPVPlayerRef player;

		/* OpenGL */
		OpenGLTexData opengl;

		/* Direct3D */
		D3D11TexData d3d;

		/* Stats */
		HPVRenderStats stats;

		bool gpu_resources_need_init;

		HPVRenderData() : gpu_resources_need_init(true) {}
	};

	/*
	* 	HPVRenderBridge: providing a bridge between the HPV Player CPU backend 
	*	and GPU pixels sink
	*/
	class HPVRenderBridge
	{
	public:
		HPVRenderBridge();
		~HPVRenderBridge();

		void load(IUnityInterfaces* unityInterfaces);
		void unload();

		int initPlayer(uint8_t node_id);
		void setRenderer(HPVRendererType renderer);
		HPVRendererType getRenderer();

		void scheduleCreateGPUResources(uint8_t node_id);

		int HPVRenderBridge::createGPUResources(uint8_t node_id);
		int deleteGPUResources();
		int nodeHasResources(uint8_t node_id);
		intptr_t getTexturePtr(uint8_t node_id);

		void updateTextures();
		
		IUnityInterfaces* s_UnityInterfaces;
		IUnityGraphics* s_Graphics;
		ID3D11Device* g_D3D11Device = NULL;

		bool s3tc_supported = false;
		bool pbo_supported = false;

	private:
		HPVRendererType m_Renderer;
		std::map<uint8_t, HPVRenderData> m_RenderData;
		std::queue<uint8_t> m_scheduled_inits;

		bool bShouldUpload = false;
	};

	HPVRenderBridge * RendererSingleton();
};

/* The HPV Renderer Singleton */
extern HPV::HPVRenderBridge m_HPVRenderer;