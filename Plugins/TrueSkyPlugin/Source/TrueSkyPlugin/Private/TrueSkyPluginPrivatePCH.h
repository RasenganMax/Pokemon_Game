// Copyright 2007-2018 Simul Software Ltd.. All Rights Reserved.

#pragma once
#include "ITrueSkyPlugin.h"

// You should place include statements to your module's private header files here.  You only need to
// add includes for headers that are used in most of your module's source files though.

#include "RenderResource.h"

DECLARE_LOG_CATEGORY_EXTERN(TrueSky, Log, All);

#ifndef UE_LOG_SIMUL_INTERNAL
#if !UE_BUILD_SHIPPING
#define UE_LOG_SIMUL_INTERNAL(a,b,c,d) { if(simul::internal_logs) {UE_LOG(a,b,c,d);}}
#endif	// !UE_BUILD_SHIPPING
#endif	// UE_LOG_SIMUL_INTERNAL.

namespace simul
{
	namespace crossplatform
	{
		enum class ResourceState : uint16_t
		{
			COMMON	= 0,
			VERTEX_AND_CONSTANT_BUFFER	= 0x1,
			INDEX_BUFFER	= 0x2,
			RENDER_TARGET	= 0x4,
			UNORDERED_ACCESS	= 0x8,
			DEPTH_WRITE	= 0x10,
			DEPTH_READ	= 0x20,
			NON_PIXEL_SHADER_RESOURCE	= 0x40,
			PIXEL_SHADER_RESOURCE	= 0x80,
			STREAM_OUT	= 0x100,
			INDIRECT_ARGUMENT	= 0x200,
			COPY_DEST	= 0x400,
			COPY_SOURCE	= 0x800,
			RESOLVE_DEST	= 0x1000,
			RESOLVE_SOURCE	= 0x2000,
			GENERAL_READ	= ( ( ( ( ( 0x1 | 0x2 )  | 0x40 )  | 0x80 )  | 0x200 )  | 0x800 ),
			SHADER_RESOURCE	= ( 0x40 | 0x80 ),
			UNKNOWN=0xFFFF
		};
		//! A cross-platform equivalent to the OpenGL and DirectX pixel formats
		enum PixelFormat
		{
			UNKNOWN
			,RGBA_32_FLOAT
			,RGBA_32_UINT
			,RGBA_32_INT
			,RGBA_16_FLOAT
			,RGBA_16_UINT
			,RGBA_16_INT
			,RGBA_16_SNORM
			,RGBA_16_UNORM
			,RGBA_8_UINT
			,RGBA_8_INT
			,RGBA_8_SNORM
			,RGBA_8_UNORM
			,RGBA_8_UNORM_COMPRESSED
			,RGBA_8_UNORM_SRGB
			,BGRA_8_UNORM

			,RGB_32_FLOAT
			,RGB_32_UINT
			,RGB_16_FLOAT
			,RGB_10_A2_UINT
			,RGB_10_A2_INT
			,RGB_11_11_10_FLOAT
			,RGB_8_UNORM
			,RGB_8_SNORM

			,RG_32_FLOAT
			,RG_32_UINT
			,RG_16_FLOAT
			,RG_16_UINT
			,RG_16_INT
			,RG_8_UNORM
			,RG_8_SNORM

			,R_32_FLOAT
			,R_32_FLOAT_X_8
			,LUM_32_FLOAT
			,INT_32_FLOAT
			,R_32_UINT
			,R_32_INT
			// depth formats:
			,D_32_FLOAT// DXGI_FORMAT_D32_FLOAT or GL_DEPTH_COMPONENT32F
			, D_32_UINT
			,D_32_FLOAT_S_8_UINT
			,D_24_UNORM_S_8_UINT
			,D_16_UNORM

			, R_16_FLOAT
			, R_8_UNORM
			, R_8_SNORM

			,RGB10_A2_UNORM
		};
	}
	extern FTextureRHIRef GetTextureRHIRef(UTexture *t);
	extern FTextureRHIRef GetTextureRHIRef(UTextureRenderTargetCube *t);

