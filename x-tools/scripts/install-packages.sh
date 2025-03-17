#!/bin/bash

set -ex

qemu_tag="20250317"
qemu_version="8.2.2+ds-0ubuntu1.6frida1"

extra_packages=()
install_qemu_user=0
for pkg in "$@"; do
  if [ $pkg = "qemu-user" ]; then
    install_qemu_user=1
  else
    extra_packages+=("$pkg")
  fi
done

export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y \
    bison \
    curl \
    file \
    flex \
    git \
    gperf \
    make \
    ninja-build \
    pip \
    pkg-config \
    python-is-python3 \
    unzip \
    valac \
    ${extra_packages[@]}

curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
apt-get install -y nodejs

pip install awscli --break-system-packages
npm install -g cloudflare-cli

if [ $install_qemu_user -ne 0 ]; then
  cd /tmp
  curl -LO https://github.com/frida/docker-images/releases/download/$qemu_tag/qemu-user_${qemu_version}_amd64.deb
  curl -LO https://github.com/frida/docker-images/releases/download/$qemu_tag/qemu-user-static_${qemu_version}_amd64.deb
  apt-get install -y ./qemu-user_${qemu_version}_amd64.deb ./qemu-user-static_${qemu_version}_amd64.deb
fi

rm -rf \
    ~/.npm \
    /tmp/*.deb \
    /var/lib/apt/lists/* \
    /tmp/install-packages.sh
