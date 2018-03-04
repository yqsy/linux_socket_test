#pragma once

#include <memory>

#include <boost/noncopyable.hpp>
#include <boost/tokenizer.hpp>

#include <muduo/net/TcpConnection.h>
#include <muduo/base/Logging.h>
#include <muduo/base/StringPiece.h>




enum ReceiveState
{
    kReadCommandLine,
    kReadValue
};

enum CommandType
{
    kInvalid,
    kSet,
    kGet,
    kQuit
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

class MemcachedServer;

class Session : public std::enable_shared_from_this<Session>,
    boost::noncopyable
{
public:
    Session(MemcachedServer* owner, const muduo::net::TcpConnectionPtr& conn);

    ~Session()
    {
        LOG_INFO << "session " << conn_->name();
    }

private:
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
        muduo::net::Buffer* buf,
        muduo::Timestamp);

    bool doRequestCommand(const muduo::StringPiece& request);

    bool doUpdate(Tokenizer::iterator begin, Tokenizer::iterator end);

    bool doGet(Tokenizer::iterator begin, Tokenizer::iterator end);

    void storeData(const muduo::StringPiece& data);

private:
    void sendClientError();

private:
    // save current state
    ReceiveState receiveState_;
    const char* curSearchPos_;      // reduce time complexity
    CommandType curCommandType_;
    std::string curKey_;
    int curFlag_;
    int curExptime_;
    int curValueLen_;

    // conn
    muduo::net::TcpConnectionPtr conn_;

    // owner (for access to shared datas)
    MemcachedServer* owner_;
};


typedef std::shared_ptr<Session> SessionPtr;
