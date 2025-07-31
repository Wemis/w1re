#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 5656
#define SERVER_IP "127.0.0.1"

int main(void) {
    char buffer[1024] = {0};

    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[-] Can't create socket");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("[-] Invalid address");
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("[-] Connection failed");
        return 1;
    }

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int max_fd = sock > STDIN_FILENO ? sock : STDIN_FILENO;

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("[-] Error");
            break;
        }
        memset(buffer, 0, sizeof(buffer));
        int val = read(sock, buffer, 1023);

        if (FD_ISSET(sock, &readfds)) {
            if (val > 0) {
                printf("%s\n", buffer);
            } else if (val == 0) {
                printf("[*] Connection closed.");
                break;

            } else {
                perror("[-] Reading error");
                break;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            memset(buffer, 0, sizeof(buffer));
            if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                buffer[strcspn(buffer, "\n")] = '\0';
                send(sock, buffer, strlen(buffer), 0);
            }
        }
    }

    close(sock);
    return 0;
}
