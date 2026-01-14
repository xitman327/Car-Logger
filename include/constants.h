// #pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
/*
 * OBD-II Mode 01 PID data length table
 * Index = PID (0x00 - 0xC8)
 * Value = number of data bytes returned
 * 0 = unsupported / undefined
 */
const uint8_t obd_pid_len[0xC9] = {
    /* 0x00 */ 4,  /* Supported PIDs 01–20 */
    /* 0x01 */ 4,  /* Monitor status since DTCs cleared */
    /* 0x02 */ 2,  /* Freeze DTC */
    /* 0x03 */ 2,  /* Fuel system status */
    /* 0x04 */ 1,  /* Calculated engine load */
    /* 0x05 */ 1,  /* Engine coolant temperature */
    /* 0x06 */ 1,  /* Short term fuel trim—Bank 1 */
    /* 0x07 */ 1,  /* Long term fuel trim—Bank 1 */
    /* 0x08 */ 1,  /* Short term fuel trim—Bank 2 */
    /* 0x09 */ 1,  /* Long term fuel trim—Bank 2 */
    /* 0x0A */ 1,  /* Fuel pressure */
    /* 0x0B */ 1,  /* Intake manifold absolute pressure */
    /* 0x0C */ 2,  /* Engine RPM */
    /* 0x0D */ 1,  /* Vehicle speed */
    /* 0x0E */ 1,  /* Timing advance */
    /* 0x0F */ 1,  /* Intake air temperature */
    /* 0x10 */ 2,  /* MAF air flow rate */
    /* 0x11 */ 1,  /* Throttle position */
    /* 0x12 */ 1,  /* Commanded secondary air status */
    /* 0x13 */ 1,  /* Oxygen sensors present */
    /* 0x14 */ 2,  /* O2 Sensor 1 */
    /* 0x15 */ 2,
    /* 0x16 */ 2,
    /* 0x17 */ 2,
    /* 0x18 */ 2,
    /* 0x19 */ 2,
    /* 0x1A */ 2,
    /* 0x1B */ 2,
    /* 0x1C */ 1,  /* OBD standards */
    /* 0x1D */ 1,  /* Oxygen sensors present (4 banks) */
    /* 0x1E */ 1,  /* Auxiliary input status */
    /* 0x1F */ 2,  /* Run time since engine start */

    /* 0x20 */ 4,  /* Supported PIDs 21–40 */
    /* 0x21 */ 2,  /* Distance traveled with MIL on */
    /* 0x22 */ 2,  /* Fuel Rail Pressure */
    /* 0x23 */ 2,
    /* 0x24 */ 4,
    /* 0x25 */ 4,
    /* 0x26 */ 4,
    /* 0x27 */ 4,
    /* 0x28 */ 4,
    /* 0x29 */ 4,
    /* 0x2A */ 1,  /* Commanded EGR */
    /* 0x2B */ 1,  /* EGR Error */
    /* 0x2C */ 1,  /* Commanded evaporative purge */
    /* 0x2D */ 1,  /* Fuel tank level input */
    /* 0x2E */ 1,  /* Warm-ups since codes cleared */
    /* 0x2F */ 1,  /* Distance since codes cleared */

    /* 0x30 */ 2,
    /* 0x31 */ 2,
    /* 0x32 */ 2,
    /* 0x33 */ 1,
    /* 0x34 */ 4,
    /* 0x35 */ 4,
    /* 0x36 */ 4,
    /* 0x37 */ 4,
    /* 0x38 */ 4,
    /* 0x39 */ 4,
    /* 0x3A */ 4,
    /* 0x3B */ 4,
    /* 0x3C */ 2,
    /* 0x3D */ 2,
    /* 0x3E */ 2,
    /* 0x3F */ 2,

    /* 0x40 */ 4,  /* Supported PIDs 41–60 */
    /* 0x41 */ 4,
    /* 0x42 */ 2,
    /* 0x43 */ 2,
    /* 0x44 */ 2,
    /* 0x45 */ 1,
    /* 0x46 */ 1,
    /* 0x47 */ 1,
    /* 0x48 */ 1,
    /* 0x49 */ 1,
    /* 0x4A */ 1,
    /* 0x4B */ 1,
    /* 0x4C */ 1,
    /* 0x4D */ 2,
    /* 0x4E */ 2,
    /* 0x4F */ 2,

    /* 0x50 */ 4,
    /* 0x51 */ 1,
    /* 0x52 */ 1,
    /* 0x53 */ 2,
    /* 0x54 */ 2,
    /* 0x55 */ 2,
    /* 0x56 */ 2,
    /* 0x57 */ 2,
    /* 0x58 */ 2,
    /* 0x59 */ 2,
    /* 0x5A */ 2,
    /* 0x5B */ 2,
    /* 0x5C */ 2,
    /* 0x5D */ 2,
    /* 0x5E */ 2,
    /* 0x5F */ 2,

    /* 0x60 */ 4,  /* Supported PIDs 61–80 */
    /* 0x61 */ 1,
    /* 0x62 */ 1,
    /* 0x63 */ 2,
    /* 0x64 */ 5,
    /* 0x65 */ 2,
    /* 0x66 */ 5,
    /* 0x67 */ 3,
    /* 0x68 */ 7,
    /* 0x69 */ 7,
    /* 0x6A */ 5,
    /* 0x6B */ 5,
    /* 0x6C */ 5,
    /* 0x6D */ 6,
    /* 0x6E */ 5,
    /* 0x6F */ 5,

    /* 0x70 */ 0,
    /* 0x71 */ 0,
    /* 0x72 */ 0,
    /* 0x73 */ 0,
    /* 0x74 */ 0,
    /* 0x75 */ 0,
    /* 0x76 */ 0,
    /* 0x77 */ 0,
    /* 0x78 */ 0,
    /* 0x79 */ 0,
    /* 0x7A */ 0,
    /* 0x7B */ 0,
    /* 0x7C */ 0,
    /* 0x7D */ 0,
    /* 0x7E */ 0,
    /* 0x7F */ 0,

    /* 0x80 */ 4,  /* Supported PIDs 81–A0 */
    /* 0x81 */ 0,
    /* 0x82 */ 0,
    /* 0x83 */ 0,
    /* 0x84 */ 0,
    /* 0x85 */ 0,
    /* 0x86 */ 0,
    /* 0x87 */ 0,
    /* 0x88 */ 0,
    /* 0x89 */ 0,
    /* 0x8A */ 0,
    /* 0x8B */ 0,
    /* 0x8C */ 0,
    /* 0x8D */ 0,
    /* 0x8E */ 0,
    /* 0x8F */ 0,

    /* 0x90 */ 0,
    /* 0x91 */ 0,
    /* 0x92 */ 0,
    /* 0x93 */ 0,
    /* 0x94 */ 0,
    /* 0x95 */ 0,
    /* 0x96 */ 0,
    /* 0x97 */ 0,
    /* 0x98 */ 0,
    /* 0x99 */ 0,
    /* 0x9A */ 0,
    /* 0x9B */ 0,
    /* 0x9C */ 0,
    /* 0x9D */ 0,
    /* 0x9E */ 0,
    /* 0x9F */ 0,

    /* 0xA0 */ 4,  /* Supported PIDs A1–C0 */
    /* 0xA1 - 0xC8 */ 
};

