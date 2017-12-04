
<!-- TOC -->

- [1. 编译](#1-编译)
- [2. 非阻塞I/O写TTCP服务端时联想到状态机](#2-非阻塞io写ttcp服务端时联想到状态机)
- [3. libev 非阻塞I/O TTCP服务端的问题](#3-libev-非阻塞io-ttcp服务端的问题)
    - [3.1. 死锁问题](#31-死锁问题)
    - [3.2. 性能差问题](#32-性能差问题)
- [4. libevent bufferevent机制 测试](#4-libevent-bufferevent机制-测试)
- [5. 其他相关](#5-其他相关)
- [6. 相关库](#6-相关库)
- [7. 测试ttcp](#7-测试ttcp)
- [8. 调试](#8-调试)
- [9. 测试rot13](#9-测试rot13)
- [10. rot13抓包](#10-rot13抓包)

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

<a id="markdown-2-非阻塞io写ttcp服务端时联想到状态机" name="2-非阻塞io写ttcp服务端时联想到状态机"></a>
# 2. 非阻塞I/O写TTCP服务端时联想到状态机

* https://blog.codingnow.com/2006/01/aeeieaiaeioeacueoe.html

如果使用阻塞I/O的话(或者协程?),条理是非常清晰的,先收8字节包头信息,再收一个一个收message,但是到了非阻塞IO的话,每次收到的是不明确长度的流,每次都要从流中取到信息,保存成一个状态,下次再收到数据时,根据状态选择去做什么操作.

如果不用状态机去操作的话,那么代码的可读性不会很高.试想应该有如下状态:

* 正在等待包头8字节状态
* 正在等待整个message (4字节长度 + payload)

<a id="markdown-3-libev-非阻塞io-ttcp服务端的问题" name="3-libev-非阻塞io-ttcp服务端的问题"></a>
# 3. libev 非阻塞I/O TTCP服务端的问题


<a id="markdown-31-死锁问题" name="31-死锁问题"></a>
## 3.1. 死锁问题
如果收的数据包太大的话,例如内核缓冲区大于1MB时,才收取一次数据,可能会导致发送方send阻塞,因为send方内核缓冲区已经满了,recv方内核缓冲区不到1MB数据,达不到收的状态. 试想这种状况在LENGTH多大时会发生?  
? 不对啊,recv方缓冲区为什么会不到1MB?这句话有问题把


知道了,我的做法是在应用层没有缓冲区,所以数据全缓存到内核缓冲区了,没成功交给应用层

内核缓冲区最大是`972544`吗`./ttcp_test --trans --port 5001 --length 972540 --number 8192` ,972540 + 包头4 能成功收发包了

```
Active Internet connections (w/o servers)
Proto Recv-Q Send-Q Local Address           Foreign Address         State       PID/Program name
tcp        0 3508616(发) 127.0.0.1:36252         127.0.0.1:5001          ESTABLISHED 14279/./ttcp_test
tcp   (收)972544      0 127.0.0.1:5001          127.0.0.1:36252         ESTABLISHED 14273/ttcp_libev_te

```

<a id="markdown-32-性能差问题" name="32-性能差问题"></a>
## 3.2. 性能差问题
猜测是系统调用的问题哈,使用`valgrind`看看

参考陈硕的书后发现,是因为没有应用层缓冲区,而epoll用的是电平触发,导致了`busy_loop`

./ttcp_test --trans --port 5003 --length 972540 --number 8192
host = 127.0.0.1, port = 5003

3081.452 Mib/s


./ttcp_test --trans --port 5001 --length 972540 --number 8192
host = 127.0.0.1, port = 5001

5065.302 Mib/s

测试工具:
* https://github.com/jrfonseca/gprof2dot (pip安装,生成图片用)
* http://valgrind.org/docs/manual/cl-manual.html (valgrind和callgrind)
* http://www.thegeekstuff.com/2012/08/gprof-tutorial (gprof)
* http://kcachegrind.sourceforge.net/html/Home.html (qt写的,配合桌面端用)

![](http://ouxarji35.bkt.clouddn.com/valgrind.png)

```bash
# 启动进程
valgrind --tool=callgrind --trace-children=yes ./ttcp_libev_test -r --port 5001

# 打印当前栈回溯
callgrind_control -e -b

# 显示每个函数的调用次数
callgrind_annotate ./callgrind.out.24988

# 生成dot
gprof2dot -f callgrind -s callgrind.out.24988 > valgrind.dot

# dot转换成png
dot -Tpng valgrind.dot -o /tmp/valgrind.png

# gprof当程序exit时生成
gmon.out

# 生成analyze报告(依赖二进制文件)
gprof ./ttcp_libevent_test gmon.out > gprof.txt

# 生成dot
gprof2dot -s gprof.txt > gprof.dot 

# 生成png
dot -Tpng gprof.dot -o /tmp/gprof.png
```

<a id="markdown-4-libevent-bufferevent机制-测试" name="4-libevent-bufferevent机制-测试"></a>
# 4. libevent bufferevent机制 测试
```bash
# 开启监听 设置`bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);`为16384
./ttcp_libevent_test -r

# 发送3OMB数据
dd if=/dev/zero bs=1M count=30 | nc -C  localhost 5001

# 可以看到发送内核缓冲区被填满了,接收内核缓冲区被填满了(但是不知道这个是哪个内核参数)
Active Internet connections (w/o servers)
Proto Recv-Q Send-Q Local Address           Foreign Address         State       PID/Program name
tcp        0 2767476 127.0.0.1:36946         127.0.0.1:5001          ESTABLISHED 51172/nc
tcp   968076      0 127.0.0.1:5001          127.0.0.1:36946         ESTABLISHED 51166/./ttcp_libeve
```

<a id="markdown-5-其他相关" name="5-其他相关"></a>
# 5. 其他相关

```bash
# 生成debug makefile(默认release)
cmake -DCMAKE_BUILD_TYPE=Debug .

# 查看cmake生成的链接选项
find . -type f -name 'link.txt' -print0 | xargs -0  grep -in '/usr/bin/'

# 查看cmake生成的编译选项
grep -r '\-Wno\-unused\-parameter'

# 好像最终的编译选项在下面的二进制文件里
./CMakeFiles/2.8.12.2/CompilerIdCXX/a.out
```

<a id="markdown-6-相关库" name="6-相关库"></a>
# 6. 相关库
* https://cmake.org/cmake/help/v3.0/module/FindBoost.html

<a id="markdown-7-测试ttcp" name="7-测试ttcp"></a>
# 7. 测试ttcp
```

while true; do ./ttcp_test --recv --port 5000; done

./ttcp_test --trans --port 5000 --length 65536 --number 8192

```

<a id="markdown-8-调试" name="8-调试"></a>
# 8. 调试
```
> /dev/null 2>&1 &
kill $(jobs -p)
cgdb list_timer 
b main
r 0.0.0.0 3000

gdb --tui ./ttcp_test --args ./ttcp_test -t --host 1.1

```

<a id="markdown-9-测试rot13" name="9-测试rot13"></a>
# 9. 测试rot13
```
tcpdump -XX -i lo port 40713
tcpdump -i lo port 40713

nc 127.0.0.1 40713

# echo默认自己会输出一个\n
echo -e '123456\n' | nc 127.0.0.1 40713

printf '123456' | nc 127.0.0.1 40713
```


<a id="markdown-10-rot13抓包" name="10-rot13抓包"></a>
# 10. rot13抓包
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