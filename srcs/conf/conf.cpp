#include "conf.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// .confファイルが指定された場合に使用し、設定ファイルを読み込む
Conf::Conf(const char *conf_path) {
  // open
  timeout_ = 60; // default
  std::ifstream ifs(conf_path);
  if (!ifs) {
    std::cerr << "conf file not found" << std::endl;
    throw std::invalid_argument("conf file not found");
  }

  // parse
  std::string line;
  while (std::getline(ifs, line)) {
    if (line.empty()) {
      continue;
    }
    if (line[0] == '#') {
      continue;
    }
    if (line.find("server ") != std::string::npos) {
      this->server_vec_.push_back(Server());
      Conf::parse_sever(ifs, this->server_vec_.back());
    } else {
      std::cout << line << std::endl;
      std::cerr << "invalid conf file 30" << std::endl;
      throw std::invalid_argument("invalid conf file");
    }
  }
}

int inet_addr(std::string ip) {
  int res = 0;
  int tmp = 0;
  int cnt = 0;

  while (ip.find(".") != std::string::npos) {
    cnt++;
    if (cnt > 3) {
      std::cerr << "invalid ip address" << std::endl;
      throw std::invalid_argument("invalid ip address");
    }
    tmp = atoi(ip.substr(0, ip.find(".")).c_str());
    if (tmp > 255 || tmp < 0) {
      std::cerr << "invalid ip address" << std::endl;
      throw std::invalid_argument("invalid ip address");
    }
    res = (res << 8) + tmp;
    ip = ip.substr(ip.find(".") + 1);
  }
  tmp = atoi(ip.c_str());
  res = (res << 8) + tmp;
  return res;
}

void Conf::parse_sever(std::ifstream &ifs, Server &server) {
  std::string line;
  while (std::getline(ifs, line)) {
    if (line.empty()) {
      continue;
    }
    if (line[0] == '#') {
      continue;
    }
    // listen, server_name, location, location_back, }のいずれかが来る

    if (line.find("listen ") != std::string::npos) {
      size_t pos = line.find("listen ");
      std::string addr = line.substr(pos + 7);
      server.listen_.sin_family = AF_INET;
      if (addr.find(":") != std::string::npos) {
        server.listen_.sin_addr.s_addr =
            htons(inet_addr(addr.substr(0, addr.find(":"))));
        server.listen_.sin_port =
            htons(atoi(addr.substr(addr.find(":") + 1).c_str()));
      } else {
        server.listen_.sin_addr.s_addr = INADDR_ANY;
        server.listen_.sin_port = htons(atoi(addr.c_str()));
      }

    } else if (line.find("server_name ") != std::string::npos) {
      std::string server_names = line.substr(line.find("server_name ") + 12);
      std::vector<std::string> sv_names_vec;
      while (server_names.length()) {
        if (server_names.find(" ") != std::string::npos) {
          sv_names_vec.push_back(
              server_names.substr(0, server_names.find(" ")));
          server_names = server_names.substr(server_names.find(" ") + 1);
        } else {
          sv_names_vec.push_back(
              server_names.substr(0, server_names.find(";")));
          server_names = server_names.substr(server_names.find(";") + 1);
        }
      }
      server.sv_names_ = sv_names_vec;

    } else if (line.find("location ") != std::string::npos) {
      server.locations_.push_back(Location());
      server.locations_.back().match_ = prefix;
      Conf::parse_location(ifs, server.locations_.back());

    } else if (line.find("location_back ") != std::string::npos) {
      server.locations_.push_back(Location());
      server.locations_.back().match_ = back;
      Conf::parse_location(ifs, server.locations_.back());

    } else if (line.find("}") != std::string::npos) {
      break;

    } else {
      std::cerr << "invalid conf file 102" << std::endl;
      throw std::invalid_argument("invalid conf file");
    }
  }
}

