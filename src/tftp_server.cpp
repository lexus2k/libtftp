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

#include "tftp_server.h"
#include "tftp_server_priv.h"

#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

static const char* ip_to_str(struct sockaddr *addr)
{
    static char buf[INET_ADDRSTRLEN];
    return inet_ntop(AF_INET, addr, buf, INET_ADDRSTRLEN);
}

static char TAG[] = "TFTP";

enum e_tftp_cmd
{
    TFTP_CMD_RRQ   = 1,
    TFTP_CMD_WRQ   = 2,
    TFTP_CMD_DATA  = 3,
    TFTP_CMD_ACK   = 4,
    TFTP_CMD_ERROR = 5,
};

enum e_error_code
{
    ERR_NOT_DEFINED          = 0,
    ERR_FILE_NOT_FOUND       = 1,
    ERR_ACCESS_VIOLATION     = 2,
    ERR_NO_SPACE             = 3,
    ERR_ILLEGAL_OPERATION    = 4,
    ERR_UNKNOWN_TRANSFER_ID  = 5,
    ERR_FILE_EXISTS          = 6,
    ERR_NO_SUCH_USER         = 7
};

// Max supported TFTP packet size
const int TFTP_DATA_SIZE = 512 + 4;

TFTP::TFTP(uint16_t port)
   : m_port( port )
{
}

TFTP::~TFTP()
{
}

int TFTP::process_write()
{
    int result = -1;
    int total_size = 0;
    int next_block_num = 1;
    while (1)
    {
        socklen_t addr_len = sizeof(m_client);
        m_data_size = recvfrom(m_sock, m_buffer, TFTP_DATA_SIZE, 0, &m_client, &addr_len);

        if (m_data_size < 0)
        {
            ESP_LOGE(TAG, "error on receive");
            break;
        }
        uint16_t code = ntohs(*(uint16_t*)(&m_buffer[0]));
        if ( code != TFTP_CMD_DATA )
        {
            ESP_LOGE(TAG, "not data packet received: [%d]", code);
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, m_buffer, m_data_size, ESP_LOG_DEBUG);
            if ( ( code == TFTP_CMD_WRQ ) && ( next_block_num == 1 ) )
            {
                // some clients repeat request several times
                send_ack(0);
                continue;
            }
            result = 1; // repeat
            break;
        }
        uint16_t block_num = ntohs(*(uint16_t*)(&m_buffer[2]));
        int data_size = m_data_size - 4;
        send_ack(block_num);
        if ( block_num < next_block_num )
        {
            // Maybe this is dup, ignore it
            ESP_LOGI(TAG, "dup packet received: [%d], expected [%d]", block_num, next_block_num);
        }
        else
        {
            next_block_num++;
            total_size += data_size;
            on_write_data(&m_buffer[4], data_size);
            ESP_LOGD(TAG, "Block size: %d", data_size);
        }
        if (m_data_size < TFTP_DATA_SIZE)
        {
            ESP_LOGI(TAG, "file received: (%d bytes)", total_size);
            result = 0;
            break;
        }
    }
    return result;
}

int TFTP::process_read()
{
    int result = -1;
    int block_num = 1;
    int total_size = 0;

    for(;;)
    {
        *(uint16_t*)(&m_buffer[0]) = htons(TFTP_CMD_DATA);
        *(uint16_t*)(&m_buffer[2]) = htons(block_num);

        m_data_size = on_read_data( &m_buffer[4], TFTP_DATA_SIZE - 4 );
        if (m_data_size < 0)
        {
            ESP_LOGE(TAG, "Failed to read data from file");
            send_error(ERR_ILLEGAL_OPERATION, "failed to read file");
            result = -1;
            break;
        }
        ESP_LOGD(TAG, "Sending data to %s, blockNumber=%d, size=%d",
                ip_to_str(&m_client), block_num, m_data_size);

        for(int r=0; r<3; r++)
        {
            result = sendto( m_sock, m_buffer, m_data_size + 4, 0, &m_client, sizeof(m_client));
            if (result < 0)
            {
                break;
            }
            if (wait_for_ack(block_num) == 0)
            {
                result = 0;
                break;
            }
            ESP_LOGE(TAG, "No ack/wrong ack, retrying");
            result = -1;
        }
        if ( result < 0 )
        {
            break;
        }
        total_size += m_data_size - 4;

        if (m_data_size < TFTP_DATA_SIZE - 4)
        {
            ESP_LOGI(TAG, "Sent file (%d bytes)", total_size);
            result = 0;
            break;
        }
        block_num++;
    }
    return result;
}

void TFTP::send_ack(uint16_t block_num)
{
    uint8_t data[4];

    *(uint16_t*)(&data[0]) = htons(TFTP_CMD_ACK);
    *(uint16_t*)(&data[2]) = htons(block_num);
    ESP_LOGD(TAG, "ack to %s, blockNumber=%d", ip_to_str(&m_client), block_num);
    sendto(m_sock, (uint8_t *)&data[0], sizeof(data), 0, &m_client, sizeof(struct sockaddr));
}


void TFTP::send_error(uint16_t code, const char *message)
{
    *(uint16_t *)(&m_buffer[0]) = htons(TFTP_CMD_ERROR);
    *(uint16_t *)(&m_buffer[2]) = htons(code);
    strcpy((char *)(&m_buffer[4]), message);
    sendto(m_sock, m_buffer, 4 + strlen(message) + 1, 0, &m_client, sizeof(m_client));
}

