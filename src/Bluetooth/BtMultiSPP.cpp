#include "BtMultiSPP.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <algorithm>

extern "C"
{
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_log.h"
#include "esp_spp_api.h"
#include "nvs_flash.h"
}

const char *BtMultiSPP::logTag_ = "BtMultiSPP";

void BtMultiSPP::sppCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    if (BtMultiSPP::s_instance != nullptr)
    {
        BtMultiSPP::s_instance->handleSppEvent(event, param);
    }
}

BtMultiSPP *BtMultiSPP::s_instance = nullptr;

BtMultiSPP::BtMultiSPP(uint32_t maxConnections)
    : maxConnections_(maxConnections), started_(false), receiveQueue_(xQueueCreate(256, sizeof(uint8_t))), transmitBufferLength_(0)
{
    s_instance = this;
}

BtMultiSPP::~BtMultiSPP()
{
    stop();
    if (receiveQueue_ != nullptr)
    {
        vQueueDelete(receiveQueue_);
    }
    s_instance = nullptr;
}

void BtMultiSPP::start(std::string deviceName)
{
    ESP_LOGI(BtMultiSPP::logTag_, "Initializing Bluetooth SPP service with device name: %s", deviceName.c_str());

    if (started_)
    {
        return;
    }

    deviceName_ = std::move(deviceName);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        (void)nvs_flash_erase();
        ret = nvs_flash_init();
    }

    if (ret != ESP_OK)
    {
        ESP_LOGE(BtMultiSPP::logTag_, "Failed to initialize NVS flash: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGW(BtMultiSPP::logTag_, "Failed to release BLE controller memory: %s", esp_err_to_name(ret));
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret                               = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(BtMultiSPP::logTag_, "Failed to initialize Bluetooth controller: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
    if (ret != ESP_OK)
    {
        ESP_LOGE(BtMultiSPP::logTag_, "Failed to enable Bluetooth controller: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret != ESP_OK)
    {
        ESP_LOGE(BtMultiSPP::logTag_, "Failed to initialize Bluedroid: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK)
    {
        ESP_LOGE(BtMultiSPP::logTag_, "Failed to enable Bluedroid: %s", esp_err_to_name(ret));
        return;
    }

    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_NONE;
    esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(iocap));

    ret = esp_bt_gap_register_callback(gapCallback);
    if (ret != ESP_OK)
    {
        ESP_LOGE(BtMultiSPP::logTag_, "Failed to register GAP callback: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_spp_register_callback(sppCallback);
    if (ret != ESP_OK)
    {
        ESP_LOGE(BtMultiSPP::logTag_, "Failed to register SPP callback: %s", esp_err_to_name(ret));
        return;
    }

    esp_spp_cfg_t spp_cfg     = BT_SPP_DEFAULT_CONFIG();
    spp_cfg.mode              = ESP_SPP_MODE_CB;
    spp_cfg.enable_l2cap_ertm = false;
    ret                       = esp_spp_enhanced_init(&spp_cfg);
    if (ret != ESP_OK)
    {
        ESP_LOGE(BtMultiSPP::logTag_, "Failed to initialize SPP service: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_gap_set_device_name(deviceName_.c_str());
    if (ret != ESP_OK)
    {
        ESP_LOGE(BtMultiSPP::logTag_, "Failed to set Bluetooth device name: %s", esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    if (ret != ESP_OK)
    {
        ESP_LOGE(BtMultiSPP::logTag_, "Set scan mode failed: %s", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(BtMultiSPP::logTag_, "Device set to connectable and discoverable.");
    }

    // Get local Bluetooth device address.
    const uint8_t *bda = esp_bt_dev_get_address();
    if (bda)
    {
        char bda_str[18];
        sprintf(bda_str, "%02X:%02X:%02X:%02X:%02X:%02X", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
        ESP_LOGI(BtMultiSPP::logTag_, "Local Bluetooth Address: %s", bda_str);
    }

    started_ = true;
}

void BtMultiSPP::stop()
{
    if (!started_)
    {
        return;
    }

    esp_spp_deinit();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    connectedClients_.clear();
    started_ = false;
}

size_t BtMultiSPP::printf(const char *format, ...)
{
    size_t remainingLength = sizeof(transmitBuffer) - transmitBufferLength_;
    if (format == nullptr || remainingLength == 0)
        return 0;

    va_list args;
    va_start(args, format);
    int length = vsnprintf(reinterpret_cast<char *>(transmitBuffer + transmitBufferLength_), remainingLength, format, args);
    va_end(args);

    if (length <= 0)
        return 0;

    size_t writtenLength = std::min(static_cast<size_t>(length), remainingLength - 1);
    transmitBufferLength_ += writtenLength;
    return writtenLength;
}

void BtMultiSPP::write(uint32_t clientIndex, const uint8_t *data, size_t length)
{
    (void)clientIndex;

    if (data == nullptr || length == 0 || transmitBufferLength_ >= sizeof(transmitBuffer))
    {
        return;
    }

    size_t writableLength = std::min((uint32_t)length, (uint32_t)sizeof(transmitBuffer) - transmitBufferLength_);
    memcpy(transmitBuffer + transmitBufferLength_, data, writableLength);
    transmitBufferLength_ += writableLength;
}

void BtMultiSPP::transmit()
{
    if (!started_ || transmitBufferLength_ == 0)
    {
        return;
    }

    for (uint32_t clientHandle : connectedClients_)
    {
        esp_spp_write(clientHandle, static_cast<int>(transmitBufferLength_), transmitBuffer);
    }

    transmitBufferLength_ = 0;
}

int BtMultiSPP::available()
{
    if (receiveQueue_ == nullptr)
    {
        return 0;
    }

    return static_cast<int>(uxQueueMessagesWaiting(receiveQueue_));
}

int BtMultiSPP::read()
{
    if (receiveQueue_ == nullptr)
    {
        return -1;
    }

    uint8_t data;
    if (xQueueReceive(receiveQueue_, &data, portMAX_DELAY) == pdTRUE)
    {
        return static_cast<int>(data);
    }

    return -1;
}

uint32_t BtMultiSPP::getMaxConnections() const
{
    return maxConnections_;
}

uint32_t BtMultiSPP::getConnectionCount() const
{
    return static_cast<uint32_t>(connectedClients_.size());
}

void BtMultiSPP::handleSppEvent(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event)
    {
    case ESP_SPP_INIT_EVT: {
        if (param != nullptr && param->init.status != ESP_SPP_SUCCESS)
        {
            ESP_LOGE(BtMultiSPP::logTag_, "SPP init failed with status: %d", param->init.status);
            break;
        }

        esp_err_t err = esp_spp_vfs_register();
        if (err != ESP_OK)
        {
            ESP_LOGE("BT", "esp_spp_vfs_register failed: %s", esp_err_to_name(err));
            break;
        }

        esp_spp_start_srv_cfg_t srv_cfg = {
            .local_scn         = 0,
            .create_spp_record = true,
            .sec_mask          = ESP_SPP_SEC_NONE,
            .role              = ESP_SPP_ROLE_SLAVE,
            .name              = deviceName_.c_str(),
        };

        esp_err_t ret = esp_spp_start_srv_with_cfg(&srv_cfg);
        if (ret != ESP_OK)
        {
            ESP_LOGE(BtMultiSPP::logTag_, "Failed to start SPP service: %s", esp_err_to_name(ret));
        }
        else
        {
            ESP_LOGI(BtMultiSPP::logTag_, "SPP server started successfully");
        }
        break;
    }
    case ESP_SPP_OPEN_EVT: {
        if (param != nullptr && param->open.status == ESP_SPP_SUCCESS)
        {
            ESP_LOGI(BtMultiSPP::logTag_, "SPP client connection opened with handle: %u", param->open.handle);
            if (getConnectionCount() < maxConnections_)
            {
                connectedClients_.push_back(param->open.handle);
            }
        }
        break;
    }
    case ESP_SPP_SRV_OPEN_EVT: {
        if (param != nullptr && param->srv_open.status == ESP_SPP_SUCCESS)
        {
            ESP_LOGI(BtMultiSPP::logTag_, "SPP server connection opened with handle: %u", param->srv_open.handle);
            if (getConnectionCount() < maxConnections_)
            {
                connectedClients_.push_back(param->srv_open.handle);
            }
            else
            {
                ESP_LOGW(BtMultiSPP::logTag_, "Maximum number of SPP connections reached");
                esp_spp_disconnect(param->srv_open.handle);
            }
        }
        break;
    }
    case ESP_SPP_CLOSE_EVT: {
        if (param != nullptr)
        {
            ESP_LOGI(BtMultiSPP::logTag_, "SPP connection closed with handle: %u", param->close.handle);
            connectedClients_.erase(std::remove(connectedClients_.begin(), connectedClients_.end(), static_cast<uint32_t>(param->close.handle)),
                                    connectedClients_.end());
        }
        break;
    }
    case ESP_SPP_START_EVT: // SPP Server Started
        if (param->start.status == ESP_SPP_SUCCESS)
        {
            ESP_LOGI(BtMultiSPP::logTag_, "SPP Server started successfully, handle: %d, sec_id: %d, scn: %d", param->start.handle,
                     param->start.sec_id, param->start.scn);
            ESP_LOGI(BtMultiSPP::logTag_, "Waiting for client connection...");
        }
        else
        {
            ESP_LOGE(BtMultiSPP::logTag_, "SPP Server start failed, status: %d", param->start.status);
        }
        break;

    case ESP_SPP_UNINIT_EVT:
    case ESP_SPP_DISCOVERY_COMP_EVT:
    case ESP_SPP_CL_INIT_EVT:
    case ESP_SPP_DATA_IND_EVT:
        if (param != nullptr && receiveQueue_ != nullptr)
        {
            for (uint16_t index = 0; index < param->data_ind.len; ++index)
            {
                (void)xQueueSend(receiveQueue_, &param->data_ind.data[index], 0);
            }
        }
        break;
    case ESP_SPP_CONG_EVT:
    case ESP_SPP_WRITE_EVT:
    case ESP_SPP_SRV_STOP_EVT:
    case ESP_SPP_VFS_REGISTER_EVT:
    case ESP_SPP_VFS_UNREGISTER_EVT:
        break;

    default:
        break;
    }
}

void BtMultiSPP::gapCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI("GAP", "Numeric confirmation request, code: %" PRIu32, param->cfm_req.num_val);
        // Automatically accept the connection
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;

    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI("GAP", "Pairing successful with: %s", param->auth_cmpl.device_name);
        }
        else
        {
            ESP_LOGE("GAP", "Pairing failed, status: %d", param->auth_cmpl.stat);
        }
        break;

    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGI(BtMultiSPP::logTag_, "GAP Mode Changed");
        break;

    default:
        break;
    }
}
