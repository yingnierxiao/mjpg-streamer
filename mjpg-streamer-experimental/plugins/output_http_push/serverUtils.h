#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#define OUT_FILE_CMD_TAKE           1
#define OUT_FILE_CMD_FILENAME       2



typedef long long int my_timestamp;

#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>



/******************************************************************************
Description.: gets socket address for given host and port
Input Value.:
	const char* address - server address
	const char* port - server port
Output value:
	struct sockaddr* addr - socket address
Return Value: 
	socket on success
******************************************************************************/
extern int su_getProperSocketAddress(const char* address,const char* port,struct sockaddr* addr);




/******************************************************************************
Description.: clones null - terminated string
Input Value.:
	const char* src - source string
Return Value: 
	copied string on success, null otherwise
******************************************************************************/
extern char* su_cloneString (const char* src);


/******************************************************************************
Description.: strToLower with fixed length
In|Out Value:
       	char* str - source string to be lower-cased
Input Value.:
	const int n - length in bytes

Return Value: -
******************************************************************************/
extern void su_strnToLower(char* str, int n);

/******************************************************************************
Description.: starts sending data with prepared POST header
Input Value.:
	int socket - socket to write to
	const char* request - POST header prototype, as in printf function.
	... - arguments for POST header prototype, as in printf function.

Return Value:
	TRUE on success
******************************************************************************/
extern bool su_startPost(int socket, const char* request, ...);



/******************************************************************************
Description.: connects and initializes socket
In|Out Value:
	int *writesock - socket descriptor
Input Value.: 
	struct sockaddr writeAddr - socket address;
Return Value:
	TRUE on success
******************************************************************************/
extern bool su_connectOutput(int *writesock, struct sockaddr writeAddr);


/******************************************************************************
Description.: sends frame data
Input Value.: 
	int socket - socket to connect;
	const char* frameHeader - header to send. must content placeholder %d for data size
	const unsigned char* frameData - data to send.
	int datasize - data size;
Return Value:
	TRUE on success
******************************************************************************/
extern bool su_sendFrame(int socket, const char* frameHeader, const unsigned char* frameData, int datasize);



/******************************************************************************
Description.: gets uSecs since day begun
Input Value.:-
Return Value:
	uSecs since day begun
******************************************************************************/
extern my_timestamp su_getCurUsec();








#endif
