FROM mcr.microsoft.com/devcontainers/cpp:ubuntu

RUN apt-get update && apt-get install -y \
    libasound2-dev \
    libfreetype6-dev \
    libfontconfig1-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libgl1-mesa-dev \
    libwebkit2gtk-4.1-dev \
    libgtk-3-dev \
    clangd \
    && rm -rf /var/lib/apt/lists/*
