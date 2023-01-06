# egl-headless-kmsgrab
Headless kmsgrab for use with obs-vkcapture

Based on: https://github.com/peko/egl-headless-render

For use with obs-vkcapture: https://github.com/nowrep/obs-vkcapture/
and with libstrangle: https://gitlab.com/torkel104/libstrangle

## Building
```
mkdir build
cd build
cmake ..
make
```

# Usage
Launch from Root with strangle (to limit fps) and with obs-gamecapture (to capture)

```
sudo strangle 60 obs-gamecapture ./main
```
