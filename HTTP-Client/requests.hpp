#ifndef _REQUESTS_
#define _REQUESTS_

#include <vector>
#include <string>
using namespace std;

// computes and returns a GET request string (query_params
// and cookies can be set to NULL if not needed)
char *compute_get_request(char *host, char *url, char *query_params,
							std::vector<std::string>& cookies, int cookies_count, string jwt_token);

// computes and returns a POST request string (cookies can be NULL if not needed)
char *compute_post_request(char *host, char *url, char* content_type,
							const char* payload, std::vector<std::string>& cookies, int cookies_count, string jwt_token);

// computes and returrns a DELETE request string
char *compute_delete_request(char *host, char *url, std::string jwt_token);

#endif
