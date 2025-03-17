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

python_version=3.12.9

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
ZGlmZiAtTnVyIFB5dGhvbi0zLjEyLjktb3JpZy9Nb2R1bGVzL19wb3NpeHN1YnByb2Nlc3MuYyBQ
eXRob24tMy4xMi45L01vZHVsZXMvX3Bvc2l4c3VicHJvY2Vzcy5jCi0tLSBQeXRob24tMy4xMi45
LW9yaWcvTW9kdWxlcy9fcG9zaXhzdWJwcm9jZXNzLmMJMjAyNS0wMi0wNCAxNDozODozOC4wMDAw
MDAwMDAgKzAwMDAKKysrIFB5dGhvbi0zLjEyLjkvTW9kdWxlcy9fcG9zaXhzdWJwcm9jZXNzLmMJ
MjAyNS0wMy0xNyAyMToyODo0MS4zNjgwMzU1MjUgKzAwMDAKQEAgLTMwMywxNyArMzAzLDI2IEBA
CiB9CiAKICNpZiBkZWZpbmVkKF9fbGludXhfXykgJiYgZGVmaW5lZChIQVZFX1NZU19TWVNDQUxM
X0gpCisKKyNpZmRlZiBTWVNfZ2V0ZGVudHM2NAorIyBkZWZpbmUgUFlfTElOVVhfR0VUREVOVFMg
U1lTX2dldGRlbnRzNjQKKyNlbHNlCisjIGRlZmluZSBQWV9MSU5VWF9HRVRERU5UUyBTWVNfZ2V0
ZGVudHMKKyNlbmRpZgorCiAvKiBJdCBkb2Vzbid0IG1hdHRlciBpZiBkX25hbWUgaGFzIHJvb20g
Zm9yIE5BTUVfTUFYIGNoYXJzOyB3ZSdyZSB1c2luZyB0aGlzCiAgKiBvbmx5IHRvIHJlYWQgYSBk
aXJlY3Rvcnkgb2Ygc2hvcnQgZmlsZSBkZXNjcmlwdG9yIG51bWJlciBuYW1lcy4gIFRoZSBrZXJu
ZWwKICAqIHdpbGwgcmV0dXJuIGFuIGVycm9yIGlmIHdlIGRpZG4ndCBnaXZlIGl0IGVub3VnaCBz
cGFjZS4gIEhpZ2hseSBVbmxpa2VseS4KICAqIFRoaXMgc3RydWN0dXJlIGlzIHZlcnkgb2xkIGFu
ZCBzdGFibGU6IEl0IHdpbGwgbm90IGNoYW5nZSB1bmxlc3MgdGhlIGtlcm5lbAogICogY2hvb3Nl
cyB0byBicmVhayBjb21wYXRpYmlsaXR5IHdpdGggYWxsIGV4aXN0aW5nIGJpbmFyaWVzLiAgSGln
aGx5IFVubGlrZWx5LgogICovCi1zdHJ1Y3QgbGludXhfZGlyZW50NjQgeworc3RydWN0IGxpbnV4
X2RpcmVudCB7CiAgICB1bnNpZ25lZCBsb25nIGxvbmcgZF9pbm87CiAgICBsb25nIGxvbmcgZF9v
ZmY7CiAgICB1bnNpZ25lZCBzaG9ydCBkX3JlY2xlbjsgICAgIC8qIExlbmd0aCBvZiB0aGlzIGxp
bnV4X2RpcmVudCAqLworI2lmZGVmIFNZU19nZXRkZW50czY0CiAgICB1bnNpZ25lZCBjaGFyICBk
X3R5cGU7CisjZW5kaWYKICAgIGNoYXIgICAgICAgICAgIGRfbmFtZVsyNTZdOyAgLyogRmlsZW5h
bWUgKG51bGwtdGVybWluYXRlZCkgKi8KIH07CiAKQEAgLTM1NSwxOSArMzY0LDE5IEBACiAgICAg
ICAgICAgICAgICAgICAgICAgICAgICAgX2JydXRlX2ZvcmNlX2Nsb3Nlcik7CiAgICAgICAgIHJl
dHVybjsKICAgICB9IGVsc2UgewotICAgICAgICBjaGFyIGJ1ZmZlcltzaXplb2Yoc3RydWN0IGxp
bnV4X2RpcmVudDY0KV07CisgICAgICAgIGNoYXIgYnVmZmVyW3NpemVvZihzdHJ1Y3QgbGludXhf
ZGlyZW50KV07CiAgICAgICAgIGludCBieXRlczsKLSAgICAgICAgd2hpbGUgKChieXRlcyA9IHN5
c2NhbGwoU1lTX2dldGRlbnRzNjQsIGZkX2Rpcl9mZCwKLSAgICAgICAgICAgICAgICAgICAgICAg
ICAgICAgICAgKHN0cnVjdCBsaW51eF9kaXJlbnQ2NCAqKWJ1ZmZlciwKKyAgICAgICAgd2hpbGUg
KChieXRlcyA9IHN5c2NhbGwoUFlfTElOVVhfR0VUREVOVFMsIGZkX2Rpcl9mZCwKKyAgICAgICAg
ICAgICAgICAgICAgICAgICAgICAgICAgKHN0cnVjdCBsaW51eF9kaXJlbnQgKilidWZmZXIsCiAg
ICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIHNpemVvZihidWZmZXIpKSkgPiAwKSB7Ci0g
ICAgICAgICAgICBzdHJ1Y3QgbGludXhfZGlyZW50NjQgKmVudHJ5OworICAgICAgICAgICAgc3Ry
dWN0IGxpbnV4X2RpcmVudCAqZW50cnk7CiAgICAgICAgICAgICBpbnQgb2Zmc2V0OwogI2lmZGVm
IF9QeV9NRU1PUllfU0FOSVRJWkVSCiAgICAgICAgICAgICBfX21zYW5fdW5wb2lzb24oYnVmZmVy
LCBieXRlcyk7CiAjZW5kaWYKICAgICAgICAgICAgIGZvciAob2Zmc2V0ID0gMDsgb2Zmc2V0IDwg
Ynl0ZXM7IG9mZnNldCArPSBlbnRyeS0+ZF9yZWNsZW4pIHsKICAgICAgICAgICAgICAgICBpbnQg
ZmQ7Ci0gICAgICAgICAgICAgICAgZW50cnkgPSAoc3RydWN0IGxpbnV4X2RpcmVudDY0ICopKGJ1
ZmZlciArIG9mZnNldCk7CisgICAgICAgICAgICAgICAgZW50cnkgPSAoc3RydWN0IGxpbnV4X2Rp
cmVudCAqKShidWZmZXIgKyBvZmZzZXQpOwogICAgICAgICAgICAgICAgIGlmICgoZmQgPSBfcG9z
X2ludF9mcm9tX2FzY2lpKGVudHJ5LT5kX25hbWUpKSA8IDApCiAgICAgICAgICAgICAgICAgICAg
IGNvbnRpbnVlOyAgLyogTm90IGEgbnVtYmVyLiAqLwogICAgICAgICAgICAgICAgIGlmIChmZCAh
PSBmZF9kaXJfZmQgJiYgZmQgPj0gc3RhcnRfZmQgJiYKZGlmZiAtTnVyIFB5dGhvbi0zLjEyLjkt
b3JpZy9jb25maWd1cmUgUHl0aG9uLTMuMTIuOS9jb25maWd1cmUKLS0tIFB5dGhvbi0zLjEyLjkt
b3JpZy9jb25maWd1cmUJMjAyNS0wMi0wNCAxNDozODozOC4wMDAwMDAwMDAgKzAwMDAKKysrIFB5
dGhvbi0zLjEyLjkvY29uZmlndXJlCTIwMjUtMDMtMTcgMjE6MDM6MjguMzQ4Njc2MDAyICswMDAw
CkBAIC02OTQ0LDYgKzY5NDQsMTEgQEAKICAgICBQTEFURk9STV9UUklQTEVUPWBlY2hvICIkUExB
VEZPUk1fVFJJUExFVCIgfCBzZWQgJ3MvbGludXgtZ251L2xpbnV4LW11c2wvJ2AKICAgICA7Owog
ICBlc2FjCisgIGNhc2UgIiRob3N0X29zIiBpbgorICBsaW51eC1tdXNsKikKKyAgICBQTEFURk9S
TV9UUklQTEVUPWBlY2hvICIkUExBVEZPUk1fVFJJUExFVCIgfCBzZWQgJ3MvbGludXgtZ251L2xp
bnV4LW11c2wvJ2AKKyAgICA7OworICBlc2FjCiAgIHsgcHJpbnRmICIlc1xuIiAiJGFzX21lOiR7
YXNfbGluZW5vLSRMSU5FTk99OiByZXN1bHQ6ICRQTEFURk9STV9UUklQTEVUIiA+JjUKIHByaW50
ZiAiJXNcbiIgIiRQTEFURk9STV9UUklQTEVUIiA+JjY7IH0KIGVsc2UK
EOF

  mkdir -p build/native
  cd build/native
  ../../configure \
      --build=x86_64-linux-gnu \
      --prefix=/src/dist \
      --disable-ipv6 \
      --with-ensurepip=no
  make -j`nproc`

  cd ..
  mkdir $xtools_host
  cd $xtools_host
  export \
      CFLAGS="$CFLAGS -Os -w" \
      CPPFLAGS="--sysroot=${xtools_sysroot}"
  ../../configure \
      --build=x86_64-linux-gnu \
      --host=${xtools_host} \
      --prefix=${xtools_prefix} \
      --disable-ipv6 \
      --with-build-python=../native/python \
      --with-pkg-config=no \
      --with-ensurepip=no \
      cross_compiling=yes \
      ac_cv_file__dev_ptmx=yes \
      ac_cv_file__dev_ptc=no
  make -j`nproc` install
)
