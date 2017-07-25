#include "HPVUnityRenderBridge.h"

namespace HPV {

	/* The HPV Renderer Singleton */
	HPVRenderBridge m_HPVRenderer;

	// Static, not part of HPVRenderBridge class
	void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
	{
		switch (eventType)
		{
		case kUnityGfxDeviceEventInitialize:
		{
			HPV_VERBOSE("\nOnGraphicsDeviceEvent(Initialize)");
			if (RendererSingleton()->getRenderer() == HPVRendererType::RENDERER_DIRECT3D11)
			{
				IUnityGraphicsD3D11* d3d11 = HPV::RendererSingleton()->s_UnityInterfaces->Get<IUnityGraphicsD3D11>();
				HPV::RendererSingleton()->g_D3D11Device = d3d11->GetDevice();
				HPV_VERBOSE("D3D11 init was succesful");
			}
			else if (RendererSingleton()->getRenderer() == HPVRendererType::RENDERER_OPENGLCORE)
			{
				glewExperimental = GL_TRUE;
				GLenum err = glewInit();
				if (GLEW_OK != err)
				{
					HPV_VERBOSE("GLEW Error: %s\n", glewGetErrorString(err));
				}
				else
				{
					if (GLEW_EXT_texture_compression_s3tc && GLEW_ARB_texture_compression)
					{
						HPV_VERBOSE("OpenGL supporting S3TC texture compression");
						RendererSingleton()->s3tc_supported = true;
					}
					else
					{
						HPV_ERROR("S3TC Compression not supported by this system")
					}

					if (GLEW_EXT_pixel_buffer_object)
					{
						HPV_VERBOSE("OpenGL supporting Pixel Buffer Objects");
						RendererSingleton()->pbo_supported = true;
					}
					else
					{
						HPV_ERROR("PBO's are not supported by your system.")
					}
					HPV_VERBOSE("Using GLEW %s\nUsing OpenGL %s", glewGetString(GLEW_VERSION), glGetString(GL_VERSION));

					HPV_VERBOSE("OpenGL init was succesful");
				}
			}

			break;
		}

		case kUnityGfxDeviceEventShutdown:
		{;
		HPV::DestroyHPVEngine();
		RendererSingleton()->unload();
		HPV_VERBOSE("OnGraphicsDeviceEvent(Shutdown)");
		break;
		}

		case kUnityGfxDeviceEventBeforeReset:
		{
			HPV_VERBOSE("OnGraphicsDeviceEvent(BeforeReset)");
			break;
		}

		case kUnityGfxDeviceEventAfterReset:
		{
			HPV_VERBOSE("OnGraphicsDeviceEvent(AfterReset)");
			break;
		}
		};
	}

	static void ReportGLError()
	{
		GLenum err = GL_NO_ERROR;
		while ((err = glGetError()) != GL_NO_ERROR)
		{
			if (err == GL_INVALID_ENUM)
			{
				HPV_ERROR("GL_INVALID_ENUM");
			}
			else if (err == GL_INVALID_VALUE)
			{
				HPV_ERROR("GL_INVALID_VALUE");
			}
			else if (err == GL_INVALID_OPERATION)
			{
				HPV_ERROR("GL_INVALID_OPERATION");
			}
			else
			{
				HPV_ERROR("Other GL Error");
			}
		}
	}

	HPVRenderBridge::HPVRenderBridge() : m_Renderer(HPVRendererType::RENDERER_NONE)
	{
		s3tc_supported = false;
		pbo_supported = false;
		bShouldUpload = false;
	}

	HPVRenderBridge::~HPVRenderBridge()
	{
		HPV_VERBOSE("~RenderBridge");
	}

	void HPVRenderBridge::setRenderer(HPVRendererType renderer)
	{
		m_Renderer = renderer;
	}

	void HPVRenderBridge::scheduleCreateGPUResources(uint8_t node_id)
	{
		m_scheduled_inits.push(node_id);
	}

	void HPVRenderBridge::load(IUnityInterfaces* unityInterfaces)
	{
		s_UnityInterfaces = unityInterfaces;
		s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();

		UnityGfxRenderer s_DeviceType = s_Graphics->GetRenderer();

		if (s_DeviceType == kUnityGfxRendererD3D11)
		{
			setRenderer(HPVRendererType::RENDERER_DIRECT3D11);
		}
		else if (s_DeviceType == kUnityGfxRendererOpenGLCore || s_DeviceType == kUnityGfxRendererOpenGL)
		{
			setRenderer(HPVRendererType::RENDERER_OPENGLCORE);
		}
		else
		{
			setRenderer(HPVRendererType::RENDERER_NONE);
		}

		s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
		HPV::OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
	}

