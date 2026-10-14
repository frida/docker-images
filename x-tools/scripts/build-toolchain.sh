#!/bin/bash

# Builds a crosstool-NG toolchain plus a sysroot Python.
#
# Linux (inside Docker): the config to use is picked up from /src/*.config, and
# the toolchain is installed into /opt/x-tools/<target>.
#
# macOS: run from a checkout as `build-toolchain.sh <target>`, where <target>
# names one of the configs in ../configs, e.g. `x86_64-linux-gnu`. The build
# happens on a case-sensitive APFS volume backed by a sparse image, and the
# result is packaged as build/x-tools-<target>-macos-<arch>.tar.xz, ready to
# be extracted into /opt/x-tools.

set -ex

texinfo_tag="20250317"
texinfo_version="7.1-3build2frida1"
texinfo_packages=( \
    "texinfo_${texinfo_version}_all.deb" \
    "texinfo-lib_${texinfo_version}_amd64.deb" \
    "info_${texinfo_version}_amd64.deb" \
    "install-info_${texinfo_version}_amd64.deb" \
)

python_version=3.12.9

build_os=$(uname -s)

case $build_os in
  Linux)
    srcdir=/src
    xtools_host=$(cd $srcdir && ls -1 *.config | cut -d. -f1)
    xtools_config=$srcdir/${xtools_host}.config
    xtools_root=/opt/x-tools/${xtools_host}
    build_triplet=x86_64-linux-gnu
    njobs=$(nproc)
    ;;
  Darwin)
    if [ $# -ne 1 ]; then
      echo "Usage: $0 <target>" >&2
      echo "Example: $0 x86_64-linux-gnu" >&2
      exit 1
    fi
    xtools_host=$1
    xtools_dir=$(cd "$(dirname "$0")/.." && pwd)
    xtools_config=${xtools_dir}/configs/${xtools_host}.config
    if [ ! -f "$xtools_config" ]; then
      echo "Unknown target '${xtools_host}'; expected one of:" >&2
      (cd "${xtools_dir}/configs" && ls -1 *.config | cut -d. -f1 | sed 's/^/  /') >&2
      exit 1
    fi
    build_dir=${XTOOLS_BUILD_DIR:-${xtools_dir}/build}
    volume_name=frida-x-tools
    volume=/Volumes/${volume_name}
    srcdir=${volume}/src
    xtools_root=${volume}/x-tools/${xtools_host}
    case $(uname -m) in
      arm64) build_arch=aarch64; macos_arch=arm64 ;;
      x86_64) build_arch=x86_64; macos_arch=x86_64 ;;
      *) echo "Unsupported macOS architecture: $(uname -m)" >&2; exit 1 ;;
    esac
    build_triplet=${build_arch}-apple-darwin$(uname -r)
    njobs=$(sysctl -n hw.ncpu)
    # The build-time Python only needs to run the build scripts, so skip the
    # OpenSSL-dependent modules rather than fight whatever Homebrew provides.
    native_python_config_opts=( \
        py_cv_module__hashlib=n/a \
        py_cv_module__ssl=n/a \
    )
    ;;
  *)
    echo "Unsupported build OS: $build_os" >&2
    exit 1
    ;;
esac

xtools_sysroot=${xtools_root}/${xtools_host}/sysroot
xtools_prefix=${xtools_sysroot}/usr

export PATH=${xtools_root}/bin:$PATH

