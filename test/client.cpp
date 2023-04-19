#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <arpa/inet.h>



#define cout std::cout
#define cerr std::cerr
#define endl std::endl
#define SERVER_PORT 8080
#define SERVER_IP "127.0.0.1"


/* 簡易的なクライアント側のコード */

int main() {
	int server_fd[100];
	for (int i = 0; i < 100; ++i) {
		server_fd[i] = socket(AF_INET, SOCK_STREAM, 0);
		if (server_fd[i] < 0) {
			cerr << "Error: socket() failed" << endl;
			return 1;
		}
		cout << "socket() success server_fd[" << i << "] == " << server_fd[i] << endl;
		struct sockaddr_in address;
		bzero(&address, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_port = htons(SERVER_PORT);
		inet_pton(AF_INET, SERVER_IP, &address.sin_addr.s_addr);
		if (connect(server_fd[i], (struct sockaddr *)&address, sizeof(address)) < 0) {
			cerr << "Error: connect() failed" << endl;
			close(server_fd[i]);
			return 1;
		}
		cout << "connect() success" << endl;

		send(server_fd[i], "Hello", 5, 0);
		cout << "send() success" << endl;
		char buffer[1024];
		recv(server_fd[i], buffer, 1024, 0);
		cout << "recv() success " << buffer << endl;
		write(STDOUT_FILENO, buffer, strlen(buffer));
		write(STDOUT_FILENO, "\n\n", 2);
		shutdown(server_fd[i], SHUT_WR);
	}
	for (int i = 0; i < 100; ++i) {
		close(server_fd[i]);
	}
	return 0;
}