	void HPVRenderBridge::unload()
	{
		s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
		setRenderer(HPVRendererType::RENDERER_NONE);
	}

	int HPVRenderBridge::initPlayer(uint8_t node_id)
	{
		m_RenderData.insert(std::pair<uint8_t, HPVRenderData>(node_id, HPVRenderData()));
		return HPV_RET_ERROR_NONE;
	}

	int HPVRenderBridge::createGPUResources(uint8_t node_id)
	{
		if (m_RenderData.find(node_id) == m_RenderData.end())
		{
			HPV_ERROR("Can't create resources, player %d was not allocated!", node_id);
			return HPV_RET_ERROR;
		}

		HPVRenderData& data = m_RenderData.at(node_id);
		data.player = ManagerSingleton()->getPlayer(node_id);
		data.stats.after_upload = 0;
		data.stats.before_upload = 0;
		data.gpu_resources_need_init = true;

		if (HPVRendererType::RENDERER_DIRECT3D11 == m_Renderer)
		{
			HRESULT hr;
			DXGI_FORMAT format = (HPVCompressionType::HPV_TYPE_DXT1_NO_ALPHA == data.player->getCompressionType()) ? DXGI_FORMAT_BC1_UNORM : DXGI_FORMAT_BC3_UNORM;
			uint8_t row_pitch_factor = (HPVCompressionType::HPV_TYPE_DXT1_NO_ALPHA == data.player->getCompressionType()) ? 8 : 16;

			// Create texture
			D3D11_TEXTURE2D_DESC desc;
			desc.Width = data.player->getWidth();
			desc.Height = data.player->getHeight();
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = format;
			// no anti-aliasing
			desc.SampleDesc.Count = 1;
			//UINT q_levels = 0;
			//g_D3D11Device->CheckMultisampleQualityLevels(format, desc.SampleDesc.Count, &q_levels);
			//HPV_VERBOSE("MS Q levels: %u", q_levels);
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA init_data;
			init_data.pSysMem = data.player->getBufferPtr();
			init_data.SysMemPitch = row_pitch_factor * (data.player->getWidth() / 4);
			//data.SysMemSlicePitch = data.SysMemPitch * (player->getHeight() / 4);
			init_data.SysMemSlicePitch = 0;

			hr = g_D3D11Device->CreateTexture2D(&desc, &init_data, &data.d3d.tex);
			if (SUCCEEDED(hr) && data.d3d.tex != 0)
			{
				HPV_VERBOSE("Succesfully created D3D Texture.");

				D3D11_SAMPLER_DESC samplerDesc;
				samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
				samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.MipLODBias = 0.0f;
				samplerDesc.MaxAnisotropy = 1;
				samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
				samplerDesc.BorderColor[0] = 0;
				samplerDesc.BorderColor[1] = 0;
				samplerDesc.BorderColor[2] = 0;
				samplerDesc.BorderColor[3] = 0;
				samplerDesc.MinLOD = 0;
				samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
				g_D3D11Device->CreateSamplerState(&samplerDesc, &sampler);

				if (SUCCEEDED(hr))
				{
					HPV_VERBOSE("Created sampler state");
				}

				HPV_VERBOSE("Creating D3D SRV.");
				D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
				memset(&SRVDesc, 0, sizeof(SRVDesc));
				SRVDesc.Format = format;
				SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				SRVDesc.Texture2D.MipLevels = 1;

				hr = g_D3D11Device->CreateShaderResourceView(data.d3d.tex, &SRVDesc, &data.d3d.tex_view);
				if (FAILED(hr))
				{
					data.d3d.tex->Release();
				}
				HPV_VERBOSE("Succesfully created D3D SRV.");

				ID3D11DeviceContext* ctx = NULL;
				g_D3D11Device->GetImmediateContext(&ctx);

				// get mapped resource row pitch for later updating the texture
				D3D11_MAPPED_SUBRESOURCE mappedResource;
				ctx->Map(data.d3d.tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

				data.d3d.tex_update_height = data.player->getHeight() / 4;
				data.d3d.buffer_stride = static_cast<UINT>(data.player->getBytesPerFrame()) / data.d3d.tex_update_height;
				data.d3d.runtime_stride = mappedResource.RowPitch;

				ctx->Unmap(data.d3d.tex, 0);

				ctx->Release();
				
				HPV_VERBOSE("Succesfully set sampler.");

				data.gpu_resources_need_init = false;

				return HPV_RET_ERROR_NONE;
			}
			else
			{
				HPV_ERROR("Error creating D3D texture.");
				return HPV_RET_ERROR;
			}
		}
		else if (HPVRendererType::RENDERER_OPENGLCORE == m_Renderer)
		{
			if (HPVCompressionType::HPV_TYPE_DXT1_NO_ALPHA == data.player->getCompressionType())
			{
				data.opengl.gl_format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			}
			else
			{
				data.opengl.gl_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			}

			glGenTextures(1, &data.opengl.tex);

			glBindTexture(GL_TEXTURE_2D, data.opengl.tex);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			// allocate texture storage for this texture
			glTexStorage2D(GL_TEXTURE_2D, 1, data.opengl.gl_format, data.player->getWidth(), data.player->getHeight());

			if (pbo_supported)
			{
				glGenBuffers(2, data.opengl.pboIds);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data.opengl.pboIds[0]);
				glBufferData(GL_PIXEL_UNPACK_BUFFER, data.player->getBytesPerFrame(), 0, GL_STREAM_DRAW);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data.opengl.pboIds[1]);
				// The contents of our second PBO will be uploaded first to the GPU texture.
				// Fill it already with frame data.
				glBufferData(GL_PIXEL_UNPACK_BUFFER, data.player->getBytesPerFrame(), data.player->getBufferPtr(), GL_STREAM_DRAW);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

			}
			glBindTexture(GL_TEXTURE_2D, 0);

			data.gpu_resources_need_init = false;

			ReportGLError();

			return HPV_RET_ERROR_NONE;
		}

