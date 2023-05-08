#ifndef EPOll_HPP
#define EPOll_HPP

#include "../conf/conf.hpp"
#include "../socket/listens.hpp"
// close()を使用するために必要
#include <unistd.h>

class Epoll {
private:
public:
  int wait_num_; // epoll_wait()で待機するファイルディスクリプタの数
  int epoll_fd_;
  Epoll(const Listens &listens); // コンストラクタ
  // リッスンソケットのファイルディスクリプタをepollに登録

  void epoll_loop(const Listens &listens);

  ~Epoll() { close(this->epoll_fd_); }; // デストラクタ

private:                     // 不使用
  Epoll();                   // デフォルトコンストラクタは不使用
  Epoll(const Epoll &other); // コピーコンストラクタは不使用(シングルトン)
  void operator=(const Epoll &other); // コピー代入演算子も不使用(シングルトン)
};

#endif // Epoll_HPP
