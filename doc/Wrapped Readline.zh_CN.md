# 5. Libocli 命令行控制接口

中文 | [English](Wrapped%20Readline.zh_CN.md)
<br>
[<< 上一级目录](README.zh_CN.md)  

作者：Digger Wu (digger.wu@linkbroad.com)

本节罗列了 Libocli 的命令行控制接口函数，多数都以 ocli_rl_ 前缀命名，大多数函数中都在 [democli.c](../example/democli.c) 的 main() 中出现过。

1. 初始化函数，用于初始化 GNU Readline 和 Libocli 运行时环境，并保存当前的终端特性设置。
    ```
    int ocli_rl_init(void);
    ```
2. 读取和设置 Libocli 权限视图。
    ```
    extern int ocli_rl_get_view(void);
    extern void ocli_rl_set_view(int view);
    ```
3. 设置终端提示字符串。
    ```
    void ocli_rl_set_prompt(char *prompt);
    ```
4. 设置终端空闲超时秒数，若空闲达到此时长，则会中断 ocli_rl_loop() 执行。Libocli 默认终端超时为 180秒。若不需要此特性，初始化后调用 ocli_rl_set_timeout(0); 
    ```
    void ocli_rl_set_timeout(int sec);
    ```
5. 设置 CTRL-D 的等效命令，如果类似 bash 将 CTRL-D 等效 "exit" 命令，那么在注册了 "exit" 命令和语法后，调用 ocli_rl_set_eof_cmd("exit");
    ```           
    void ocli_rl_set_eof_cmd(char *cmd);
    ```
6. 使能或禁用 TAB 键语法自动补齐 和 '?' 自动帮助提示，ocli_rl_loop() 会自动使能自动补齐。
    ```
    int ocli_rl_set_auto_completion(int enabled);
    ```
7. 启动命令行读取循环，直到全局变量 ocli_rl_finished 变量被设置为 1 。
    ```
    extern int ocli_rl_finished;
    void ocli_rl_loop(void);
    ```   
8. 释放 Libocli 资源，并恢复 ocli_rl_init() 被调用前的终端设置。
    ```
    void ocli_rl_exit(void);
    ```
9. 从终端读取一行文本，读取过程中禁用 TAB 自动补齐 和 '?' 自动帮助提示；读取完毕后则恢复TAB 自动补齐 和 '?' 自动帮助提示。典型应用场景，在需要用户输入用户名，或对某提示确认 yes / no 时，调用此函数。
    ```
    char *read_bare_line(char *prompt);
    ```
10. 从终端读取密码，读取过程中禁用 TAB 自动补齐 和 '?' 自动帮助提示，自动关闭终端回显；读取完毕后则恢复TAB 自动补齐 和 '?' 自动帮助提示，恢复终端回显。
    ```
    char *read_password(char *prompt);
    ```



