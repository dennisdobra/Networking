// #include "common.hpp"

// #include <sys/socket.h>
// #include <sys/types.h>

// /*
// 	Function that receives exactly 'len' bytes into the buffer
// */
// int recv_all(int sockfd, void *buffer, size_t len) {
// 	size_t bytes_received = 0;
// 	size_t bytes_remaining = len;
// 	char *buff = (char *)buffer;

// 	while(bytes_remaining > 0) {
// 		size_t bytes = recv(sockfd, buff, bytes_remaining, 0);

// 		// Error during receiving
// 		if (bytes < 0) {
// 			return -1;
// 		} else if (bytes == 0) {
// 			break;
// 		}

// 		bytes_received += bytes;
// 		bytes_remaining -= bytes;
// 		buff += bytes;
// 	}
	
// 	// return exactly how many bytes we have read
// 	return bytes_received;
// }

// /*
// 	Function that sends exactly 'len' bytes from the buffer
// */
// int send_all(int sockfd, void *buffer, size_t len) {
//     size_t bytes_sent = 0;
//     size_t bytes_remaining = len;
//     char *buff = (char *)buffer;
    
//     while(bytes_remaining > 0) {
// 		size_t bytes = send(sockfd, buff, bytes_remaining, 0);

// 		if (bytes < 0) {
// 			return -1;
// 		}

// 		bytes_sent += bytes;
// 		bytes_remaining -= bytes;
// 		buff += bytes;
//     }

//     // return exactly how many bytes we have sent
//     return bytes_sent;
// }


#include "common.hpp"

#include <sys/socket.h>
#include <sys/types.h>

// functia recv_all face primirea a exact len octeti din buffer
int recv_all(int sockfd, void *buffer, size_t len) {
	size_t bytes_received = 0;    // nr de bytes primiti pana in mom curent
	size_t bytes_remaining = len; // nr de bytes care mai trb primiti
	char *buff = (char *)buffer;
	
	while(bytes_remaining) {      // cat timp mai sunt octeti de primit
		size_t bytes = recv(sockfd, buff, bytes_remaining, 0);

		// s-a produs o eroare sau conexiunea s a inchis
		if (bytes == 0)
			return bytes;
		
		bytes_received += bytes;
		bytes_remaining -= bytes;
		buff += bytes;
	}

	return bytes_received;
}


// functia send_all face trimiterea a exact len octeti din buffer
int send_all(int sockfd, void *buffer, size_t len) {
	size_t bytes_sent = 0;
	size_t bytes_remaining = len;
	char *buff = (char *)buffer;
	
	while(bytes_remaining) {
			size_t bytes = send(sockfd, buff, bytes_remaining, 0);

			// verific daca a aparut vreo eroare
			if (bytes == 0)
				return bytes;
			
			bytes_sent += bytes;
			bytes_remaining -= bytes;
			buff += bytes;
	}

	return bytes_sent;
}