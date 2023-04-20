#ifndef LISTEN_HPP
#define LISTEN_HPP

#include "../conf/conf.hpp"
#include <string>
#include <vector>

/* webserverにおける、リッスンソケットを扱うクラス */
class Listen {
private:
  int port_num_;         // ポートの数
  std::vector<int> fds_; // リッスンソケットのファイルディスクリプタ
  Listen(const Listen &other); // コピーコンストラクタは不使用(シングルトン)
  void operator=(const Listen &other); // コピー代入演算子も不使用(シングルトン)
  Listen(); // デフォルトコンストラクタは不使用
public:
  Listen(const std::vector<in_port_t> &ports); // コンストラクタ
  ~Listen(){};                                 // デストラクタ
  bool operator==(const int fd) const;
  // 比較演算子
  // 新たな接続要求がクライアントからあったかどうかを判定するために使用
};

#endif
