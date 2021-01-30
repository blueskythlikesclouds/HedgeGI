#pragma once

// DirectXTex
#include <DirectXTex.h>

// Embree
#include <rtcore.h>

// HedgeLib
#include <hedgelib/hl_internal.h>
#include <hedgelib/hl_math.h>
#include <hedgelib/hl_endian.h>
#include <hedgelib/hl_text.h>
#include <hedgelib/io/hl_hh.h>
#include <hedgelib/io/hl_path.h>
#include <hedgelib/archives/hl_archive.h>
#include <hedgelib/archives/hl_hh_archive.h>
#include <hedgelib/archives/hl_pacx.h>
#include <hedgelib/models/hl_hh_model.h>
#include <hedgelib/materials/hl_hh_material.h>
#include <hedgelib/textures/hl_hh_texture.h>
#include <hedgelib/terrain/hl_hh_terrain_instance_info.h>

#include "hl_hh_light.h"

// Eigen
#include <Eigen/Core>
#include <Eigen/Geometry>

// seamoptimizer
#include <seamoptimizer/seamoptimizer.h>

// std
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <execution>
#include <filesystem>
#include <random>
#include <unordered_set>
#include <vector>

// opencv
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>

// parallel_hashmap
#include <phmap.h>

// Other
#include <INIReader.h>

#undef min
#undef max

// HedgeGI
#include "FileStream.h"
#include "Math.h"
#include "Path.h"
#include "Random.h"
#include "RaytracingDevice.h"