<!-- TOC -->

- [1. 简要](#1-简要)
- [2. 用c++代码来拼接html?](#2-用c代码来拼接html)
- [3. 进程信息](#3-进程信息)
- [4. 我自己的分析](#4-我自己的分析)
- [5. 库](#5-库)
- [6. 抗锯齿](#6-抗锯齿)
- [7. 调试](#7-调试)
- [benchmark](#benchmark)

<!-- /TOC -->


<a id="markdown-1-简要" name="1-简要"></a>
# 1. 简要

两种方式
* 侵入式,写成库,不过语言局限
* 非侵入式,只能监控一些常见信息.不能得知程序内部的信息

画图
* Standlone chart server <img src="http://chart/?data=1,0.0,0.8,0.2"> (专门的图片生成服务器)
* Render with JavaScript: jqPlot,flot (浏览器画图)
* Dynamically generate PNG imagge with libgd(<img src="/cpu.png">) (兼容浏览器容易)

为什么不用其他方式?
* nginx 或 lighttpd  + cgi 或 fastcgi + 脚本业务逻辑

专用的http服务器,而不是通用的  
绘制历史曲线,占用资源尽可能小,一体化设计  

<a id="markdown-2-用c代码来拼接html" name="2-用c代码来拼接html"></a>
# 2. 用c++代码来拼接html?

有什么方便的方式吗?

* https://colorlib.com/wp/top-templating-engines-for-javascript/
* https://www.zhihu.com/question/20355486 (nodejs模板引擎)
* https://github.com/mozilla/nunjucks (推荐取代swig?)
* https://github.com/paularmstrong/swig

jinja
* https://en.wikipedia.org/wiki/Jinja_(template_engine) (Jinja)
* https://github.com/hughperkins/Jinja2CppLight (试下这个把,c++ jinja)
* http://jinja2test.tk/ (jinja 在线)

= = 此swig非彼swig,技术真是多
* http://www.swig.org/ (swig)
* http://www.swig.org/Doc3.0/SWIGPlus.html#SWIGPlus (swig with c++)

<a id="markdown-3-进程信息" name="3-进程信息"></a>
# 3. 进程信息

* https://www.redhat.com/archives/axp-list/2001-January/msg00355.html (/proc/pid/stat 的说明)

<a id="markdown-4-我自己的分析" name="4-我自己的分析"></a>
# 4. 我自己的分析

开源的如下,问题就是太重量级了.一堆数据,颜色摆在你的面前,你都不知道如何去取舍(关键是懂得东西太少了,不知道如何从大量的数据中提取关键的信息)
* https://london.my-netdata.io/default.html#menu_system_submenu_swap;theme=slate;help=true

我觉得有必要自己来做个简单的,应用级别的http监控服务.因为开源的都是通用的,如果为了适应开源监控组件,而去强制监控的接口.到了换监控组件的时候该怎么办呢?  
况且我只想要一个简易的(自己做一些简单的应用).http或telnet或rpc接口.  
我觉的还是用js有前途一些,c/c++去画图标太复杂了  
或者研究一下vps厂商的画法  

监控信息如下:
* cpu
* 磁盘I/O
* 网络I/O
* 内存
* 轻量级业务信息

时间维度:
* 过去24小时
* 过去7天
* 过去一个月

<a id="markdown-5-库" name="5-库"></a>
# 5. 库

* https://libgd.github.io/manuals/2.2.4/files/gd-c.html
* https://www.zhihu.com/question/21222208/answer/17604137 (js画图库)

```bash

# 画图
yum install gd-devel -y

# c++ jinja
cd /opt
git clone https://github.com/hughperkins/Jinja2CppLight.git
cd Jinja2CppLight/
mkdir build
cd build
cmake ..
make
make install

/usr/local/include/Jinja2CppLight/Jinja2CppLight.h
/usr/local/lib/libJinja2CppLight.so

echo 'export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/usr/local/lib"' >> ~/.bashrc

# 我只能说fuck c++了, 一个string搞那么多版本,根本就不好去学习.
# typedef __gnu_cxx::__sso_string string; 你用这个实现 怎么和别人的库兼容?????写点小东西验证想法,烦死了

```

<a id="markdown-6-抗锯齿" name="6-抗锯齿"></a>
# 6. 抗锯齿

libgd 生成的图片有锯齿

* https://www.zhihu.com/question/65195520/answer/228487516


<a id="markdown-7-调试" name="7-调试"></a>
# 7. 调试

```
cgdb procmon -ex 'set args 1 80' -ex 'b Procmon::fill_over_fiew' -ex 'r'
```

<a id="markdown-benchmark" name="benchmark"></a>
# benchmark

```
# 长连接
ab -k -n 100000 http://localhost:80/

# 短连接
ab -n 100000 http://localhost:80/

# 测试图片
ab -k -n 10000 http://localhost:80/cpu.png


# muduo的
Requests per second:    9706.50 [#/sec] (mean)

# 我的(jinja)
Requests per second:    2950.52 [#/sec] (mean)

# 我的(jinjia + cpu)
Requests per second:    946.16 [#/sec] (mean)

```
