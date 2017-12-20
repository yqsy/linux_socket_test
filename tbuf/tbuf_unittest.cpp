#include <tbuf/tbuf.h>

#include <netinet/in.h>
#include <stdio.h>

#include <string>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

// 普通的的测试
BOOST_AUTO_TEST_CASE(test_tbuf_simple)
{
  Tbuf tbuf;
  BOOST_CHECK_EQUAL(tbuf.writeable_bytes(), 1024);
  BOOST_CHECK_EQUAL(tbuf.readable_bytes(), 0);
  tbuf.append(std::string(1024, '1'));
  BOOST_CHECK_EQUAL(tbuf.readable_bytes(), 1024);
  BOOST_CHECK_EQUAL(tbuf.writeable_bytes(), 0);
}

// 缓冲区增长
BOOST_AUTO_TEST_CASE(test_tbuf_grow)
{
  Tbuf tbuf;
  BOOST_CHECK_EQUAL(tbuf.writeable_bytes(), 1024);
  BOOST_CHECK_EQUAL(tbuf.readable_bytes(), 0);
  tbuf.append(std::string(1024, '1'));
  BOOST_CHECK_EQUAL(tbuf.readable_bytes(), 1024);
  BOOST_CHECK_EQUAL(tbuf.writeable_bytes(), 0);
  // 和上面一样

  tbuf.append(std::string(1, '1'));
  BOOST_CHECK_EQUAL(tbuf.readable_bytes(), 1025);
  BOOST_CHECK_EQUAL(tbuf.writeable_bytes(), 0);
}

// 缓冲区腾挪
BOOST_AUTO_TEST_CASE(test_tbuf_move)
{
  Tbuf tbuf;
  BOOST_CHECK_EQUAL(tbuf.writeable_bytes(), 1024);
  BOOST_CHECK_EQUAL(tbuf.readable_bytes(), 0);
  tbuf.append(std::string(1024, '1'));
  BOOST_CHECK_EQUAL(tbuf.readable_bytes(), 1024);
  BOOST_CHECK_EQUAL(tbuf.writeable_bytes(), 0);
  // 和上面一样

  tbuf.retrieve(1);
  BOOST_CHECK_EQUAL(tbuf.readable_bytes(), 1023);
  BOOST_CHECK_EQUAL(tbuf.writeable_bytes(), 0); // 虽然读了一个但是后面还是没有

  tbuf.append(std::string(1, '1'));
  BOOST_CHECK_EQUAL(tbuf.readable_bytes(), 1024);
  BOOST_CHECK_EQUAL(tbuf.writeable_bytes(), 0);
}

// 读网络字节序
BOOST_AUTO_TEST_CASE(test_tbuf_readint)
{
  Tbuf tbuf;
  int32_t n1 = 65536;
  n1 = htonl(n1);

  tbuf.append(boost::string_ref(reinterpret_cast<char *>(&n1), sizeof(n1)));

  BOOST_CHECK_EQUAL(tbuf.readable_bytes(), 4);
  BOOST_CHECK_EQUAL(tbuf.writeable_bytes(), 1020);

  int32_t n2 = tbuf.read_int32();
  BOOST_CHECK_EQUAL(n2, 65536);
  BOOST_CHECK_EQUAL(tbuf.readable_bytes(), 0);
  BOOST_CHECK_EQUAL(tbuf.writeable_bytes(), 1024);
}
