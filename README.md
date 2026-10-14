# docker-images

Docker images for building portable binaries.

## Building Linux binaries from macOS

The `x-tools` toolchains can also be built natively on macOS, so that a Mac
can cross-compile Frida for Linux without Docker. Prebuilt tarballs are
published as release assets by the *macOS* workflow, named like
`x-tools-x86_64-linux-gnu-macos-arm64.tar.xz`.

To build one yourself, with Xcode Command Line Tools and Homebrew installed:

```sh
x-tools/scripts/build-toolchain.sh x86_64-linux-gnu
```

The script installs the Homebrew packages it needs, builds on a case-sensitive
sparse disk image under `x-tools/build/` (which it mounts at
`/Volumes/frida-x-tools`), and writes the result to
`x-tools/build/x-tools-x86_64-linux-gnu-macos-arm64.tar.xz`. Any other
`x-tools/configs/*.config` name can be given instead of `x86_64-linux-gnu`.

Install the tarball like the Docker images lay it out, and put it on PATH:

```sh
sudo mkdir -p /opt/x-tools
sudo tar -C /opt/x-tools -xJf x-tools/build/x-tools-x86_64-linux-gnu-macos-arm64.tar.xz
export PATH=/opt/x-tools/x86_64-linux-gnu/bin:$PATH
```

Frida's build system looks up `<triplet>-gcc` and friends on PATH, so in a
Frida checkout you can then do:

```sh
./configure --host=x86_64-linux-gnu
make
```

Unlike the Docker images, the macOS toolchains are built without the `gold`
linker, as it cannot be built on a macOS host. Extracting onto a regular,
case-insensitive volume is fine: the only paths that differ by case alone are
a handful of Linux 2.6.18 netfilter headers that nothing in Frida uses.
