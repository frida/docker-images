#!/bin/bash

set -ex

xtools_host=$(cd /src && ls -1 *.config | cut -d. -f1)
xtools_root=/opt/x-tools/${xtools_host}
xtools_sysroot=${xtools_root}/${xtools_host}/sysroot
xtools_prefix=${xtools_sysroot}/usr

texinfo_tag="20250317"
texinfo_version="7.1-3build2frida1"
texinfo_packages=( \
    "texinfo_${texinfo_version}_all.deb" \
    "texinfo-lib_${texinfo_version}_amd64.deb" \
    "info_${texinfo_version}_amd64.deb" \
    "install-info_${texinfo_version}_amd64.deb" \
)

python_version=3.10.7

export PATH=${xtools_root}/bin:$PATH
export DEBIAN_FRONTEND=noninteractive

apt-get update
apt-get install -y curl

cd /tmp
texinfo_pkg_paths=()
for pkg in ${texinfo_packages[@]}; do
  curl -LO https://github.com/frida/docker-images/releases/download/$texinfo_tag/$pkg
  texinfo_pkg_paths+=("./$pkg")
done
apt-get install -y ${texinfo_pkg_paths[@]}

apt-get install -y \
    autoconf \
    bison \
    build-essential \
    file \
    flex \
    gawk \
    git \
    help2man \
    libtool-bin \
    ncurses-dev \
    python-is-python3 \
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
ZGlmZiAtTnVyIFB5dGhvbi0zLjEwLjctb3JpZy9jb25maWd1cmUgUHl0aG9uLTMuMTAuNy9jb25m
aWd1cmUKLS0tIFB5dGhvbi0zLjEwLjctb3JpZy9jb25maWd1cmUJMjAyMi0wOS0wNSAxMzowMDow
Mi4wMDAwMDAwMDAgKzAwMDAKKysrIFB5dGhvbi0zLjEwLjcvY29uZmlndXJlCTIwMjUtMDMtMTUg
MTg6NTI6NDcuMDg4NTI5MDU2ICswMDAwCkBAIC01Mzc2LDYgKzUzNzYsOSBAQAogCiBpZiAkQ1BQ
ICRDUFBGTEFHUyBjb25mdGVzdC5jID5jb25mdGVzdC5vdXQgMj4vZGV2L251bGw7IHRoZW4KICAg
UExBVEZPUk1fVFJJUExFVD1gZ3JlcCAtdiAnXiMnIGNvbmZ0ZXN0Lm91dCB8IGdyZXAgLXYgJ14g
KiQnIHwgdHIgLWQgJyAJJ2AKKyAgaWYgdGVzdCAiJHtob3N0X29zfSIgPSAibGludXgtbXVzbCI7
IHRoZW4KKyAgICBQTEFURk9STV9UUklQTEVUPWBlY2hvICRQTEFURk9STV9UUklQTEVUIHwgc2Vk
ICdzLC1nbnUkLC1tdXNsLCdgCisgIGZpCiAgIHsgJGFzX2VjaG8gIiRhc19tZToke2FzX2xpbmVu
by0kTElORU5PfTogcmVzdWx0OiAkUExBVEZPUk1fVFJJUExFVCIgPiY1CiAkYXNfZWNobyAiJFBM
QVRGT1JNX1RSSVBMRVQiID4mNjsgfQogZWxzZQpkaWZmIC1OdXIgUHl0aG9uLTMuMTAuNy1vcmln
L3NldHVwLnB5IFB5dGhvbi0zLjEwLjcvc2V0dXAucHkKLS0tIFB5dGhvbi0zLjEwLjctb3JpZy9z
ZXR1cC5weQkyMDIyLTA5LTA1IDEzOjAwOjAyLjAwMDAwMDAwMCArMDAwMAorKysgUHl0aG9uLTMu
MTAuNy9zZXR1cC5weQkyMDI1LTAzLTE1IDE4OjUzOjM3Ljk4ODU2MjE4MiArMDAwMApAQCAtNjg2
LDcgKzY4Niw3IEBACiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgJy91c3IvaW5jbHVkZS8n
ICsgbXVsdGlhcmNoX3BhdGhfY29tcG9uZW50KQogICAgICAgICAgICAgcmV0dXJuCiAKLSAgICAg
ICAgaWYgbm90IGZpbmRfZXhlY3V0YWJsZSgnZHBrZy1hcmNoaXRlY3R1cmUnKToKKyAgICAgICAg
aWYgVHJ1ZToKICAgICAgICAgICAgIHJldHVybgogICAgICAgICBvcHQgPSAnJwogICAgICAgICBp
ZiBDUk9TU19DT01QSUxJTkc6Cg==
EOF
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
