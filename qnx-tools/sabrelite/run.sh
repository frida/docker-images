#!/bin/bash

payload=$(realpath "$1")
script=$2

intdir=$(mktemp -d)
fwdir=$intdir/firmware
ifsfile=$intdir/ifs-mx6q-sabrelite.elf
buildfile=$fwdir/armle-v7/boot/build/sabrelite.build
qemu_pid=0

exit_pattern='^::set-exit-status ([[:digit:]]+)$'

cleanup() {
  [[ $qemu_pid -ne 0 ]] && kill $qemu_pid
  rm -rf "$intdir"
}
trap cleanup EXIT

set -e

cp -a /opt/sabrelite/assets "$fwdir"
mkdir -p "$fwdir/opt/frida"
tar -C "$fwdir/opt/frida" -xf "$payload"
echo "[+raw] /opt=$fwdir/opt" >> "$buildfile"
cat << EOF > "$fwdir/opt/frida/run.sh"
(set -ex; $script)
echo "::set-exit-status \$?"
EOF
chmod 755 "$fwdir/opt/frida/run.sh"
sed -ie "s,ksh &,/opt/frida/run.sh &," "$buildfile"
mkifs "-r$fwdir" "$buildfile" "$ifsfile"

mkfifo "$intdir/serial.in" "$intdir/serial.out"

qemu-system-arm \
  -M sabrelite \
  -smp 4 \
  -m 1G \
  -display none \
  -serial null \
  -serial "pipe:$intdir/serial" \
  -kernel "$ifsfile" \
  -nic user,hostfwd=tcp::8000-:8000 \
  &>"$intdir/qemu.log" &
qemu_pid=$!

while read line; do
  line=$(echo "$line" | tr -d "\r")

  if [[ "$line" =~ $exit_pattern ]]; then
    exit ${BASH_REMATCH[1]}
  fi

  echo "$line"
done < "$intdir/serial.out"
