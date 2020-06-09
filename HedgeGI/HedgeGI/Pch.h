#pragma once

// DirectXTex
#include <DirectXTex.h>

// Embree
#include <rtcore.h>

// HedgeLib
#include <HedgeLib/Archives/Archive.h>
#include <HedgeLib/Geometry/HHMesh.h>
#include <HedgeLib/Geometry/HHModel.h>
#include <HedgeLib/Geometry/HHTerrainInstanceInfo.h>
#include <HedgeLib/HedgeLib.h>
#include <HedgeLib/IO/File.h>
#include <HedgeLib/IO/HedgehogEngine.h>
#include <HedgeLib/IO/Path.h>
#include <HedgeLib/Materials/HHMaterial.h>
#include <HedgeLib/Textures/HHTexture.h>

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

#undef min
#undef max

// HedgeGI
#include "AxisAlignedBoundingBox.h"
#include "FileStream.h"
#include "HHLight.h"
#include "HHPackedFileInfo.h"
#include "Math.h"
#include "Path.h"
#include "Random.h"
#include "RaytracingDevice.h"