typedef enum {
    /* 0x00 – 0x1F */
    PID_SUPPORTED_01_20            = 0x00,
    PID_MONITOR_STATUS             = 0x01,
    PID_FREEZE_DTC                 = 0x02,
    PID_FUEL_SYSTEM_STATUS         = 0x03,
    PID_ENGINE_LOAD                = 0x04,
    PID_ENGINE_COOLANT_TEMP        = 0x05,
    PID_SHORT_FUEL_TRIM_BANK1      = 0x06,
    PID_LONG_FUEL_TRIM_BANK1       = 0x07,
    PID_SHORT_FUEL_TRIM_BANK2      = 0x08,
    PID_LONG_FUEL_TRIM_BANK2       = 0x09,
    PID_FUEL_PRESSURE              = 0x0A,
    PID_INTAKE_MANIFOLD_PRESSURE   = 0x0B,
    PID_ENGINE_RPM                 = 0x0C,
    PID_VEHICLE_SPEED              = 0x0D,
    PID_TIMING_ADVANCE             = 0x0E,
    PID_INTAKE_AIR_TEMP            = 0x0F,
    PID_MAF_AIR_FLOW               = 0x10,
    PID_THROTTLE_POSITION          = 0x11,
    PID_SECONDARY_AIR_STATUS       = 0x12,
    PID_OXYGEN_SENSORS_PRESENT     = 0x13,
    PID_O2_SENSOR_1                = 0x14,
    PID_O2_SENSOR_2                = 0x15,
    PID_O2_SENSOR_3                = 0x16,
    PID_O2_SENSOR_4                = 0x17,
    PID_O2_SENSOR_5                = 0x18,
    PID_O2_SENSOR_6                = 0x19,
    PID_O2_SENSOR_7                = 0x1A,
    PID_O2_SENSOR_8                = 0x1B,
    PID_OBD_STANDARD               = 0x1C,
    PID_OXYGEN_SENSORS_PRESENT_4   = 0x1D,
    PID_AUX_INPUT_STATUS           = 0x1E,
    PID_RUNTIME_SINCE_START        = 0x1F,

    /* 0x20 – 0x3F */
    PID_SUPPORTED_21_40            = 0x20,
    PID_DISTANCE_WITH_MIL          = 0x21,
    PID_FUEL_RAIL_PRESSURE         = 0x22,
    PID_FUEL_RAIL_PRESSURE_DIRECT  = 0x23,
    PID_O2_SENSOR_1_WR             = 0x24,
    PID_O2_SENSOR_2_WR             = 0x25,
    PID_O2_SENSOR_3_WR             = 0x26,
    PID_O2_SENSOR_4_WR             = 0x27,
    PID_O2_SENSOR_5_WR             = 0x28,
    PID_O2_SENSOR_6_WR             = 0x29,
    PID_O2_SENSOR_7_WR             = 0x2A,
    PID_O2_SENSOR_8_WR             = 0x2B,
    PID_COMMANDED_EGR              = 0x2C,
    PID_EGR_ERROR                  = 0x2D,
    PID_COMMANDED_EVAP_PURGE       = 0x2E,
    PID_FUEL_TANK_LEVEL            = 0x2F,
    PID_WARMUPS_SINCE_CLEAR        = 0x30,
    PID_DISTANCE_SINCE_CLEAR       = 0x31,
    PID_EVAP_SYSTEM_VAP_PRESSURE   = 0x32,
    PID_BAROMETRIC_PRESSURE        = 0x33,
    PID_O2_SENSOR_1_WIDE           = 0x34,
    PID_O2_SENSOR_2_WIDE           = 0x35,
    PID_O2_SENSOR_3_WIDE           = 0x36,
    PID_O2_SENSOR_4_WIDE           = 0x37,
    PID_O2_SENSOR_5_WIDE           = 0x38,
    PID_O2_SENSOR_6_WIDE           = 0x39,
    PID_O2_SENSOR_7_WIDE           = 0x3A,
    PID_O2_SENSOR_8_WIDE           = 0x3B,
    PID_CATALYST_TEMP_BANK1_SENSOR1= 0x3C,
    PID_CATALYST_TEMP_BANK2_SENSOR1= 0x3D,
    PID_CATALYST_TEMP_BANK1_SENSOR2= 0x3E,
    PID_CATALYST_TEMP_BANK2_SENSOR2= 0x3F,

    /* 0x40 – 0x5F */
    PID_SUPPORTED_41_60            = 0x40,
    PID_MONITOR_STATUS_THIS_DRIVE  = 0x41,
    PID_CONTROL_MODULE_VOLTAGE     = 0x42,
    PID_ABSOLUTE_ENGINE_LOAD       = 0x43,
    PID_COMMANDED_EQUIV_RATIO      = 0x44,
    PID_RELATIVE_THROTTLE_POS      = 0x45,
    PID_AMBIENT_AIR_TEMP           = 0x46,
    PID_ABSOLUTE_THROTTLE_POS_B    = 0x47,
    PID_ABSOLUTE_THROTTLE_POS_C    = 0x48,
    PID_ACCELERATOR_POS_D          = 0x49,
    PID_ACCELERATOR_POS_E          = 0x4A,
    PID_ACCELERATOR_POS_F          = 0x4B,
    PID_COMMANDED_THROTTLE_ACT     = 0x4C,
    PID_RUNTIME_WITH_MIL           = 0x4D,
    PID_TIME_SINCE_CLEAR           = 0x4E,
    PID_MAX_VALUES                 = 0x4F,

    /* 0x50 – 0x6F */
    PID_SUPPORTED_61_80            = 0x50,
    PID_FUEL_TYPE                  = 0x51,
    PID_ETHANOL_PERCENT            = 0x52,
    PID_ABSOLUTE_EVAP_PRESSURE     = 0x53,
    PID_EVAP_PRESSURE_ALT          = 0x54,
    PID_SHORT_O2_TRIM_B1           = 0x55,
    PID_LONG_O2_TRIM_B1            = 0x56,
    PID_SHORT_O2_TRIM_B2           = 0x57,
    PID_LONG_O2_TRIM_B2            = 0x58,
    PID_FUEL_RAIL_PRESSURE_ABS     = 0x59,
    PID_RELATIVE_ACCEL_POS         = 0x5A,
    PID_HYBRID_BATTERY_LIFE        = 0x5B,
    PID_ENGINE_OIL_TEMP            = 0x5C,
    PID_FUEL_INJECTION_TIMING      = 0x5D,
    PID_ENGINE_FUEL_RATE           = 0x5E,
    PID_EMISSION_REQUIREMENTS     = 0x5F,

    /* 0x60 – 0x7F */
    PID_SUPPORTED_81_A0            = 0x60,
    PID_DEMAND_ENGINE_TORQUE       = 0x61,
    PID_ACTUAL_ENGINE_TORQUE       = 0x62,
    PID_ENGINE_REFERENCE_TORQUE    = 0x63,
    PID_ENGINE_PERCENT_TORQUE_DATA = 0x64,
    PID_AUX_INPUT_OUTPUT_STATUS    = 0x65,
    PID_MASS_AIR_FLOW_SENSOR       = 0x66,
    PID_ENGINE_COOLANT_TEMP_ALT    = 0x67,
    PID_INTAKE_AIR_TEMP_ALT        = 0x68,
    PID_EXHAUST_GAS_TEMP           = 0x69,
    PID_EXHAUST_GAS_TEMP_ALT       = 0x6A,
    PID_THROTTLE_ACTUATOR_CTRL     = 0x6B,
    PID_FUEL_PRESSURE_CTRL         = 0x6C,
    PID_INJECTION_PRESSURE_CTRL    = 0x6D,
    PID_TURBOCHARGER_RPM           = 0x6E,
    PID_TURBOCHARGER_TEMP          = 0x6F,

    /* 0x70 – 0x7F (mostly reserved) */
    PID_RESERVED_70                = 0x70,
    PID_RESERVED_71                = 0x71,
    PID_RESERVED_72                = 0x72,
    PID_RESERVED_73                = 0x73,
    PID_RESERVED_74                = 0x74,
    PID_RESERVED_75                = 0x75,
    PID_RESERVED_76                = 0x76,
    PID_RESERVED_77                = 0x77,
    PID_RESERVED_78                = 0x78,
    PID_RESERVED_79                = 0x79,
    PID_RESERVED_7A                = 0x7A,
    PID_RESERVED_7B                = 0x7B,
    PID_RESERVED_7C                = 0x7C,
    PID_RESERVED_7D                = 0x7D,
    PID_RESERVED_7E                = 0x7E,
    PID_RESERVED_7F                = 0x7F,

    /* 0x80 – 0xC8 (mostly manufacturer / future) */
    PID_SUPPORTED_A1_C0            = 0x80,
    PID_MANUFACTURER_A1            = 0xA1,
    PID_MANUFACTURER_A2            = 0xA2,
    PID_MANUFACTURER_A3            = 0xA3,
    PID_MANUFACTURER_A4            = 0xA4,
    PID_MANUFACTURER_A5            = 0xA5,
    PID_MANUFACTURER_A6            = 0xA6,
    PID_MANUFACTURER_A7            = 0xA7,
    PID_MANUFACTURER_A8            = 0xA8,
    PID_MANUFACTURER_A9            = 0xA9,
    PID_MANUFACTURER_AA            = 0xAA,
    PID_MANUFACTURER_AB            = 0xAB,
    PID_MANUFACTURER_AC            = 0xAC,
    PID_MANUFACTURER_AD            = 0xAD,
    PID_MANUFACTURER_AE            = 0xAE,
    PID_MANUFACTURER_AF            = 0xAF,

    PID_MANUFACTURER_B0            = 0xB0,
    PID_MANUFACTURER_B1            = 0xB1,
    PID_MANUFACTURER_B2            = 0xB2,
    PID_MANUFACTURER_B3            = 0xB3,
    PID_MANUFACTURER_B4            = 0xB4,
    PID_MANUFACTURER_B5            = 0xB5,
    PID_MANUFACTURER_B6            = 0xB6,
    PID_MANUFACTURER_B7            = 0xB7,
    PID_MANUFACTURER_B8            = 0xB8,
    PID_MANUFACTURER_B9            = 0xB9,
    PID_MANUFACTURER_BA            = 0xBA,
    PID_MANUFACTURER_BB            = 0xBB,
    PID_MANUFACTURER_BC            = 0xBC,
    PID_MANUFACTURER_BD            = 0xBD,
    PID_MANUFACTURER_BE            = 0xBE,
    PID_MANUFACTURER_BF            = 0xBF,

    PID_MANUFACTURER_C0            = 0xC0,
    PID_MANUFACTURER_C1            = 0xC1,
    PID_MANUFACTURER_C2            = 0xC2,
    PID_MANUFACTURER_C3            = 0xC3,
    PID_MANUFACTURER_C4            = 0xC4,
    PID_MANUFACTURER_C5            = 0xC5,
    PID_MANUFACTURER_C6            = 0xC6,
    PID_MANUFACTURER_C7            = 0xC7,
    PID_MANUFACTURER_C8            = 0xC8

} obd_pid_t;

