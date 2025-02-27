#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "helpers.hpp"
#include "requests.hpp"
#include "json.hpp"

using namespace std;
using json_t = nlohmann::json;

#define REGISTER_ROUTE "/api/v1/tema/auth/register"
#define LOGIN_ROUTE "/api/v1/tema/auth/login"
#define ACCESS_ROUTE "/api/v1/tema/library/access"
#define BOOKS_ROUTE "/api/v1/tema/library/books"
#define LOGOUT_ROUTE "/api/v1/tema/auth/logout"
#define PAYLOAD_TYPE "application/json"

/* FUNCTII AUXILIARE */

/*
    verifica corectitudinea input-ului pt username si password
    cazuri de eroare:
    - inputul este NULL
    - inputul este ""
    - inputul contine spatii
*/
int check_input(char *s) {
    if (strlen(s) == 0 || s == NULL) {
        return 0;
    }

    for (size_t i = 0; i < strlen(s); i++) {
        if (s[i] == ' ') {
            return 0;
        }
    }
    return 1;
}

/*
    verifica corectitudinea unui numar (id sau page_count)
    cazuri de eroare:
    - numarul este NULL
    - numarul este ""
    - numarul contine alte caractere in afara de cifre
*/
int check_number(char *s)
{
    if (strlen(s) == 0 || s == NULL) {
        return 0;
    }

    for (size_t i = 0; i < strlen(s); i++) {
        if (!isdigit(s[i])) {
            return 0;
        }
    }
    return 1;
}

/*
    iau din response primul cookie din lista de cookie-uri
*/
string get_cookie(char *response)
{
    char *cookie_start = strstr(response, "Set-Cookie: ");
    if (cookie_start != nullptr) {
        // pozitia de start a cookie-ului
        cookie_start += strlen("Set-Cookie: ");

        // pozitia de final a cookie-ului
        char *cookie_end = strstr(cookie_start, ";");
        if (cookie_end != nullptr) {
            size_t cookie_length = cookie_end - cookie_start;
            return string(cookie_start, cookie_length);
        } else {
            return string(cookie_start);
        }
    }
    return "";
}

/*
    iau din response token-ul primit
*/
string get_jwt_token(char *response)
{
    char *json_start = strchr(response, '{');
    if (json_start == nullptr) {
        return "";
    }

    char *token_start = strstr(json_start, "\"token\":\"");
    if (token_start != nullptr) {
        token_start += strlen("\"token\":\"");

        char *token_end = strchr(token_start, '"');
        if (token_end != nullptr) {
            size_t token_length = token_end - token_start;
            return string(token_start, token_length);
        }
    }
    return "";
}

int return_status_code(char *response)
{
    char *header_start = strstr(response, "HTTP/1.1 ");
    if (header_start == nullptr) {
        return -1; // linia cu 'HTTP/1.1' nu este gasita
    }

    // ma mut cu pointerul la inceputul codului
    header_start += strlen("HTTP/1.1 ");

    // iau codul
    char status_code[4];
    strncpy(status_code, header_start, 3);
    status_code[3] = '\0';

    // convertesc codul la int
    return atoi(status_code);
}

void show_all_books(char *response)
{
    char *json_start = strchr(response, '[');
    if (json_start == nullptr) {
        cout << "No books found.\n";
        return ;
    }

    json_t books = json_t::parse(json_start);

    cout << "[" << '\n';
    for (const auto& book : books) {
        cout << "   {\n";
        cout << "   id: " << book["id"] << ",\n";
        cout << "   title: " << book["title"] << '\n';
        cout << "   },\n";
    }
    cout << "]\n";
}

void show_book_info(char *response, int book_id)
{
    char *json_start = strchr(response, '{');
    if (json_start == nullptr) {
        std::cout << "No JSON data found in the response." << std::endl;
        return;
    }

    json_t book = json_t::parse(json_start);

    cout << "{" << endl;
    cout << "    \"id\": " << book["id"] << "," << endl;
    cout << "    \"title\": " << book["title"] << "," << endl;
    cout << "    \"author\": " << book["author"] << "," << endl;
    cout << "    \"publisher\": " << book["publisher"] << "," << endl;
    cout << "    \"genre\": " << book["genre"] << "," << endl;
    cout << "    \"page_count\": " << book["page_count"] << endl;
    cout << "}" << endl;
}

