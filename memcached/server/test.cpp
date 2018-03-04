#include <unordered_map>
#include <unordered_set>
#include <string>
#include <iostream>
#include <cstdio>
#include <array>
#include <memory>

#include <boost/noncopyable.hpp>
#include <boost/tokenizer.hpp>

#include <muduo/net/Buffer.h>
#include <muduo/base/Timestamp.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/StringPiece.h>

#include <memcached/server/item.h>

using namespace muduo;
using namespace muduo::net;

void TestInsert()
{
    std::unordered_map <std::string, std::string>  hashMap;

    Timestamp start = Timestamp::now();

    for (int i = 0; i < 5000000; ++i)
    {
        hashMap[std::to_string(i)] = std::to_string(i);
    }

    Timestamp end = Timestamp::now();

    double seconds = timeDifference(end, start);

    std::cout << seconds << std::endl;
}


struct Hash
{
    size_t operator() (const ConstItemPtr& x) const
    {
        return x->hash();
    }
};


struct Equal
{
    bool operator() (const ConstItemPtr& x, const ConstItemPtr& y) const
    {
        return x->key() == y->key();
    }
};

typedef std::unordered_set<ConstItemPtr, Hash, Equal> ItemMap;


void TestUnorderedSet()
{
    auto itemMap = ItemMap();

    std::string keyArg = "x\r\n";
    int flag = 0;
    int exptime = 20;
    int valueLen = 10;

    ItemPtr itemPtr = Item::makeItem(keyArg, flag, exptime, valueLen);

    //static ItemPtr makeItem(muduo::StringPiece keyArg, int flag, int exptime, int valueLen)
    //{
    //    return std::make_shared<Item>(keyArg, flag, exptime, valueLen);
    //}

    itemMap.insert(itemPtr);
    itemMap.insert(itemPtr); // replace

    keyArg = "y\r\n";
    itemPtr = Item::makeItem(keyArg, flag, exptime, valueLen);
    itemMap.insert(itemPtr);

    for (auto it = itemMap.begin(); it != itemMap.end(); ++it)
    {
        std::cout << (*it)->key().as_string() << "\n";
    }
}


struct MapWithLock
{
    ItemMap items;
    mutable muduo::MutexLock mutex;
};

void TestHashBucket()
{
    const static int  kShards = 4096;

    std::array<MapWithLock, kShards> shards_;


    // generate a item
    std::string keyArg = "x\r\n";
    int flag = 0;
    int exptime = 20;
    int valueLen = 10;

    ItemPtr itemPtr = Item::makeItem(keyArg, flag, exptime, valueLen);


    // simulate sotre item
    muduo::MutexLock& mutex = shards_[itemPtr->hash() % kShards].mutex;
    ItemMap& items = shards_[itemPtr->hash() % kShards].items;

    {
        muduo::MutexLockGuard lock(mutex);

        items.insert(itemPtr);
    }
    
}

enum ReadState
{
    kNewCommandLine = 0,
    kReadValue
};


struct SpaceSeparator
{
    void reset() {}
    template <typename InputIterator, typename Token>
    bool operator()(InputIterator& next, InputIterator end, Token& tok)
    {
        while (next != end && *next == ' ')
            ++next;
        if (next == end)
        {
            tok.clear();
            return false;
        }
        InputIterator start(next);
        const char* sp = static_cast<const char*>(memchr(start, ' ', end - start));
        if (sp)
        {
            tok.set(start, static_cast<int>(sp - start));
            next = sp;
        }
        else
        {
            tok.set(start, static_cast<int>(end - next));
            next = end;
        }
        return true;
    }
};

typedef boost::tokenizer<SpaceSeparator, const char*, muduo::StringPiece> Tokenizer;

struct Reader
{
    Reader(Tokenizer::iterator& beg, Tokenizer::iterator end) 
        : first_(beg),
          last_(end)
    {
    }

    template<typename T>
    bool read(T* val)
    {
        if (first_ == last_)
        {
            return false;
        }

        char* end = NULL;
        uint64_t x = strtoull((*first_).data(), &end, 10);

        if (end == (*first_).end())
        {
            *val = static_cast<T>(x);
            ++first_;
            return true;
        }

        return false;
    }

private:
    Tokenizer::iterator first_;
    Tokenizer::iterator last_;
};


void TestServerStatement()
{
    // save
    // set x 0 20 10\r\n
    // helloworld\r\n
    // STORED\r\n

    // session for connection.
    // finite state machine

    ReadState readState = kNewCommandLine;

    Buffer buffer;


    int valueLen = 0;


    while (buffer.readableBytes() > 0)
    {
        if (readState == kNewCommandLine)
        {
            const char* crlf = buffer.findCRLF();
            if (crlf)
            {
                int len = static_cast<int>(crlf - buffer.peek());

                StringPiece request(buffer.peek(), len);

                // process request


                // get valueLen

                buffer.retrieveUntil(crlf + 2);
            }
            else
            {
                break;
            }
        }
        else if (readState == kReadValue)
        {
            // process line

            //if (buffer.readableBytes() - 2 >= valueLen)
            //{
                // parse read

                buffer.retrieve(valueLen + 2);
            //}
        }
    }
}


const int kLongestKeySize = 250;

void TestTokenizer()
{
    StringPiece request("set x 0 20 10");

    SpaceSeparator sep;
    Tokenizer tok(request.begin(), request.end(), sep);
    Tokenizer::iterator beg = tok.begin();
    Tokenizer::iterator end = tok.end();

    if (beg == tok.end())
    {
        std::cout << "end";
    }

    std::string command;
    (*beg).CopyToString(&command);
    beg++;

    std::cout << "command: " << command << std::endl;

    if (command == "set")
    {
        // do update
    }

    StringPiece key = (*beg);
    ++beg;

    bool good = key.size() <= kLongestKeySize;

    std::cout << "key: " << key.as_string() << std::endl;

    int flag;  //
    int exptime; // check > 0
    int valueLen; // check > 0

    Reader r(beg, end);

    good = good && r.read(&flag) && r.read(&exptime) && r.read(&valueLen);

    if (good)
    {
        std::cout << "flag: " << flag << std::endl;
        std::cout << "exptime: " << exptime << std::endl;
        std::cout << "valueLen: " << valueLen << std::endl;
    }
}


int main()
{
    //TestUnorderedSet();

    // TestHashBucket();

    // TestTokenizer();

    return 0;
}
