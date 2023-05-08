#include "epoll.hpp"
#include "../conf/conf.hpp"
#include "../socket/clients.hpp"
#include "../socket/listens.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

Epoll::Epoll(const Listens &listens) {
  this->wait_num_ = listens.get_port_num();
  this->epoll_fd_ = epoll_create(1);
  if (epoll_fd_ < 0) {
    std::cerr << "Error: epoll_create() failed" << std::endl;
    throw std::runtime_error("epoll_create() failed");
  }

  for (auto itr = listens.listen_fds_.begin(); itr != listens.listen_fds_.end();
       itr++) {
    struct epoll_event event;
    event.data.fd = itr->first;
    event.events = EPOLLIN;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, itr->first, &event) < 0) {
      std::cerr << "Error: epoll_ctl() failed" << std::endl;
      close(itr->first);
      close(epoll_fd_);
      throw std::runtime_error("epoll_ctl() failed");
    }
  }
}

void Epoll::epoll_loop(const Listens &listens) {
  Clients clients;

  while (true) {
    struct epoll_event *events = new struct epoll_event[wait_num_];
    int event_count = epoll_wait(epoll_fd_, events, wait_num_, 1000);
    clients.check_timeout(*this, 10);

    if (event_count < 0) {
      delete[] events;
      std::cerr << "Error: epoll_wait() failed" << std::endl;
      throw std::runtime_error("epoll_wait() failed");
    }

    for (int i = 0; i < event_count; i++) {
      int event_flag = events[i].events;
      if (event_flag & EPOLLERR) { // エラー
        std::cout << "EPOLLERR" << std::endl;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0) {
          std::cerr << "Error: epoll_ctl() failed" << std::endl;
          throw std::runtime_error("epoll_ctl() failed");
        }
        close(events[i].data.fd);
        wait_num_--;
        continue;
      }
      if (event_flag & EPOLLHUP) { // ソケットが切断された
        std::cout << "EPOLLHUP" << std::endl;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0) {
          std::cerr << "Error: epoll_ctl() failed" << std::endl;
          throw std::runtime_error("epoll_ctl() failed");
        }
        close(events[i].data.fd);
        wait_num_--;
        continue;
      }
      if (listens == events[i].data.fd) {
        int client_fd = clients.accept(events[i].data.fd,
                                       listens.listen_fds_[events[i].data.fd]);
        if (client_fd < 0) {
          std::cerr << "Error: accept() failed" << std::endl;
          // 終了しないで
          continue;
        }
        struct epoll_event event;
        event.data.fd = client_fd;
        event.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &event) < 0) {
          std::cerr << "Error: epoll_ctl() failed" << std::endl;
          throw std::runtime_error("epoll_ctl() failed");
        }
        wait_num_++;
      } else if (event_flag & EPOLLIN) {
        char buf[1024];
        // ここでリクエストを読み込む
        // すべて読み込むまでループする
        while (1) {
          int read_size = recv(events[i].data.fd, buf, 1024, MSG_DONTWAIT);
          if (read_size < 0) {
            std::cerr << "Error: read() failed" << std::endl;
            throw std::runtime_error("read() failed");
          }

          if (read_size < 1024) {
            break;
          }
        }
        std::cout << "EPOLLIN" << std::endl;
        std::cout << "------- client fd : " << events[i].data.fd
                  << "message ----------" << std::endl;
        write(STDOUT_FILENO, buf, read_size);
        std::cout << "------------ fin ----------------" << std::endl;
        // ここでリクエストを解析する
        // ここでレスポンスを作成する -> client bufferにためておいて

        // client fd に書き込み可能になったら書き込む
        struct epoll_event event;
        event.data.fd = events[i].data.fd;
        event.events = EPOLLIN | EPOLLOUT | EPOLLET;
        epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, events[i].data.fd, &event);

      } else if (event_flag & EPOLLOUT) {
        std::cout << "EPOLLOUT" << std::endl;
        // ここでレスポンスを書き込む
        std::ifstream file("./var/www/html/index.html", std::ios::binary);

        std::string file_contents;

        if (file) {
          std::ostringstream ss;
          ss << file.rdbuf(); // reading data
          file_contents = ss.str();
        } else {
          std::cerr << "Error: file open failed" << std::endl;
          throw std::runtime_error("file open failed");
        }

        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response +=
            "Content-Length: " + std::to_string(file_contents.size()) + "\r\n";
        response += "\r\n";
        response += file_contents;

        int write_size =
            write(events[i].data.fd, response.c_str(), response.size());
        if (write_size < 0) {
          std::cerr << "Error: write() failed" << std::endl;
          throw std::runtime_error("write() failed");
        }
        // ここでクライアントを削除する
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0) {
          std::cerr << "Error: epoll_ctl() failed" << std::endl;
          throw std::runtime_error("epoll_ctl() failed");
        }

      } else {
        std::cerr << "Error: Unknown event" << std::endl;
        throw std::runtime_error("Unknown event");
      }
    }
    delete[] events;
  }
}
