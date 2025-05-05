#!/bin/bash
set -e

cd /app/android

# Build the APK
./gradlew assembleRelease

# The APK will be available at app/build/outputs/apk/release/app-release.apk
echo "Build completed! APK is available at app/build/outputs/apk/release/app-release.apk" 