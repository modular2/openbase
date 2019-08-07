#include <string>
#include <memory>
#include <cstdlib>
#include <fstream>
#include <restbed>
#include <chrono>
#include <streambuf>
#include <sstream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "udpRPC/udpRPC.h"
#include "udpRPC/methodtable.h"
#include "websocket/websocket.h"
#include "mqtt/mqtt.h"
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include "base64B.h"
using namespace std;
using namespace restbed;
using namespace rapidjson;
using namespace std::chrono;

#define NUM_io_rpc  7
#define NUM_base_rpc 6
#define appMethod 0 
//serialRPC interface;
udpRPC interface;
uint8_t rBuf[1024];
uint8_t ioBuf[1024];
sem_t response_sem;
mqtt Mqtt;
int  base_id;
std::mutex mtx;
//RPC 注册表
struct app_register_entry {
    int port;
    int status;
};
struct rpc_register_entry {
    string owner;
    int method_index;
    string path;
};
const string base_rpc_table[]={"ping","register.rpc","register.app","get.rpclist","get.applist","read.stream"};
const string io_rpc_table[]={"digitalOut.write","digitalOut.read","digitalOut.flipflop","analog.start","interruptin.start","modbus.query","modbus_wait_query"};
struct rpc_queue_entry {
string caller_key;   //调用者socket  Key
//string callee_key  //被调用者socket Key
string path;
int  income_id  ; //进入id
int  outgoing_id ; //转出 id 
int  result_mode;//返回方式
int  timeout   ; //超时计数器
};
//bool find_rpc_caller(string key,int caller_id, rpc_queue_entry *rpc);
//bool find_rpc_outgoing_id(string key,int outgoing_id,rpc_queue_entry *rpc);
 int remove_queue_entry(string callee_key,int outgoing_id);
