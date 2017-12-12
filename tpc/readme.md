
<!-- TOC -->

- [1. 不正常关闭连接的现象](#1-不正常关闭连接的现象)

<!-- /TOC -->


<a id="markdown-1-不正常关闭连接的现象" name="1-不正常关闭连接的现象"></a>
# 1. 不正常关闭连接的现象

server recv缓冲区有数据时 close socket 直接发送rst分节


不安全释放
```
正常(server recv缓冲区没数据)
11:17:46.509775 IP 127.0.0.1.rfe > 127.0.0.1.54838: Flags [P.], seq 9920445:9920513, ack 1, win 342, options [nop,nop,TS val 450116322 ecr 450116322], length 68
11:17:46.509883 IP 127.0.0.1.rfe > 127.0.0.1.54838: Flags [.], seq 9920513:9985981, ack 1, win 342, options [nop,nop,TS val 450116323 ecr 450116322], length 65468
11:17:46.509922 IP 127.0.0.1.54838 > 127.0.0.1.rfe: Flags [.], ack 9985981, win 10161, options [nop,nop,TS val 450116323 ecr 450116322], length 0
11:17:46.509931 IP 127.0.0.1.rfe > 127.0.0.1.54838: Flags [P.], seq 9985981:9986049, ack 1, win 342, options [nop,nop,TS val 450116323 ecr 450116323], length 68
11:17:46.510077 IP 127.0.0.1.rfe > 127.0.0.1.54838: Flags [FP.], seq 9986049:10000001, ack 1, win 342, options [nop,nop,TS val 450116323 ecr 450116323], length 13952
11:17:46.514676 IP 127.0.0.1.54838 > 127.0.0.1.rfe: Flags [.], ack 10000002, win 20005, options [nop,nop,TS val 450116327 ecr 450116323], length 0

server recv缓冲区有数据
11:18:39.477585 IP 127.0.0.1.54842 > 127.0.0.1.rfe: Flags [.], ack 9871293, win 10214, options [nop,nop,TS val 450169290 ecr 450169290], length 0
11:18:39.477600 IP 127.0.0.1.rfe > 127.0.0.1.54842: Flags [P.], seq 9871293:9887745, ack 11, win 342, options [nop,nop,TS val 450169290 ecr 450169290], length 16452
11:18:39.477712 IP 127.0.0.1.rfe > 127.0.0.1.54842: Flags [.], seq 9887745:9953213, ack 11, win 342, options [nop,nop,TS val 450169290 ecr 450169290], length 65468
11:18:39.477772 IP 127.0.0.1.54842 > 127.0.0.1.rfe: Flags [.], ack 9953213, win 10147, options [nop,nop,TS val 450169290 ecr 450169290], length 0
11:18:39.477795 IP 127.0.0.1.rfe > 127.0.0.1.54842: Flags [P.], seq 9953213:9969665, ack 11, win 342, options [nop,nop,TS val 450169290 ecr 450169290], length 16452
11:18:39.477935 IP 127.0.0.1.rfe > 127.0.0.1.54842: Flags [R.], seq 9969665, ack 11, win 342, options [nop,nop,TS val 0 ecr 450169290], length 0
```


安全释放,shutdown + read 返回0 + close
```
11:21:19.215456 IP 127.0.0.1.rfe > 127.0.0.1.54846: Flags [.], seq 9818673:9884141, ack 11, win 342, options [nop,nop,TS val 450329028 ecr 450329028], length 65468
11:21:19.215509 IP 127.0.0.1.rfe > 127.0.0.1.54846: Flags [.], seq 9884141:9949609, ack 11, win 342, options [nop,nop,TS val 450329028 ecr 450329028], length 65468
11:21:19.215615 IP 127.0.0.1.54846 > 127.0.0.1.rfe: Flags [.], ack 9949609, win 308, options [nop,nop,TS val 450329028 ecr 450329028], length 0
11:21:19.215782 IP 127.0.0.1.54846 > 127.0.0.1.rfe: Flags [.], ack 9949609, win 1330, options [nop,nop,TS val 450329028 ecr 450329028], length 0
11:21:19.215817 IP 127.0.0.1.rfe > 127.0.0.1.54846: Flags [FP.], seq 9949609:10000001, ack 11, win 342, options [nop,nop,TS val 450329028 ecr 450329028], length 50392
11:21:19.256322 IP 127.0.0.1.54846 > 127.0.0.1.rfe: Flags [.], ack 10000002, win 18982, options [nop,nop,TS val 450329069 ecr 450329028], length 0
```