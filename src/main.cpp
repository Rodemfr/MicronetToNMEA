#include "BoardConfig.h"
#include "BtMultiSPP.h"
#include "CC1101Driver.h"
#include "Globals.h"
#include "MenuAbout.h"
#include "MenuAttachNetwork.h"
#include "MenuCalibrateXtal.h"
#include "MenuConfigMtn.h"
#include "MenuConvertToNmea.h"
#include "MenuManager.h"
#include "MenuScanMicronetTraffic.h"
#include "MenuTestRfQuality.h"

extern "C"
{
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
}

const MenuEntry_t mainMenuDesc[] = {{"MicronetToNMEA", nullptr},
                                    {"About MicronetToNMEA", MenuAbout},
                                    {"Calibrate RF oscillator", MenuCalibrateXtal},
                                    {"Attach to closest Micronet network", MenuAttachNetwork},
                                    {"Convert to NMEA", MenuConvertToNmea},
                                    {"Scan surrounding Micronet traffic", MenuScanMicronetTraffic},
                                    {"Configure MicronetToNMEA", MenuConfigMtn},
                                    {"Test RF quality", MenuTestRfQuality},
                                    {nullptr, nullptr}};

void intialize_app();

extern "C" void app_main(void)
{
    intialize_app();

    gMenuManager.SetMenuDescription(mainMenuDesc);

    // If this is the first loop, we verify if we are already attached to a Micronet network. if yes,
    // We directly jump to NMEA conversion mode.
    if (gConfiguration.eeprom.networkId != 0)
    {
        // Menu 4 is NMEA Conversion
        gMenuManager.ActivateMenu(4);
    }

    // Display serial menu
    gMenuManager.PrintMenu();
    gMenuManager.PrintPrompt();

    while (1)
    {
        gMenuManager.PushChar(CONSOLE.read());
    }
}

void intialize_app()
{
    // Initialize LED GPIO
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    // Load configuration from EEPROM
    gConfiguration.LoadFromEeprom();

    gBtMultiSPP.start("TickTack");

    // Init NMEA0183 input serial link
    NMEA0183_IN.start(NMEA0183_IN_BAUDRATE);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Initialize UBlox M6N/M8N GNSS module if applicable
    GNSS.start(GNSS_BAUDRATE);
#if (GNSS_IS_UBLOXM8N == 1)
    CONSOLE.printf("Configuring UBlox M6N/M8N GNSS\r\n");
    CONSOLE.transmit();
    gM8nDriver.Start(M8N_GGA_ENABLE | M8N_VTG_ENABLE | M8N_RMC_ENABLE);
#endif

    CONSOLE.printf("Initializing CC1101 ... \r\n");
    // Check connection to CC1101
    if (!gRfReceiver.Init(&gRxMessageFifo, gConfiguration.eeprom.rfFrequencyOffset_MHz))
    {
        CONSOLE.printf("Failed, verify connection to CC1101 board\r\n");
        while (1)
        {
            gpio_set_level(LED_GPIO, 1);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_level(LED_GPIO, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
    CONSOLE.printf("OK\r\n");
    CONSOLE.transmit();

    CONSOLE.printf("Initializing navigation compass ... \r\n");
    if (!gNavCompass.Init())
    {
        CONSOLE.printf("NOT DETECTED\r\n");
        gConfiguration.ram.navCompassAvailable = false;
    }
    else
    {
        CONSOLE.printf("%s\r\n", gNavCompass.GetDeviceName().c_str());
        CONSOLE.printf("Found\r\n");
        gConfiguration.ram.navCompassAvailable = true;
    }
    CONSOLE.transmit();

    // Start listening
    gRfReceiver.RestartReception();
}