map <string,app_register_entry> applist;
multimap <string,rpc_register_entry> plist;
multimap <string,rpc_queue_entry> rpc_queue;
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
bool isNumber(string s) 
{ 
    for (int i = 0; i < s.length(); i++) 
        if (isdigit(s[i]) == false) 
            return false; 
  
    return true; 
}
int getPinAddress(string path)
{
      char * writable = new char[path.size() + 1];
       std::copy(path.begin(), path.end(), writable);
       writable[path.size()] = '\0';
       splitstring uri(writable);
       vector<string> detail= uri.split('/') ;
        delete[] writable;
        int pin= ((stoi(detail[1])<<8)&0xff00)|stoi(detail[2]);
   return pin;     
}
string getDeviceName(string path)
{
      char * writable = new char[path.size() + 1];
       std::copy(path.begin(), path.end(), writable);
       writable[path.size()] = '\0';
       splitstring uri(writable);
       vector<string> detail= uri.split('/') ;
        delete[] writable;
   return detail[0];     
}
string clearPinsInPath(string path)
{
      char * writable = new char[path.size() + 1];
       std::copy(path.begin(), path.end(), writable);
       writable[path.size()] = '\0';
       splitstring uri(writable);
       vector<string> detail= uri.split('/') ;
        delete[] writable;
   return detail[0]+"/"+detail[1]+"/000";
}
string  sendEventMessage(const char * method,string path,uint8_t *buff,int len)
{   int i;
    Document message_json;
    message_json.SetObject();
    Document::AllocatorType& allocator = message_json.GetAllocator();
    Value vstring(kStringType);
    vstring.SetString(method, strlen(method), allocator); 
    message_json.AddMember("method",vstring,allocator);
     vstring.SetString(path.c_str(),path.size(), allocator);
    message_json.AddMember("path",vstring,allocator);
    Value data(kArrayType);
    for (i=0;i<len;i++)
       data.PushBack(buff[i], allocator); 
       message_json.AddMember("value", data, allocator);
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    message_json.Accept(writer);
     string s=buffer.GetString();
   return s;
}

 void * reciever(void *arg)
{  uint8_t code;
int len;
int i;
int num_io;
int pin,id;
   
 multimap<string,rpc_queue_entry>::iterator it; 
 //cout <<"recieve thread running .."<<endl;
	while(true)
	{
	//len= interface.waitResult(rBuf);
     len=interface.receiveData(rBuf,&num_io);
    // cout<<"len:"<<len<<endl;
    if (len>0)
     { 
        code=rBuf[0];	
        pin=((num_io<<8)&0xff00)+rBuf[1];
        id=((rBuf[2]<<8)&0xff00)+rBuf[3];
  //  cout<<"recieved->method: "<<unsigned(code)<<" pin: "<<pin<<"id:"<<id<<endl;  
   mtx.lock(); 	   
  //  for (it=rpc_queue.begin(); it!=rpc_queue.end(); it++)
  it=rpc_queue.begin();
  bool flg=true;
  while((it!=rpc_queue.end())&&flg)
    {  
       auto  callee=(*it).first;  
      rpc_queue_entry queue_entry=(*it).second;
         if ((callee.compare("base")==0)||(queue_entry.outgoing_id==id))
        {               Document res;
                        res.SetObject();
                        Value result(kObjectType); 
                        Document::AllocatorType& allocator = res.GetAllocator();
                        
                        int mode=queue_entry.result_mode; 
                       // cout<<" mode: "<<mode<<endl;
                       string path= queue_entry.path; 
                       int income_id=queue_entry.income_id;
                   //    cout<<"income_id :"<<income_id<<endl;
                          string caller_key=queue_entry.caller_key;                     
                          Value methodName (kStringType);
                        methodName.SetString(io_rpc_table[code-5].c_str(),allocator);                         
                        result.AddMember("status", methodName,allocator);
                        std::string encoded = base64_encodeB(reinterpret_cast<const unsigned char*>(&rBuf[4]), len);
                        Value data(kStringType);
                        data.SetString(encoded.c_str(),allocator);
                         result.AddMember("value",data,allocator); 
                         res.AddMember("result",result,allocator);
                         Value vstring(kStringType);
                          vstring.SetString(path.c_str(),path.size(), allocator);
                         res.AddMember("path",vstring,allocator);
                        res.AddMember("id",income_id,allocator);
                        StringBuffer buffer;
                        Writer<StringBuffer> writer(buffer);
                        res.Accept(writer);
                        string response_body=buffer.GetString();
                  //   cout<<"send to :"<<caller_key<<"response body"<<response_body<<endl; 
                        switch(mode)
                        {
                            case Normal:{ //Normal
                                 
                                rpc_queue.erase(it);
                                flg=false;
                                send_data(response_body,caller_key); 
                                break; 
                            }
                            case StartStream:{
                                 send_data(response_body,caller_key); 
                                break;
                            }
                            case startReadStream:{
                                send_data(response_body,caller_key); 
                                break;
                            }
                            case MQTTStream:{
                               
                              //  char * message = new char[response_body.size() + 1];
                              //  std::copy(response_body.begin(), response_body.end(), message);
                              // message[response_body.size()] = '\0'; // don't forget the terminating 
                              //   Mqtt.publish(method_table[code-2].c_str(), message,response_body.size());
                              //  delete[] message;
                                 break;
                            }
                        }  
             }
       it++;      
     
       }  
       mtx.unlock();                         	 
    }   
	}
}
 
bool app_exist(string appname)
{
    for ( auto entry : applist )
	 {
         string name=entry.first;
        if (name.compare(appname)==0)
        {
            return true;
        }
     }
   return false;  
}
int findRPCMethodIndex(string method_name,string path)
{
	 
	for ( auto entry : plist )
	 {
        string name=entry.first;
       // cout<<"name"<<name<<endl;
         rpc_register_entry att=entry.second;
         
         if ((name.compare(method_name)==0)&&(att.path.compare(path)==0))
           {             
               if (att.owner.compare("base")==0)
                 return att.method_index;
                 else
                   return 0;
           }
           
     }
     return -1;
};
string findRPCOwner(string method_name,string path)
{
    for ( auto entry : plist )
	 {
        string name=entry.first;
       // cout<<"name"<<name<<endl;
         rpc_register_entry att=entry.second;
         
         if ((name.compare(method_name)==0)&&(att.path.compare(path)==0))
           {             
            return att.owner;
           }
           
     }
     return "0";
}
int init_rgister_list()
{   int i;
        for (i=0;i<NUM_base_rpc;i++)
    {
        rpc_register_entry entry={"base",i+128,"000/000/000"} ;
        plist.insert(pair<string, rpc_register_entry>(base_rpc_table[i],entry));
    }; 
    for (i=0;i<NUM_io_rpc;i++)
    {
        rpc_register_entry entry={"base",i+5,"000/000/000"} ;
        plist.insert(pair<string, rpc_register_entry>(io_rpc_table[i],entry));
    }; 
       for (i=0;i<NUM_io_rpc;i++)
    {
        rpc_register_entry entry={"base",i+5,"000/001/000"} ;
        plist.insert(pair<string, rpc_register_entry>(io_rpc_table[i],entry));
    };
  return 0;  
};

