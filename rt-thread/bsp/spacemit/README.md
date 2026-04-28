# 1. 环境准备

编译环境为WSL2下的Ubuntu22.04。

``` shell
$ lsb_release -a
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 22.04.5 LTS
Release:        22.04
Codename:       jammy
```
## 1.1 交叉编译工具链设置
和spacemit-k3-rcpu-fw相同即可

# 2.编译内核
K3两个rt24的实时核需要分开编译，必要的环境变量设置等操作已经在`SConstruct`写好了，直接编译即可：
``` shell
chuan@localhost:~/oerv-bsp/spacemit-k3-rcpu-fw/components/esos-lite/rt-thread/bsp/spacemit$ scons
scons: Reading SConscript files ...
scons: done reading SConscript files.
scons: Building targets ...
scons: building associated VariantDir targets: build
LINK esos_lite-rt24.elf
/home/chuan/spacemit-k3-rcpu-fw/tools/toolchain/spacemit-toolchain-elf-newlib-x86_64-v1.0.9/bin/../lib/gcc/riscv64-unknown-elf/14.2.1/../../../../riscv64-unknown-elf/bin/ld: warning: esos_lite-rt24.elf has a LOAD segment with RWX permissions
riscv64-unknown-elf-objcopy -O binary esos_lite-rt24.elf esos_lite.bin
riscv64-unknown-elf-size esos_lite-rt24.elf
   text    data     bss     dec     hex filename
  19040     184   16026   35250    89b2 esos_lite-rt24.elf
```
这种rtt标准构建方式，最终得到了`esos_lite.bin`这个内核文件。这个产物被cp到上层`spacemit-k3-rcpu-fw`仓库的bsp目录下面，上层内核构建会自动调用