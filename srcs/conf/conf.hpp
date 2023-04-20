/*
webserverの設定を扱うクラス
設定ファイルのパスを引数にとり、設定ファイルを読み込む
設定ファイルが無い場合はデフォルトの内容を設定する
設定ファイルのフォーマットは以下の通り
・設定ファイルの拡張子は.conf



扱う情報は以下の通り
・サーバーのポート番号
・サーバーのルートディレクトリ
・サーバーのindexファイル
・サーバーのエラーページ
*/

#ifndef CONF_HPP
#define CONF_HPP
#include <iostream>

class conf {
private:
  conf(const conf &other); // コピーコンストラクタは不使用(シングルトン)
  conf &operator=(const conf &other); // コピー代入演算子も不使用(シングルトン)
public:
  conf(); // デフォルトコンストラクタはコンフファイルが指定されなかった場合に使用
  conf(const std::string &conf_path); // コンフファイルが指定された場合に使用
  ~conf();                            // デストラクタ
};

#endif
