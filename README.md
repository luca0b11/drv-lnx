# Linux Kernel Module on QEMU - Group8

This project aims to create and test a custom Linux kernel module on a QEMU emulator using an ARM (aarch64) virtual machine architecture. This README details the setup instructions, kernel module creation, and testing procedures.

## Device description
The `virt-mulmatr` device is a virtual hardware component that performs matrix-vector multiplication. This device enables matrix-vector operations by taking inputs from specified memory-mapped registers. 
The core functionality involves multiplying a *size x size* matrix (stored in `matrA`) by a vector (stored in `matrB`), with the resulting matrix stored in `matrC`.
The operation is controlled by setting specific bits in the control register, including initiating and resetting the multiplication process.

**Memory Mapped Registers:**

It uses designated memory regions to store matrix data (`matrA` and `matrB`) and result data (`matrC`). Control and status registers manage and monitor the device’s operation state. Control bits handle device enabling, operation start, interrupt enabling, and status reset.
Status bits indicate if an operation has started or ended, and interrupts can be triggered upon operation completion if enabled.
An ID register uniquely identifies the device (`0xc1a0`), and size configuration is limited to a maximum of 10.

Here a detailed description of the registers:

*Control_reg* 
- bit 0 -> enable/disable device
- bit 1 -> enable/disable interrupt on operation end
- bit 2 -> 1 start operation
- bit 3 -> 1 to reset status register

*Status_reg*
- bit 0 -> operation started (busy)
- bit 1 -> operation finished (ready)

*ID_reg (readonly)*
- device ID

*Size_reg*
- matrix size (max 10)

*matrA*
- size x size

*matrB*
- 1 x size

*matrC*
- 1 x size

This device can be used to simulate matrix-vector multiplication for testing purposes or as a computational unit within a larger virtual system in QEMU.

## Prerequisites

Ensure the following packages are installed:

```bash
sudo apt install git libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev \
  ninja-build build-essential python3 python3-pip libaio-dev libcap-ng-dev \
  libiscsi-dev libattr1-dev libnfs-dev libudev-dev libxen-dev libepoxy-dev \
  libdrm-dev libgbm-dev libvirglrenderer-dev libgtk-3-dev libspice-protocol-dev \
  libspice-server-dev libusb-1.0-0-dev libpulse-dev libsdl2-dev libslirp-dev \
  libncurses5-dev libncursesw5-dev libx11-dev libxext-dev libxt-dev \
  libpng-dev libjpeg8-dev libvte-2.91-dev libfuse-dev git make install rsync
```

## 1. Setting Up the Linux Environment with Buildroot
To set up the Linux environment using Buildroot, follow these steps:

**1. Clone the Buildroot repository:**
```bash
git clone https://github.com/buildroot/buildroot.git
cd buildroot/
``` 

**2. Configure Buildroot for QEMU with aarch64 architecture:** 
use the predefined QEMU configuration for aarch64 (ARM 64-bit) by running:
```bash
make qemu_aarch64_virt_defconfig
```

**3. Build the Environment:**
initiate the Buildroot build process, which will generate the Linux image and root filesystem, with `make`.
This may take some time depending on system resources, as it will download and compile all necessary packages for the environment.
Once these steps are complete, the Linux environment for QEMU with aarch64 architecture will be set up and ready for further modifications and testing.


## 2. Building QEMU
**1.** Clone the QEMU repository:
```bash
git clone git://git.qemu.org/qemu.git
cd qemu/
```

**2.** Checkout to a stable branch:
```bash
git checkout stable-4.1
git submodule init
git submodule update --recursive
```

**3.** Correctly configure and build QEMU:
```bash
mkdir build
cd build
../configure --target-list=aarch64-softmmu
make
```

**4.** The machine can be finally started using the script [run_aarch64.sh](./).


## 3. Custom Core Additions in QEMU
**1.** Include the device in the file `qemu/include/hw/arm/virt.h`, adding a new entry in the enum:
```c
VIRT_MULMATR
```
**2.** Modify the file `qemu/hw/arm/virt.c`:
- include the following library
    ```c
    #include "qemu/log.h"
    ```

- after the `create_virtio_devices(vms, pic)` function, add the custom device creation function from [additions_virt.c](QEMU_Core/aarch64/additions_virt.c).

- inside the funtion `machvirt_init()`, immediately after `create_virtio_devices(vms, pic)`, call the newly created function:
    ```c
    create_virt_mulmatr_device(const VirtMachineState *vms, qemu_irq *pic)
    ```
- add the following line to the `a15irqmap` vector:
    ```c
    [VIRT_MULMATR] = 112 + PLATFORM_BUS_NUM_IRQS
    ```
- add the following line to the `base_memmap` vector:
    ```c
    [VIRT_MULMATR] = { 0x0b000000, 0x00000500 },
    ```
- add the file [virt_mulmatr.c](QEMU_Core/aarch64/virt_mulmatr.c) into `qemu/hw/misc`.

**3.** In `qemu/hw/misc/Makefile.objs`, add the line:
```c
common-obj-y += virt_mulmatr.o
```
in order to define the custom device in the make list.

**4.** Rebuild QEMU if necessary:
```bash
./configure --target-list=aarch64-softmmu --disable-werror
make
```


## 4. Adding the Kernel Driver
**1.** In the directory `buildroot/output/build/linux-x.x.x/drivers/platform`, edit the Makefile to include:
```makefile
obj-y += virt_mulmatr/
```
**2.** Create a directory `virt_mulmatr` with the following files:

- a new Makefile containing: `obj-y += virt_mulmatr.o`;
- the `virt_mulmatr.c` file.

**3.** From the buildroot directory, rebuild the Linux kernel:
```bash
make linux-rebuild
```



## 5. Testing the Setup
**1.** Set the permissions to the root directory on the Guest Machine:
```bash
chmod 777 /root
```

**2.** Mount the guest `root/` directory on the Host Machine:
```bash
sudo mount -o loop /home/.../buildRoot/buildroot/output/images/rootfs.ext4 /mnt/rootfs_mount
```

**3.** Cross-Compiling the test code. Install the cross-compiler with:
```bash
sudo apt-get install gcc-aarch64-linux-gnu
```

**4.** Write Test code [main.c](TEST/aarch64/main.c) and compile:
```bash
nano main.c
aarch64-linux-gnu-gcc -mcpu=cortex-a53 main.c -o main
```

**5.** Copy the test binary to the guest filesystem:
```bash
sudo cp main /mnt/rootfs_mount/root
sudo umount /mnt/rootfs_mount
```

**6.** Running the Test on the Guest

Execute the test binary for your custom device:
```bash
./main_arm -p /sys/bus/platform/devices/b000000.virt_mulmatr/ #(for the multifile driver)
./test_driver_v2 
```

## 6. Step automation with scripts
**1.** Cross-compile the test file and move it into filesystem:
```bash
./script/cc_mnt.sh
```

**2.** Emulate the custom architecture and the kernel Linux:
```bash
./script/run_aarch64.sh
```
