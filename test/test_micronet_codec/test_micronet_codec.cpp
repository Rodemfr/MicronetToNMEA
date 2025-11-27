/**
 * @file test_micronet_codec.cpp
 * @brief Unit tests for the MicronetCodec class.
 *
 * This file contains unit tests for the MicronetCodec class using the Unity test framework.
 * It verifies the correct functionality of methods within the MicronetCodec.
 */
#include "Micronet/MicronetCodec.h"
#include <Arduino.h>
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
 * @brief Tests the GetNetworkId() method of the MicronetCodec class.
 *
 * This test verifies that the GetNetworkId() method correctly extracts the
 * 4-byte network ID from a Micronet message.
 */
void test_GetNetworkId(void)
{
    // Arrange: Create a codec instance and a test message.
    MicronetCodec     codec;
    MicronetMessage_t message;

    // Define a test network ID (e.g., 0x12345678) and populate the message data.
    const uint32_t expectedNetworkId       = 0x12345678;
    message.data[MICRONET_NUID_OFFSET + 0] = 0x12;
    message.data[MICRONET_NUID_OFFSET + 1] = 0x34;
    message.data[MICRONET_NUID_OFFSET + 2] = 0x56;
    message.data[MICRONET_NUID_OFFSET + 3] = 0x78;

    // Act: Call the method under test.
    uint32_t actualNetworkId = codec.GetNetworkId(&message);

    // Assert: Check if the extracted network ID matches the expected value.
    TEST_ASSERT_EQUAL_UINT32(expectedNetworkId, actualNetworkId);
}

/**
 * @brief Runs all the Unity tests.
 * @return int The result of the test run from UNITY_END().
 */
int runUnityTests(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_GetNetworkId);
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
