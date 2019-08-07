#include <cstdlib>
#include <map>
#include <chrono>
#include <string>
#include <cstring>
#include <iomanip> // setprecision cout
#include <memory>
#include <utility>
#include <cstdlib>
#include <fstream>
#include <restbed>
#include <system_error>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
 #include "rapidjson/document.h"
 #include "rapidjson/writer.h"
 #include "rapidjson/stringbuffer.h"
#include "baseservice.h"
using namespace std;
using namespace restbed;
using namespace rapidjson;
string base64_encode( const unsigned char* input, int length );

void get_wsocket_handler( const shared_ptr< Session > session );
void data_handler( void );
//void send_data(uint8_t *buffer,int length);
void send_data(string message,string key);