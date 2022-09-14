all:
	rm -rf sdp
	mkdir sdp
	tar -C / -cf - \
		--exclude opt/qnx650/host/linux/x86/usr/bin/*-4.4.2 \
		--exclude opt/qnx650/host/linux/x86/etc/qcc/gcc/4.4.2 \
		--exclude opt/qnx650/host/linux/x86/unsupported \
		--exclude opt/qnx650/host/linux/x86/usr/lib/gcc/arm-unknown-nto-qnx6.5.0/4.4.2 \
		--exclude opt/qnx650/host/linux/x86/usr/lib/gcc/arm-unknown-nto-qnx6.5.0eabi/4.4.2 \
		--exclude opt/qnx650/target/qnx6/armle-v7/lib/gcc/4.4.2 \
		--exclude opt/qnx650/target/qnx6/armle/lib/gcc/4.4.2 \
		--exclude opt/qnx650/target/qnx6/opt/webkit \
		--exclude opt/qnx650/target/qnx6/usr/include/c++/4.4.2 \
		etc/qnx \
		opt/qnx650 \
		| tar -C sdp -xf -
	patch -i $$(pwd)/patches/qnx-toolchain-compatibility.patch -d sdp/opt/qnx650/target/qnx6 -p0
	$(MAKE) -C sabrelite/bsp

.PHONY: all
