#include "conf.hpp"

// .confファイルが指定されなかった場合に使用し、デフォルトの設定を行う
conf::conf() {}

// .confファイルが指定された場合に使用し、設定ファイルを読み込む
conf::conf(const std::string &conf_path) {}

// デストラクタ
conf::~conf() {}
