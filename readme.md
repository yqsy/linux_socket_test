
<!-- TOC -->

- [1. 编译](#1-编译)
- [2. 调试](#2-调试)
- [3. 测试rot13](#3-测试rot13)
- [4. rot13抓包](#4-rot13抓包)

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

g++ hello_world.cpp -g -levent -o hello_world
g++ low_level_rot13_server.cpp -g -levent -o low_level_rot13_server
g++ simper_rot13_server.cpp -g -levent -o simper_rot13_server
```

<a id="markdown-2-调试" name="2-调试"></a>
# 2. 调试
```
> /dev/null 2>&1 &
kill $(jobs -p)
cgdb list_timer 
b main
r 0.0.0.0 3000
```

<a id="markdown-3-测试rot13" name="3-测试rot13"></a>
# 3. 测试rot13
```
tcpdump -XX -i lo port 40713
tcpdump -i lo port 40713

nc 127.0.0.1 40713

# echo默认自己会输出一个\n
echo -e '123456\n' | nc 127.0.0.1 40713

printf '123456' | nc 127.0.0.1 40713
```


<a id="markdown-4-rot13抓包" name="4-rot13抓包"></a>
# 4. rot13抓包
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