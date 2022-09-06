# 5. Readline Control Interface

English | [中文](Wrapped%20Readline.zh_CN.md)
<br>
[<< 上一级目录](README.md)  

Author: Digger Wu (digger.wu@linkbroad.com)

This section lists Libocli's command line control interfaces. Most of the functions are named with the ocli_rl_ prefix, and appear in main() of [democli.c](../example/democli.c).


1. Initialize GNU Readline and Libocli runtime environments, and save current terminal settings.
    ```c
    int ocli_rl_init(void);
    ```
2. Get or set current Libocli VIEW.
    ```c
    extern int ocli_rl_get_view(void);
    extern void ocli_rl_set_view(int view);
    ```
3. Set the realine prompt.
    ```c
    void ocli_rl_set_prompt(char *prompt);
    ```
4. Set the terminal idle timeout in seconds. If the idle timeout reaches, the execution of ocli_rl_loop() will be terminated. The default terminal timeout is 300 seconds. If this feature is not required, call ocli_rl_set_timeout(0) after ocli_rl_init() .
    ```c
    void ocli_rl_set_timeout(int sec);
    ```
5. Set the equivalent command of CTRL-D. In the democli example we set it "exit" like bash does.
    ```c
    void ocli_rl_set_eof_cmd(char *cmd);
    ```
6. Enable or disable TAB auto completion and '?' auto help. The ocli_rl_loop() will automatically enable this.
    ```c
    int ocli_rl_set_auto_completion(int enabled);
    ```
7. Start the command line reading loop until the global variable **ocli_rl_finished** being set to 1 .
    ```c
    extern int ocli_rl_finished;
    void ocli_rl_loop(void);
    ```   
8. Free Libocli runtime resources and restore the terminal settings before ocli_rl_init() call.
    ```c
    void ocli_rl_exit(void);
    ```
9. This is a wrapped GNU readline() with auto completion totally disabled, and it resumes auto comepletion after realine() calls ends. This is useful in Libocli context when you need user to input something like username, or to confirm "Yes / No".
    ```c
    char *read_bare_line(char *prompt);
    ```
10. This is a wrapped GNU readline() with terminal ECHO and auto completion disabled, and it resumes auto comepletion after realine() calls ends. This is useful in Libocli context when you need user to input password.
    ```c
    char *read_password(char *prompt);
    ```

