#include "serverUtils.h"
#include "../../mjpg_streamer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include <sys/time.h>

#include <netdb.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <err.h>




int su_getProperSocketAddress(const char* address,const char* port,struct sockaddr* addr)
{
    struct addrinfo hints, *res, *res0;
    int error,s;
    const char* cause=0;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol= IPPROTO_TCP;

    error = getaddrinfo(address, port, &hints, &res0);
    if (error) {
        errx(1, "%s", gai_strerror(error));         /*NOTREACHED*/
    }
    s = -1;
    for (res = res0; res; res = res->ai_next) {
       s = socket(res->ai_family, res->ai_socktype,res->ai_protocol);
       if (s < 0) {
            cause = "socket";
            continue;
       }
       if ((error=connect(s, res->ai_addr, res->ai_addrlen)) < 0) {
           cause = "connect";
           close(s);
           s = -1;
           continue;
       }

       memcpy(addr,res->ai_addr,res->ai_addrlen);// this is our!
       close(s);
       break;  /* okay we got one */
    }
    if (s < 0) {
       OPRINT("Error finding host: %s", cause);
    }
    freeaddrinfo(res0);
    return s;
}






char* su_cloneStringN (const char* src,  const int srclen)
{

	int copysize; 
	char * clone = NULL;
	if(src==NULL)
	{
		return NULL;
	}	
	// null-terminated
	if(srclen==0)
	{
		copysize = strlen(src)+1;
	}else
	{
		copysize = srclen;
	}

	if(copysize<=0)
	{
		return NULL;
	}
	
	clone = (char*) malloc(copysize);
	if(!clone)
	{
		return NULL;
	}	

	memcpy(clone,src,copysize);
	
	return clone;
}

char* su_cloneString (const char* src)
{
	return su_cloneStringN(src,0);
}



void su_strnToLower(char* str, const int n)
{
    int i;
    for(i=0;i<n;i++)
    {
        str[i]= tolower(str[i]);
    }
}



bool su_startPost(int socket, const char* request, ...)
{
    char* buffer=0;
    static char stat_buffer[1024]={0};
    int size;
    int sSize=0;
    buffer=stat_buffer;
    va_list arg_ptr;

    va_start(arg_ptr, request);
    size=vsnprintf(buffer,sizeof(stat_buffer),request,arg_ptr);/*token,sName,writePort*/
    va_end(arg_ptr);
    if(size>=sizeof(stat_buffer))
    {
        buffer=malloc(size+1);
        buffer[size]=0;
    }
    va_start(arg_ptr, request);
    size=vsnprintf(buffer,size+1,request,arg_ptr);
    va_end(arg_ptr);
    DBG("header: /n%s",buffer);
    sSize=send(socket, buffer, size, MSG_NOSIGNAL|MSG_MORE);

    if(size>sizeof(stat_buffer))/*+1 was */
    {
        free(buffer);
    }else
    {
	bzero(stat_buffer,sizeof(stat_buffer));
    }

    if(sSize==size)
        return true;

    return false;
}




my_timestamp su_getCurUsec()
{
    static struct timespec ts;  
    clock_gettime(CLOCK_REALTIME,&ts);

//gettimeofday(&tv, NULL);
    return 1000000ll*ts.tv_sec+ts.tv_nsec/1000;
}


bool su_connectOutput(int* writesock, struct sockaddr writeAddr)
{
	int error=0;
    	int localSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
   


    if((error=connect(localSocket, &writeAddr, sizeof(writeAddr))) < 0)
    {
        *writesock=0;
        close(localSocket);
        DBG("Connect() failed! error %d %s\n",errno,strerror(errno));
        return false;
    }

    struct timeval time = {5, 0};
    if (setsockopt (localSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&time, sizeof(time))!=0)
    {
	*writesock=0;
	close(localSocket);       
 	DBG("setsockopt() failed! error %d %s\n",errno,strerror(errno));
        return false;
    }
    int flag = 1;
    if(setsockopt(localSocket,IPPROTO_TCP,TCP_NODELAY,(char *)&flag,sizeof(flag)) !=0)
    {
        *writesock=0;
        close(localSocket);
        DBG("setsockopt() failed! error %d %s\n",errno,strerror(errno));
        return false;
    }

    *writesock=localSocket;
    return true;
}


const char eol[]="\r\n";

bool su_sendFrame(int socket, const char* frameHeader, const unsigned char* frameData, int datasize)
{
	static char stat_buffer[1024]={0};
	int size, sSize;	
	size=snprintf(stat_buffer,sizeof(stat_buffer),frameHeader,datasize);
    	DBG("frameHeader: \n%s",stat_buffer);
    	sSize=send(socket, stat_buffer, size, MSG_NOSIGNAL|MSG_MORE);
	if(size !=sSize)
	{
		return false;
	}
	sSize =send(socket,frameData, datasize, MSG_NOSIGNAL|MSG_MORE);
	if(datasize !=sSize)
	{
		return false;
	}

	sSize=send(socket,eol,sizeof(eol),MSG_NOSIGNAL);

	return sSize==sizeof(eol);
}



///// EOCHANGES

