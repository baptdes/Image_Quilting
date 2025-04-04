wget http://vismod.media.mit.edu/pub/VisTex/VisTex.tar.gz
tar -xf VisTex.tar.gz
cd VisionTexture/
source buildVisTex
mv VisTex/FLAT/ ../Textures/
cd ..
rm -rf VisionTexture/
rm VisTex.tar.gz