void cleanUp(string key)
{   
    map<string,rpc_register_entry>::iterator it1;
    multimap<string,rpc_queue_entry>::iterator it2;
   
   for (it1=plist.begin(); it1!=plist.end(); ++it1)
   {
       auto name=(*it1).first;
       rpc_register_entry e=(*it1).second;
       if (e.owner.compare(key)==0)
       {
           plist.erase(it1);
           cout<<"clean register method:"<<name<<endl;
       }
   }

     for (it2=rpc_queue.begin(); it2!=rpc_queue.end(); ++it2)
    {
      auto  callee=(*it2).first;  
      rpc_queue_entry e=(*it2).second;
         if ((e.caller_key.compare(key)==0)||(e.caller_key.compare(key)==0))
        {
             rpc_queue.erase(it2);
             cout<<"clean  call method:"<< e.caller_key<< endl;
        }
    }
}



string find_starter_by_path(string path,rpc_queue_entry *rpc)
{
  rpc_queue_entry  e;
    for ( auto entry : rpc_queue)
    {
      auto  callee=entry.first;  
        e=entry.second;
      if (e.path.compare(path)==0)
        {
            *rpc=e;
            return callee;
        } 
    }
    return NULL;
}

 

 int  Stop_stream(string path)
 {
       multimap<string,rpc_queue_entry>::iterator it;
      for (it=rpc_queue.begin(); it!=rpc_queue.end(); ++it)
    {
      auto  callee=(*it).first;  
      rpc_queue_entry e=(*it).second;
     if (e.path.compare(path)==0)
       {   
            rpc_queue.erase(it);
           
       }       
    }
   
    return 0;
 }

 int stop_readStop(string key,string path)
 {
        multimap<string,rpc_queue_entry>::iterator it;
      for (it=rpc_queue.begin(); it!=rpc_queue.end(); ++it)
    {
      auto  callee=(*it).first;  
      rpc_queue_entry e=(*it).second;
     if ((e.path.compare(path)==0)&&(e.caller_key.compare(key)))
       {   
            rpc_queue.erase(it);
            return 0;
       }       
    }  
    return -1;
 }
bool id_is_used(string callee_key,int id)
{
       multimap<string,rpc_queue_entry>::iterator it;
      for (it=rpc_queue.begin(); it!=rpc_queue.end(); ++it)
    {
      auto  callee=(*it).first;  
      rpc_queue_entry e=(*it).second;
     if ((callee.compare(callee_key)==0)&&(e.outgoing_id==id))
       {   
            
            return true;
       }       
    }  
    return false;
}
 int get_going_id(string callee_key)
 {
     base_id=base_id+1;
     if (base_id>5000) base_id=0;
     while(id_is_used(callee_key,base_id))
     {
       base_id=base_id+1;
       if (base_id>5000) base_id=0;
     }
     
     return base_id;    
 }

 int error_handling(int error_code,int income_id,string key)
 {
    Document res;
    Value result(kObjectType);
    Document::AllocatorType& allocator = res.GetAllocator();
    res.SetObject();
    cout<<"error with Code:"<<error_code<<endl;
    result.AddMember("status","err",allocator);
     Value data(kArrayType);
      data.PushBack(error_code, allocator); 
    result.AddMember("value",data,allocator);
    res.AddMember("result",result,allocator);
    res.AddMember("id",income_id,allocator);
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    res.Accept(writer);
    string response_body=buffer.GetString();
    send_data(response_body,key);
    return 0;
 } 

 bool streamExist(string path)
 {
         rpc_queue_entry  e;
    for ( auto entry : rpc_queue)
    {
      auto  callee=entry.first;  
        e=entry.second;
      if (e.path.compare(path)==0)
        {         
            return true;
        } 
    }
    return false;
 }

