#define app_rpc_request        0
#define app_rpc_Result         1
// base service rpc method code
#define app_rpc_ping           128 
#define app_rpc_Register_rpc       129
#define app_rpc_Register_app   130
#define app_rpc_get_rpclist     131
#define app_rpc_get_applist     132
#define app_rpc_Readstream      133
// io rpc method code
#define digitalOut_Write  	   5
#define digitalOut_Read    	   6
#define digitalOut_Flipflop    7
#define analog_Start           8
#define interruptin_Start      9
#define modbus_query           10
#define modbus_wait_query      11

 // RPC method mode
#define METHOD_ONLY        0
#define Normal               1
#define StartStream          2
#define StopStream           3
#define startReadStream      4
#define stopReadStream       5
#define MQTTStream           6

// error_code
#define OK                         0
#define ERR_REQ_SYNTAX             1
#define ERR_REQ_SYNTAX1             11
#define ERR_REQ_ID                 2
#define ERR_REQ_NO_SUCH_METHODE    3
#define ERR_REQ_NO_METHOD_RESULT   4
#define ERR_REQ_START_NOT_INT      5
#define ERR_REQ_NO_NAME            6
#define ERR_REQ_NO_TYPE            7
#define ERR_REQ_NO_VALUE          8
#define ERR_REQ_NO_PARAMS         9
#define ERR_REQ_NO_PIN            10
#define ERR_REQ_NO_MODE           12
#define ERR_REQ_IL_MODE           13
#define ERR_REQ_NO_PATH           14  
#define ERR_REQ_STREAM_EXIST      15
#define ERR_REQ_STREAM_NAME_NOT_EXIST 16        
#define ERR_REQ_NO_PORT            17  
#define ERR_REQ_APP_EXIST          18                   