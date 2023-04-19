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
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		cerr << "Error: socket() failed" << endl;
	}
	cout << "socket() success listen_fd == " << listen_fd << endl;
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT); // 8080番ポートを使用
	if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		cerr << "Error: bind() failed" << endl;
		close(listen_fd);
		return 1;
	}
	cout << "bind() success IPv4 addrece localhost port 8080" << endl;
	if (listen(listen_fd, BACKLOG) < 0) {
		cerr << "Error: listen() failed" << endl;
		close(listen_fd);
		return 1;
	}

	int epoll_fd = epoll_create(1);
	if (epoll_fd < 0) {
		cerr << "Error: epoll_create() failed" << endl;
		close(listen_fd);
		return 1;
	}
	cout << "epoll_create() success epoll_fd == " << epoll_fd << endl;

	struct epoll_event event;
	event.data.fd = listen_fd;
	event.events = EPOLLIN;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) < 0) {
		cerr << "Error: epoll_ctl() failed" << endl;
		close(listen_fd);
		close(epoll_fd);
		return 1;
	}
	cout << "epoll_ctl() success" << endl;
	struct epoll_event events[MAX_EVENTS];

	while (true) {
		usleep(1000 * 100); // 0.1秒待つ(デバッグ用
		int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000); // 最大１０個のイベントを返す
		cout << "\n\nepoll_wait() success event_count == " << event_count << endl;
		if (event_count == 0) {
			cout << "event_count == 0 continue" << endl;
			continue;
		}
		if (event_count < 0) {
			cerr << "Error: epoll_wait() failed" << endl;
		}
		for (int i = 0; i < event_count; i++) {
			int event_flag = events[i].events;
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
			if (events[i].data.fd == listen_fd) { // リッスンソケットにイベントがある→新しいクライアントからの接続
				cout << "new client connection" << endl;
				int client_fd = accept(listen_fd, NULL, NULL); // クライアントのソケットを取得
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
				cout << "epoll_ctl() success\n" << endl;
			} else {
				if (event_flag & EPOLLIN) { // クライアントからのデータ受信
					cout << "EPOLLIN" << endl;
					char buffer[MAX_BUFFER];
					int read_size = read(events[i].data.fd, buffer, MAX_BUFFER);
					if (read_size < 0) {
						cerr << "Error: read() failed" << endl;
					}
					// if (read_size == 0) { // クライアントが切断した
					// 	cout << "read() success read_size == 0" << endl;
					// 	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0) {
					// 		cerr << "Error: epoll_ctl() failed" << endl;
					// 	}
					// 	close(events[i].data.fd);
					// }
					else { // クライアントからのデータをそのまま返す
						cout << "read() success read_size == " << read_size << endl;
						cout << "client send data == " << buffer << endl;
						if (write(events[i].data.fd, buffer, read_size) < 0) {
							cerr << "Error: write() failed" << endl;
						}
					}
				}
				if (event_flag & EPOLLHUP) { // クライアントが切断した
					if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL) < 0) {
						cerr << "Error: epoll_ctl() failed" << endl;
					}
					cout << "EPOLLHUP" << endl;
					close(events[i].data.fd);
				}
				if (event_flag & EPOLLRDHUP) { // クライアントがshutdown(SHUT_WR)した
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
