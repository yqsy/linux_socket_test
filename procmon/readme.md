<!-- TOC -->

- [1. 简要](#1-简要)
- [2. 用c++代码来拼接html?](#2-用c代码来拼接html)
- [3. 库](#3-库)
- [4. 调试](#4-调试)

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

<a id="markdown-3-库" name="3-库"></a>
# 3. 库

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

```

<a id="markdown-4-调试" name="4-调试"></a>
# 4. 调试

```
cgdb procmon -ex 'set args 1 80' -ex 'b Procmon::fill_over_fiew' -ex 'r'
```