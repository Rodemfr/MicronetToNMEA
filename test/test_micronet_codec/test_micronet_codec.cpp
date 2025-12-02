/**
 * @file test_micronet_codec.cpp
 * @brief Unit tests for the MicronetCodec class.
 *
 * This file contains unit tests for the MicronetCodec class using the Unity test framework.
 * It verifies the correct functionality of methods within the MicronetCodec.
 */
#include "Micronet/MicronetCodec.h"
#include "OSWrapper.h"
#include <unity.h>

/**
 * @brief Sets up the test environment before each test.
 * @note This function is required by the Unity test framework but is not used in this test suite.
 */
void setUp(void)
{
    // set stuff up here
}

/**
 * @brief Cleans up the test environment after each test.
 * @note This function is required by the Unity test framework but is not used in this test suite.
 */
void tearDown(void)
{
    // clean stuff up here
}

/**
 * @brief Tests the parsing of all header fields from a Micronet message.
 *
 * This test constructs a Micronet message with known header values and then
 * uses the MicronetCodec getter methods to verify that each field is
 * extracted correctly. It also tests the header CRC calculation and verification.
 */
void test_MessageParsing(void)
{
    // Arrange
    MicronetCodec     codec;
    MicronetMessage_t message = {0};

    // Define expected values for each header field
    const uint32_t expectedNetworkId = 0x12345678;
    const uint8_t  expectedDeviceType     = 0x81; // Example: Dual Display
    const uint32_t expectedDeviceId       = (expectedDeviceType << 24) | 0xCDEF01;
    const uint8_t  expectedMessageId      = 0x02; // Example: Send Data
    const uint8_t  expectedSource         = 0x01; // Example: Measurement device
    const uint8_t  expectedSignalStrength = 0x07; // Example: Signal strength 7
    const uint8_t  expectedMessageLen     = 14;   // Total message length for header only
    const uint8_t  expectedLenField       = expectedMessageLen - 2; // Value for MICRONET_LEN_OFFSET_1/2

    // Populate the message data array with these expected values
    // Network ID (4 bytes)
    message.data[MICRONET_NUID_OFFSET + 0] = (expectedNetworkId >> 24) & 0xFF;
    message.data[MICRONET_NUID_OFFSET + 1] = (expectedNetworkId >> 16) & 0xFF;
    message.data[MICRONET_NUID_OFFSET + 2] = (expectedNetworkId >> 8) & 0xFF;
    message.data[MICRONET_NUID_OFFSET + 3] = expectedNetworkId & 0xFF;

    // Device ID (4 bytes)
    message.data[MICRONET_DUID_OFFSET + 0] = (expectedDeviceId >> 24) & 0xFF;
    message.data[MICRONET_DUID_OFFSET + 1] = (expectedDeviceId >> 16) & 0xFF;
    message.data[MICRONET_DUID_OFFSET + 2] = (expectedDeviceId >> 8) & 0xFF;
    message.data[MICRONET_DUID_OFFSET + 3] = expectedDeviceId & 0xFF;

    // Message ID (1 byte)
    message.data[MICRONET_MI_OFFSET] = expectedMessageId;

    // Source/Device Flags (1 byte)
    message.data[MICRONET_DF_OFFSET] = expectedSource;

    // Signal Strength (1 byte)
    message.data[MICRONET_SS_OFFSET] = expectedSignalStrength;

    // Calculate the expected CRC for the header (bytes 0 to 10 inclusive)
    uint8_t expectedCrc = 0;
    for (int i = 0; i < MICRONET_CRC_OFFSET; i++)
    {
        expectedCrc += message.data[i];
    }

    // Set the calculated CRC at the correct offset
    message.data[MICRONET_CRC_OFFSET] = expectedCrc;

    // Set the message length fields (duplicated)
    message.data[MICRONET_LEN_OFFSET_1] = expectedLenField;
    message.data[MICRONET_LEN_OFFSET_2] = expectedLenField;

    // Set the total message length in the struct
    message.len = expectedMessageLen;

    // Act & Assert for each getter method
    TEST_MESSAGE("Testing GetNetworkId()");
    TEST_ASSERT_EQUAL_UINT32(expectedNetworkId, codec.GetNetworkId(&message));

    TEST_MESSAGE("Testing GetDeviceId()");
    TEST_ASSERT_EQUAL_UINT32(expectedDeviceId, codec.GetDeviceId(&message));

    TEST_MESSAGE("Testing GetDeviceType()");
    TEST_ASSERT_EQUAL_UINT8(expectedDeviceType, codec.GetDeviceType(&message));

    TEST_MESSAGE("Testing GetMessageId()");
    TEST_ASSERT_EQUAL_UINT8(expectedMessageId, codec.GetMessageId(&message));

    TEST_MESSAGE("Testing GetSource()");
    TEST_ASSERT_EQUAL_UINT8(expectedSource, codec.GetSource(&message));

    TEST_MESSAGE("Testing GetSignalStrength()");
    TEST_ASSERT_EQUAL_UINT8(expectedSignalStrength, codec.GetSignalStrength(&message));

    TEST_MESSAGE("Testing GetHeaderCrc()");
    TEST_ASSERT_EQUAL_UINT8(expectedCrc, codec.GetHeaderCrc(&message));

    TEST_MESSAGE("Testing VerifyHeaderCrc() with valid CRC");
    TEST_ASSERT_TRUE(codec.VerifyHeaderCrc(&message));

    // Invalidate CRC and verify VerifyHeaderCrc() fails
    TEST_MESSAGE("Testing VerifyHeaderCrc() with invalid CRC");
    message.data[MICRONET_CRC_OFFSET] = 0x00; // Change CRC to an incorrect value
    TEST_ASSERT_FALSE(codec.VerifyHeaderCrc(&message));
}

/**
 * @brief Runs all the Unity tests.
 * @return int The result of the test run from UNITY_END().
 */
int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_MessageParsing);
    return UNITY_END();
}

/**
 * @brief Main function for native development platforms.
 * @return int The exit code of the test runner.
 */
int main(void)
{
    return runUnityTests();
}

/**
 * @brief Setup function for the Arduino framework.
 * Initializes serial communication and runs the tests.
 */
void setup()
{
    // Wait ~2 seconds before the Unity test runner
    // establishes connection with a board Serial interface
    delay(2000);

    runUnityTests();
}

/**
 * @brief Loop function for the Arduino framework.
 * This function is empty as tests are run only once in setup().
 */
void loop()
{
}

/**
 * @brief Main entry point for the ESP-IDF framework.
 */
void app_main()
{
    runUnityTests();
}
