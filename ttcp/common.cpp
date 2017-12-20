#include <ttcp/common.h>

#include <netdb.h>

#include <iostream>

const int MAX_BUFFER_LENGTH = 1024 * 1024 * 10; // MAX 10 M

bool pase_sub_desc(const po::parsed_options &parsed,
                   const po::options_description &subdesc)
{

  po::variables_map sub_vm;
  std::vector<std::string> opts =
      po::collect_unrecognized(parsed.options, po::include_positional);

  opts.erase(opts.begin());
  po::store(po::command_line_parser(opts).options(subdesc).run(), sub_vm);
  po::notify(sub_vm);

  if (sub_vm.count("help"))
  {
    std::cout << subdesc << std::endl;
    return false;
  }
  return true;
}

bool parse_commandline(int argc, char *argv[], Options *opt)
{
  po::options_description desc("Allowed options");

  // clang-format off
  desc.add_options()
    ("command", po::value<std::string>(),"server client")
    ("subargs", po::value<std::vector<std::string> >(), "Arguements for command")
    ;

  po::positional_options_description pos;
  pos.add("command", 1).
      add("subargs", -1);

  po::variables_map vm;

  po::parsed_options parsed = po::command_line_parser(argc, argv).
      options(desc).
      positional(pos).
      allow_unregistered().
      run();

  po::store(parsed, vm);
  // clang-format on

  if (!vm.count("command"))
  {
    std::cout << desc << std::endl;
    return false;
  }

  std::string cmd = vm["command"].as<std::string>();

  if (cmd == "server")
  {
    po::options_description subdesc("");
    // clang-format off
    subdesc.add_options()
      ("help", "Help")
      //("host,h", po::value<std::string>(&opt->listen_host)->default_value("127.0.0.1"), "listen_host")
      ("port,p", po::value<uint16_t>(&opt->listen_port)->default_value(5001), "listen_port")
      ;
    // clang-format on

    if (!pase_sub_desc(parsed, subdesc))
      return false;

    opt->command = k_server;
  }
  else if (cmd == "client")
  {
    po::options_description subdesc("");
    // clang-format off
    subdesc.add_options()
      ("help", "Help")
      ("host,h", po::value<std::string>(&opt->server_host)->default_value("127.0.0.1"), "server_host")
      ("port,p", po::value<uint16_t>(&opt->server_port)->default_value(5001), "server_port")
      ("number,n", po::value<int>(&opt->number)->default_value(8192), "buffer_number")
      ("length,l", po::value<int>(&opt->length)->default_value(65536), "buffer_length")
      ;
    // clang-format on

    if (!pase_sub_desc(parsed, subdesc))
      return false;

    opt->command = k_client;
  }
  else
  {
    std::cout << desc << std::endl;
    return false;
  }

  return true;
}

struct sockaddr_in resolve_or_die(const char *host, uint16_t port)
{
  struct hostent *he = ::gethostbyname(host);

  if (!he)
  {
    perror("gethostbyname");
    exit(1);
  }

  assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
  return addr;
}