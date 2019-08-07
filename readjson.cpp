#include <string>
#include <memory>
#include <cstdlib>
#include <fstream>
#include <restbed>
#include <chrono>
#include <streambuf>
#include <sstream>
#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <rapidjson/filereadstream.h>
using namespace std;
using namespace rapidjson;
using namespace std::chrono;
string fileName="package.json";
FILE* pFile = fopen(fileName.c_str(), "rb");
char buffer[65536];
int main()
{ 
    cout<<"read package.json file"<<endl;
FileReadStream is(pFile, buffer, sizeof(buffer));
Document document;
document.ParseStream<0, UTF8<>, FileReadStream>(is);
cout<<"name:"<<document["name"].GetString()<<endl;
}