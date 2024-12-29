#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-


if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    echo "## Deep clean kernel ${KERNEL_VERSION}"
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE mrproper

    echo "## Config using defconfig"
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig

    echo "## Compile kernel ${KERNEL_VERSION}"
    make -j8 ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE all

    echo "## Compile modules"
    make -j8 ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE modules

    echo "## Compile dts"
    make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE dtbs
fi
echo "Adding the Image in outdir"
cp -r $OUTDIR/linux-stable/arch/$ARCH/boot/Image $OUTDIR


echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
echo "## Create rootfs"
ROOTFSDIR="$OUTDIR/rootfs"
mkdir -p rootfs 
cd rootfs

mkdir -p bin sbin dev etc home lib lib64 proc sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    echo "## Config busybox"
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
echo "## Compile and install busybox"
make ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE
make CONFIG_PREFIX=$ROOTFSDIR ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE install
cd "$ROOTFSDIR"


echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
echo "Add dependencies"

AARCHTOOLCHAIN=$(aarch64-none-linux-gnu-gcc -print-sysroot)
TOOLCHAINLIB64="$AARCHTOOLCHAIN/lib64"
cp $TOOLCHAINLIB64/libm.so.6 $ROOTFSDIR/lib64
cp $TOOLCHAINLIB64/libresolv.so.2 $ROOTFSDIR/lib64
cp $TOOLCHAINLIB64/libc.so.6 $ROOTFSDIR/lib64

TOOLCHAINLIBC="$AARCHTOOLCHAIN/lib"
cp $TOOLCHAINLIBC/ld-linux-aarch64.so.1 $ROOTFSDIR/lib

# TODO: Make device nodes
echo "Make nodes"
cd "$ROOTFSDIR"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

# TODO: Clean and build the writer utility
echo "Compile writer"
cd $FINDER_APP_DIR
make clean
make CROSS_COMPILE=$CROSS_COMPILE

# TODO: Copy the finder related scripts and executables to the /home directory
echo "Copy scripts to rootfs"
cp -r finder.sh writer finder-test.sh autorun-qemu.sh "$ROOTFSDIR"/home/
cp -r conf/ "$ROOTFSDIR"/home/conf/

# on the target rootfs
cd "$ROOTFSDIR"
# TODO: Chown the root directory

echo "Chown the root directory"
sudo chown root:root "${OUTDIR}/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio


# TODO: Create initramfs.cpio.gz
echo "Create initramfs.cpio.gz"
cd "$OUTDIR"
gzip -f initramfs.cpio