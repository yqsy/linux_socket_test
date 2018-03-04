#include <memcached/server/session.h>

#include <functional>

#include <muduo/base/StringPiece.h>
#include <muduo/base/LogStream.h>

#include <memcached/server/memcached_server.h>



Session::Session(MemcachedServer* owner, const muduo::net::TcpConnectionPtr& conn) : 
      receiveState_(kReadCommandLine),
      curSearchPos_(nullptr),
      curCommandType_(kInvalid),
      curKey_(),
      curFlag_(0),
      curExptime_(0),
      curValueLen_(0),
      conn_(conn),
      owner_(owner)
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;

    conn_->setMessageCallback(
        std::bind(&Session::onMessage, this, _1, _2, _3)); // ?weak ptr?
}


void Session::onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp)
{
    // consumer all buffer
    while (true)
    {
        if (kReadCommandLine == receiveState_)
        {
            const char* crlf;

            if (curSearchPos_ != nullptr)
            {
                crlf = buf->findCRLF(curSearchPos_);
            }
            else
            {
                crlf = buf->findCRLF();
            }
            
            if (crlf)
            {
                auto len = static_cast<int>(crlf - buf->peek());
                auto commandReq = muduo::StringPiece(buf->peek(), len);

                bool good = doRequestCommand(commandReq);

                if (good)
                {
                    buf->retrieveUntil(crlf + 2);
                    curSearchPos_ = nullptr;

                    if (curCommandType_ == kSet)
                    {
                        // if command is update, wait for value to deal together.
                        receiveState_ = kReadValue;
                    }
                }
                else
                {
                    // error command line
                    buf->retrieveUntil(crlf + 2);
                    curSearchPos_ = nullptr;
                    // TOOD err and return
                }
            }
            else
            {
                curSearchPos_ = buf->peek() + buf->readableBytes();
                break;
            }
        }
        else if (kReadValue == receiveState_)
        {
            // include crlf
            if ((buf->readableBytes()) >= (static_cast<size_t>(curValueLen_) + 2))
            {
                auto crlf = buf->peek() + curValueLen_; // crlf is behind the data

                if (std::string(crlf, 2) == "\r\n")
                {
                    auto data = muduo::StringPiece(buf->peek(), curValueLen_);

                    storeData(data);
                    
                    if (curCommandType_ == kSet)
                    {
                        conn_->send("STORED\r\n");
                    }

                    buf->retrieveUntil(crlf + 2);
                    receiveState_ = kReadCommandLine;
                }
                else
                {
                    buf->retrieveUntil(crlf + 2);
                    receiveState_ = kReadCommandLine;
                    // TODO err and return
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            assert(false);
        }
    }
}

bool Session::doRequestCommand(const muduo::StringPiece& request)
{
    SpaceSeparator sep;
    Tokenizer tok(request.begin(), request.end(), sep);
    Tokenizer::iterator beg = tok.begin();
    Tokenizer::iterator end = tok.end();

    if (beg == end)
    {
        return false;
    }


    std::string command;
    (*beg).CopyToString(&command);
    beg++;

    // parse command
    if (command == "get")
    {
        curCommandType_ = kGet;
    }
    else if (command == "set")
    {
        curCommandType_ = kSet;
    }
    else if (command == "quit")
    {
        curCommandType_ = kQuit;
    }
    else
    {
        return false;
    }

    // dispatch command
    bool good = false;

    if (curCommandType_ == kSet)
    {
        good = doUpdate(beg, end);
    }
    else if (curCommandType_ == kGet)
    {
        good = doGet(beg, end);
    }
    else if (curCommandType_ == kQuit)
    {
        conn_->shutdown();
    }

    return good;
}

bool Session::doUpdate(Tokenizer::iterator begin, Tokenizer::iterator end)
{
    if (begin == end)
    {
        sendClientError();
        return false;
    }

    curKey_ = (*begin).as_string();
    ++begin;

    const int kLongestKeySize = 250;
    bool good = curKey_.size() <= kLongestKeySize;

    Reader r(begin, end);

    good = good && r.read(&curFlag_) && r.read(&curExptime_) && r.read(&curValueLen_);

    good = good && curFlag_ >= 0;
    good = good && curExptime_ >= 0;
    good = good && curValueLen_ >= 0;

    if (!good)
    {
        sendClientError();
        return false;
    }

    if (curValueLen_ >= 1024 * 1024)
    {
        conn_->send("SERVER_ERROR object too large for cache\r\n");
        return false;
    }

    return true;
}

bool Session::doGet(Tokenizer::iterator begin, Tokenizer::iterator end)
{
    if (begin == end)
    {
        sendClientError();
        return false;
    }

    muduo::StringPiece key = (*begin).as_string();
    ++begin;

    auto keyItem = Item::makeKeyItem(key);

    auto item = owner_->getItem(keyItem);

    muduo::net::Buffer outBuf;
    outBuf.append("VALUE ");
    outBuf.append(key);

    muduo::LogStream steam;
    steam << ' ' << item->flag() << ' ' << item->valueLen() << "\r\n";

    outBuf.append(steam.buffer().data(), steam.buffer().length());
    outBuf.append(item->value());
    outBuf.append("\r\n");

    outBuf.append("END\r\n");

    conn_->send(&outBuf);
    return true;
}


void Session::storeData(const muduo::StringPiece& data)
{
    auto item = Item::makeItem(curKey_, curFlag_, curExptime_, data);

    owner_->storeItem(item);
}

void Session::sendClientError()
{
    conn_->send("CLIENT_ERROR bad command line format\r\n");
}
