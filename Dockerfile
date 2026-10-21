FROM debian:12
RUN apt-get update && apt-get install -y build-essential wget xz-utils libgmp-dev libmpfr-dev libmpc-dev texinfo flex bison && rm -rf /var/lib/apt/lists/*
WORKDIR /build
RUN wget -q https://ftp.gnu.org/gnu/binutils/binutils-2.40.tar.xz && tar xf binutils-2.40.tar.xz \
 && mkdir bb && cd bb && ../binutils-2.40/configure --target=bfin-elf --prefix=/opt/bfin --disable-nls --disable-werror --disable-gdb --disable-sim \
 && make -j32 && make install && cd .. && rm -rf bb binutils-2.40*
ENV PATH=/opt/bfin/bin:$PATH
RUN wget -q https://ftp.gnu.org/gnu/gcc/gcc-12.3.0/gcc-12.3.0.tar.xz && tar xf gcc-12.3.0.tar.xz \
 && mkdir bg && cd bg && ../gcc-12.3.0/configure --target=bfin-elf --prefix=/opt/bfin --enable-languages=c --without-headers --with-newlib --disable-nls --disable-libssp --disable-libquadmath --disable-libgomp --disable-shared --disable-threads \
 && make -j32 all-gcc all-target-libgcc && make install-gcc install-target-libgcc && cd .. && rm -rf bg gcc-12.3.0*
WORKDIR /work
