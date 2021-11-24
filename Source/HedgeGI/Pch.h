#pragma once

// DirectXTex
#include <DirectXTex.h>

// Embree
#include <rtcore.h>

// HedgeLib
#include <hedgelib/hl_internal.h>
#include <hedgelib/hl_math.h>
#include <hedgelib/hl_text.h>
#include <hedgelib/io/hl_file.h>
#include <hedgelib/io/hl_path.h>
#include <hedgelib/io/hl_stream.h>
#include <hedgelib/io/hl_mem_stream.h>
#include <hedgelib/archives/hl_archive.h>
#include <hedgelib/archives/hl_hh_archive.h>
#include <hedgelib/archives/hl_pacx.h>
#include <hedgelib/models/hl_hh_model.h>
#include <hedgelib/materials/hl_hh_material.h>
#include <hedgelib/textures/hl_hh_texture.h>
#include <hedgelib/terrain/hl_hh_terrain.h>

#include "hl_hh_light.h"
#include "hl_hh_shlf.h"
#include "hl_hh_gi_texture.h"

// Eigen
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <unsupported/Eigen/AlignedVector3>

using Vector2 = Eigen::Vector2f;
using Vector3 = Eigen::AlignedVector3<float>;
using Vector4 = Eigen::Vector4f;
using Color3 = Eigen::Array3f;
using Color4 = Eigen::Array4f;
using Color4i = Eigen::Array4<uint8_t>;
using Matrix3 = Eigen::Matrix3f;
using Matrix4 = Eigen::Matrix4f;
using Affine3 = Eigen::Affine3f;
using AABB = Eigen::AlignedBox3f;
using Quaternion = Eigen::Quaternionf;

// std
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <execution>
#include <filesystem>
#include <fstream>
#include <future>
#include <random>
#include <unordered_set>
#include <vector>

// parallel_hashmap
#include <phmap.h>

// OpenGL
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

// tinyxml2
#include <tinyxml2.h>

// meshoptimizer
#include <meshoptimizer/meshoptimizer.h>

// xatlas
#include <xatlas/xatlas.h>

// imgui
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

// im3d
#include <im3d/im3d.h>
#include <im3d/im3d_config.h>
#include <im3d/im3d_math.h>

// Common types
class CriticalSection
{
public:
	CRITICAL_SECTION criticalSection;

	CriticalSection() 
	{
		InitializeCriticalSection(&criticalSection);
	}

	~CriticalSection()
	{
		DeleteCriticalSection(&criticalSection);
	}

	void lock()
	{
		EnterCriticalSection(&criticalSection);
	}

	void unlock()
	{
		LeaveCriticalSection(&criticalSection);
	}
};

#undef min
#undef max