int TFTP::wait_for_ack(uint16_t block_num)
{
    uint8_t data[4];

    ESP_LOGD(TAG, "waiting for ack");
    socklen_t len = sizeof(m_client);
    int sizeRead = recvfrom(m_sock, (uint8_t *)&data, sizeof(data), 0, &m_client, &len);

    if ( (sizeRead != sizeof(data)) ||
         (ntohs(*(uint16_t *)(&data[0])) != TFTP_CMD_ACK) )
    {
        ESP_LOGE(TAG, "received wrong ack packet: %d", ntohs(*(uint16_t *)(&data[0])));
        send_error(ERR_NOT_DEFINED, "incorrect ack");
        return -1;
    }

    if (ntohs(*(uint16_t *)(&data[2])) != block_num)
    {
        ESP_LOGE(TAG, "received ack not in order");
        return 1;
    }
    return 0;
}

int TFTP::parse_wrq()
{
    uint8_t *ptr = m_buffer + 2;
    char *filename = (char *)ptr;
    ptr += strlen(filename) + 1;
    char *mode = (char *)ptr;
    ptr += strlen(mode) + 1;
    if ( on_write(filename) < 0)
    {
        ESP_LOGE(TAG, "failed to open file %s for writing", filename);
        send_error(ERR_ACCESS_VIOLATION, "cannot open file");
        return -1;
    }
    if ( (ptr - m_buffer < m_data_size) && !strcmp((char *)ptr, "blksize") )
    {
        uint8_t data[] = { 0, 6, 'b', 'l', 'k', 's', 'i', 'z', 'e', 0, '5', '1', '2', 0 };
        sendto(m_sock, data, sizeof(data), 0, &m_client, sizeof(m_client));
        ESP_LOGI(TAG, "Extended block size is requested. rejecting");
    }
    else
    {
        send_ack(0);
    }
    ESP_LOGI(TAG, "receiving file: %s", filename);
    return 0;
}

int TFTP::parse_rrq()
{
    uint8_t *ptr = m_buffer + 2;
    char *filename = (char *)ptr;
    ptr += strlen(filename) + 1;
    char *mode = (char *)ptr;
    ptr += strlen(mode) + 1;
    if ( on_read(filename) < 0)
    {
        ESP_LOGE(TAG, "failed to open file %s for reading", filename);
        send_error(ERR_FILE_NOT_FOUND, "cannot open file");
        return -1;
    }
    ESP_LOGI(TAG, "sending file: %s", filename);
    return 0;
}


int TFTP::parse_rq()
{
    int result = -1;
    for(;;)
    {
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, m_buffer, m_data_size, ESP_LOG_DEBUG);
        uint16_t cmd = ntohs(*(uint16_t*)(&m_buffer[0])); /* parse command */
        switch (cmd)
        {
            case TFTP_CMD_WRQ:
                result = parse_wrq();
                if (result == 0)
                {
                    result = process_write();
                    on_close();
                }
                else
                {
                    result = 0; // it is ok since parsing is not network issue
                }
                break;
            case TFTP_CMD_RRQ:
                result = parse_rrq();
                if (result == 0)
                {
                    result = process_read();
                    on_close();
                }
                else
                {
                    result = 0; // it is ok since parsing is not network issue
                }
                break;
            default: 
                ESP_LOGW(TAG, "unknown command %d", cmd);
        }
        if (result <= 0)
        {
            break;
        }
    }
    return result;
}

int TFTP::start()
{
    if (m_sock >= 0)
    {
         return 0;
    }
    m_sock = socket( AF_INET, SOCK_DGRAM, 0);
    if (m_sock < 0)
    {
         return -1;
    }
    m_buffer = (uint8_t *)malloc(TFTP_DATA_SIZE);
    if ( m_buffer == nullptr )
    {
        close(m_sock);
        m_sock = -1;
        return -1;
    }

    int optval = 1;
    setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    struct timeval tv;
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(m_sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)m_port);
    if (bind( m_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        ESP_LOGE(TAG, "binding error");
        return 0;
    }

    ESP_LOGI(TAG, "Started on port %d", m_port);
    return 0;
}

int TFTP::run(bool wait_for)
{
    socklen_t len = sizeof(struct sockaddr);
    m_data_size = recvfrom(m_sock, m_buffer, TFTP_DATA_SIZE, wait_for ? 0 : MSG_DONTWAIT, &m_client, &len);
    if (m_data_size < 0)
    {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
        {
             return 0;
        }
        return -1;
    }

    return parse_rq();
}

void TFTP::stop()
{
    if (m_sock >= 0)
    {
        ESP_LOGI(TAG, "Stopped");
        close(m_sock);
        m_sock = -1;
        free(m_buffer);
    }
    m_buffer = nullptr;
}

int TFTP::on_read(const char *file)
{
    return -1;
}

int TFTP::on_write(const char *file)
{
    return -1;
}

int TFTP::on_read_data(uint8_t *buffer, int len)
{
    return -1;
}

int TFTP::on_write_data(uint8_t *buffer, int len)
{
    return -1;
}

void TFTP::on_close()
{
    return;
}


