FROM ubuntu:22.04 AS base
COPY scripts/install-shared-packages.sh /tmp
COPY scripts/install-platform-packages.sh /tmp
RUN /tmp/install-shared-packages.sh

FROM base AS android
# Download and install the Android NDK and delete the downloaded archive so
# it doesn't take up space in the image layer.
RUN curl -L https://dl.google.com/android/repository/android-ndk-r25c-linux.zip -o android-ndk-r25c-linux.zip \
    && unzip android-ndk-r25c-linux.zip -d /opt \
    && rm android-ndk-r25c-linux.zip
ENV ANDROID_NDK_ROOT=/opt/android-ndk-r25c

FROM base AS x86
ENV XTOOLS_HOST=i686-linux-gnu
RUN /tmp/install-platform-packages.sh gcc-multilib lib32stdc++-11-dev

FROM base AS x86_64
ENV XTOOLS_HOST=x86_64-linux-gnu
RUN /tmp/install-platform-packages.sh g++-x86-64-linux-gnu

FROM android AS arm
ENV XTOOLS_HOST=android-arm
RUN /tmp/install-platform-packages.sh g++-arm-linux-gnueabi

FROM x-tools-linux-armbe8 as armbe8
ENV XTOOLS_HOST=armbe8-linux-gnueabi

FROM android AS arm64
ENV XTOOLS_HOST=android-arm64
RUN /tmp/install-platform-packages.sh g++-aarch64-linux-gnu

FROM x-tools-linux-arm64be as arm64be
ENV XTOOLS_HOST=aarch64_be-linux-gnu

FROM x-tools-linux-arm64beilp32 as arm64beilp32
ENV XTOOLS_HOST=aarch64_be_ilp32-linux-gnu

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
