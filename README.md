# 1. 介绍

本项目在STM32F103C8T6上实现了RISC-V调试spec，可以通过openocd上位机实现对STM32寄存器和内存的读写。

为什么做这个项目？

- 加深对RISC-V调试的理解；
- 为FPGA的实现提供参考；

# 2.使用环境

## 2.1操作系统

Windows 7或以上。

## 2.2开发环境

IAR for ARM 7.0或以上。

# 3.如何使用

首先下载stm32cubef1开发包，可以在STM32官网上下载或者通过百度网盘(https://pan.baidu.com/s/1Rv1Sk7LFY3DCeNwnDAvZ6w ，提取码：mz6t )下载。

下载完成后解压，然后将本项目拷贝到解压后的目录的Projects子目录里。

最后打开IAR工程，重新编译一次即可。

openocd程序可以从[tinyriscv](https://gitee.com/liangkangnan/tinyriscv)里的tools/openocd目录里获得。

