# Libocli
Libocli is a C library to provide Cisco style command line interface. It is developed on Linux and depends on GNU readline and pcre libraries. The project also incudes an example which demonstrates the capability of libocli as below.

Keywords auto completion, "man" to get syntax help:  
![image](https://github.com/diggerwoo/blobs/blob/main/img/democli1.gif)

Multi-view support, "no" & "show" examples:  
![image](https://github.com/diggerwoo/blobs/blob/main/img/democli2.gif)

How to build and install:
```
make
make install
make demo
```
After making process, the libocli.so and libocli.a will be installed into /usr/local/lib, library headers will be installed into /usr/local/include/ocli, and a "democli" program which demonstrates the videos above will be gernerated inside the work directory.

For details of Libocli development please refer to the manual in doc directory.