void Conf::parse_location(std::ifstream &ifs, Location &location) {
  location.is_cgi_ = false;
  location.autoindex_ = false;
  location.client_max_body_size_ = 1 * 1024 * 1024; // デフォルトは1MB
  std::string line;
  while (std::getline(ifs, line)) {
    if (line.empty()) {
      continue;
    }
    if (line[0] == '#') {
      continue;
    }
    // allow_ method, client_max_body_size, root, index, is_cgi, cgi_executor,
    // error_page, autoindex, return, }のいずれかが来る

    if (line.find("allow_method ") != std::string::npos) {
      std::string method = line.substr(line.find("allow_method ") + 13);
      while (method.length()) {
        std::string tmp;
        if (method.find(" ") != std::string::npos) {
          tmp = method.substr(0, method.find(" "));
          method = method.substr(method.find(" ") + 1);
        } else {
          tmp = method.substr(0, method.find(";"));
          method = method.substr(method.find(";") + 1);
        }
        if (tmp == "GET") {
          location.allow_method_ += GET;
        } else if (tmp == "POST") {
          location.allow_method_ += POST;
        } else if (tmp == "DELETE") {
          location.allow_method_ += DELETE;
        } else {
          std::cerr << "invalid conf file 133" << std::endl;
          throw std::invalid_argument("invalid conf file");
        }
      }

    } else if (line.find("client_max_body_size ") != std::string::npos) {
      std::string size_str =
          line.substr(line.find("client_max_body_size ") + 21);
      size_str.erase(size_str.end() - 1);
      size_t multiplier = 1;
      size_t value = 0;

      char unit = size_str[size_str.size() - 1];
      if (unit == 'k' || unit == 'K') {
        multiplier = 1024;
        size_str.erase(size_str.end() - 1);
      } else if (unit == 'm' || unit == 'M') {
        multiplier = 1024 * 1024;
        size_str.erase(size_str.end() - 1);
      } else if (unit == 'g' || unit == 'G') {
        multiplier = 1024 * 1024 * 1024;
        size_str.erase(size_str.end() - 1);
      }

      try {
        value = std::stoull(size_str);
      } catch (const std::invalid_argument &e) {
        throw std::runtime_error("Invalid client_max_body_size format");
      } catch (const std::out_of_range &e) {
        throw std::runtime_error("Value out of range");
      }
      location.client_max_body_size_ = value * multiplier;

    } else if (line.find("root ") != std::string::npos) {
      location.root_ = line.substr(line.find("root ") + 5);
      location.root_.erase(location.root_.end() - 1);

    } else if (line.find("index ") != std::string::npos) {
      std::string tmp = line.substr(line.find("index ") + 6);
      while (tmp.length()) {
        if (tmp.find(" ") != std::string::npos) {
          location.index_.push_back(tmp.substr(0, tmp.find(" ")));
          tmp = tmp.substr(tmp.find(" ") + 1);
        } else {
          location.index_.push_back(tmp.substr(0, tmp.find(";")));
          tmp = tmp.substr(tmp.find(";") + 1);
        }
      }
    } else if (line.find("is_cgi ") != std::string::npos) {
      if (line.find("on") != std::string::npos) {
        location.is_cgi_ = true;
      } else if (line.find("off") != std::string::npos) {
        location.is_cgi_ = false;
      } else {
        std::cerr << "invalid conf file" << std::endl;
        throw std::invalid_argument("invalid conf file");
      }

    } else if (line.find("cgi_executor ") != std::string::npos) {
      location.cgi_executor_ = line.substr(line.find("cgi_executor ") + 13);
      location.cgi_executor_.erase(location.cgi_executor_.end() - 1);

    } else if (line.find("error_page ") != std::string::npos) {
      std::string error_code =
          line.substr(line.find("error_page ") + 11,
                      line.rfind(" ") - (line.find("error_page ") + 11));
      std::string error_page = line.substr(line.rfind(" ") + 1);
      error_page.erase(error_page.end() - 1);
      while (error_code.length()) {
        if (error_code.find(' ') != std::string::npos) {
          std::string tmp = error_code.substr(0, error_code.find(' '));
          location.error_pages_[atoi(tmp.c_str())] = error_page;
          error_code = error_code.substr(error_code.find(' ') + 1);
        } else {
          location.error_pages_[atoi(error_code.c_str())] = error_page;
          break;
        }
      }
    } else if (line.find("autoindex ") != std::string::npos) {
      if (line.find("on") != std::string::npos) {
        location.autoindex_ = true;
      } else if (line.find("off") != std::string::npos) {
        location.autoindex_ = false;
      } else {
        std::cerr << "invalid conf file" << std::endl;
        throw std::invalid_argument("invalid conf file");
      }

    } else if (line.find("return ") != std::string::npos) {
      location.return_ = line.substr(line.find("return ") + 7);
      location.return_.erase(location.return_.end() - 1);

    } else if (line.find("}") != std::string::npos) {
      break;

    } else {
      std::cerr << "invalid conf file 172" << std::endl;
      throw std::invalid_argument("invalid conf file");
    }
  }
}

void Conf::print_conf() {
  std::cout << "server num: " << this->server_vec_.size() << std::endl;
  for (size_t i = 0; i < this->server_vec_.size(); i++) {
    std::cout << "server[" << i << "]" << std::endl;
    std::cout << "  listen: " << this->server_vec_[i].listen_.sin_addr.s_addr
              << std::endl;
    std::cout << "  port: " << ntohs(this->server_vec_[i].listen_.sin_port)
              << std::endl;
    for (size_t j = 0; j < this->server_vec_[i].sv_names_.size(); j++) {
      std::cout << "  server_name: " << this->server_vec_[i].sv_names_[j]
                << std::endl;
    }
    std::cout << "  location num: " << this->server_vec_[i].locations_.size()
              << std::endl;
    for (size_t j = 0; j < this->server_vec_[i].locations_.size(); j++) {
      std::cout << "  location[" << j << "]" << std::endl;
      std::cout << "    match: " << this->server_vec_[i].locations_[j].match_
                << std::endl;
      std::cout << "    allow_method: "
                << this->server_vec_[i].locations_[j].allow_method_
                << std::endl;
      std::cout << "    client_max_body_size: "
                << this->server_vec_[i].locations_[j].client_max_body_size_
                << std::endl;
      std::cout << "    root: " << this->server_vec_[i].locations_[j].root_
                << std::endl;
      std::cout << "    index: " << this->server_vec_[i].locations_[j].index_[0]
                << std::endl;
      std::cout << "    is_cgi: "
                << (this->server_vec_[i].locations_[j].is_cgi_ ? "true"
                                                               : "false")
                << std::endl;
      std::cout << "    cgi_executor: "
                << this->server_vec_[i].locations_[j].cgi_executor_
                << std::endl;

      for (std::map<int, std::string>::iterator it =
               this->server_vec_[i].locations_[j].error_pages_.begin();
           it != this->server_vec_[i].locations_[j].error_pages_.end(); it++) {
        std::cout << "    error_page: " << std::endl;
        std::cout << "      " << it->first << ": " << it->second << std::endl;
      }

      std::cout << "    autoindex: "
                << (this->server_vec_[i].locations_[j].autoindex_ ? "true"
                                                                  : "false")
                << std::endl;
      std::cout << "    return: " << this->server_vec_[i].locations_[j].return_
                << std::endl;
    }
  }
}

// int main(int ac, char **ag) {
//   Conf conf(ag[1]);
//   conf.print_conf();
// }

// デストラクタ
Conf::~Conf() {}
