#!/bin/bash

set -ex

xtools_host=$(cd /src && ls -1 *.config | cut -d. -f1)
xtools_root=/opt/x-tools/${xtools_host}
xtools_sysroot=${xtools_root}/${xtools_host}/sysroot
xtools_prefix=${xtools_sysroot}/usr

python_version=3.10.7

export PATH=${xtools_root}/bin:$PATH
export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y \
    autoconf \
    bison \
    build-essential \
    curl \
    file \
    flex \
    gawk \
    git \
    help2man \
    libtool-bin \
    ncurses-dev \
    python-is-python3 \
    texinfo \
    unifdef \
    unzip

git clone https://github.com/frida/crosstool-ng.git /src/crosstool-ng
(
  cd /src/crosstool-ng
  ./bootstrap
  ./configure
  make install
)
(
  mkdir /src/x-tools
  cd /src/x-tools
  cp /src/${xtools_host}.config .config
  ct-ng oldconfig
  ct-ng build
)

(
  cd /src
  curl -sL https://www.python.org/ftp/python/${python_version}/Python-${python_version}.tar.xz | tar -xJf -
  cd Python-${python_version}
  base64 -d << EOF | patch -p1
LS0tIFB5dGhvbi0zLjEwLjcvY29uZmlndXJlLW9yaWcJMjAyMy0wMi0xNSAxMzo1NjowNi41NDY1
OTEyNTEgKzAwMDAKKysrIFB5dGhvbi0zLjEwLjcvY29uZmlndXJlCTIwMjMtMDItMTUgMTQ6MDU6
MjQuNjkzMjQ0NDAyICswMDAwCkBAIC01Mzc2LDYgKzUzNzYsOSBAQAogCiBpZiAkQ1BQICRDUFBG
TEFHUyBjb25mdGVzdC5jID5jb25mdGVzdC5vdXQgMj4vZGV2L251bGw7IHRoZW4KICAgUExBVEZP
Uk1fVFJJUExFVD1gZ3JlcCAtdiAnXiMnIGNvbmZ0ZXN0Lm91dCB8IGdyZXAgLXYgJ14gKiQnIHwg
dHIgLWQgJyAJJ2AKKyAgaWYgdGVzdCAiJHtob3N0X29zfSIgPSAibGludXgtbXVzbCI7IHRoZW4K
KyAgICBQTEFURk9STV9UUklQTEVUPWBlY2hvICRQTEFURk9STV9UUklQTEVUIHwgc2VkICdzLC1n
bnUkLC1tdXNsLCdgCisgIGZpCiAgIHsgJGFzX2VjaG8gIiRhc19tZToke2FzX2xpbmVuby0kTElO
RU5PfTogcmVzdWx0OiAkUExBVEZPUk1fVFJJUExFVCIgPiY1CiAkYXNfZWNobyAiJFBMQVRGT1JN
X1RSSVBMRVQiID4mNjsgfQogZWxzZQo=
EOF
  sed -i -e "s,if not find_executable('dpkg-architecture'):,if True:," setup.py
  export \
      CFLAGS="$CFLAGS -Os -w" \
      CPPFLAGS="--sysroot=${xtools_sysroot}"
  ./configure \
      --build=x86_64-linux-gnu \
      --host=${xtools_host} \
      --prefix=${xtools_prefix} \
      --disable-ipv6 \
      cross_compiling=yes \
      ac_cv_file__dev_ptmx=yes \
      ac_cv_file__dev_ptc=no
  make -j`nproc` install
)
