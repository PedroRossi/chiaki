# Building Chiaki Android APK with Docker

This guide explains how to build the Chiaki Android APK using Docker.

## Build Instructions

```bash
# Ensure git submodules are initialized first
git submodule update --init --recursive
# Build the base image with correct CLI tools and deps
docker build -t chiaki-android-builder .
# For the actual build process we need to mount from the root folder to use the CMakeLists.txt file
docker run --rm -it \
  -v $(pwd)/..:/app \
  chiaki-android-builder
# Once inside docker run
./gradlew clean
./gradlew assembleRelease
# Generate keystore.jks
keytool -genkey -v -keystore keystore.jks -alias my-key-alias -keyalg RSA -keysize 2048 -validity 10000
# Copy the apk to the root folder
cp app/build/outputs/apk/release/app-release-unsigned.apk .
# Align the apk for better ram usage
zipalign -v -p 4 app-release-unsigned.apk app-release-unsigned-aligned.apk
# Sign apk
apksigner sign --ks keystore.jks --out app-release-signed.apk app-release-unsigned-aligned.apk
# Verify that sign is ok
apksigner verify app-release-signed.apk
```

## Build Output

After a successful build, you can find the APK in the root android folder with the name `app-release-signed.apk`.

## Troubleshooting

If you encounter any issues:

1. Make sure you have enough disk space
2. Check that Docker has sufficient memory allocated
3. Ensure you have proper permissions in the build directories
4. Try cleaning the build directories if you encounter build errors:
```bash
rm -rf build/ app/build/
```