// static const char * const PID_NAMES[0x74] = {
//     [0x00] = "Supported PIDs 00-1F",
//     [0x01] = "Monitor status since DTC clear",
//     [0x02] = "Freeze DTC",
//     [0x03] = "Fuel system status",
//     [0x04] = "Calculated engine load",
//     [0x05] = "Engine coolant temperature",
//     [0x06] = "Short-term fuel trim Bank 1",
//     [0x07] = "Long-term fuel trim Bank 1",
//     [0x08] = "Short-term fuel trim Bank 2",
//     [0x09] = "Long-term fuel trim Bank 2",
//     [0x0A] = "Fuel pressure",
//     [0x0B] = "Intake manifold absolute pressure",
//     [0x0C] = "Engine RPM",
//     [0x0D] = "Vehicle speed",
//     [0x0E] = "Timing advance",
//     [0x0F] = "Intake air temperature",

//     [0x10] = "MAF air flow rate",
//     [0x11] = "Throttle position",
//     [0x12] = "Commanded secondary air",
//     [0x13] = "O2 sensors present (banks 1-2)",
//     [0x14] = "O2 S1 B1 voltage / STFT",
//     [0x15] = "O2 S2 B1 voltage / STFT",
//     [0x16] = "O2 S3 B1 voltage / STFT",
//     [0x17] = "O2 S4 B1 voltage / STFT",
//     [0x18] = "O2 S1 B2 voltage / STFT",
//     [0x19] = "O2 S2 B2 voltage / STFT",
//     [0x1A] = "O2 S3 B2 voltage / STFT",
//     [0x1B] = "O2 S4 B2 voltage / STFT",
//     [0x1C] = "OBD standard the vehicle conforms to",
//     [0x1D] = "O2 sensors present (banks)",
//     [0x1E] = "Auxiliary input status",
//     [0x1F] = "Run time since engine start",