	extern bool internal_logs;
	enum PluginStyle
	{
		DEFAULT_STYLE			= 0,
		UNREAL_STYLE			= 1,
		UNITY_STYLE				= 2,
		UNITY_STYLE_DEFERRED	= 6,
		VISION_STYLE			= 8,
		CUBEMAP_STYLE			= 16,
		VR_STYLE				= 32,	// without right eye flag, this means left eye.
		VR_STYLE_RIGHT_EYE		= 64,
		POST_TRANSLUCENT		= 128,	// rain, snow etc.
		VR_STYLE_SIDE_BY_SIDE	= 256,
		DEPTH_BLENDING			= 512,
		DONT_COMPOSITE			= 1024,
		CLEAR_SCREEN			=2048
	};

	inline PluginStyle operator|(PluginStyle a, PluginStyle b)
	{
		return static_cast<PluginStyle>(static_cast<int>(a) | static_cast<int>(b));
	}

	inline PluginStyle operator&(PluginStyle a, PluginStyle b)
	{
		return static_cast<PluginStyle>(static_cast<int>(a) & static_cast<int>(b));
	}

	inline PluginStyle operator~(PluginStyle a)
	{
		return static_cast<PluginStyle>(~static_cast<int>(a));
	}

	//! Graphics API list
	enum GraphicsDeviceType
	{
		GraphicsDeviceUnknown				= -1,
		GraphicsDeviceOpenGL				=  0,	// Desktop OpenGL
		GraphicsDeviceD3D9					=  1,	// Direct3D 9
		GraphicsDeviceD3D11					=  2,	// Direct3D 11
		GraphicsDeviceGCM					=  3,	// PlayStation 3
		GraphicsDeviceNull					=  4,	// "null" device (used in batch mode)
		GraphicsDeviceXenon					=  6,	// Xbox 360
		GraphicsDeviceOpenGLES20			=  8,	// OpenGL ES 2.0
		GraphicsDeviceOpenGLES30			= 11,	// OpenGL ES 3.0
		GraphicsDeviceGXM					= 12,	// PlayStation Vita
		GraphicsDevicePS4					= 13,	// PlayStation 4
		GraphicsDeviceXboxOne				= 14,	// Xbox One        
		GraphicsDeviceMetal					= 16,	// iOS Metal
		GraphicsDeviceD3D12					= 18,	// Direct3D 12
		GraphicsDeviceVulkan				= 21,	// Vulkan
        GraphicsDeviceNVN                   = 22,   // Nintendo Switch NVN API
		GraphicsDeviceD3D11_FastSemantics	= 1002, // Direct3D 11 (fast semantics)
	};

	//! Event types
	enum GraphicsEventType
	{
		GraphicsEventInitialize = 0,
		GraphicsEventShutdown	= 1,
		GraphicsEventBeforeReset= 2,
		GraphicsEventAfterReset	= 3
	};

	struct Viewport
	{
		int x,y,w,h;
	};
	struct ExternalTexture
	{
		static const int static_version = 3;
		int version;
		void *texturePtr;
		int width, height, depth;
		simul::crossplatform::PixelFormat pixelFormat;
		unsigned sampleCount:16; 
		unsigned resourceState:16;
	};

	struct Orbit
	{
		double LongitudeOfAscendingNode=125.1228;
		double LongitudeOfAscendingNodeRate=-0.0529538083;
		double Inclination=5.1454;
		double ArgumentOfPericentre=318.0634;
		double ArgumentOfPericentreRate=0.1643573223;
		double MeanDistance=60.2666;
		double Eccentricity=0.054900;
		double MeanAnomaly=115.3654;
		double MeanAnomalyRate=13.0649929509;
	};
	struct ExternalMoon
	{
		static const int static_version = 1;
		int version=static_version;
		Orbit orbit;
		float radiusArcMinutes;
		const char *name=nullptr;
		bool illuminated=true;
		ExternalTexture texture;
	};
	struct ExternalMoonProxy
	{
		bool AddDelete=true;	// add if true, delete otherwise.
		Orbit orbit;
		float RadiusArcMinutes;
		FString Name;
		bool Illuminated=true;
		FTextureRHIRef Texture;
	};
	struct LightningBolt
	{
		int     id;
		float   pos[3];
		float   endpos[3];
		float   brightness;
		float   colour[3];
		int     age;
	};

	struct Query
	{
		Query():Enum(0),LastFrame(0),Float(0.0f),uid(0),Valid(false){}
		int64 Enum;
		FVector Pos;
		int32 LastFrame;
		union
		{
			float Float;
			int32 Int;
		};
		int32 uid;
		bool Valid;
	};
	typedef TMap<int64,Query> QueryMap;
}
