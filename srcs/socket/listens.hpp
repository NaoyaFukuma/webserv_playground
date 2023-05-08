#ifndef LISTENS_HPP
#define LISTENS_HPP

#include "../conf/conf.hpp"
#include <map>
#include <string>
#include <vector>

/* webserverにおける、リッスンソケットを扱うクラス */
class Listens {
private:
  int port_num_; // ポートの数

public:
  std::map<int, std::vector<Server>> listen_fds_;
  Listens(const Conf &conf); // コンストラクタ
  ~Listens();                // デストラクタ
  bool operator==(const int fd) const;
  // 比較演算子
  // 新たな接続要求がクライアントからあったかどうかを判定するために使用

  int get_port_num() const { return this->port_num_; } // ポートの数を返す

  // int get_fd(const int i) const { return this->fds_[i]; }

private:                         // 不使用
  Listens(const Listens &other); // コピーコンストラクタは不使用(シングルトン)
  void
  operator=(const Listens &other); // コピー代入演算子も不使用(シングルトン)
  Listens(); // デフォルトコンストラクタは不使用
};

#endif
