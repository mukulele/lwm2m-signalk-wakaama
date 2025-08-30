/**
 * @file test_marine_sensors.c
 * @brief Marine Sensor Integration Test Suite
 */

#include "test_framework.h"

static int setup_marine_sensor_tests(void) { return 0; }
static int teardown_marine_sensor_tests(void) { return 0; }

static void test_gps_sensor_integration(void)
{
    marine_sensor_data_t* gps_data = test_create_marine_sensor_data(SENSOR_TYPE_GPS);
    CU_ASSERT_PTR_NOT_NULL(gps_data);
    CU_ASSERT_MARINE_SENSOR_DATA_VALID(gps_data);
    test_free(gps_data);
    
    if (g_test_config.verbose) {
        printf("✓ GPS sensor integration validated\n");
    }
}

static void test_wind_sensor_integration(void)
{
    marine_sensor_data_t* wind_data = test_create_marine_sensor_data(SENSOR_TYPE_WIND);
    CU_ASSERT_PTR_NOT_NULL(wind_data);
    CU_ASSERT_MARINE_SENSOR_DATA_VALID(wind_data);
    test_free(wind_data);
    
    if (g_test_config.verbose) {
        printf("✓ Wind sensor integration validated\n");
    }
}

CU_ErrorCode register_marine_sensor_tests(void)
{
    CU_pSuite suite = CU_add_suite("Marine Sensor Tests", 
                                   setup_marine_sensor_tests,
                                   teardown_marine_sensor_tests);
    
    if (suite == NULL) {
        return CU_get_error();
    }
    
    if ((CU_add_test(suite, "GPS Sensor Integration", test_gps_sensor_integration) == NULL) ||
        (CU_add_test(suite, "Wind Sensor Integration", test_wind_sensor_integration) == NULL)) {
        return CU_get_error();
    }
    
    return CUE_SUCCESS;
}
