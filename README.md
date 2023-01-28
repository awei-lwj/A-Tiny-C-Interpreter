# A-Tiny-C-Interpreter

## Introduction

Tiny-Interpreter是一个简易但功能完善的C语言解释器，也是一个能够自举的该项目是对项目[C4](https://github.com/rswier/c4)的重构。

- 实现了自己的虚拟机，仿照x86汇编指令设计并实现了自己的一个指令集，更加深入的了解计算机的工作原理。
- 根据EBNF([Extended Backus-Naur Form](https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form))方法，手写递归下降分析器的方式实现词法/语法分析部分，没有采用常见的Flex/Bison自动工具分别生成词法分析器和语法分析器。
  - [ ] Flex 用来描述 EBNF 中的终结符部分, 也就是描述 token 的形式和种类. 使用正则表达式来描述 token.
  - [ ] Bison 用来描述 EBNF 本身, 其依赖于 Flex 中的终结符描述. 它会生成一个 LALR parser.
- 实现了该项目的主体部分TinyInterpreter的自举。

## How to run the code

Build

~~~shell
gcc -o TinyInterpreter TinyInterpreter.c
~~~

Test

~~~shell
./TinyInterpreter ./test/*.c  // 运行test中任意一个C程序
~~~

自举

~~~shell
./TinyInterpreter TinyInterpreter.c ./test/*.c  // 运行test中任意一个C程序
~~~

## Reference

- [write-a-C-interpreter](https://github.com/lotabout/write-a-C-interpreter)
- [北大编程编译原理实践课](https://pku-minic.github.io/online-doc/#/)
- [斯坦福大学的编程实践课](http://web.stanford.edu/class/cs143/)

## TODO

- 支持解析结构体，完善Comment的分析，加入更多的一个变量类型的解析
- 完善虚拟机和指令集，为语法分析和词法分析提供更多的功能

## 困难点

- 指令集的设计以及相应功能的验证
- 非常难以调试，而且在语法分析和词法分析中存在非常多重复枯燥的内容
- expression()处理中的指针移动
