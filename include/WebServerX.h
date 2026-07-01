#pragma once
#include <WebServer.h>
#include <SdFatConfig.h>
#include <sdfat.h>
class WebServerX :
    public WebServer
{
    public:
		WebServerX(int port = 80) :WebServer{ port } {}
        // We have to use getName because .name() from SD does not exist in SdFat
        size_t streamFileX(FsFile& file, const String& contentType) {
            char name[100];
            file.getName(name, sizeof(name) - 1);
            _streamFileCore(file.size(), name, contentType);
            return _currentClient.write(file);
		}
};

