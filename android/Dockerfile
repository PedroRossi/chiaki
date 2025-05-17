# Use the official Android SDK image as base
FROM ubuntu:22.04

# Set environment variables
ENV ANDROID_HOME=/opt/android-sdk
ENV ANDROID_NDK_HOME=${ANDROID_HOME}/ndk/25.2.9519653
ENV PATH=${PATH}:${ANDROID_HOME}/tools:${ANDROID_HOME}/platform-tools:${ANDROID_NDK_HOME}

# Install required packages
RUN apt-get update && apt-get install -y \
    openjdk-11-jdk \
    wget \
    unzip \
    git \
    cmake \
    ninja-build \
    pkg-config \
    protobuf-compiler \
    python3-protobuf \
    zipalign \
    apksigner \
    && rm -rf /var/lib/apt/lists/*

# Download and install Android SDK
RUN mkdir -p ${ANDROID_HOME} && \
    wget -q https://dl.google.com/android/repository/commandlinetools-linux-9477386_latest.zip -O android-sdk.zip && \
    unzip android-sdk.zip -d ${ANDROID_HOME}/cmdline-tools && \
    rm android-sdk.zip

# Set up Android SDK
RUN cd ${ANDROID_HOME}/cmdline-tools && \
    mv cmdline-tools latest && \
    yes | ${ANDROID_HOME}/cmdline-tools/latest/bin/sdkmanager --licenses && \
    ${ANDROID_HOME}/cmdline-tools/latest/bin/sdkmanager --update && \
    ${ANDROID_HOME}/cmdline-tools/latest/bin/sdkmanager \
    "platform-tools" \
    "platforms;android-33" \
    "build-tools;33.0.2" \
    "cmake;3.22.1" \
    "ndk;25.2.9519653" \
    "ndk;23.1.7779620"

# Set working directory
WORKDIR /app

ENTRYPOINT ["bash"]