//     [0x20] = "Supported PIDs 21-40",
//     [0x21] = "Distance traveled with MIL on",
//     [0x22] = "Fuel rail pressure (vacuum reference)",
//     [0x23] = "Fuel rail gauge pressure (diesel/direct)",
//     [0x24] = "O2 S1 wideband lambda (eq ratio / V)",
//     [0x25] = "O2 S2 wideband lambda (eq ratio / V)",
//     [0x26] = "O2 S3 wideband lambda (eq ratio / V)",
//     [0x27] = "O2 S4 wideband lambda (eq ratio / V)",
//     [0x28] = "O2 S5 wideband lambda (eq ratio / V)",
//     [0x29] = "O2 S6 wideband lambda (eq ratio / V)",
//     [0x2A] = "O2 S7 wideband lambda (eq ratio / V)",
//     [0x2B] = "O2 S8 wideband lambda (eq ratio / V)",
//     [0x2C] = "Commanded EGR",
//     [0x2D] = "EGR error",
//     [0x2E] = "Commanded evaporative purge",
//     [0x2F] = "Fuel level input",

//     [0x30] = "Warm-ups since DTC clear",
//     [0x31] = "Distance traveled since DTC clear",
//     [0x32] = "Evap system vapor pressure",
//     [0x33] = "Barometric pressure",
//     [0x34] = "O2 S1 wideband lambda (current)",
//     [0x35] = "O2 S2 wideband lambda (current)",
//     [0x36] = "O2 S3 wideband lambda (current)",
//     [0x37] = "O2 S4 wideband lambda (current)",
//     [0x38] = "O2 S5 wideband lambda (current)",
//     [0x39] = "O2 S6 wideband lambda (current)",
//     [0x3A] = "O2 S7 wideband lambda (current)",
//     [0x3B] = "O2 S8 wideband lambda (current)",
//     [0x3C] = "Catalyst temperature B1S1",
//     [0x3D] = "Catalyst temperature B2S1",
//     [0x3E] = "Catalyst temperature B1S2",
//     [0x3F] = "Catalyst temperature B2S2",

