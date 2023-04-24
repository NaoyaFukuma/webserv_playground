#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <map>

class Clients {
private:
  std::map<int, time_t> fds_;
  // ファイルディスクリプタとその最終通信時刻を格納する

  Clients(const Clients &other); // コピーコンストラクタは不使用(シングルトン)
  void

  operator=(const Clients &other); // コピー代入演算子も不使用(シングルトン)

public:
  Clients();    // デフォルトコンストラクタのみ使用
  ~Clients(){}; // デストラクタ
};

#endif
