#if !defined(udpRPC_H)
#define udpRPC_H 
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>   
#include <string.h>
#include <stdlib.h>
class udpRPC 
{
    public:
       udpRPC();
      void init(const char *server_ip_address);
      bool callMethod(uint8_t method,uint16_t address,uint16_t id,uint8_t *data,int length );
      int  receiveData(uint8_t *buffer,int * ip_address_index);
     private:
         const char* ip_address_table[3] = { "192.168.31.18", "192.168.31.19", "192.168.31.20" };
        int serverSocket, clientSocket;
        struct sockaddr_in serverAddr, clientAddr, from;    
        socklen_t   client_addr_size;
        uint8_t _method;
	    uint8_t _address;
	    uint8_t sbuf[1024];
        uint8_t rbuf[1024];
        int sendData(const char * ip_address,uint8_t *buffer,int len);
};
#endif