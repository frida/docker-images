#!/bin/bash

set -ex

apt-get install -y $@

rm -rf \
    ~/.npm \
    /var/lib/apt/lists/* \
    /tmp/install-shared-packages.sh \
    /tmp/install-platform-packages.sh
