#include "listen.hpp"
#include "../conf/conf.hpp"
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// コンストラクタ
Listen::Listen(const std::vector<in_port_t> &ports)
    : port_num_(ports.size()), fds_(port_num_) {
  for (int i = 0; i < port_num_; i++) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
      throw std::runtime_error("socket() failed");
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = ports[i];
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      throw std::runtime_error("bind() failed");
    }
    if (listen(fd, SOMAXCONN) == -1) {
      throw std::runtime_error("listen() failed");
    }
    fds_[i] = fd;
  }
}

bool Listen::operator==(const int fd) const {
  for (int i = 0; i < this->port_num_; i++) {
    if (this->fds_[i] == fd) {
      return true;
    }
  }
  return false;
}

/* テストmain */
// int main() {
//   std::vector<in_port_t> ports;
//   ports.push_back(htons(80));
//   ports.push_back(htons(8080));
//   Listen listen(ports);
//   return 0;
// }
