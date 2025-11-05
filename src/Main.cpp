#include "BoardConfig.h"
#include "Globals.h"
#include "MenuAbout.h"
#include "MenuAttachNetwork.h"
#include "MenuCalibrateCompass.h"
#include "MenuCalibrateXtal.h"
#include "MenuConfigMtn.h"
#include "MenuConvertToNmea.h"
#include "MenuManager.h"
#include "MenuScanMicronetTraffic.h"
#include "MenuTestRfQuality.h"

#include <Arduino.h>
#include <SPI.h>
#include <USB.h>

MenuEntry_t mainMenuDesc[] = {{"MicronetToNMEA", nullptr},
                              {"General info on MicronetToNMEA", MenuAbout},
                              {"Attach converter to closest network", MenuAttachNetwork},
                              {"Start NMEA conversion", MenuConvertToNmea},
                              {"Scan surrounding Micronet traffic", MenuScanMicronetTraffic},
                              {"Calibrate RF XTAL", MenuCalibrateXtal},
                              {"Calibrate compass", MenuCalibrateCompass},
                              {"Test RF quality", MenuTestRfQuality},
                              {"Configuration", MenuConfigMtn},
                              {nullptr, nullptr}};

void RfIsr();

void setup()
{
    // Initialize BLE link
    gBTSerial.begin("MicronetToNMEAv3");

    // Initialize console link
    CONSOLE.begin(CONSOLE_BAUDRATE);
    while (!Serial)
    {
    }

    // Load configuration from EEPROM
    gConfiguration.LoadFromEeprom();

    CONSOLE.print("Initializing CC1101 ... ");
    // Check connection to CC1101
    if (!gRfReceiver.Init(&gRxMessageFifo, gConfiguration.eeprom.rfFrequencyOffset_MHz))
    {
        CONSOLE.println("Failed");
        CONSOLE.println("Aborting execution : Verify connection to CC1101 board");
        CONSOLE.println("Halted");

        pinMode(LED_BUILTIN, OUTPUT);
        while (1)
        {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(500);
            digitalWrite(LED_BUILTIN, LOW);
            delay(500);
        }
    }
    CONSOLE.println("OK");

    // Start listening
    gRfReceiver.RestartReception();

    // Attach callback to GDO0 pin
    // According to CC1101 configuration this callback will be executed when CC1101 will have detected Micronet's sync word
    attachInterrupt(digitalPinToInterrupt(GDO0_PIN), RfIsr, HIGH);

    // Display serial menu
    gMenuManager.SetMenuDescription(mainMenuDesc);
    gMenuManager.PrintMenu();
    gMenuManager.PrintPrompt();
}

void loop()
{
    // Process console input
    while (CONSOLE.available() > 0)
    {
        gMenuManager.PushChar(CONSOLE.read());
    }
}

void RfIsr()
{
    gRfReceiver.RfIsr();
}
