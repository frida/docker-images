FROM ubuntu:24.04 AS base
COPY scripts/install-shared-packages.sh /tmp
COPY scripts/install-platform-packages.sh /tmp
RUN /tmp/install-shared-packages.sh

FROM base AS android
RUN curl -L https://dl.google.com/android/repository/android-ndk-r29-linux.zip -o android-ndk-r29-linux.zip \
    && unzip android-ndk-r29-linux.zip -d /opt \
    && rm android-ndk-r29-linux.zip
ENV ANDROID_NDK_ROOT=/opt/android-ndk-r29

FROM base AS x86
ENV XTOOLS_HOST=i686-linux-gnu
RUN /tmp/install-platform-packages.sh gcc-multilib lib32stdc++-11-dev

FROM base AS x86_64
ENV XTOOLS_HOST=x86_64-linux-gnu
RUN /tmp/install-platform-packages.sh g++-x86-64-linux-gnu

FROM android AS arm
ENV XTOOLS_HOST=android-arm
RUN /tmp/install-platform-packages.sh g++-arm-linux-gnueabi

FROM ghcr.io/frida/x-tools-linux-armbe8 as armbe8
ENV XTOOLS_HOST=armeb-linux-gnueabi
COPY scripts/install-shared-packages.sh /tmp
RUN /tmp/install-shared-packages.sh

FROM android AS arm64
ENV XTOOLS_HOST=android-arm64
RUN /tmp/install-platform-packages.sh g++-aarch64-linux-gnu

FROM ghcr.io/frida/x-tools-linux-arm64be as arm64be
ENV XTOOLS_HOST=aarch64_be-linux-gnu
COPY scripts/install-shared-packages.sh /tmp
RUN /tmp/install-shared-packages.sh

FROM ghcr.io/frida/x-tools-linux-arm64beilp32 as arm64beilp32
ENV XTOOLS_HOST=aarch64_be-linux-gnu_ilp32
COPY scripts/install-shared-packages.sh /tmp
RUN /tmp/install-shared-packages.sh

FROM base AS mips
ENV XTOOLS_HOST=mips-linux-gnu
RUN /tmp/install-platform-packages.sh g++-$XTOOLS_HOST

FROM base AS mipsel
ENV XTOOLS_HOST=mipsel-linux-gnu
RUN /tmp/install-platform-packages.sh g++-$XTOOLS_HOST

FROM base AS mips64
ENV XTOOLS_HOST=mips64-linux-gnuabi64
RUN /tmp/install-platform-packages.sh g++-$XTOOLS_HOST

FROM base AS mips64el
ENV XTOOLS_HOST=mips64el-linux-gnuabi64
RUN /tmp/install-platform-packages.sh g++-$XTOOLS_HOST
