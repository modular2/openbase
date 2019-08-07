#include "udpRPC.h"
#include <iostream>
#define CLIENT_PORT  5200
#define SERVER_IP_ADDRESS "192.168.31.98"
#define SERVER_PORT  3200
#define NUM_IO  3
udpRPC::udpRPC()
{

}
void udpRPC::init(const char * server_ip_address)
{
 /*Create UDP socket*/
  serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
  clientSocket = socket(AF_INET, SOCK_DGRAM, 0); 
  /*Configure settings in address struct*/
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(SERVER_PORT);
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero); 
 // serverStorage_size = sizeof serverStorage; 
  /*Bind socket with address struct*/
  bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

  clientAddr.sin_family = AF_INET;  
  clientAddr.sin_addr.s_addr = inet_addr(ip_address_table[0]);  
  clientAddr.sin_port = htons(CLIENT_PORT); 
   memset(clientAddr.sin_zero, '\0', sizeof clientAddr.sin_zero); 
 
}
bool udpRPC::callMethod(uint8_t method,uint16_t address,uint16_t id,uint8_t *data,int length )
{    int index,i;
     
      index=0;  
    sbuf[index++]=method;
    sbuf[index++]=address&0xff;
    sbuf[index++]=(id>>8)&0xff;
    sbuf[index++]=id&0xff;
    sbuf[index++]=(length>>8)&0xff;
    sbuf[index++]=length&0xff;
	for (i=0;i<length;i++)
		sbuf[index++]=data[i];

    sendData(ip_address_table[(address>>8)&0x00ff],sbuf,index);
    return true;
}
int udpRPC::sendData( const char * ip_address,uint8_t *buffer,int len)
{ 
    int nBytes;
  clientAddr.sin_addr.s_addr = inet_addr(ip_address);  
  nBytes =sendto(clientSocket,buffer,len,0,(struct sockaddr *)&clientAddr,sizeof(clientAddr));
  return nBytes ;
}

 int udpRPC::receiveData(uint8_t *buffer,int * ip_address_index)
 {
  int length,i,nBytes ;
  socklen_t len;
  len=sizeof(struct sockaddr);
 nBytes = recvfrom(serverSocket,rbuf,1024,0,(sockaddr *)&from, &len);
  char * income_ip_address=   inet_ntoa(from.sin_addr);
  *ip_address_index=-1;
     for (i=0;i<NUM_IO;i++)
       if (strcmp(ip_address_table[i],income_ip_address)==0)
           *ip_address_index=i;
     buffer[0]=rbuf[0];//method
     buffer[1]=rbuf[1];//adress
      buffer[2]=rbuf[2];//id hi byte
      buffer[3]=rbuf[3];// id lq byte
     length=((rbuf[4]<<8)&0xff00)|rbuf[5];
    // for (i=0;i<length;i++)
    //     buffer[i+4]=rbuf[6+i];
    memcpy(&buffer[4],&rbuf[6],length);
   return length;
 }