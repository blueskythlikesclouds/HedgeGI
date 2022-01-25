# HedgeGI
HedgeGI is a tool that allows you to bake global illumination and light field data for Hedgehog Engine games. Currently, it supports the following:
* Sonic Generations
* Sonic Lost World
* Sonic Forces

# Requirements
* Windows 10 (or later) with latest updates.
* x64 CPU with AVX2 support.
    * SSE2 builds are going to be provided in the future.
* NVIDIA GPU on the latest drivers if you wish to use the Optix AI Denoiser.
    * AMD/Intel users can use oidn given their CPU supports SSE4.1.

# Building 
* [Stable (release) builds](https://github.com/blueskythlikesclouds/HedgeGI/releases)
* [Unstable (development) builds](https://ci.appveyor.com/project/blueskythlikesclouds/hedgegi/build/artifacts)

## Manually building
* [Install CUDA 11.4 or newer.](https://developer.nvidia.com/cuda-downloads) 
    * Make sure there's an environment variable called `CUDA_PATH` which is set to the path of your CUDA installation, eg. `C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.4`
    * If you don't want to set this environment variable, you can change the paths manually in project settings:
        * C/C++ -> General -> Additional Include Directories
        * Linker -> General -> Additional Library Directories
* Open the solution in the latest version of Visual Studio 2019 (or later).
* Build the solution.