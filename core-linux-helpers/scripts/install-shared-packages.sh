#!/bin/bash

set -ex

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
    valac

curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
apt-get install -y nodejs
