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
void FatalError();

void setup()
{
    // Initialize console link first
    CONSOLE.begin(CONSOLE_BAUDRATE);
    // If console is not mapped to bluetooth, wait for serial connection
    if ((void *)&CONSOLE != (void *)&gBTSerial)
    {
        while (!CONSOLE)
        {
            delay(10);
        }
    }

    // Initialize BLE link with a more distinctive name
    if (!gBTSerial.begin("MicronetToNMEA"))
    {
        CONSOLE.println("Bluetooth initialization failed");
        FatalError();
    }
    else
    {
        CONSOLE.println("Bluetooth initialized successfully");
    }

    // Load configuration from EEPROM
    gConfiguration.LoadFromEeprom();

    CONSOLE.print("Initializing CC1101 ... ");
    // Check connection to CC1101
    if (!gRfReceiver.Init(&gRxMessageFifo, gConfiguration.eeprom.rfFrequencyOffset_MHz))
    {
        CONSOLE.println("Failed, verify connection to CC1101 board");
        FatalError();
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

void FatalError()
{
    CONSOLE.println("FATAL ERROR : System halted.");
    pinMode(LED_BUILTIN, OUTPUT);
    while (1)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
    }
}