case $build_os in
  Linux)
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
    ;;
  Darwin)
    if ! xcode-select -p > /dev/null 2>&1; then
      echo "Xcode Command Line Tools are required; install them with: xcode-select --install" >&2
      exit 1
    fi
    if ! command -v brew > /dev/null 2>&1; then
      echo "Homebrew is required; see https://brew.sh" >&2
      exit 1
    fi
    if ! command -v python3 > /dev/null 2>&1; then
      echo "python3 is required; the Xcode Command Line Tools provide one" >&2
      exit 1
    fi

    brew_packages=( \
        autoconf \
        automake \
        bash \
        binutils \
        bison \
        coreutils \
        flex \
        gawk \
        gettext \
        gnu-sed \
        gnu-tar \
        gpatch \
        grep \
        help2man \
        libtool \
        m4 \
        make \
        ncurses \
        texinfo \
        wget \
        xz \
    )
    missing_packages=()
    for pkg in ${brew_packages[@]}; do
      brew list --versions $pkg > /dev/null 2>&1 || missing_packages+=($pkg)
    done
    if [ ${#missing_packages[@]} -ne 0 ]; then
      brew install ${missing_packages[@]}
    fi
    brew_prefix=$(brew --prefix)

    # crosstool-NG needs its work and install directories on a case-sensitive
    # file system, so we build inside a sparse image formatted as such.
    mkdir -p "$build_dir"
    image=${build_dir}/${volume_name}.sparseimage
    if [ ! -d "$volume" ]; then
      if [ ! -f "$image" ]; then
        hdiutil create \
            -size 64g \
            -type SPARSE \
            -fs "Case-sensitive APFS" \
            -volname $volume_name \
            "$image"
      fi
      hdiutil attach -nobrowse -mountpoint "$volume" "$image"
    fi

    # GNU binutils must not shadow Apple's linker and friends, so we only
    # expose the tools that crosstool-NG itself needs.
    hosttools=${volume}/hosttools/bin
    mkdir -p "$hosttools"
    for tool in objcopy objdump readelf; do
      ln -sf "${brew_prefix}/opt/binutils/bin/$tool" "$hosttools/$tool"
    done

    gnubin_path=""
    for pkg in coreutils gawk gnu-sed gnu-tar grep make; do
      gnubin_path="${gnubin_path}${brew_prefix}/opt/$pkg/libexec/gnubin:"
    done
    bin_path=""
    for pkg in bison flex gpatch help2man libtool m4 texinfo; do
      bin_path="${bin_path}${brew_prefix}/opt/$pkg/bin:"
    done
    export PATH="${hosttools}:${gnubin_path}${bin_path}${brew_prefix}/bin:${volume}/ct-ng/bin:$PATH"

    mkdir -p "$srcdir"
    ;;
esac

rm -rf $srcdir/crosstool-ng
git clone https://github.com/frida/crosstool-ng.git $srcdir/crosstool-ng
(
  cd $srcdir/crosstool-ng
  ./bootstrap
  case $build_os in
    Linux) ./configure ;;
    Darwin) ./configure --prefix=${volume}/ct-ng ;;
  esac
  make
  make install
)
(
  rm -rf $srcdir/x-tools
  mkdir $srcdir/x-tools
  cd $srcdir/x-tools
  cp $xtools_config .config
  if [ $build_os = Darwin ]; then
    # Install and cache downloads on the case-sensitive volume.
    sed -i.orig \
        -e "s|^CT_PREFIX_DIR=.*|CT_PREFIX_DIR=\"${volume}/x-tools/\${CT_TARGET}\"|" \
        -e "s|^CT_LOCAL_TARBALLS_DIR=.*|CT_LOCAL_TARBALLS_DIR=\"${volume}/tarballs\"|" \
        .config
    mkdir -p ${volume}/tarballs

    # Drop gold, which cannot be built on a macOS host.
    sed -i.orig \
        -e '/^CT_BINUTILS_LINKER_/d' \
        -e '/^CT_BINUTILS_LINKERS_LIST=/d' \
        -e '/^CT_BINUTILS_GOLD_/d' \
        -e '/^CT_CC_GCC_GOLD=/d' \
        .config
    echo 'CT_BINUTILS_LINKER_LD=y' >> .config

    # Make binutils and GCC use our zlib rather than their bundled copies,
    # which, like the zlib version used on Linux, do not build against
    # current macOS SDKs.
    sed -i.orig \
        -e 's|^CT_BINUTILS_EXTRA_CONFIG_ARRAY="\(.*\)"|CT_BINUTILS_EXTRA_CONFIG_ARRAY="\1 --with-system-zlib"|' \
        -e '/^# CT_CC_GCC_SYSTEM_ZLIB is not set/d' \
        .config
    echo 'CT_CC_GCC_SYSTEM_ZLIB=y' >> .config

    # Bump host-side companion libraries whose versions used on Linux no
    # longer build with current macOS SDKs and clang.
    for pkg_version in ZLIB:1_3_1 GETTEXT:0_22_5; do
      pkg=${pkg_version%%:*}
      version=${pkg_version##*:}
      sed -i.orig -e "/^CT_${pkg}_V_/d" -e "/^CT_${pkg}_VERSION=/d" .config
      echo "CT_${pkg}_V_${version}=y" >> .config
    done

    rm .config.orig
  fi
  ct-ng olddefconfig < /dev/null
  ct-ng build
)

(
  cd $srcdir

  if [ $build_os = Darwin ]; then
    # crosstool-NG leaves the toolchain read-only; we still need to add Python.
    chmod -R u+w $xtools_root
  fi

  rm -rf Python-${python_version}
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
      --build=${build_triplet} \
      --prefix=$srcdir/dist \
      --disable-ipv6 \
      --with-ensurepip=no \
      "${native_python_config_opts[@]}"
  make -j${njobs}

  cd ..
  mkdir $xtools_host
  cd $xtools_host
  export \
      CFLAGS="$CFLAGS -Os -w -fPIC" \
      CPPFLAGS="--sysroot=${xtools_sysroot}"
  ../../configure \
      --build=${build_triplet} \
      --host=${xtools_host} \
      --prefix=${xtools_prefix} \
      --disable-ipv6 \
      --with-build-python=../native/python \
      --with-pkg-config=no \
      --with-ensurepip=no \
      cross_compiling=yes \
      ac_cv_file__dev_ptmx=yes \
      ac_cv_file__dev_ptc=no
  make -j${njobs} install
)

if [ $build_os = Darwin ]; then
  tarball=${build_dir}/x-tools-${xtools_host}-macos-${macos_arch}.tar.xz
  rm -f "$tarball"
  XZ_OPT=-T0 tar -C ${volume}/x-tools -cJf "$tarball" ${xtools_host}

  set +x
  echo
  echo "Toolchain packaged as: $tarball"
  echo
  echo "To install it alongside the Docker image layout:"
  echo "  sudo mkdir -p /opt/x-tools"
  echo "  sudo tar -C /opt/x-tools -xJf \"$tarball\""
  echo "  export PATH=/opt/x-tools/${xtools_host}/bin:\$PATH"
  echo
  echo "Then, in a Frida checkout:"
  echo "  ./configure --host=${xtools_host}"
  echo "  make"
fi
