#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <vector>
#include <string>
using namespace std;

#include "helpers.hpp"
#include "requests.hpp"

char *compute_get_request(char *host, char *url, char *query_params,
                          std::vector<std::string>& cookies, int cookies_count, std::string jwt_token)
{
    char *message = (char *)calloc(BUFLEN, sizeof(char));
    char *line = (char *)calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }
    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3: add the Authorization header if JWT token is provided
    if (!jwt_token.empty()) {
        sprintf(line, "Authorization: Bearer %s", jwt_token.c_str());
        compute_message(message, line);
    }

    memset(line, 0, sizeof(char) * LINELEN);
    sprintf(line, "Connection: keep-alive");
    compute_message(message, line);

    // Step 4 (optional): add headers and/or cookies, according to the protocol format
    if (!cookies.empty()) {
        strcpy(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i].c_str());
            if (i < cookies_count - 1) {
                strcat(line, " ; ");
            }
        }
        compute_message(message, line);
    }

    // Step 5: add final new line
    compute_message(message, "");

    free(line);
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type,
                            const char* payload, std::vector<std::string>& cookies, int cookies_count, std::string jwt_token) {
    char *message = (char *)calloc(BUFLEN, sizeof(char));
    char *line = (char *)calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    // Calculate content length based on the payload size
    sprintf(line, "Content-Length: %zu", strlen(payload));
    compute_message(message, line);

    // Step 4: add the Authorization header if JWT token is provided
    if (!jwt_token.empty()) {
        sprintf(line, "Authorization: Bearer %s", jwt_token.c_str());
        compute_message(message, line);
    }

    memset(line, 0, sizeof(char) * LINELEN);
    sprintf(line, "Connection: keep-alive");
    compute_message(message, line);

    // Step 5 (optional): add cookies
    if (!cookies.empty()) {
        strcpy(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i].c_str());
            if (i < cookies_count - 1) {
                strcat(line, " ; ");
            }
        }
        compute_message(message, line);
    }

    // Step 6: add a new line at the end of the header section
    compute_message(message, "");

    // Step 7: add the actual payload data
    strcat(message, payload);

    free(line);
    return message;
}

char *compute_delete_request(char *host, char *url, std::string jwt_token) {
    char *message = (char *)calloc(BUFLEN, sizeof(char));
    char *line = (char *)calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3: add the Authorization header if JWT token is provided
    if (!jwt_token.empty()) {
        sprintf(line, "Authorization: Bearer %s", jwt_token.c_str());
        compute_message(message, line);
    }

    memset(line, 0, sizeof(char) * LINELEN);
    sprintf(line, "Connection: keep-alive");
    compute_message(message, line);

    // Step 4: add a new line at the end of the header section
    compute_message(message, "");
    return message;
}
