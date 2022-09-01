# Libocli
Libocli is a Linux C library / framework which is helpful for quickly building Cisco style command line interface. A example named "democli" is also incuded to demonstrates the capability of libocli as below.

Keywords auto completion, "man" to get syntax help:  
![image](https://github.com/diggerwoo/blobs/blob/main/img/democli1.gif)

Multi-view support, "no" & "show" examples:  
![image](https://github.com/diggerwoo/blobs/blob/main/img/democli2.gif)

## How to install:
Libocli depends on GNU readline and pcre libraries. Please make sure both realine and pcre development packages are installed in your system. The Building and installation processes are quite straightforward as below:
```
make
make install
make demo
```
After making processes, libocli.so and libocli.a will be installed into /usr/local/lib, library headers will be installed into /usr/local/include/ocli, and a "democli" program which shown in the above videos will be gernerated inside the working directory.

For details please refer to the API manual in the doc directory.
