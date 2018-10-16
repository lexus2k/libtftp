# libtftp
Lightweight cross-platform TFTP server library

## ESP32 usage

Running server in blocking mode
```.cpp
#include "tftp_server.h"

TFTP server;

server.start();
while (server.run() >= 0);
server.stop();
```

Running server in non-blocking mode
```.cpp
#include "tftp_server.h"

TFTP server;

server.start();
while (server.run(false) >= 0)
{
    vTaskDelay(2);
}
server.stop();
```

Refer to TftpOtaServer as example OTA implementation for ESP32 via TFTP

## Building for ESP32

Just put library to components folder of your project

## Linux

Running server in blocking mode
```.cpp
#include "tftp_server.h"

TFTP server;

server.start();
while (server.run() >= 0);
server.stop();
```

Running server in non-blocking mode
```.cpp
#include "tftp_server.h"

TFTP server;

server.start();
while (server.run(false) >= 0)
{
    usleep(1000);
}
server.stop();
```
# Building for Linux

> make -f Makefile.linux<br>

