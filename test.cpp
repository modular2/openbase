#include <string>
#include <cstring>
#include <memory>
#include <cstdlib>
#include <fstream>
#include <restbed>
#include <chrono>
#include <streambuf>
#include <sstream>
#include <restbed>
#include <iostream>
using namespace std;
using namespace restbed;
class splitstring : public string {
    vector<string> flds;
public:
    splitstring(char *s) : string(s) { };
    vector<string>& split(char delim, int rep=0);
};

vector<string>& splitstring::split(char delim, int rep) {
    if (!flds.empty()) flds.clear();  // empty vector if necessary
    string work = data();
    string buf = "";
    int i = 0;
    while (i < work.length()) {
        if (work[i] != delim)
            buf += work[i];
        else if (rep == 1) {
            flds.push_back(buf);
            buf = "";
        } else if (buf.length() > 0) {
            flds.push_back(buf);
            buf = "";
        }
        i++;
    }
    if (!buf.empty())
        flds.push_back(buf);
    return flds;
}
bool sendFile(string path,string type,const shared_ptr< Session > session )
{
  ifstream stream( path, ifstream::in );
    
    if ( stream.is_open() )
    {
        const string body = string( istreambuf_iterator< char >( stream ), istreambuf_iterator< char >( ) );
        
        const multimap< string, string > headers
        {
            { "Content-Type", type },
            { "Content-Length", to_string( body.length( ) ) }
        };
        
        session->close( OK, body, headers );
    }
    else
    {
        session->close( NOT_FOUND );
    };
    return true;
}
void post_method_handler( const shared_ptr< Session > session )
{
     const auto& request = session->get_request( );
    string path=request->get_path();
      cout<<"request path: "<<path<<endl;
   
      const string body ="{\"jsonrpc\":\"2.0\",\"result\":\"OK\"}";  
        const multimap< string, string > headers
        {
            { "Content-Type", "application/x-www-form-urlencoded"},
            { "Content-Length", to_string( body.length( ) ) }
        };
        
        session->close( OK, body, headers );
}
void get_method_handler( const shared_ptr< Session > session )
{
    const auto& request = session->get_request( );
    string path=request->get_path();
     cout<<"request path: "<<path<<endl;
 
        char * writable = new char[path.size() + 1];
       std::copy(path.begin(), path.end(), writable);
       writable[path.size()] = '\0';
       splitstring uri(writable);
       vector<string> detail= uri.split('/') ;
        delete[] writable;  
     const string filename = detail[2];
     string type=detail[1];
    if (type.compare("views")==0)
    { 
      sendFile("./views/"+filename,"text/html",session);
     
     } 
     else if (type.compare("css")==0)
    {
       sendFile("./css/"+filename,"text/css",session);
    }
    else if  (type.compare("js")==0)
    {
        sendFile("./js/"+filename,"text/js",session);
    } else if  (type.compare("images")==0)
    {      char * p=strchr(path.c_str(), '.');
           string ext(p);
          sendFile("./images/"+filename,"image/"+ext,session);
    } else
        session->close( NOT_FOUND );
   
}

int main( const int, const char** )
{
    auto resource = make_shared< Resource >( );
    resource->set_path( "/home/.*/.*" );
    resource->set_method_handler( "GET", get_method_handler );

    auto resource1 = make_shared< Resource >( );
     resource1->set_path( "/api/.*" );
    resource1->set_method_handler( "POST", post_method_handler );

    auto settings = make_shared< Settings >( );
    settings->set_port( 2019 );
    settings->set_default_header( "Connection", "close" );
    
    Service service;
    service.publish( resource );
    service.publish( resource1 );
    service.start( settings );
    
    return EXIT_SUCCESS;
}