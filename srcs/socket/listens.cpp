#include "listens.hpp"
#include "../conf/conf.hpp"
#include <netinet/in.h>
#include <set>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include <arpa/inet.h> // inet_ntoa()　デバッグ用

#include <iostream>

struct MyListenComp {
  bool operator()(const struct sockaddr_in &lhs,
                  const struct sockaddr_in &rhs) const {
    if (lhs.sin_addr.s_addr != rhs.sin_addr.s_addr) {
      return lhs.sin_addr.s_addr < rhs.sin_addr.s_addr;
    }
    return lhs.sin_port < rhs.sin_port;
  }
};
// コンストラクタ
Listens::Listens(const Conf &conf) : port_num_(conf.server_vec_.size()) {

  // liste ip:port の重複をなくしたい
  std::map<struct sockaddr_in, std::vector<Server>, MyListenComp> listen_map;

  //
  for (int i = 0; i < port_num_; i++) {
    listen_map[conf.server_vec_[i].listen_].push_back(conf.server_vec_[i]);
  }

  // 重複をなくした結果、ポートの数が減る可能性がある
  port_num_ = listen_map.size();

  // 重複をなくした上で、struct addr から fd にバインドしたlisten fdを作成し、
  // その fd に対応する serverの配列を保持する

  // listen_mapの要素を順番に見ていく
  for (auto itr = listen_map.begin(); itr != listen_map.end(); itr++) {
    // listen_mapの要素のうち、fdを作成する
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
      throw std::runtime_error("socket() failed");
    }
    // fdに対応するserverの配列を保持する
    this->listen_fds_[fd] = itr->second;
    // fdに対応するstruct addrを取得する
    struct sockaddr_in addr = itr->first;
    // fdに対応するstruct addrにバインドする
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
      throw std::runtime_error("bind() failed");
    }
    // fdに対応するstruct addrでlistenする
    if (listen(fd, SOMAXCONN) == -1) {
      throw std::runtime_error("listens() failed");
    }
  }
  // デバッグ用プリント
  std::cout << "debug print: Listens::Listens()" << std::endl;
  for (auto itr = listen_fds_.begin(); itr != listen_fds_.end(); itr++) {
    std::cout << "\nliste fd: " << itr->first << std::endl;
    std::cout << "server num: " << itr->second.size() << std::endl;

    for (auto itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++) {
      std::cout << "\tip: " << inet_ntoa(itr2->listen_.sin_addr) << std::endl;
      std::cout << "\tport: " << ntohs(itr2->listen_.sin_port) << std::endl;
      for (auto itr3 = itr2->sv_names_.begin(); itr3 != itr2->sv_names_.end();
           itr3++) {
        std::cout << "\tserver_names: " << *itr3 << std::endl;
      }
    }
  }
  std::cout << "---------------\n" << std::endl;
}

bool Listens::operator==(const int fd) const {
  for (auto itr = this->listen_fds_.begin(); itr != this->listen_fds_.end();
       itr++) {
    if (itr->first == fd) {
      return true;
    }
  }
  return false;
}

Listens::~Listens() {
  // this->liste_fds_ のkey である fd を 全てclose する
  for (auto itr = this->listen_fds_.begin(); itr != this->listen_fds_.end();
       itr++) {
    close(itr->first);
  }
}

/* テストmain */
// int main() {
//   std::vector<in_port_t> ports;
//   ports.push_back(htons(80));
//   ports.push_back(htons(8080));
//   Listens listens(ports);
//   return 0;
// }
