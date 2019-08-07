#include <memory>
#include <string>
#include <cstdlib>
#include <sstream>
#include <restbed>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
using namespace std;
using namespace restbed;
using namespace rapidjson;
void get_method_handler( const shared_ptr< Session > session )
{
    const auto request = session->get_request( );

    size_t content_length = request->get_header( "Content-Length", 0 );

    session->fetch( content_length, [ ]( const shared_ptr< Session >& session, const Bytes& body )
    {
        Document req;
		  const char* json = "{\"project\":\"rapidjson\",\"stars\":10}";
		 
     req.Parse(string( body.begin( ), body.end( ) ).c_str());
       
         cout<<"request:"<<req["method"].GetString()<<endl;
        //perform awesome solutions logic...
        Document res;
		res.SetObject();
		Document::AllocatorType& allocator = res.GetAllocator();
		//Value value(kObjectType);
		res.AddMember("result","OK",allocator);
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        res.Accept(writer);
        string response_body=buffer.GetString();
        session->close( OK, response_body, { { "Content-Length", ::to_string( response_body.length())}, { "Content-Type", "application/json" } } );
    } );
}

int main( const int, const char** )
{    
    cout<<"JSON web API Test"<<endl;
    auto resource = make_shared< Resource >( );
    resource->set_path( "/resource" );
    resource->set_method_handler( "GET", get_method_handler );

    auto settings = make_shared< Settings >( );
    settings->set_port( 1984 );
    settings->set_default_header( "Connection", "close" );

    Service service;
    service.publish( resource );
    service.start( settings );

    return EXIT_SUCCESS;
}