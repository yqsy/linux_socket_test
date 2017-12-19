<!-- TOC -->

- [1. 资料](#1-资料)
- [2. 流程](#2-流程)

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