int   excutiveRequest(string request,string key)
{ 
    //  cout<<"request: "<<request<<endl;
     int method_index; 
    int error_Code=0;
     int id,income_id;
     int mode;
     int start_id;
     string path,path1;
     string appname;
     int appport;
     Document req;
	 Document res;
     Value params(kObjectType);
     string methodName;
    std::map<string,rpc_register_entry>::iterator it;
	 res.SetObject();
     
	Document::AllocatorType& allocator = res.GetAllocator();
    Value result(kObjectType);
      uint8_t buf[32];
   //   cout<<request.c_str()<<endl ;
   
       req.Parse<rapidjson::kParseStopWhenDoneFlag, rapidjson::UTF8<> >(request.c_str());
        
      if (req.HasParseError()) {
       
          error_handling(ERR_REQ_SYNTAX,0,key);
         return -1;
       };
       if (!req.IsObject())
       {
         error_handling(ERR_REQ_SYNTAX1,0,key);
         return -1;
       }
           if (req.HasMember("id"))
       { 
        if (req["id"].IsInt())
		 id=req["id"].GetInt();
         else {
              error_handling(ERR_REQ_ID,0,key);          
               return -1;
         }
       } else
       {
          error_handling(ERR_REQ_ID,0,key);          
         return -1;
       }
    
     if (req.HasMember("method")) 
     {  
         // rpc request
          
         methodName=req["method"].GetString();
     //   cout<<"request method"<<methodName<<endl;
         if (methodName.compare("ping")==0) 
                {
                  // error_handling(OK,id,key);
                   return 0;
                }       

        if( req.HasMember("mode")&&(req["mode"].IsInt()))
                {
                    mode=req["mode"].GetInt();
                } else
                {
                    error_handling(ERR_REQ_NO_MODE,id,key);          
                        return -1;
                }

        if( req.HasMember("path"))
                {
                    path=req["path"].GetString();
                } else
                {
                    error_handling(ERR_REQ_NO_PATH,id,key);          
                        return -1;
                }     
        cout<<"Method:"<<methodName<<"path: "<<path<<"mode: "<<mode<<"id:  "<<id<<endl;
        if (req.HasMember("params")) 
                    {   
                    params =  req["params"];
                    } else
                    {
                    error_handling(ERR_REQ_NO_PARAMS,id,key);
                    return -1;
                    }
                    string path1=clearPinsInPath(path);
                    method_index=findRPCMethodIndex(methodName,path1); 
                    if (method_index<0)
                            {

                            error_handling(ERR_REQ_NO_SUCH_METHODE,id,key);
                            return -1;
                            }
                        
      } else
     if (req.HasMember("result"))
     {
        // result =  req["result"];  
         method_index=1;
        if( req.HasMember("path"))
            {
                path=req["path"].GetString();
            } else
            {
                error_handling(ERR_REQ_NO_PATH,id,key);          
                    return -1;
            }   
     } else
       {
             error_handling(ERR_REQ_NO_METHOD_RESULT,id,key);
                        return -1;
       }
 
  	 cout<<"method:"<<methodName<<",index=:"<<method_index<<endl;
       cout<<"mode:"<<mode<<",path:"<<path<<endl;

		switch(method_index)
		{
            case app_rpc_request:
            {    string path1=clearPinsInPath(path);
                 string callee_key=findRPCOwner(methodName,path1);
                   income_id=id;
                  int going_id =get_going_id(callee_key);
                  
                  switch (mode)
                  {
                      case Normal:{
                            rpc_queue_entry queue_entry={key,"NONE",income_id, going_id,mode,0};
                            rpc_queue.insert(make_pair(callee_key,queue_entry));
                          break;
                      }
                      case StartStream:{
                         // caller key , stream_name caller id, income id,result_mode,timeout
                                    rpc_queue_entry queue_entry={key,path,income_id, going_id,mode,0};
                                    rpc_queue.insert(make_pair(callee_key,queue_entry));                               
                                    break;
                                    }
                      case StopStream:
                      {
                          
                              if (streamExist(path))
                                    {
                                        Stop_stream(path);
                                    }
                                                  
                          break;
                      }
                      case MQTTStream:
                      {  
                          cout<<"under develop"<<endl;
                          break;
                      }
                  }
                // transfer to callee
                 Value m_name;
                m_name.SetString(methodName.c_str(),allocator);
                res.AddMember("method",m_name,allocator);
                res.AddMember("mode",mode,allocator); 
                Value vstring(kStringType);
                 vstring.SetString(path.c_str(),path.size(), allocator);
                res.AddMember("path",vstring,allocator);     
                res.AddMember("params",params,allocator);
                res.AddMember("id", going_id,allocator); 
                //modify outgoing id
            
                StringBuffer buffer;
                Writer<StringBuffer> writer(buffer);
                res.Accept(writer);
                string response_body=buffer.GetString();
                send_data(response_body,callee_key);
                 break;
            }
            case app_rpc_Result:{
                 
                int goback_id=req["id"].GetInt();  
               // res.AddMember("result",result,allocator);                
                 multimap<string,rpc_queue_entry>::iterator it;
                 mtx.lock();
                 it=rpc_queue.begin();
                 bool flg=true;
                        while((it!=rpc_queue.end())&&flg)
                            {  
                
                            auto  callee=(*it).first;          
                            rpc_queue_entry queue_entry=(*it).second;
                            if ((callee.compare(key)==0)||(queue_entry.outgoing_id==goback_id))
                                {
                        
                                    income_id=queue_entry.income_id;
                                    string caller_key=queue_entry.caller_key;
                                   // string callee_key=key;
                                    int mode=queue_entry.result_mode;
                                    if (mode==Normal)
                                    {
                                        rpc_queue.erase(it);
                                        flg=false;
                                    }  
                                  // res.AddMember("id",income_id,allocator);
                                   Value& v= req["id"];
                                     v.SetInt(income_id);
                                    StringBuffer buffer;
                                    Writer<StringBuffer> writer(buffer);
                                    req.Accept(writer);
                                    string response_body=buffer.GetString();
                               //  cout<<"result msg: "<<response_body<<endl;
                                    send_data(response_body,caller_key); 
                                }
                            it++; 
                            }
                    mtx.unlock();               
                    break;
                 }
             case app_rpc_Register_rpc:{
                string name;
                cout<<"register rpc"<<endl;
                if (params.HasMember("name"))
                { 
                 name=params["name"].GetString();   
                }
                 else {
                         error_handling(ERR_REQ_NO_NAME,id,key);          
                         return -1;
                      }; 
                if (params.HasMember("path"))
                { 
                 path1=params["path"].GetString();   
                }
                 else {
                         error_handling(ERR_REQ_NO_NAME,id,key);          
                         return -1;
                      };             
                income_id=id; 
             //   cout<<"register RPC: "<<name<<"path : " <<path1<<"id: "<<id<<endl;                                 
                rpc_register_entry entry={key,0,path1};
                plist.insert(pair<string,rpc_register_entry>(name,entry));
                result.AddMember("status","OK",allocator);
                res.AddMember("result",result,allocator);
                res.AddMember("id",income_id,allocator);            
                StringBuffer buffer;
                Writer<StringBuffer> writer(buffer);
                res.Accept(writer);
                string response_body=buffer.GetString();
                send_data(response_body,key);                                   
                break;
            }
          case  app_rpc_Register_app:{
                   cout<<" register app"<<endl;
              if( params.HasMember("name"))
                    {
                        appname=params["name"].GetString();
                    } else
                    {
                        error_handling(ERR_REQ_NO_NAME,id,key);          
                                return -1;
                    }
               if( params.HasMember("port"))
                    {
                        appport=params["port"].GetInt();
                    } else
                    {
                        error_handling(ERR_REQ_NO_PORT,id,key);          
                                return -1;
                    }
                if (!app_exist(appname))
                { 
                 app_register_entry app_entry={appport,0};
                 applist.insert(make_pair(appname,app_entry)); 
                } else
                {
                    error_handling(ERR_REQ_APP_EXIST,id,key);          
                     return -1;
                }   
              break;
          }  
          case  app_rpc_get_rpclist:{
                cout<<"get rpc methods"<<endl;
                 income_id=id; 
                Value rpcList(kArrayType);
                result.AddMember("status","get.rpclist",allocator);
                 for ( auto entry : plist )
                        {
                        string name=entry.first;
                        rpc_register_entry att=entry.second;
                        Value item(kObjectType);
                        Value s(kStringType);
                        s.SetString(name.c_str(),allocator);
                        item.AddMember("rpcname",s,allocator);
                        s.SetString(att.path.c_str(),allocator);
                        item.AddMember("path",s,allocator); 
                         item.AddMember("index",att.method_index,allocator); 
                         rpcList.PushBack(item,allocator);                          
                        }
                result.AddMember("value",rpcList,allocator);        
                res.AddMember("result",result,allocator);
                res.AddMember("id",income_id,allocator); 
                StringBuffer buffer;
                Writer<StringBuffer> writer(buffer);
                res.Accept(writer);
                string response_body=buffer.GetString();
              //  cout<<"response with: "<<response_body<<endl;
                send_data(response_body,key);  
              break;
          } 
          case app_rpc_get_applist:{
                 income_id=id;
             cout<<"get app list"<<endl;
                Value appList(kArrayType);
                result.AddMember("status","get.applist",allocator);
                 for ( auto entry : applist )
                        {
                        string name=entry.first;
                        app_register_entry att=entry.second;
                        Value item(kObjectType);
                        Value s(kStringType);
                        s.SetString(name.c_str(),allocator);
                        item.AddMember("name",s,allocator); 
                        item.AddMember("port",att.port,allocator); 
                         appList.PushBack(item,allocator);                          
                        }
                result.AddMember("value",appList,allocator);        
                res.AddMember("result",result,allocator);
                res.AddMember("id",income_id,allocator); 
                StringBuffer buffer;
                Writer<StringBuffer> writer(buffer);
                res.Accept(writer);
                string response_body=buffer.GetString();
             //   cout<<"response with: "<<response_body<<endl;
                send_data(response_body,key);  
              break;
          }
          case app_rpc_Readstream:{
                  cout<<"read stream"<<endl;             
                  if (mode==startReadStream)
                  { 
                    if (params.HasMember("path"))
                    { 
                    path1=params["path"].GetString();   
                    }
                    else {
                            error_handling(ERR_REQ_NO_NAME,id,key);          
                            return -1;
                        };       
                      if (streamExist(path1))
                      { 

                       income_id=id; 
                      rpc_queue_entry e;
                        string callee_key=find_starter_by_path(path1,&e);
                       int going_id=e.outgoing_id;
                       rpc_queue_entry queue_entry={key,path1,income_id, going_id,mode,0};
                       rpc_queue.insert(make_pair(callee_key,queue_entry));                      
                      }
                  }
                  else
                  if (mode==stopReadStream)
                  {
                       if (streamExist(path))
                       {
                            stop_readStop(key,path);
                       }
                       else
                       {
                         error_handling(ERR_REQ_STREAM_NAME_NOT_EXIST,id,key);          
                         return -1;
                       }
                  }
                  else
                  {
                      error_handling(ERR_REQ_IL_MODE,id,key);          
                         return -1;
                  }
                  break;
             } 
           default:{
               cout<<"io method name:"<<methodName<<" method index:  "<<method_index<<endl;
                  string path1=clearPinsInPath(path);
                     int pin;              
                pin=getPinAddress(path); 
                income_id=id;             
                 string callee_key=findRPCOwner(methodName,path1);               
                   int going_id=get_going_id("base");
                switch(mode)
                 {
                     case Normal:{
                         //req["id"].GetInt();
                         mtx.lock();
                        rpc_queue_entry queue_entry={key,path,income_id,going_id,mode,0};// caller key ,caller id, income id,result_mode timeout
                         rpc_queue.insert(make_pair(callee_key,queue_entry));
                         mtx.unlock();
                         break;
                     }
                    case StartStream: {
                               //   cout<<"income_id :"<<income_id<<endl;                                         
                                 rpc_queue_entry queue_entry={key,path,income_id,going_id,mode,0};// caller key ,caller id, income id,result_mode timeout
                                 rpc_queue.insert(make_pair(callee_key,queue_entry));                         
                                 break;    
                      } 
                     case StopStream:{  
                                Stop_stream(path);
                                break;
                        }
                 }   
                  //get value array
                    int p=0; 
                  if( params.HasMember("value"))
                  { 
                    if(params["value"].IsArray())
                    { 
                   const rapidjson::Value& data = params["value"]; 
                 
                   for (rapidjson::Value::ConstValueIterator itr = data.Begin(); itr != data.End(); ++itr) 
                        {  
                            const rapidjson::Value& d = *itr;
                            if (d.IsInt())
                            {
                                ioBuf[p++]=d.GetInt();
                           //     cout<<"d="<<d.GetInt()<<endl;
                            } else
                            {
                                error_handling(ERR_REQ_NO_VALUE,id,key);          
                                    return -1;
                            }       
                        }
                    } else
                    {
                       error_handling(ERR_REQ_NO_VALUE,id,key);          
                                    return -1;
                    }    
                  } else
                  {
                       error_handling(ERR_REQ_NO_VALUE,id,key);          
                                    return -1;
                  }    
                  // send to IO dumule   
            
             //     cout<<"method code: " <<method_index<<" value: "<<unsigned(ioBuf[0])<<" len:"<< p<<" pin:"<<pin<<" id "<<id<<endl;                
                   interface.callMethod(method_index,pin,going_id,ioBuf,p);      
           }   
        }      
       return 0;     
}

