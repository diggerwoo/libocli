# 2. Libocli Symbol Definition and Callback Arguments

English | [中文](Symbol%20Definition.zh_CN.md)
<br>
[<< Upper Level](README.md)  

Author: Digger Wu (digger.wu@linkbroad.com)

In the section of [Quick Start Guide](Quick%20Start%20Guide.md) we introduce that the first step to build a command is to build a symbol table. A symbol table is an array of struct (symbol_t). Initializing such an array could be tedious. Libocli provides couple of macros in order to optimize the symbol definition/initilization. Hopefully that can be helpful to ease the code and improve the readability. Please refer to [ocli.h](../src/ocli.h) to understand the related structures and macros.

## 2.1 Define a Symbol

The macros used to define a symbol are listed below:
| Name | Description | Parameters |
| :--- | :--- | :--- |
| DEF_KEY | Define a symbol of keyword type | (sym_name, help_text) |
| DEF_KEY_ARG | Define a symbol of keyword type with callback arg | (sym_name, help_text, arg_name) |
| DEF_VAR | Define a symbol of variable type | (sym_name, help_text, lex_type, arg_name) |
| DEF_VAR_RANGE | Define a symbol of ranged varible type | (sym_name, help_text, lex_type, arg_name, min_val, max_val) |

Key points and  examples:
- The first two parameters of all DEF_ macros are alway sym_name and help_text and both are of string type. The help_text is used to display help information when user pressing '?'. For example, define the first keyword "ping":
  > 
  ```
  DEF_KEY ("ping", "Ping utility")
  ```
- DEF_VAR must have a lex_type parameter, e.g. LEX_IP_ADDR, EX_DOMAIN_NAME, or LEX_INT. For details of lexcical types please refer to the netxt section [Libocli Lexcical Parsing Interface](Lexical%20Parsing.md).


- All DEF_ macros except DEF_KEY should have a arg_name parameter that is the name of the callback argument. The arg_name is a string too. In the example we use ARG macro to define a arg_name. E.g. ARG(DST_HOST) is exactly the "DST_HOST" .

  ```  
  /* The { HOST_IP | HOST } in the ping syntax are an alternative, so they can use
   * the same arg_name "DST_HOST", that can simplify the callback process */
  DEF_VAR ("HOST_IP", "Destination IP address", LEX_IP_ADDR, ARG(DST_HOST)),  
  DEF_VAR ("HOST", "Destination domain name", LEX_DOMAIN_NAME, ARG(DST_HOST))
  ```
- DEF_VAR_RANGE must have a lex_type of either LEX_INT or LEX_DECIMAL, and gives the min and max value. Below example limit the the count of request between 1 to 100.
  >
  ```
  DEF_VAR_RANGE	("COUNT", "<1-100> count of requests", LEX_INT, ARG(REQ_COUNT), 1, 100)
  ```

## 2.2 Passing Callback Arguments

When a command line is parsed successfully, an pointer to array of cmd_arg_t will be passed to the callback function. The cmd_arg_t is actually a name value pair as defined below.
```
typedef struct cmd_arg {
	char	*name;		/* arg name */
	char	*value;		/* arg value */
} cmd_arg_t;
```
For example you input a ping command line:
>ping -c 5 www.bing.com

The parsing/matching process of Libocli will be:  
1. "ping" matches the symbol "ping", no callback argument.
2. “-c" matches the symbol "-c", no callback argument.
3. "5" matches symbol "COUNT" with LEX_INT type and passes the range check 1 < 5 < 100. The symbol has callback argument "REQ_COUNT". So the first element cmd_arg[0] is generated with name="REQ_COUNT" and value="5".
4. “www.bing.com” matches symbol "HOST" with LEX_DOMAIN_NAME type, The symble has callback argument "DST_HOST", so the second element cmd_arg[1] is gernerated with name="DST_HOST" and value="www.bing.com".

Then the pointer to cmd_arg[] will be passed to the callback function cmd_ping(). By calling for_each_cmd_arg(cmd_arg, i, name, value) to iterate the cmd_arg, cmd_ping() gets all the callback arguments and saves them into local variable req_count and dst_host[].

```
	for_each_cmd_arg(cmd_arg, i, name, value) {
		if (IS_ARG(name, REQ_COUNT))
			req_count = atoi(value);
		else if (IS_ARG(name, DST_HOST))
			strncpy(dst_host, value, sizeof(dst_host)-1);
		/* ... */
	}
```

In the above code, macro IS_ARG is used to compare the argument name. IS_ARG(name, REQ_COUNT) is exactly (strcmp(name, "REQ_COUNT") == 0). Hopefully the ARG and IS_ARG macros can ease the code and improve the readabilty.
