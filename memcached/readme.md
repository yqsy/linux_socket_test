<!-- TOC -->

- [1. 数据结构选择](#1-数据结构选择)
    - [1.1. Naive key-value (天真的)](#11-naive-key-value-天真的)
    - [1.2. Minimalize critical section](#12-minimalize-critical-section)
    - [1.3. Condensed,save memory](#13-condensedsave-memory)
    - [1.4. Sharded, further reduce contention](#14-sharded-further-reduce-contention)
- [2. 设想与比较](#2-设想与比较)

<!-- /TOC -->


<a id="markdown-1-数据结构选择" name="1-数据结构选择"></a>
# 1. 数据结构选择

<a id="markdown-11-naive-key-value-天真的" name="11-naive-key-value-天真的"></a>
## 1.1. Naive key-value (天真的)
* hash_map<string, value*> (临界区保护整个读写过程)
* hash_map<string, unique_ptr<value>> (不需要删除)
* hash_map<string, value> // 右值


临界区内系统调用
```
hash_map<string,value*> theMap;

string key = xxx;
lock();
value* val = theMap.get(key);
send(val); # 这个系统调用会消耗大量的时间 (10ms)
unlock();
```

临界区内只是拷贝内存
```
string key = xxx;
value val;
lock();
val = *theMap.get(key); # 内存拷贝缩小了一个数量级,100us
unlock();
send(val);
```

<a id="markdown-12-minimalize-critical-section" name="12-minimalize-critical-section"></a>
## 1.2. Minimalize critical section
* hash_map<string, shared_ptr<value>>


临界区拷贝指针
set值时生成新的对象
```
typedef shared_ptr<const value> ValuePtr; # 注意是const的
hash_map<string,ValuePtr> theMap;
ValuePtr val;
lock();
val = theMap.get(key); # 指针拷贝时间更短,<1us
unlock();
send(*val);
```

<a id="markdown-13-condensedsave-memory" name="13-condensedsave-memory"></a>
## 1.3. Condensed,save memory
* hash_map<shared_ptr<item>> (key value放一起 节省空间) (自己实现比较函数)

```
unordered_set<shared_ptr<const Item>, Hash, Equal>
```

<a id="markdown-14-sharded-further-reduce-contention" name="14-sharded-further-reduce-contention"></a>
## 1.4. Sharded, further reduce contention
* 上千个hash_map,每个hash_map都有自己的锁,避免全局锁争用(多线程)

```
struct Shard 
{
    mutex mu_;
    hash_map<...> map_;
};


int x = key.hash() % 1024;
ValuePtr val;
shards[x].mu_.lock();
val = shards[x].map_.get(key);
shard[x].mu_.unlock();


Shard shards[1024];
```


<a id="markdown-2-设想与比较" name="2-设想与比较"></a>
# 2. 设想与比较

属于半定制的,使用了标准库的hash map和shared ptr,而memcached是全定制的

基本开销是120N,memcached是48N

