# Libocli
English | [中文](README.zh_CN.md)

Library of Open Command Line Interface - is a Linux C library / framework which is helpful for the fast building of Cisco style command line interface. A example named "democli" is also included to simply demonstrates the features of libocli as below.

- Keywords auto completion by TAB, lexical help by '?',  and syntax help by builtin "man" command:  
>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;![image](https://github.com/diggerwoo/blobs/blob/main/img/democli1.gif)

- “enable", "conf t" to change priority VIEWs, and "no" & "show" examples: 
>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;![image](https://github.com/diggerwoo/blobs/blob/main/img/democli2.gif)

## How to install:
Libocli depends on GNU readline and pcre libraries. Please make sure that both readline and pcre development packages are present in your system before installation, then make as below:
```
make
make install
make demo
```
After making processes, libocli.so and libocli.a will be installed into /usr/local/lib, and library headers will be installed into /usr/local/include/ocli . The "make demo" generates an executable file "democli" shown up in above vidoes in the working directory.

For details please refer to the API manual in the doc directory.
