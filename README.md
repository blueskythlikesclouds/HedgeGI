# HedgeGI

HedgeGI is a tool that allows you to bake global illumination and light field data for Hedgehog Engine games. Currently, it supports the following:

* Sonic Unleashed
* Sonic Generations
* Sonic Lost World
* Sonic Forces

## Requirements

* Windows 10 (or later) with latest updates.
* x64 CPU with AVX support.
* NVIDIA GPU on the latest drivers if you wish to use the Optix AI Denoiser.
  * AMD/Intel users can use oidn as an alternative.

## Building

Nightly development builds can be downloaded from the [Releases](https://github.com/blueskythlikesclouds/HedgeGI/releases) page.

### Manually building

* [Install CUDA 11.4 or newer.](https://developer.nvidia.com/cuda-downloads)
  * Make sure there's an environment variable called `CUDA_PATH` which is set to the path of your CUDA installation, eg. `C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.4`
  * If you don't want to set this environment variable, you can change the paths manually in project settings:
    * C/C++ -> General -> Additional Include Directories
    * Linker -> General -> Additional Library Directories
* Open the solution in Visual Studio 2022.
* Set the configuration to Release.
* Build the solution.

## Screenshot

![HedgeGI Screnshot](https://i.imgur.com/L2ooCB7.png)