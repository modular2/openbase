#include "websocket.h"
using namespace std;
using namespace restbed;
using namespace std::chrono;
shared_ptr< Service > service = nullptr;
map< string, shared_ptr< WebSocket > > sockets = { };

string base64_encode( const unsigned char* input, int length )
{
    BIO* bmem, *b64;
    BUF_MEM* bptr;
    
    b64 = BIO_new( BIO_f_base64( ) );
    bmem = BIO_new( BIO_s_mem( ) );
    b64 = BIO_push( b64, bmem );
    BIO_write( b64, input, length );
    ( void ) BIO_flush( b64 );
    BIO_get_mem_ptr( b64, &bptr );
    
    char* buff = ( char* )malloc( bptr->length );
    memcpy( buff, bptr->data, bptr->length - 1 );
    buff[ bptr->length - 1 ] = 0;
    
    BIO_free_all( b64 );
    
    return buff;
}

multimap< string, string > build_websocket_handshake_response_headers( const shared_ptr< const Request >& request )
{
    auto key = request->get_header( "Sec-WebSocket-Key" );
    key.append( "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" );
    
    Byte hash[ SHA_DIGEST_LENGTH ];
    SHA1( reinterpret_cast< const unsigned char* >( key.data( ) ), key.length( ), hash );
    
    multimap< string, string > headers;
    headers.insert( make_pair( "Upgrade", "websocket" ) );
    headers.insert( make_pair( "Connection", "Upgrade" ) );
    headers.insert( make_pair( "Sec-WebSocket-Accept", base64_encode( hash, SHA_DIGEST_LENGTH ) ) );
    
    return headers;
}

void ping_handler( void )
{
    for ( auto entry : sockets )
    {
        auto key = entry.first;
        auto socket = entry.second;
        
        if ( socket->is_open( ) )
        {
            socket->send( WebSocketMessage::PING_FRAME );
        }
        else
        {
            socket->close( );
        }
    }
}
/*void data_handler( void )
{
    for ( auto entry : sockets )
    {
        auto key = entry.first;
        auto socket = entry.second;
        
        if ( socket->is_open( ) )
        {      stringstream stream;
	          stream << fixed << setprecision(2)<<u(temperature);
               socket->send( "Temperature:"+stream.str(), [ ]( const shared_ptr< WebSocket > socket )
                    {
                        const auto key = socket->get_key( );
                        sockets.insert( make_pair( key, socket ) );
                        
                        fprintf( stderr, "Send Data message to %s.\n", key.data( ) );
                    } );
        }
        else
        {
            socket->close( );
        }
    }
}*/
/*void send_data(string message)
{
    for ( auto entry : sockets )
    {
        auto key = entry.first;
        auto socket = entry.second;
        
        if ( socket->is_open( ) )
        {    //  stringstream stream;
	        //  stream << fixed << setprecision(2)<<u(temperature);
            Bytes raw(buffer,buffer+length);
               socket->send( raw, [ ]( const shared_ptr< WebSocket > socket )
                    {
                        const auto key = socket->get_key( );
                        sockets.insert( make_pair( key, socket ) );
                        
                        fprintf( stderr, "Send Data message to %s.\n", key.data( ) );
                    } );
        }
        else
        {
            socket->close( );
        }
    }
}*/
void send_data(string message,string key)
{
    if (sockets.count(key)==1)
    { 
    auto  socket= sockets[key];
     if ( socket->is_open( ) )
     { 
   Bytes raw(message.begin(),message.end());
    socket->send( raw ,[ ]( const shared_ptr< WebSocket > socket ) { } );
     }
    }
}
void close_handler( const shared_ptr< WebSocket > socket )
{
    if ( socket->is_open( ) )
    {
        auto response = make_shared< WebSocketMessage >( WebSocketMessage::CONNECTION_CLOSE_FRAME, Bytes( { 10, 00 } ) );
        socket->send( response );
    }
    
    const auto key = socket->get_key( );
    sockets.erase( key );
    
    fprintf( stderr, "Closed connection to %s.\n", key.data( ) );
}

void error_handler( const shared_ptr< WebSocket > socket, const error_code error )
{
    const auto key = socket->get_key( );
    cleanUp(key);
    fprintf( stderr, "WebSocket Errored '%s' for %s.\n", error.message( ).data( ), key.data( ) );
}

void message_handler( const shared_ptr< WebSocket > source, const shared_ptr< WebSocketMessage > message )
{
    const auto opcode = message->get_opcode( );
    
    if ( opcode == WebSocketMessage::PING_FRAME )
    {
        auto response = make_shared< WebSocketMessage >( WebSocketMessage::PONG_FRAME, message->get_data( ) );
        source->send( response );
    }
    else if ( opcode == WebSocketMessage::PONG_FRAME )
    {
     
        return;
    }
    else if ( opcode == WebSocketMessage::CONNECTION_CLOSE_FRAME )
    {
        
        source->close( );
    }
    else if ( opcode == WebSocketMessage::BINARY_FRAME )
    {
        //We don't support binary data.
        auto response = make_shared< WebSocketMessage >( WebSocketMessage::CONNECTION_CLOSE_FRAME, Bytes( { 10, 03 } ) );
        source->send( response );
    }
    else if ( opcode == WebSocketMessage::TEXT_FRAME )
    {
        auto response = make_shared< WebSocketMessage >( *message );
        response->set_mask( 0 );      
        const auto key = source->get_key( );
        const auto data = String::format( "Received message '%.*s' from %s\n", message->get_data( ).size( ), message->get_data( ).data( ), key.data( ) );
        int len=message->get_data( ).size( );
        string reqBody(len,'0');
        int i ;
        for (i=0;i<len;i++)
            reqBody[i]= message->get_data( ).data( )[i];
        excutiveRequest(reqBody,key.data());   
    }
}

void get_wsocket_handler( const shared_ptr< Session > session )
{
    const auto request = session->get_request( );
    const auto connection_header = request->get_header( "connection", String::lowercase );
    const string device_id = request->get_path_parameter( "deviceid" );
    printf("device ID: %s\n",device_id.c_str());
    if ( connection_header.find( "upgrade" ) not_eq string::npos )
    {
        if ( request->get_header( "upgrade", String::lowercase ) == "websocket" )
        {
            const auto headers = build_websocket_handshake_response_headers( request );
            
            session->upgrade( SWITCHING_PROTOCOLS, headers, [ ]( const shared_ptr< WebSocket > socket )
            {
                if ( socket->is_open( ) )
                {
                    socket->set_close_handler( close_handler );
                    socket->set_error_handler( error_handler );
                    socket->set_message_handler( message_handler );
                    uint8_t Buf[2];
                    Buf[0]=0;
                    string messageBody=sendEventMessage("welcome","0/0/0",Buf,1);
                    Bytes raw(messageBody.begin(),messageBody.end());
                   socket->send( raw, [ ]( const shared_ptr< WebSocket > socket )
                    {
                        const auto key = socket->get_key( );                       
                        sockets.insert( make_pair( key, socket ) );
                        
                        printf( "Sent welcome message to %s.\n", key.data( ) );
                    } ); 
                }
                else
                {
                    fprintf( stderr, "WebSocket Negotiation Failed: Client closed connection.\n" );
                }
            } );
            
            return;
        }
    }
    
    session->close( BAD_REQUEST );
}
