#include <memory>
#include <string>
#include <cstdlib>
#include <sstream>
#include <jsonbox.h>
#include <restbed>

using namespace std;
using namespace restbed;

void get_method_handler( const shared_ptr< Session > session )
{
    const auto request = session->get_request( );

    size_t content_length = request->get_header( "Content-Length", 0 );

    session->fetch( content_length, [ ]( const shared_ptr< Session >& session, const Bytes& body )
    {
        JsonBox::Value json;
        json.loadFromString( string( body.begin( ), body.end( ) ) );

        //perform awesome solutions logic...

        stringstream stream;
        json.writeToStream( stream );
        string response_body = stream.str( );

        session->close( OK, response_body, { { "Content-Length", ::to_string( response_body.length( ) }, { "Content-Type": "application/json" } } );
    } );
}

int main( const int, const char** )
{
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