#include <ttcp/common.h>

#include <iostream>

bool pase_sub_desc(const po::parsed_options &parsed,
                   const po::options_description &subdesc) {

  po::variables_map sub_vm;
  std::vector<std::string> opts =
      po::collect_unrecognized(parsed.options, po::include_positional);

  opts.erase(opts.begin());
  po::store(po::command_line_parser(opts).options(subdesc).run(), sub_vm);

  if (sub_vm.count("help")) {
    std::cout << subdesc << std::endl;
    return false;
  }
  return true;
}

bool parse_commandline(int argc, char *argv[], Options *opt) {
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

  if (!vm.count("command")) {
    std::cout << desc << std::endl;
    return false;
  }

  std::string cmd = vm["command"].as<std::string>();

  if (cmd == "server") {
    po::options_description subdesc("");
    // clang-format off
    subdesc.add_options()
      ("help,h", "Help")
      ("host,h", po::value<std::string>(&opt->listen_host)->default_value("127.0.0.1"), "listen_host")
      ("port,p", po::value<uint16_t>(&opt->listen_port)->default_value(5001), "listen_port")
      ;
    // clang-format on

    if (!pase_sub_desc(parsed, subdesc))
      return false;

    opt->command = k_server;

  } else if (cmd == "client") {
    po::options_description subdesc("");
    // clang-format off
    subdesc.add_options()
      ("help,h", "Help")
      ("host,h", po::value<std::string>(&opt->server_host)->default_value("127.0.0.1"), "server_host")
      ("port,p", po::value<uint16_t>(&opt->server_port)->default_value(5001), "server_port")
      ("number,n", po::value<int>(&opt->buffer_number)->default_value(8192), "buffer_number")
      ("length,l", po::value<int>(&opt->buffer_length)->default_value(65536), "buffer_length")
      ;
    // clang-format on

    if (!pase_sub_desc(parsed, subdesc))
      return false;

    opt->command = k_server;

  } else {
    std::cout << desc << std::endl;
    return false;
  }

  return true;
}