		return HPV_RET_ERROR_NONE;
	}

	int HPVRenderBridge::nodeHasResources(uint8_t node_id)
	{
		return static_cast<int>(!m_RenderData[node_id].gpu_resources_need_init);
	}

	int HPVRenderBridge::deleteGPUResources()
	{
		bShouldUpload = false;

		for (auto& data : m_RenderData)
		{
			if (data.second.gpu_resources_need_init)
			{
				continue;
			}

			if (HPVRendererType::RENDERER_DIRECT3D11 == m_Renderer)
			{
				data.second.d3d.tex->Release();
				data.second.d3d.tex = nullptr;

				data.second.d3d.tex_view->Release();
				data.second.d3d.tex_view = nullptr;

				data.second.d3d.buffer_stride = 0;
				data.second.d3d.runtime_stride = 0;
				data.second.d3d.tex_update_height = 0;
				data.second.gpu_resources_need_init = true;
			}
			else if (HPVRendererType::RENDERER_OPENGLCORE == m_Renderer)
			{
				glDeleteTextures(1, &data.second.opengl.tex);
				glDeleteBuffers(2, &data.second.opengl.pboIds[0]);
			}
		}

		m_RenderData.clear();

		std::queue<uint8_t> empty;
		std::swap(m_scheduled_inits, empty);

		HPV_VERBOSE("Deleted GPU resources");

		return HPV_RET_ERROR_NONE;
	}

	intptr_t HPVRenderBridge::getTexturePtr(uint8_t node_id)
	{
		if (HPVRendererType::RENDERER_DIRECT3D11 == m_Renderer)
		{
			return reinterpret_cast<intptr_t>(m_RenderData[node_id].d3d.tex_view);
		}
		else if (HPVRendererType::RENDERER_OPENGLCORE == m_Renderer)
		{
			return m_RenderData[node_id].opengl.tex;
		}
		else return 0;
	}

	HPVRendererType HPVRenderBridge::getRenderer()
	{
		return m_Renderer;
	}

	void HPVRenderBridge::updateTextures()
	{
		// check first if we need to create gpu resources
		if (!m_scheduled_inits.empty())
		{
			int ret = this->createGPUResources(m_scheduled_inits.front());
			m_scheduled_inits.pop();

			if (HPV_RET_ERROR == ret)
			{
				HPV_ERROR("Error creating resources for HPVPlayer");
			}
		}

		std::vector<bool> update_flags = ManagerSingleton()->update();

		for (uint8_t player_idx = 0; player_idx < update_flags.size(); ++player_idx)
		{
			// get update flag for this player
			bool should_update = update_flags[player_idx];

			if (should_update)
			{
				// get specifics for this player
				HPVRenderData& render_data = m_RenderData[player_idx];

				if (render_data.gpu_resources_need_init)
					return;

				// update is necessary
				if (HPVRendererType::RENDERER_DIRECT3D11 == m_Renderer)
				{
					ID3D11DeviceContext* ctx = NULL;
					g_D3D11Device->GetImmediateContext(&ctx);

					if (render_data.d3d.tex)
					{
						if (render_data.player->_gather_stats) render_data.stats.before_upload = ns();

						ctx->PSSetSamplers(0, 1, &sampler);

						D3D11_MAPPED_SUBRESOURCE mappedResource;
						ctx->Map(render_data.d3d.tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

						BYTE* mappedData = reinterpret_cast<BYTE*>(mappedResource.pData);
						BYTE* dxt_buffer = render_data.player->getBufferPtr();

						for (UINT i = 0; i < render_data.d3d.tex_update_height; ++i)
						{
							memcpy(mappedData, dxt_buffer, render_data.d3d.buffer_stride);

							mappedData += render_data.d3d.runtime_stride;
							dxt_buffer += render_data.d3d.buffer_stride;
						}
						ctx->Unmap(render_data.d3d.tex, 0);

						if (render_data.player->_gather_stats)
						{
							render_data.stats.after_upload = ns();
							render_data.player->_decode_stats.gpu_upload_time = render_data.stats.after_upload - render_data.stats.before_upload;
						}
					}

					ctx->Release();
				}
				else if (HPVRendererType::RENDERER_OPENGLCORE == m_Renderer)
				{
					// be sure to unbind any unpack buffer before start
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

					glBindTexture(GL_TEXTURE_2D, render_data.opengl.tex);

					if (render_data.player->_gather_stats) render_data.stats.before_upload = ns();

					// http://www.songho.ca/opengl/gl_pbo.html
					if (pbo_supported)
					{
						int pbo_fill_index = 0;

						// tex_fill_index is used to copy pixels from a PBO to a texture object
						// pbo_fill_index is used to update pixels in a PBO
						render_data.opengl.tex_fill_index = (render_data.opengl.tex_fill_index + 1) % 2;
						pbo_fill_index = (render_data.opengl.tex_fill_index + 1) % 2;

						glBindBuffer(GL_PIXEL_UNPACK_BUFFER, render_data.opengl.pboIds[render_data.opengl.tex_fill_index]);

						// don't use pointer for uploading data (last parameter = 0), data will come from bound PBO
						glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, render_data.player->getWidth(), render_data.player->getHeight(), render_data.opengl.gl_format, static_cast<GLsizei>(render_data.player->getBytesPerFrame()), 0);

						// bind PBO to update pixel values
						glBindBuffer(GL_PIXEL_UNPACK_BUFFER, render_data.opengl.pboIds[pbo_fill_index]);
						glBufferData(GL_PIXEL_UNPACK_BUFFER, render_data.player->getBytesPerFrame(), 0, GL_STREAM_DRAW);

						// map pointer for memcpy from our frame buffer
						GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
						if (ptr)
						{
							memcpy(ptr, render_data.player->getBufferPtr(), render_data.player->getBytesPerFrame());
							glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
						}

						glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
					}
					// when PBO's are not supported, fall back to traditional texture upload
					else
					{
						glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, render_data.player->getWidth(), render_data.player->getHeight(), render_data.opengl.gl_format, static_cast<GLsizei>(render_data.player->getBytesPerFrame()), render_data.player->getBufferPtr());
					}

					glBindTexture(GL_TEXTURE_2D, 0);

					if (render_data.player->_gather_stats)
					{
						render_data.stats.after_upload = ns();
						render_data.player->_decode_stats.gpu_upload_time = render_data.stats.after_upload - render_data.stats.before_upload;
					}

					ReportGLError();
				}
			}
		}
	}

	HPVRenderBridge * RendererSingleton()
	{
		return &m_HPVRenderer;
	}
}