bool ReadFileContent(string filename,string * buffer)
{
      streampos size;
  char * memblock;
ifstream rfile;
string line;
//cout<<"open file :"<<filename<<endl;
rfile.open(filename.c_str(),ios::binary|ios::ate);
if (rfile.is_open()) {
  size = rfile.tellg();
 // cout<<"file size :"<<size<<endl;
    memblock = new char [size];
    rfile.seekg (0, ios::beg);
    rfile.read (memblock, size);
    rfile.close();
    string ss(memblock,size);// char * to string Convert
     *buffer= ss;
     return true;
} else
    return false;
 
 
}
 bool sendFile(string path,string type,const shared_ptr< Session > session )
{
 // ifstream stream( path, ifstream::in );
    
 //   if ( stream.is_open() )
  //  {
      //  const string body =string( istreambuf_iterator< char >( stream ), istreambuf_iterator< char >( ) );
      string body;
        ReadFileContent(path,&body);
        const multimap< string, string > headers
        {
            { "Content-Type", type },
            { "Content-Length", to_string( body.length( ) ) }
        };
        
        session->close( OK, body, headers );
  //  }
 //   else
 //   {
   //     session->close( NOT_FOUND );
  //  };
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
    } else if  (type.compare("font")==0)
    {      char * p=strchr(path.c_str(), '.');
           string ext(p);
          sendFile("./font/"+filename,"image/"+ext,session);
    }
        session->close( NOT_FOUND );
   
}

int main()
{  
   pthread_t a_thread;
    cout<<"BASE SERVICE V1.0"<<endl;
    init_rgister_list();
    interface.init("192.168.31.98");
    Mqtt.connect();
    pthread_create(&a_thread, NULL, reciever, NULL);
    auto resource1 = make_shared< Resource >( );
    resource1->set_path( "/home/.*/.*" );
    resource1->set_method_handler( "GET", get_method_handler );

    auto resource2 = make_shared< Resource >( );
     resource2->set_path( "/api/.*" );
    resource2->set_method_handler( "POST", post_method_handler );

     auto resource3 = make_shared< Resource >( );
    resource3->set_path( "/iosocket/{deviceid: [0-9]*}" );
    resource3->set_method_handler( "GET", get_wsocket_handler );
  
    auto settings = make_shared< Settings >( );
    settings->set_port( 2019 );
    settings->set_default_header( "Connection", "close" );
         
    Service service;
    service.publish( resource1 );
	service.publish( resource2 );
	service.publish( resource3 );
    service.start( settings ); 
	sem_init(&response_sem, 0, 0);

    return EXIT_SUCCESS;
}