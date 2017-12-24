<!-- TOC -->

- [1. 资料](#1-资料)
    - [1.1. cgi fastcgi wsgi](#11-cgi-fastcgi-wsgi)
    - [1.2. 知乎](#12-知乎)
    - [1.3. 可以参考开源项目](#13-可以参考开源项目)
- [2. 流程](#2-流程)
- [3. 简单的报文](#3-简单的报文)
- [4. 我认为的难点](#4-我认为的难点)
- [5. benchmark](#5-benchmark)
- [6. 调试语句](#6-调试语句)

<!-- /TOC -->




<a id="markdown-1-资料" name="1-资料"></a>
# 1. 资料

* https://stackoverflow.com/questions/176409/build-a-simple-http-server-in-c (stackoverflow)
* https://tools.ietf.org/html/rfc2616 (rfc)
* https://en.wikipedia.org/wiki/Berkeley_sockets (bsd sockets)
* https://sourceforge.net/projects/tinyhttpd/ (tiny httpd)
* http://man7.org/tlpi/code/online/dist/sockets/read_line.c.html (纯c的比较难写,readline)
* https://www.jmarshall.com/easy/http/ (简单的http例子)
* https://en.wikipedia.org/wiki/Escape_sequences_in_C
* http://www.asciitable.com/ (ascii表)
* https://www.zhihu.com/question/32249717/answer/55255166 (知乎)
* http://www.apuebook.com/ (apue看一下)
* https://github.com/shihyu/Linux_Programming/blob/master/books/Advanced.Programming.in.the.UNIX.Environment.3rd.Edition.0321637739.pdf (apue 卷3)
* https://github.com/shihyu/Linux_Programming/blob/master/books/UNIX%20Network%20Programming(Volume1%2C3rd).pdf (apue 卷1)
* https://stackoverflow.com/questions/3135307/send-post-request-with-netcat (使用netcat发送post)

<a id="markdown-11-cgi-fastcgi-wsgi" name="11-cgi-fastcgi-wsgi"></a>
## 1.1. cgi fastcgi wsgi

* http://blog.51reboot.com/cgi-fastcgi-wsgi/ (cgi fastcgi wsgi)
* https://www.jmarshall.com/easy/cgi/ (cgi)
* https://en.wikipedia.org/wiki/Web_Server_Gateway_Interface (wsgi)
* https://en.wikipedia.org/wiki/FastCGI (fastcgi)

<a id="markdown-12-知乎" name="12-知乎"></a>
## 1.2. 知乎
比如你做 web开发，你选一门语言，python，语言就做好语言的事情。外部的网络框架，可以用 django，flask, web.py等等，接口可以用 fastcgi / cgi / wsgi / uwsgi / apache_mod, 而外部的服务器，可以用 apache, nginx, lighttpd。清晰的被分成：语言层、框架层、协议层、服务层 四个不同的层次，每个层次若干备选方案，互相兼容，web.py过时了，我换 flask，apache过时了，我换nginx。每个产品都专注做好自己的事情，并前后适配其他层次的方案，python出问题了，我换 ruby，换php，协议任然用 apache_mod或者 fastcgi，这就是每个层次都可以替换的设计。


<a id="markdown-13-可以参考开源项目" name="13-可以参考开源项目"></a>
## 1.3. 可以参考开源项目

* https://github.com/lighttpd/lighttpd2 (libev + 链表缓冲区)
* https://github.com/apache/httpd (31w行代码)
* https://sourceforge.net/projects/tinyhttpd/ (read从内核缓冲区拷贝单字节...吞吐量?)

<a id="markdown-2-流程" name="2-流程"></a>
# 2. 流程

1. Get your basic TCP sockets layer running (listen on port/ports, accept client connections and send/receive data).
2. Implement a buffered reader so that you can read requests one line (delimited by CRLF) at a time.
3. Read the very first line. Parse out the method, the request version and the path.
4. Implement header parsing for the "Header: value" syntax. Don't forget unfolding folded headers.
5. Check the request method, content type and content size to determine how/if the body will be read.
6. Implement decoding of content based on content type.
7. If you're going to support HTTP 1.1, implement things like "100 Continue", keep-alive, chunked transfer.
8. Add robustness/security measures like detecting incomplete requests, limiting max number of clients etc.
9. Shrink wrap your code and open-source it :)


<a id="markdown-3-简单的报文" name="3-简单的报文"></a>
# 3. 简单的报文

```
GET /123/123 HTTP/1.1\r\n
Host: vm1:50000\r\n
Connection: keep-alive\r\n
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.84 Safari/537.36\r\n
Upgrade-Insecure-Requests: 1\r\n
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n
Accept-Encoding: gzip, deflate\r\n
Accept-Language: zh-CN,zh;q=0.9,en;q=0.8,zh-TW;q=0.7\r\n
\r\n
```

应答 keep-Alive
```
HTTP/1.1 200 OK\r\n
Content-Length: 11\r\n
Connection: Keep-Alive\r\n
Content-Type: text/plain\r\n
\r\n
hello world
```

应答 close
```
HTTP/1.1 200 OK\r\n
Connection: close\r\n
Content-Type: text/plain\r\n
\r\n
hello world
```


post语句
```
POST /color.cgi HTTP/1.1\r\n
Connection: keep-alive\r\n
Content-Length: 10\r\n
\r\n
color=blue
```

```
head="POST /color.cgi HTTP/1.1\r\nConnection: keep-alive\r\nContent-Length: 10\r\n\r\ncolor=blue"; 

echo -ne $head | nc localhost 80
```

得到cgi的应答
```
HTTP/1.1 200 OK
Content-Length: 368
Connection: Keep-Alive

<!DOCTYPE html
        PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
         "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-US" xml:lang="en-US">
<head>
<title>BLUE</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
</head>
<body bgcolor="blue">
<h1>This is blue</h1>
</body>
```

<a id="markdown-4-我认为的难点" name="4-我认为的难点"></a>
# 4. 我认为的难点

* 阻塞I/O read时 <0,==0怎么处理
* 阻塞I/O 一次read多少字节的数据,怎么做CRLF的判断
* http context的解析,用状态机
* cgi使用什么方式调用?

<a id="markdown-5-benchmark" name="5-benchmark"></a>
# 5. benchmark

* https://en.wikipedia.org/wiki/ApacheBench

```bash
yum install httpd-tools -y

ab [options] [http[s]://]hostname[:port]/path

ab -n 2000 -c 25 http://127.0.0.1/


# tinyhttpd
# Requests per second:    4228.45 [#/sec] (mean)

# muduo-http example
# Requests per second:    25514.43 [#/sec] (mean)

# http_blocking
# Requests per second:    4855.68 [#/sec] (mean)

ab -n 2000 -c 25 http://127.0.0.1/color.cgi?color=fuck
# http_blocking with cgi (perl)
# Requests per second:    219.72 [#/sec] (mean)
```

<a id="markdown-6-调试语句" name="6-调试语句"></a>
# 6. 调试语句

```bash
cmake -DCMAKE_BUILD_TYPE=Debug . && make

# got_all之后
gdb --tui  http_blocking -ex 'set args 80' --ex 'b deal_with_request' -ex 'r'
nc -l -4 0.0.0.0 80 > req.tmp

# b HttpContext::parse_request
gdb --tui  http_unittest --ex 'b http_unittest.cpp:170'  --ex 'r'

# 调试主进程
gdb --tui http_blocking --ex 'set args 80' --ex 'b server_cgi' --ex 'r'

# 调试子进程
gdb --tui http_blocking --ex 'set follow-fork-mode child' \
-ex 'set args 80' --ex 'b server_cgi' --ex 'r'

# do_with_buffer
gdb --tui  http_blocking -ex 'set args 80' --ex 'b do_with_buffer' -ex 'r'

# 调试tinyhttpd的fork cgi
gdb --tui httpd --ex 'set follow-fork-mode child' --ex 'b execute_cgi' -ex 'r'

# 调试tinyhttpd的主体
gdb --tui httpd --ex 'b execute_cgi' -ex 'r'
```