//     [0x40] = "Supported PIDs 41-60",
//     [0x41] = "Monitor status this drive cycle",
//     [0x42] = "Control module voltage",
//     [0x43] = "Absolute load value",
//     [0x44] = "Commanded equivalence ratio",
//     [0x45] = "Relative throttle position",
//     [0x46] = "Ambient air temperature",
//     [0x47] = "Absolute throttle position B",
//     [0x48] = "Absolute throttle position C",
//     [0x49] = "Accelerator pedal position D",
//     [0x4A] = "Accelerator pedal position E",
//     [0x4B] = "Accelerator pedal position F",
//     [0x4C] = "Commanded throttle actuator",
//     [0x4D] = "Time run with MIL on",
//     [0x4E] = "Time since DTCs cleared",
//     [0x4F] = "Max values for eq ratio, O2, MAP",

//     [0x50] = "Fuel type",
//     [0x51] = "Ethanol fuel percentage",
//     [0x52] = "Absolute evap system vapor pressure",
//     [0x53] = "Evap system vapor pressure",
//     [0x54] = "ST secondary O2 trim B1/B3",
//     [0x55] = "LT secondary O2 trim B1/B3",
//     [0x56] = "ST secondary O2 trim B2/B4",
//     [0x57] = "LT secondary O2 trim B2/B4",
//     [0x58] = "Fuel rail absolute pressure",
//     [0x59] = "Relative accelerator pedal position",
//     [0x5A] = "Hybrid battery pack remaining life",
//     [0x5B] = "Engine oil temperature",
//     [0x5C] = "Fuel injection timing",
//     [0x5D] = "Engine fuel rate",
//     [0x5E] = "Emission requirements (design)",
//     [0x5F] = "GM proprietary / reserved (0x5F)",

