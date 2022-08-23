#pragma once
#include "C:\Users\marti\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.6\libraries\WebServer\src\WebServer.h"
#include <SdFatConfig.h>
#include <sdfat.h>
class WebServerX :
    public WebServer
{
    public:
		WebServerX(int port = 80) {  };
        size_t streamFileX(FsFile& file, const String& contentType) {
            char name[100];
            file.getName(name, sizeof(name) - 1);
            _streamFileCore(file.size(), name, contentType);
            return _currentClient.write(file);
		}
};

