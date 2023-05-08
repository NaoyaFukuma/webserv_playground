#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../conf/conf.hpp"
#include "../epoll/epoll.hpp"
#include "../socket/listens.hpp"
#include <map>
#include <utility>
#include <vector>

class Clients {
private:
  std::map < int, pair<time_t, std::vector<Server>> fds_;
  // ファイルディスクリプタとその最終通信時刻を格納する

public:
  std::stringstream recv_ss_buf_;
  std::stringstream send_ff_buf_;
  Clients();    // デフォルトコンストラクタのみ使用
  ~Clients(){}; // デストラクタ

  int accept(const int listen_fd, const std::vector<Server>);

  int RecvToBuf(const int fd);

  void check_timeout(Epoll &epoll, const int timeout);

private:                         // 不使用
  Clients(const Clients &other); // コピーコンストラクタは不使用(シングルトン)
  void
  operator=(const Clients &other); // コピー代入演算子も不使用(シングルトン)
};

#endif
