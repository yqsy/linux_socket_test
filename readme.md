
<!-- TOC -->

- [1. 编译](#1-编译)
- [调试](#调试)

<!-- /TOC -->


<a id="markdown-1-编译" name="1-编译"></a>
# 1. 编译

```bash
g++ check_byte_order.cpp -o check_byte_order
g++ back_log.cpp -o back_log
g++ dup_cgi.cpp -o dup_cgi
g++ test_signal.cpp -o test_signal -std=c++11 -lpthread
g++ timer_singal_test.cpp -o timer_singal_test -std=c++11 -lpthread
g++ list_timer.cpp -g -o list_timer

g++ hello_world.cpp -g -levent -o hello_world
```

<a id="markdown-调试" name="调试"></a>
# 调试
```
cgdb list_timer 
b main
r 0.0.0.0 3000
```