//     [0x60] = "Supported PIDs 61-80",
//     [0x61] = "Driver demand engine percent torque",
//     [0x62] = "Actual engine percent torque",
//     [0x63] = "Engine reference torque",
//     [0x64] = "Engine percent torque data (idle/points)",
//     [0x65] = "Aux input/output supported (GM)",
//     [0x66] = "Mass air flow sensor high-res (GM)",
//     [0x67] = "Engine coolant temp high-res (GM)",
//     [0x68] = "Intake air temp high-res (GM)",
//     [0x69] = "Commanded EGR / EGR error (GM)",
//     [0x6A] = "Evap purge flow (GM)",
//     [0x6B] = "Turbo / supercharger boost (GM)",
//     [0x6C] = "Charge air cooler outlet temp (GM)",
//     [0x6D] = "Exhaust gas temperature sensor (GM)",
//     [0x6E] = "Diesel particulate filter temp/pressure (GM)",
//     [0x6F] = "Engine friction percent torque",
//     [0x70] = "GM proprietary / reserved (0x70)",
//     [0x71 - 0xC8] = "nothing",
//     [0xC9] = "Adapter Temperature",
//     [0xCA] = "Adapter Humidity"
// };

// static inline const char *pid_name(uint8_t pid)
// {
//     size_t PID_COUNT = sizeof(PID_NAMES) / sizeof(PID_NAMES[0]);

