#include "clients.hpp"
#include "../epoll/epoll.hpp"
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <time.h>

Clients::Clients() {}

int Clients::accept(const int listen_fd, const std::vector<Server> servers) {
  int client_fd = ::accept(listen_fd, NULL, NULL);
  if (client_fd < 0) {
    std::cerr << "Error: accept() failed" << std::endl;
    return -1;
  }
  fds_[client_fd].fist = time(NULL);
  fds_[client_fd].second = servers;
  return client_fd;
}

void Clients::check_timeout(Epoll &epoll, const int timeout) {
  std::cout << timeout << " sec --> check_timeout()"
            << " clients : " << fds_.size() << std::endl;
  if (!this->fds_.size()) {
    return;
  }
  time_t now = time(NULL);
  std::vector<std::map<int, time_t>::iterator> erase_fds;
  for (std::map<int, time_t>::iterator it = fds_.begin(); it != fds_.end();
       it++) {
    time_t diff = now - it->second;
    std::cout << "  client fd: " << it->first << "  Elapsed time " << diff
              << " sec " << std::endl;
    if (now - it->second > timeout) {
      std::cout << "  fd: " << it->first << " client timeout!!" << std::endl;
      if (epoll_ctl(epoll.epoll_fd_, EPOLL_CTL_DEL, it->first, NULL) < 0) {
        std::cerr << "Error: epoll_ctl() failed in check_timeout()"
                  << std::endl;
        throw std::runtime_error("epoll_ctl() failed");
      }
      close(it->first);
      erase_fds.push_back(it);
      epoll.wait_num_--;
    }
  }
  for (std::vector<std::map<int, time_t>::iterator>::iterator it =
           erase_fds.begin();
       it != erase_fds.end(); it++) {
    fds_.erase(*it);
  }
}
