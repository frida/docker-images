FROM ubuntu:24.04

RUN apt-get update \
	&& export DEBIAN_FRONTEND=noninteractive \
	&& apt-get install -y \
		build-essential \
		curl \
		git \
		gperf \
		lib32z1 \
		libc6-i386 \
		pip \
		python-is-python3 \
		qemu-system-arm \
	&& curl -fsSL https://deb.nodesource.com/setup_20.x | bash - \
	&& apt-get install -y nodejs \
	&& pip install awscli --break-system-packages \
	&& npm install -g cloudflare-cli \
	&& rm -rf \
		~/.npm \
		/var/lib/apt/lists/*

COPY sdp/opt/qnx650 /opt/qnx650
COPY sdp/etc/qnx /etc/qnx
COPY sabrelite/bsp/install /opt/sabrelite/assets
COPY sabrelite/run.sh /opt/sabrelite

ENV QNX_HOST=/opt/qnx650/host/linux/x86 \
    QNX_TARGET=/opt/qnx650/target/qnx6 \
    MAKEFLAGS=-I/opt/qnx650/target/qnx6/usr/include \
    PATH=${PATH}:/opt/qnx650/host/linux/x86/usr/bin:/etc/qnx/bin

ENTRYPOINT ["/bin/bash"]
