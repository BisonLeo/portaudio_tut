# cmake to build 'portaudio' lib first
mkdir portaudio_src/bld
cd portaudio_src/bld
cmake .. -G "${tool chain to use}" -A "{Win32|x64}" -DCMAKE_INSTALL_PREFIX=../../portaudio_install
cmake --build . --config Debug
cmake --build . --config Release
cmake --install . --config Debug
cmake --install . --config Release

