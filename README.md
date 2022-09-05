# Libocli
English | [中文](README.zh_CN.md)

Library of Open Command Line Interface - is a Linux C library / framework for quickly building Cisco style command line programs. The [example directory](example) includes a "democli" program, which demonstates how to use Libocli to implement the "enable",  "configure terminal", "ping", “show", and "route" commands. [Libocli Quick Start Guide](doc/Quick%20Start%20Guide.md) describes the keys steps of how to build a simple "ping" command, including symbol definition, syntax registration, and callback implementation. For details please refer to the [Libocli API Manual](doc/README.md).

Below GIFs demonstate the runnning effect of the example program "democli".

- Keywords auto completion by TAB, lexical help by '?',  and syntax help by builtin "man" command:  
>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;![image](https://github.com/diggerwoo/blobs/blob/main/img/democli1.gif)

- “enable", "conf t" to change priority VIEWs, and "no" / "show" commands: 
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

