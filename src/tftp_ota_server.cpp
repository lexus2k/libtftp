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

#include "tftp_ota_server.h"
#include <esp_ota_ops.h>
#include <esp_log.h>
#include <string.h>

int TftpOtaServer::on_write(const char *file)
{
    if ((strlen(file) < 4) || (strcmp(&file[strlen(file) - 4], ".bin")))
    {
        ESP_LOGW("TFTP", "file extension is not .bin");
        return -1;
    }
    const esp_partition_t* active_partition = esp_ota_get_running_partition();
    m_next_partition = esp_ota_get_next_update_partition(active_partition);
    if (!m_next_partition)
    {
        ESP_LOGE("TFTP", "failed to prepare partition");
        return -1;
    }
    esp_err_t err = esp_ota_begin(m_next_partition, OTA_SIZE_UNKNOWN, &m_ota_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE("TFTP", "failed to prepare partition 2");
        return -1;
    }
    return 0;
}

int TftpOtaServer::on_write_data(uint8_t *buffer, int len)
{
    if (m_ota_handle)
    {
        if ( esp_ota_write(m_ota_handle, buffer, len) != ESP_OK)
        {
            ESP_LOGE("TFTP", "failed to write to partition");
            return -1;
        }
    }
    return len;
}

void TftpOtaServer::on_close()
{
    if (m_ota_handle)
    {
        if ( esp_ota_end(m_ota_handle) == ESP_OK )
        {
            ESP_LOGI("TFTP", "Upgrade successful");
            esp_ota_set_boot_partition(m_next_partition);
            m_ota_handle = 0;
            fflush(stdout);
            esp_restart();
        }
        ESP_LOGE("TFTP", "Failed flash file verification");
    }
}



    // init flash write