int main(int argc, char *argv[]) {
    char serverIP[] = "34.246.184.49";
    int serverPORT = 8080;

    string session_cookie, jwt_token;
    bool logged_in = false;
    bool library_access = false;

    char *message;
    char *response;
    int sockfd;

    while(1) {
        char command[256];
        cin.getline(command, 256);

        if (strcmp(command, "register") == 0) {
            char username[256];
            char password[256];
            cout << "username=";
            cin.getline(username, 256);
            cout << "password=";
            cin.getline(password, 256);

            // validarea datelor
            if (check_input(username) == 0 || check_input(password) == 0) {
                cout << "ERROR: Invalid username or password\n";
                continue;
            }

            // Convertirea datelor de intrare în format JSON
            json_t json_obj;
            json_obj["username"] = username;
            json_obj["password"] = password;
            string payload = json_obj.dump();

            // setez REGISTER_ROUTE & PAYLOAD_TYPE
            char url[] = REGISTER_ROUTE;
            char pay_type[] = PAYLOAD_TYPE;

            // creez un vector<string> cookies gol deoarece nu pot da mai jos ca param NULL
            std::vector<std::string> cookies;

            sockfd = open_connection(serverIP, serverPORT, AF_INET, SOCK_STREAM, 0);
            message = compute_post_request(serverIP, url, pay_type, payload.c_str(), cookies, 0, "");
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            // check status code
            if (return_status_code(response) >= 200 && return_status_code(response) < 300) {
                cout << "SUCCESS: Registered successfully\n";
            } else {
                cout << "ERROR: The username " << username << " is taken!\n";
            }
        } else if (strcmp(command, "login") == 0) {
            char username[256];
            char password[256];
            cout << "username=";
            cin.getline(username, 256);
            cout << "password=";
            cin.getline(password, 256);

            // validarea datelor
            if (check_input(username) == 0 || check_input(password) == 0) {
                cout << "ERROR: Invalid username or password\n";
                continue;
            }

            if (logged_in == true) {
                cout << "ERROR: You must logout first\n";
                continue;
            }

            // Convertirea datelor de intrare în format JSON
            json_t json_obj;
            json_obj["username"] = username;
            json_obj["password"] = password;
            string payload = json_obj.dump();

            // setez REGISTER_ROUTE & PAYLOAD_TYPE
            char url[] = LOGIN_ROUTE;
            char pay_type[] = PAYLOAD_TYPE;

            std::vector<std::string> cookies;
            sockfd = open_connection(serverIP, serverPORT, AF_INET, SOCK_STREAM, 0);
            message = compute_post_request(serverIP, url, pay_type, payload.c_str(), cookies, 0, "");
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            // iau cookie-ul din mesaj
            session_cookie = get_cookie(response);

            // check status code
            if (return_status_code(response) >= 200 && return_status_code(response) < 300) {
                cout << "SUCCESS: login successfully\n";
                logged_in = true;
            } else {
                cout << "ERROR: No account with this username\n";
            }
        } else if (strcmp(command, "enter_library") == 0) {
            if (logged_in == false) {
                cout << "ERROR: you must login first\n";
                continue;
            }

            char url[] = ACCESS_ROUTE;

            // trb sa creez un mesaj pe care sa il trimit la 'url' care
            // sa contina cookie urile primite mai devreme
            vector<string> cookies;
            cookies.push_back(session_cookie);

            sockfd = open_connection(serverIP, serverPORT, AF_INET, SOCK_STREAM, 0);
            message = compute_get_request(serverIP, url, NULL, cookies, cookies.size(), "");
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            // trebuie sa iau jwt_token din response - il voi folosi mai departe
            // acesta este dovada accesului in biblioteca
            jwt_token = get_jwt_token(response);

            // check status code
            if (return_status_code(response) >= 200 && return_status_code(response) < 300) {
                cout << "SUCCESS: Access granted\n";
                library_access = true;
            } else {
                cout << "ERROR: Access denied\n";
            }
        } else if (strcmp(command, "get_books") == 0) {
            if (library_access == false) {
                cout << "ERROR: You don't have access in library\n";
                continue;
            }

            char url[] = BOOKS_ROUTE;
            vector<string> cookies;

            sockfd = open_connection(serverIP, serverPORT, AF_INET, SOCK_STREAM, 0);
            message = compute_get_request(serverIP, url, NULL, cookies, 0, jwt_token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            // check status code
            if (return_status_code(response) >= 200 && return_status_code(response) < 300) {
                cout << "SUCCESS\n";
                show_all_books(response);
            } else {
                cout << "ERROR: Could not enter the library\n";
            }
        } else if (strcmp(command, "get_book") == 0) {
            char id[256];
            cout << "id=";
            cin.getline(id, 256);

            // verific daca id ul este numar
            if (check_number(id) == 0) {
                cout << "Invalid book id\n";
                continue;
            }

            if (library_access == false) {
                cout << "ERROR: You don't have access in library\n";
                continue;
            }

            // obtin url ul catre cartea pe care o doresc
            char url[] = BOOKS_ROUTE;
            strcat(url, "/");
            strcat(url, id);

            vector<string> cookies;

            sockfd = open_connection(serverIP, serverPORT, AF_INET, SOCK_STREAM, 0);
            message = compute_get_request(serverIP, url, NULL, cookies, 0, jwt_token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            // check status code
            if (return_status_code(response) >= 200 && return_status_code(response) < 300) {
                cout << "SUCCESS\n";
                show_book_info(response, atoi(id));
            } else {
                cout << "ERROR: No book was found with the given id\n";
            }
        } else if (strcmp(command, "add_book") == 0) {
            char title[256], author[256], genre[256], publisher[256], page_count[256];
            cout << "title=";
            cin.getline(title, 256);
            cout << "author=";
            cin.getline(author, 256);
            cout << "genre=";
            cin.getline(genre, 256);
            cout << "publisher=";
            cin.getline(publisher, 256);
            cout << "page_count=";
            cin.getline(page_count, 256);

            // verific daca page_count este valid
            if (check_number(page_count) == 0) {
                cout << "ERROR: Invalid number for page_count\n";
                continue;
            }

            if (library_access == false) {
                cout << "ERROR: You don't have access in library\n";
                continue;
            }

            // Convertirea datelor de intrare în format JSON
            json_t json_obj;
            json_obj["title"] = title;
            json_obj["author"] = author;
            json_obj["genre"] = genre;
            json_obj["publisher"] = publisher;
            json_obj["page_count"] = stoi(page_count);
            string payload = json_obj.dump();

            char url[] = BOOKS_ROUTE;
            char pay_type[] = PAYLOAD_TYPE;

            vector<string> cookies;

            sockfd = open_connection(serverIP, serverPORT, AF_INET, SOCK_STREAM, 0);
            message = compute_post_request(serverIP, url, pay_type, payload.c_str(), cookies, 0, jwt_token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            // check status code
            if (return_status_code(response) >= 200 && return_status_code(response) < 300) {
                cout << "SUCCESS\n";
            } else {
                cout << "ERROR: Could not add book\n";
            }
        } else if (strcmp(command, "delete_book") == 0) {
            char id[256];
            cout << "id=";
            cin.getline(id, 256);

            // verific daca id ul este valid
            if (check_number(id) == 0) {
                cout << "Invalid book id\n";
                continue;
            }

            if (library_access == false) {
                cout << "ERROR: You don't have access in library\n";
                continue;
            }

            char url[] = BOOKS_ROUTE;
            strcat(url, "/");
            strcat(url, id);

            sockfd = open_connection(serverIP, serverPORT, AF_INET, SOCK_STREAM, 0);
            message = compute_delete_request(serverIP, url, jwt_token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            // check status code
            if (return_status_code(response) >= 200 && return_status_code(response) < 300) {
                cout << "SUCCESS\n";
            } else {
                cout << "ERROR: Could not delete book or book is already deleted\n";
            }
        } else if (strcmp(command, "logout") == 0) {
            char url[] = LOGOUT_ROUTE;

            vector<string> cookies;
            cookies.push_back(session_cookie);

            sockfd = open_connection(serverIP, serverPORT, AF_INET, SOCK_STREAM, 0);
            message = compute_get_request(serverIP, url, NULL, cookies, cookies.size(), jwt_token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            close(sockfd);

            // trb sa sterg session_cookie si jwt_token
            session_cookie = "";
            jwt_token = "";

            // check status code
            if (return_status_code(response) >= 200 && return_status_code(response) < 300) {
                cout << "SUCCESS: logout successfully\n";
                logged_in = false;
                library_access = false;
            } else {
                cout << "ERROR: Already logged out\n";
            }
        } else if (strcmp(command, "exit") == 0) {
            break;
        } else {
            cout << "Invalid command.\n";
        }
    }

    return 0;
}
