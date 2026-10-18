#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
}

class BtMultiSPP
{
  public:
    BtMultiSPP(uint32_t maxConnections);
    ~BtMultiSPP();

    void start(std::string deviceName);
    void stop();

    size_t printf(const char *format, ...);

    void write(uint32_t clientIndex, const uint8_t *data, size_t length);
    void transmit();

    int read();
    int available();

    uint32_t getMaxConnections() const;
    uint32_t getConnectionCount() const;

  private:
    static void        sppCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
    void               handleSppEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
    static void        gapCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
    static BtMultiSPP *s_instance;

    uint32_t              maxConnections_;
    bool                  started_;
    std::string           deviceName_;
    std::vector<uint32_t> connectedClients_;
    QueueHandle_t         receiveQueue_;
    uint8_t                transmitBuffer[4096];
    size_t                 transmitBufferLength_;
    static const char    *logTag_;
};
