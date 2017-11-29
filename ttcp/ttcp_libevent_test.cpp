
#include <assert.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>

#include "common.h"
#include "ttcp_libevent_test.h"

int get_listen_fd(uint16_t port) {
  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(fd >= 0);

  int yes = 1;
  int rtn = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  assert(rtn == 0);

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  rtn = bind(fd, (sockaddr *)(&addr), sizeof(addr));
  assert(rtn == 0);

  rtn = listen(fd, 5);
  assert(rtn == 0);
}

int main(int argc, char *argv[]) {

  Option option;

  if (parse_command_line(argc, argv, &option)) {
    if (option.receive) {
      int listen_fd = get_listen_fd(option.port);

      printf("listening on port = %d, looping\n", option.port);

      close(listen_fd);
    } else {
      assert(0);
    }
  }

  return 0;
}