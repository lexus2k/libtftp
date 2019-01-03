# libtftp
Lightweight cross-platform TFTP server library

[tocstart]: # (toc start)

  * [ESP32 usage](#esp32-usage)
  * [Building for ESP32](#building-for-esp32)
  * [Linux](#linux)
  * [Building for Linux](#building-for-linux)
  * [License](#license)

[tocend]: # (toc end)

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

## License

The library is free. If this project helps you, you can give me a cup of coffee.

MIT License

Copyright (c) 2018, Alexey Dynda

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
