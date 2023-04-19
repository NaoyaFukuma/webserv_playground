#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <fcntl.h>


#define PORT 8080
#define MAX_BUFFER 1024
#define cout std::cout
#define cerr std::cerr
#define endl std::endl
#define MAX_EVENTS 10 // 10個以上のイベントがあれば次回のepoll_wait()で返される
#define BACKLOG 3 // listen()の第２引数

int main(int ac, char **ag) {
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		cerr << "Error: socket() failed" << endl;
	}
	cout << "socket() success server_fd == " << server_fd << endl;
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT); // 8080番ポートを使用
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		cerr << "Error: bind() failed" << endl;
		close(server_fd);
		return 1;
	}
	cout << "bind() success IPv4 addrece localhost port 8080" << endl;
	if (listen(server_fd, BACKLOG) < 0) {
		cerr << "Error: listen() failed" << endl;
		close(server_fd);
		return 1;
	}

	int epoll_fd = epoll_create(1);
	if (epoll_fd < 0) {
		cerr << "Error: epoll_create() failed" << endl;
		close(server_fd);
		return 1;
	}
	cout << "epoll_create() success epoll_fd == " << epoll_fd << endl;

	struct epoll_event event;
	event.data.fd = server_fd;
	event.events = EPOLLIN;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
		cerr << "Error: epoll_ctl() failed" << endl;
		close(server_fd);
		close(epoll_fd);
		return 1;
	}
	cout << "epoll_ctl() success" << endl;
	struct epoll_event events[MAX_EVENTS];

	while (true) {
		int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1); // -1で無限に待つ 最大１０個のイベントを返す
		cout << "epoll_wait() success event_count == " << event_count << endl;
		if (event_count < 0) {
			cerr << "Error: epoll_wait() failed" << endl;
		}
		for (int i = 0; i < event_count; i++) {
			int event_flag = events[i].events;
			cout << "events[" << i << "].events == " << event_flag << endl;
			if (event_flag & EPOLLERR) { // エラー
				cerr << "Error: epoll_wait() failed" << endl;
				if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0) {
					cerr << "Error: epoll_ctl() failed" << endl;
				}
				close(events[i].data.fd);
				continue;
			}
			if (event_flag & EPOLLHUP) { // ソケットが切断された
				cout << "EPOLLHUP" << endl;
				if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0) {
					cerr << "Error: epoll_ctl() failed" << endl;
				}
				close(events[i].data.fd);
				continue;
			}
			if (events[i].data.fd == server_fd) { // リッスンソケットにイベントがある→新しいクライアントからの接続
				cout << "new client connection" << endl;
				int client_fd = accept(server_fd, NULL, NULL); // クライアントのソケットを取得
				cout << "accept() success client_fd == " << client_fd << endl;
				fcntl(client_fd, F_SETFL, O_NONBLOCK); // クライアントのソケットをノンブロッキングに設定
				if (client_fd < 0) {
					cerr << "Error: accept() failed" << endl;
				}
				event.data.fd = client_fd;
				event.events = EPOLLIN | EPOLLRDHUP;
				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
					cerr << "Error: epoll_ctl() failed" << endl;
				}
				cout << "epoll_ctl() success" << endl;
			} else {
				if (event_flag & EPOLLIN) { // クライアントからのデータ受信
					cout << "EPOLLIN" << endl;
					char buffer[MAX_BUFFER];
					int read_size = read(events[i].data.fd, buffer, MAX_BUFFER);
					if (read_size < 0) {
						cerr << "Error: read() failed" << endl;
					}
					if (read_size == 0) { // クライアントが切断した
						cout << "read() success read_size == 0" << endl;
						if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0) {
							cerr << "Error: epoll_ctl() failed" << endl;
						}
						close(events[i].data.fd);
					} else { // クライアントからのデータをそのまま返す
						cout << "read() success read_size == " << read_size << endl;
						if (write(events[i].data.fd, buffer, read_size) < 0) {
							cerr << "Error: write() failed" << endl;
						}
					}
				} else if (event_flag & EPOLLHUP) { // クライアントが切断した
					if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0) {
						cerr << "Error: epoll_ctl() failed" << endl;
					}
					cout << "EPOLLHUP" << endl;
					close(events[i].data.fd);
				} else if (event_flag & EPOLLRDHUP) { // クライアントがshutdown(SHUT_WR)した
					cout << "EPOLLRDHUP" << endl;
					shutdown(events[i].data.fd, SHUT_RD);
					if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0) {
						cerr << "Error: epoll_ctl() failed" << endl;
					}
					// close(events[i].data.fd);//サーバー→クライアントへの送信がすべて終わったら、サーバー→クライアントへの送信用のソケットを閉じる
				}
			}
		}
	}

}
