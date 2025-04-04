# Image Quilting for Texture Synthesis and Transfer

The goal of this project is to implement the Image Quilting process  presented by Efros and Freeman in their [paper “Image Quilting for Texture Synthesis and Transfer”](https://people.eecs.berkeley.edu/~efros/research/quilting/quilting.pdf).

# Usage

Download Textures from [MIT Vision Texture (VisTex) Dataset](https://vismod.media.mit.edu/vismod/imagery/VisionTexture/vistex.html) to have textures to work with or import your textures
```bash
source download_textures.sh
```

Then, you can build and execute the project using CMake

```bash
mkdir build
cd build
cmake ..
make
./quilting
```