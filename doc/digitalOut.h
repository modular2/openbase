#if !defined(DIGITALOUT_H)
#define DIGITALOUT_H 
#include "serialRPC.hpp"
#include <iostream>
#include <iomanip>
#include "methodTable.h"
extern serialRPC interface;
class DigitalOut
{   private:    
	  int _gpio;
	  bool _value;
	  uint8_t buf[32];
	  
	public:
	   DigitalOut(int gpio);
	   void write(bool value);
	   bool read ();	
	 char  flipflops();

};
#endif