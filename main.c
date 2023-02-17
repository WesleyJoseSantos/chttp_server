/**
 * @file main.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-02-17
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdio.h>
#include <string.h>
#include "WinSock2.h"

typedef enum http_method {
    HTTP_METHOD_UNKNOWN,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE
    //...
} http_method_t;

typedef struct {
    http_method_t method;
    char url[256];
    char body[1023];
} http_request_t;

static SOCKET listen_socket;

static int http_server_start(void);
static int http_server_loop(void);
static void http_server_stop(void);

http_method_t http_server_request_get_method(char *request);
int http_server_request_get_url(char* request, char* buf, size_t len);
int http_server_request_get_body(char *request, char *buffer, size_t len);
int http_server_parse_request(char* request_str, http_request_t* request);

int main()
{
    if(http_server_start() == 0)
    {
        http_server_loop();
    }
    http_server_stop();
}

static int http_server_start(void)
{
    WSADATA wsa_data;
    int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (result != 0)
    {
        printf("Erro ao iniciar o Winsock.\n");
        return 1;
    }

    listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET)
    {
        printf("Erro ao criar o socket.\n");
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(80);

    result = bind(listen_socket, (SOCKADDR *)&server_address, sizeof(server_address));
    if (result == SOCKET_ERROR)
    {
        printf("Erro ao realizar o bind.\n");
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    result = listen(listen_socket, SOMAXCONN);
    if (result == SOCKET_ERROR)
    {
        printf("Erro ao escutar o socket.\n");
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    printf("Servidor HTTP iniciado.\n");
}

static int http_server_loop(void)
{
    while (1)
    {
        SOCKET client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET)
        {
            printf("Erro ao aceitar a conexão.\n");
            closesocket(listen_socket);
            WSACleanup();
            return 1;
        }

        char url[128] = {0};
        char body[256] = {0};
        char request[1024] = {0};
        int request_size = recv(client_socket, request, sizeof(request), 0);
        if (request_size == SOCKET_ERROR)
        {
            printf("Erro ao receber a requisição.\n");
            closesocket(client_socket);
            closesocket(listen_socket);
            WSACleanup();
            return 1;
        }

        printf("%.*s", request_size, request);

        http_request_t req;
        http_server_parse_request(request, &req);

        printf("\nMETHOD: %d\n", req.method);
        printf("URL: %s\n", req.url);
        printf("BODY: %s\n", req.body);

        char response[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Hello World!</h1></body></html>";
        int response_size = strlen(response);

        int result = send(client_socket, response, response_size, 0);
        if (result == SOCKET_ERROR)
        {
            printf("Erro ao enviar a resposta.\n");
            closesocket(client_socket);
            closesocket(listen_socket);
            WSACleanup();
            return 1;
        }

        closesocket(client_socket);
    } 
    return 0;
}

static void http_server_stop(void)
{
    closesocket(listen_socket);
    WSACleanup();
}

http_method_t http_server_request_get_method(char *request) {
    if (strncmp(request, "GET", 3) == 0) {
        return HTTP_METHOD_GET;
    } else if (strncmp(request, "POST", 4) == 0) {
        return HTTP_METHOD_POST;
    } else if (strncmp(request, "PUT", 3) == 0) {
        return HTTP_METHOD_PUT;
    } else if (strncmp(request, "DELETE", 6) == 0) {
        return HTTP_METHOD_DELETE;
    } else {
        return HTTP_METHOD_UNKNOWN;
    }
}

int http_server_request_get_url(char* request, char* buf, size_t len) {
    // Procura pela primeira ocorrência de ' ' e pula o espaço
    const char* first_space = strchr(request, ' ');
    if (!first_space) {
        return -1;  // Erro: não encontrou espaço antes da URL
    }
    const char* url_start = first_space + 1;

    // Procura pela próxima ocorrência de ' ' que termina a URL
    const char* url_end = strchr(url_start, ' ');
    if (!url_end) {
        return -1;  // Erro: não encontrou espaço depois da URL
    }

    // Copia a URL para o buffer fornecido e termina com nulo
    size_t url_len_with_null = len - 1;
    size_t url_len_copied = url_end - url_start;
    if (url_len_copied > url_len_with_null) {
        url_len_copied = url_len_with_null;
    }
    memcpy(buf, url_start, url_len_copied);
    buf[url_len_copied] = '\0';

    return 0;  // Sucesso
}

int http_server_request_get_body(char *request, char *buffer, size_t len)
{
    size_t pos = strlen(request);
    int crlf_count = 0;

    // Procura a posição do final do cabeçalho
    while (pos > 0 && crlf_count < 2) {
        if (request[pos] == '\n' && request[pos - 1] == '\r') {
            crlf_count++;
        }
        pos--;
    }

    // Verifica se o corpo da mensagem está presente na requisição
    if (crlf_count < 2) {
        return -1;
    }

    // Copia o corpo da mensagem para o buffer
    size_t body_len = strlen(request + pos);
    if (body_len > len) {
        return -2;
    }
    memcpy(buffer, request + pos + 4, body_len);
    buffer[body_len - 4] = '\0';

    return 0;
}

int http_server_parse_request(char* request_str, http_request_t* request) {
    request->method = http_server_request_get_method(request_str);

    if (http_server_request_get_url(request_str, request->url, sizeof(request->url)) == -1) {
        return -1;
    }

    if (http_server_request_get_body(request_str, request->body, sizeof(request->body)) == -1) {
        return -1;
    } 

    return 0;
}
