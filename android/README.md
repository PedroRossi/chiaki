# Building Chiaki Android APK with Docker

This guide explains how to build the Chiaki Android APK using Docker.

## Build Instructions

```bash
# Ensure git submodules are initialized first
git submodule update --init --recursive
# Build the base image with correct CLI tools and deps
docker build -t chiaki-android-builder .
# For the actual build process we need to mount from the root folder to use the CMakeLists.txt file
docker run --rm \
  -v $(pwd)/..:/app \
  chiaki-android-builder
```

## Build Output

After a successful build, you can find the APK at:
```
app/build/outputs/apk/release/app-release.apk
```

## Troubleshooting

If you encounter any issues:

1. Make sure you have enough disk space
2. Check that Docker has sufficient memory allocated
3. Ensure you have proper permissions in the build directories
4. Try cleaning the build directories if you encounter build errors:
```bash
rm -rf build/ app/build/
```