//     if (pid < PID_COUNT && PID_NAMES[pid]) {
//         return PID_NAMES[pid];
//     }

//     static char buf[20];
//     sprintf(buf, "Unknown PID 0x%02X", pid);
//     return buf;
// }


// Returns a pointer to a string literal when known,
// otherwise returns a formatted "Unknown PID 0xXX" string.
static inline const char *pid_name(uint8_t pid)
{
    switch (pid) {
        case 0x00: return "Sup PIDs 00-1F";
        case 0x01: return "Mon stat clr";
        case 0x02: return "Freeze DTC";
        case 0x03: return "Fuel sys stat";
        case 0x04: return "Calc eng load";
        case 0x05: return "coolant temp";
        case 0x06: return "ST fuel trim B1";
        case 0x07: return "LT fuel trim B1";
        case 0x08: return "ST fuel trim B2";
        case 0x09: return "LT fuel trim B2";
        case 0x0A: return "Fuel press";
        case 0x0B: return "Intake MAP";
        case 0x0C: return "Engine RPM";
        case 0x0D: return "Vehicle spd";
        case 0x0E: return "Timing adv";
        case 0x0F: return "Intake air T";
        case 0x10: return "MAF rate";
        case 0x11: return "Throttle pos";
        case 0x12: return "Cmd sec air";
        case 0x13: return "O2 sens pres";
        case 0x14: return "O2 B1 S1 V";
        case 0x15: return "O2 B1 S2 V";
        case 0x16: return "O2 B1 S3 V";
        case 0x17: return "O2 B1 S4 V";
        case 0x18: return "O2 B2 S1 V";
        case 0x19: return "O2 B2 S2 V";
        case 0x1A: return "O2 B2 S3 V";
        case 0x1B: return "O2 B2 S4 V";
        case 0x1C: return "OBD standard";
        case 0x1D: return "O2 sens banks";
        case 0x1E: return "Aux input stat";
        case 0x1F: return "Run time eng";

        case 0x20: return "Sup PIDs 21-40";
        case 0x21: return "Dist w/ MIL";
        case 0x22: return "Fuel rail vac";
        case 0x23: return "Fuel rail dir";
        case 0x24: return "O2 WB S1";
        case 0x25: return "O2 WB S2";
        case 0x26: return "O2 WB S3";
        case 0x27: return "O2 WB S4";
        case 0x28: return "O2 WB S5";
        case 0x29: return "O2 WB S6";
        case 0x2A: return "O2 WB S7";
        case 0x2B: return "O2 WB S8";
        case 0x2C: return "Commanded EGR";
        case 0x2D: return "EGR error";
        case 0x2E: return "Cmd evap purge";
        case 0x2F: return "Fuel level";
        case 0x30: return "Warmups clr";
        case 0x31: return "Dist since clr";
        case 0x32: return "Evap vap press";
        case 0x33: return "Baro press";
        case 0x34: return "O2 WB S1 I";
        case 0x35: return "O2 WB S2 I";
        case 0x36: return "O2 WB S3 I";
        case 0x37: return "O2 WB S4 I";
        case 0x38: return "O2 WB S5 I";
        case 0x39: return "O2 WB S6 I";
        case 0x3A: return "O2 WB S7 I";
        case 0x3B: return "O2 WB S8 I";
        case 0x3C: return "Cat temp B1S1";
        case 0x3D: return "Cat temp B2S1";
        case 0x3E: return "Cat temp B1S2";
        case 0x3F: return "Cat temp B2S2";

        case 0x40: return "Sup PIDs 41-60";
        case 0x41: return "Mon stat drive";
        case 0x42: return "Ctrl mod volt";
        case 0x43: return "Abs load";
        case 0x44: return "Cmd eq ratio";
        case 0x45: return "Rel throttle";
        case 0x46: return "Ambient air T";
        case 0x47: return "Abs thr pos B";
        case 0x48: return "Abs thr pos C";
        case 0x49: return "Accel pos D";
        case 0x4A: return "Accel pos E";
        case 0x4B: return "Accel pos F";
        case 0x4C: return "Cmd throttle";
        case 0x4D: return "Time w/ MIL";
        case 0x4E: return "Time since clr";
        case 0x4F: return "Max eq/O2/MAP";
        case 0x50: return "Fuel type";
        case 0x51: return "Ethanol pct";
        case 0x52: return "Abs evap press";
        case 0x53: return "Evap vap press";
        case 0x54: return "ST sec O2 B1";
        case 0x55: return "LT sec O2 B1";
        case 0x56: return "ST sec O2 B2";
        case 0x57: return "LT sec O2 B2";
        case 0x58: return "Fuel rail abs";
        case 0x59: return "Rel accel pos";
        case 0x5A: return "Hybrid batt %";
        case 0x5B: return "Eng oil temp";
        case 0x5C: return "Fuel inj time";
        case 0x5D: return "Eng fuel rate";
        case 0x5E: return "Emiss req";
        case 0x5F: return "GM prop/resv";

        case 0x60: return "Sup PIDs 61-80";
        case 0x61: return "Drv dem torque";
        case 0x62: return "Act eng torque";
        case 0x63: return "Eng ref torque";
        case 0x64: return "Eng % torque";
        case 0x65: return "Aux in/out sup";
        case 0x66: return "MAF sensor hi";
        case 0x67: return "Coolant T hi";
        case 0x68: return "Intake T hi";
        case 0x69: return "Cmd EGR err";
        case 0x6A: return "Evap purge";
        case 0x6B: return "Turbo boost";
        case 0x6C: return "CAC out temp";
        case 0x6D: return "Exh gas temp";
        case 0x6E: return "DPF temp/press";
        case 0x6F: return "Eng fric %";
        case 0x70: return "GM prop/resv";

        // Your custom PIDs (examples — rename as you like)
        case 0xC9: return "Adapter temp";
        case 0xCA: return "Adapter humid";
        case 0xCC: return "Adapter Volt";
        case 0xCD: return "Adapter Aux1";
        case 0xCE: return "Adapter Aux2";
        case 0xCF: return "Adapter Aux3";

        default:
            static char buf[24];
            snprintf(buf, sizeof(buf), "Unknown PID 0x%02X", pid);
            return buf;
            break;
    }

    return "";
}
