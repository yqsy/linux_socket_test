#include <memcached/server/item.h>

#include <cassert>
#include <cstdlib>

#include <boost/functional/hash/hash.hpp>

Item::Item(muduo::StringPiece keyArg, int flag, int exptime, muduo::StringPiece valueArg) :
    keyLen_(keyArg.size()),
    flag_(flag),
    exptime_(exptime),
    valueLen_(valueArg.size()),
    hash_(boost::hash_range(keyArg.begin(), keyArg.end())),
    data_(static_cast<char*>(::malloc(totalLen())))
{
    assert(keyLen_ > 0);
    assert(exptime_ >= 0);
    assert(valueLen_ >= 0);
    assert(data_ != nullptr);

    memcpy(data_, keyArg.data(), keyLen_);
    memcpy(data_ + keyLen_, valueArg.data(), valueLen_);
}

Item::~Item()
{
    ::free(data_);
}
