#include "digitalOut.h"
DigitalOut::DigitalOut(int gpio)
	   {
		  _gpio= gpio;
		  
	   }

void DigitalOut::write(bool value)	
	   {   
	       buf[0]=_gpio;
		   buf[1]=value;
		   interface.callMethod(0,digitalOut_Write,buf,2);
		   interface.waitResult(buf);
		   _value=value;
	   }
bool DigitalOut::read ()
	   {
		    buf[0]=_gpio;
		   interface.callMethod(0,digitalOut_Read,buf,1);
		    interface.waitResult(buf);
		   return _value;
	   }
char  DigitalOut::flipflops()
	  {    cout<<"Send Flipflop"<< endl;
		   buf[0]=_gpio;
		   interface.callMethod(0,digitalOut_Filpflop,buf,1);		   
		   interface.waitResult(buf);
		   cout <<"result:0x"<< std::setfill('0') << std::setw(2);
		   cout<<hex<<(unsigned int)buf[0]<<endl;
		  _value=!_value;
		  return buf[0];
	  }
	  