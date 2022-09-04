# Libocli
中文 | [English](README.md)

Library of Open Command Line Interface - 是一个开源 Linux C 库 / 框架，可用于快速构建具有 Cisco 风格的命令行程序。
[example 目录](example) 中包含了一个 "democli" 例子程序，演示了如何利用 Libocli 实现 "enable", "configure terminal", "ping", “show", "route" 等命令。
[Libocli 快速入门指南](doc/Quick%20Start%20Guide.zh_CN.md) 以一个简单的 ping 命令为例，讲解了构建命令行所需的几个关键步骤，包括符号表定义，语法注册，和回调函数实现。

”democli" 例子程序运行效果如下：
- TAB 键自动补齐，'?' 键获得本词和下一词的帮助，内建的 "man" 命令用于获得一个命令的整体语法概貌：
>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;![image](https://github.com/diggerwoo/blobs/blob/main/img/democli1.gif)

- "enable" 和 "conf t" 多层权限视图支持, 以及 "no" 和 "show" 命令的演示：
>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;![image](https://github.com/diggerwoo/blobs/blob/main/img/democli2.gif)

## 编译和安装
Libocli 依赖于 GNU readline 和 pcre 库。编译安装前请确认系统中已安装 readline 和 pcre 开发包和头文件，之后在工作目录中执行如下 make 序列：
```
make
make install
make demo
```
编译安装完毕后，库文件 libocli.so 和 libocli.a 会被安装到 /usr/local/lib 目录，头文件会被安装到 /usr/local/include/ocli 目录，
make demo 会在工作目录中生成上面演示视频所用的 "democli" 可执行文件。

详细开发资料请参考 doc 目录下的 [Libocli API 手册](doc/README.zh_CN.md)。
