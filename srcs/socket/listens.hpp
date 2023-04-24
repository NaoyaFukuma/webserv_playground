#ifndef LISTENS_HPP
#define LISTENS_HPP

#include "../conf/conf.hpp"
#include <string>
#include <vector>

/* webserverにおける、リッスンソケットを扱うクラス */
class Listens {
private:
  int port_num_;         // ポートの数
  std::vector<int> fds_; // リッスンソケットのファイルディスクリプタ
  Listens(const Listens &other); // コピーコンストラクタは不使用(シングルトン)
  void
  operator=(const Listens &other); // コピー代入演算子も不使用(シングルトン)
  Listens(); // デフォルトコンストラクタは不使用
public:
  Listens(const std::vector<Conf> &conf); // コンストラクタ
  ~Listens(){};                           // デストラクタ
  bool operator==(const int fd) const;
  // 比較演算子
  // 新たな接続要求がクライアントからあったかどうかを判定するために使用
};

#endif
