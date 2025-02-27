# HTTP Client for Online Library System

## Overview
This program implements an HTTP client for interacting with an online library system. The system is hosted on a server that stores a digital library with books. The client allows users to register, authenticate, access the library, manage books, and log out. Communication with the server is done via HTTP requests, with responses processed accordingly.

## Functionality
The program operates in a loop, executing commands from STDIN until the `exit` command is encountered.

## HTTP Requests & Server Interactions
- The server hosts a digital library containing books.
- Requests such as `GET`, `POST`, and `DELETE` are used for different operations.
- JSON responses from the server are parsed to extract relevant data.

## Available Commands

### 1. `register`
Registers a new user by sending a `POST` request with a username and password. Input is validated to ensure non-empty credentials without spaces.

### 2. `login`
Authenticates an existing user via a `POST` request. If successful, a session cookie is stored.

### 3. `enter_library`
Grants access to the library by sending a `GET` request with the session cookie. If successful, a JWT token is retrieved and stored.

### 4. `get_books`
Retrieves all books from the library using a `GET` request with the JWT token. The response is displayed in JSON format.

### 5. `get_book`
Retrieves details of a specific book by ID. The ID is validated (digits only), and a `GET` request is sent. The response is parsed and displayed.

### 6. `add_book`
Adds a new book to the library via a `POST` request. The user provides title, author, genre, publisher, and page count, all of which are validated before submission.

### 7. `delete_book`
Deletes a book by ID using a `DELETE` request. The ID is validated before sending the request.

### 8. `logout`
Logs out the user by sending a `GET` request with the session cookie and JWT token. Upon success, both credentials are deleted.

### 9. `exit`
Terminates the program.

## JSON Handling with `nlohmann/json`
The `nlohmann/json` library is used for:
1. Creating JSON objects for `POST` requests.
2. Parsing JSON responses from the server.

## Notes
- Input validation ensures correctness and security.
- Functions handle HTTP requests, response parsing, and credential management.
- Extracts session cookies, JWT tokens, status codes, and book data in JSON format.
