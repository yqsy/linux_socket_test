<!-- TOC -->

- [1. 资料](#1-资料)
- [2. 流程](#2-流程)
- [3. 简单的请求报文](#3-简单的请求报文)

<!-- /TOC -->




<a id="markdown-1-资料" name="1-资料"></a>
# 1. 资料

* https://stackoverflow.com/questions/176409/build-a-simple-http-server-in-c (stackoverflow)
* https://tools.ietf.org/html/rfc2616 (rfc)
* https://en.wikipedia.org/wiki/Berkeley_sockets (bsd sockets)
* https://sourceforge.net/projects/tinyhttpd/ (tiny httpd)
* http://man7.org/tlpi/code/online/dist/sockets/read_line.c.html (纯c的比较难写,readline)
* https://www.jmarshall.com/easy/http/ (简单的http例子)

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


<a id="markdown-3-简单的请求报文" name="3-简单的请求报文"></a>
# 3. 简单的请求报文

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