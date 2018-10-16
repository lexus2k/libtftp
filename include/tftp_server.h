/*
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
*/

#pragma once

#define TFTP_DEFAULT_PORT (69)

#include <stdint.h>
#include <string>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>


class TFTP
{
public:
    TFTP(uint16_t port = TFTP_DEFAULT_PORT);
    ~TFTP();

    /**
     * Starts server.
     * Returns 0 if server is started successfully
     */
    int start();

    /**
     * Listens for incoming requests. Can be executed in blocking and non-blocking mode
     * @param wait_for true if blocking mode is required, false if non-blocking mode is required
     */
    int run(bool wait_for = true);

    /**
     * Stops server
     */
    void stop();

protected:
    void send_ack(uint16_t block_num);
    void send_error(uint16_t code, const char *message);
    int wait_for_ack(uint16_t block_num);

    /**
     * This method is called, when new read request is received.
     * Override this method and add implementation for your system.
     * @param file name of the file requested
     * @return return 0 if file can be read, otherwise return -1
     */
    virtual int on_read(const char *file);

    /**
     * This method is called, when new write request is received.
     * Override this method and add implementation for your system.
     * @param file name of the file requested
     * @return return 0 if file can be written, otherwise return -1
     */
    virtual int on_write(const char *file);

    /**
     * This method is called, when new data are required to be read from file for sending.
     * Override this method and add implementation for your system.
     * @param buffer buffer to fill with data from file
     * @param len maximum length of buffer
     * @return return number of bytes read to buffer
     * @important if length of read data is less than len argument value, it is considered
     *            as end of file
     */
    virtual int on_read_data(uint8_t *buffer, int len);

    /**
     * This method is called, when new data arrived for writing to file.
     * Override this method and add implementation for your system.
     * @param buffer buffer with received data
     * @param len length of received data
     * @return return number of bytes written
     */
    virtual int on_write_data(uint8_t *buffer, int len);

    /**
     * This method is called, when transfer operation is complete.
     * Override this method and add implementation for your system.
     */
    virtual void on_close();

private:
    uint16_t m_port;
    int m_sock = -1;
    struct sockaddr m_client;
    uint8_t *m_buffer = nullptr;
    int m_data_size;

    int process_write();
    int process_read();
    int parse_wrq();
    int parse_rrq();
    int parse_rq();
};

