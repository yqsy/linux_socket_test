
<!-- TOC -->

- [1. 编译](#1-编译)
- [2. 其他相关](#2-其他相关)
- [3. 相关库](#3-相关库)
- [4. 测试ttcp](#4-测试ttcp)
- [5. 调试](#5-调试)
- [6. 测试rot13](#6-测试rot13)
- [7. rot13抓包](#7-rot13抓包)

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
g++ fork_test.cpp -g -o fork_test
g++ create_zombine_process.cpp -g -o create_zombine_process
g++ deadlock.cpp -g -lpthread -o deadlock
g++ half_sync_half_async.cpp -g -o half_sync_half_async

g++ timer_test.cpp -g -lev -o timer_test

g++ hello_world.cpp -g -levent -o hello_world
g++ low_level_rot13_server.cpp -g -levent -o low_level_rot13_server
g++ simper_rot13_server.cpp -g -levent -o simper_rot13_server

g++ ttcp_test.cpp -std=c++11 -lboost_program_options -g -o ttcp_test
```

<a id="markdown-2-其他相关" name="2-其他相关"></a>
# 2. 其他相关

```bash
# 查看cmake生成的链接选项
find . -type f -name 'link.txt' -print0 | xargs -0  grep -in '/usr/bin/c++'

# 查看cmake生成的编译选项
find . -type f -name '*' -print0 | xargs -0  grep -in 'build flags'

# 好像最终的编译选项在下面的二进制文件里
./CMakeFiles/2.8.12.2/CompilerIdCXX/a.out
```

<a id="markdown-3-相关库" name="3-相关库"></a>
# 3. 相关库
* https://cmake.org/cmake/help/v3.0/module/FindBoost.html

<a id="markdown-4-测试ttcp" name="4-测试ttcp"></a>
# 4. 测试ttcp
```

while true; do ./ttcp_test --recv --port 5000; done

./ttcp_test --trans --port 5000 --length 65536 --number 8192

```

<a id="markdown-5-调试" name="5-调试"></a>
# 5. 调试
```
> /dev/null 2>&1 &
kill $(jobs -p)
cgdb list_timer 
b main
r 0.0.0.0 3000

gdb --tui ./ttcp_test --args ./ttcp_test -t --host 1.1

```

<a id="markdown-6-测试rot13" name="6-测试rot13"></a>
# 6. 测试rot13
```
tcpdump -XX -i lo port 40713
tcpdump -i lo port 40713

nc 127.0.0.1 40713

# echo默认自己会输出一个\n
echo -e '123456\n' | nc 127.0.0.1 40713

printf '123456' | nc 127.0.0.1 40713
```


<a id="markdown-7-rot13抓包" name="7-rot13抓包"></a>
# 7. rot13抓包
```
# printf '123456\n' | nc 127.0.0.1 40713

09:51:46.700508 IP localhost.51966 > localhost.40713: Flags [S], seq 3344154490, win 43690, options [mss 65495,sackOK,TS val 122305741 ecr 0,nop,wscale 7], length 0
04:25:59.168535 IP localhost.40713 > localhost.51966: Flags [S.], seq 3558941313, ack 3344154491, win 43690, options [mss 65495,sackOK,TS val 122305741 ecr 122305741,nop,wscale 7], length 0
09:51:46.700580 IP localhost.51966 > localhost.40713: Flags [.], ack 1, win 342, options [nop,nop,TS val 122305741 ecr 122305741], length 0

09:51:46.700649 IP localhost.51966 > localhost.40713: Flags [P.], seq 1:8, ack 1, win 342, options [nop,nop,TS val 122305741 ecr 122305741], length 7
09:51:46.700657 IP localhost.40713 > localhost.51966: Flags [.], ack 8, win 342, options [nop,nop,TS val 122305741 ecr 122305741], length 0

# 发完7个字节就发FIN关闭连接
09:51:46.700669 IP localhost.51966 > localhost.40713: Flags [F.], seq 8, ack 1, win 342, options [nop,nop,TS val 122305741 ecr 122305741], length 0

# 给到应答
09:51:46.700680 IP localhost.40713 > localhost.51966: Flags [P.], seq 1:8, ack 8, win 342, options [nop,nop,TS val 122305741 ecr 122305741], length 7
# 应答ack
09:51:46.700708 IP localhost.51966 > localhost.40713: Flags [.], ack 8, win 342, options [nop,nop,TS val 122305741 ecr 122305741], length 0

# 挥手应答
09:51:46.743950 IP localhost.40713 > localhost.51966: Flags [.], ack 9, win 342, options [nop,nop,TS val 122305784 ecr 122305741], length 0

# 服务器FIN
09:52:02.920125 IP localhost.40713 > localhost.51966: Flags [F.], seq 8, ack 9, win 342, options [nop,nop,TS val 122321960 ecr 122305741], length 0
09:52:02.920140 IP localhost.51966 > localhost.40713: Flags [.], ack 9, win 342, options [nop,nop,TS val 122321960 ecr 122321960], length 0
```

```
# printf '123456\n123456\n' | nc 127.0.0.1 40713

10:07:59.736814 IP localhost.51976 > localhost.40713: Flags [S], seq 2137327003, win 43690, options [mss 65495,sackOK,TS val 123278777 ecr 0,nop,wscale 7], length 0
13:18:37.862220 IP localhost.40713 > localhost.51976: Flags [S.], seq 756520678, ack 2137327004, win 43690, options [mss 65495,sackOK,TS val 123278777 ecr 123278777,nop,wscale 7], length 0
10:07:59.736850 IP localhost.51976 > localhost.40713: Flags [.], ack 1, win 342, options [nop,nop,TS val 123278777 ecr 123278777], length 0

# 发送14个字节
10:07:59.736912 IP localhost.51976 > localhost.40713: Flags [P.], seq 1:15, ack 1, win 342, options [nop,nop,TS val 123278777 ecr 123278777], length 14
10:07:59.736920 IP localhost.40713 > localhost.51976: Flags [.], ack 15, win 342, options [nop,nop,TS val 123278777 ecr 123278777], length 0

# 发完马上就关闭nc自己的发送管道(意思是这样)
10:07:59.736933 IP localhost.51976 > localhost.40713: Flags [F.], seq 15, ack 1, win 342, options [nop,nop,TS val 123278777 ecr 123278777], length 0

# 应答直接在一个packet里面
10:07:59.737032 IP localhost.40713 > localhost.51976: Flags [P.], seq 1:15, ack 16, win 342, options [nop,nop,TS val 123278777 ecr 123278777], length 14
10:07:59.737060 IP localhost.51976 > localhost.40713: Flags [.], ack 15, win 342, options [nop,nop,TS val 123278777 ecr 123278777], length 0

10:07:59.737081 IP localhost.40713 > localhost.51976: Flags [F.], seq 15, ack 16, win 342, options [nop,nop,TS val 123278777 ecr 123278777], length 0
10:07:59.737086 IP localhost.51976 > localhost.40713: Flags [.], ack 16, win 342, options [nop,nop,TS val 123278777 ecr 123278777], length 0
```