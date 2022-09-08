# Libocli
English | [中文](README.zh_CN.md)

Library of Open Command Line Interface - is a Linux C library / framework for quickly building Cisco style command line programs. The [example directory](example) includes a "democli" program, which demonstates how to use Libocli to implement the "enable", "configure terminal",  "interface", and “show" commands. [Libocli Quick Start Guide](doc/Quick%20Start%20Guide.md) describes the keys steps of how to build a simple "ping" command, including symbol definition, syntax registration, and callback implementation. For more details please refer to the [Libocli API Manual](doc/README.md).

Below are running effect of the "democli" program:

- Keyword auto completion by TAB, '?' to get lexical help, and "man" to get syntax help:  
>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;![image](https://github.com/diggerwoo/blobs/blob/main/img/democli1.gif)

- “enable", "conf t" to change privilege VIEWs, and "no route", "show run" commands: 
>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;![image](https://github.com/diggerwoo/blobs/blob/main/img/democli2.gif)

## How to install
Libocli depends on GNU readline and pcre libraries. Please make sure that both readline and pcre development packages are present in your system before installation, then make as below:
```
make
make install
make demo
```
After making processes, libocli.so and libocli.a will be installed into /usr/local/lib, and library headers will be installed into /usr/local/include/ocli . The "make demo" generates an executable "democli" used in above GIFs in the working directory.

