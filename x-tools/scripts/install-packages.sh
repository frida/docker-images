#!/bin/bash

set -ex

extra_packages=$@

export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y \
    awscli \
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
    $extra_packages

curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
apt-get install -y nodejs

npm install -g cloudflare-cli

rm -rf \
    ~/.npm \
    /var/lib/apt/lists/* \
    /tmp/install-packages.sh
