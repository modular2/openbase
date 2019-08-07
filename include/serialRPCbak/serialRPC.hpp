#if !defined(serialRPC_H)
#define serialRPC_H 
#include "serialPort.hpp"
#include <stdint.h>
class serialRPC 
{
private:
   
	uint8_t _method;
	uint8_t _address;
	uint8_t sbuf[256];
    uint8_t rbuf[256];
	unsigned int calculateCRC16( uint8_t *puchMsg,unsigned int  usDataLen);
public:
    serialRPC();
    bool init(const char * dev);	
    bool callMethod(uint8_t address,uint8_t Method,uint8_t *data,int length );
	int waitResult(uint8_t * data);
};
#endif