#pragma once

#include <cstdio>
#include <memory>

#include <boost/noncopyable.hpp>

#include <muduo/base/StringPiece.h>

class Item;

typedef std::shared_ptr<Item> ItemPtr;
typedef std::shared_ptr<const Item> ConstItemPtr;

class Item : public boost::noncopyable
{
public:
    Item(muduo::StringPiece keyArg, int flag, int exptime, muduo::StringPiece valueArg);

    ~Item();

    static ItemPtr makeItem(muduo::StringPiece keyArg, int flag, int exptime, muduo::StringPiece valueArg)
    {
        return std::make_shared<Item>(keyArg, flag, exptime, valueArg);
    }

    static ItemPtr makeKeyItem(muduo::StringPiece keyArg)
    {
        return std::make_shared<Item>(keyArg, 0, 0, "");
    }

    size_t hash() const
    {
        return hash_;
    }

    muduo::StringPiece key() const
    {
        return muduo::StringPiece(data_, keyLen_);
    }

    muduo::StringPiece value() const
    {
        return muduo::StringPiece(data_ + keyLen_, valueLen_);
    }

    int flag() const
    {
        return flag_;
    }

    int valueLen() const
    {
        return valueLen_;
    }

    size_t totalLen() const
    {
        return keyLen_ + valueLen_;
    }

private:
    const int keyLen_;      // include \r\n
    const int flag_;
    const int exptime_;
    const int valueLen_;    // include \r\n
    const size_t hash_;
    char* data_;            // save key + data
};


