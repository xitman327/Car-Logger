#include "OBD2_KLine.h"

// #define ELM_RX 35  // Using your original pins
// #define ELM_TX 36

OBD2_KLine KLine(Serial1, 10400, ELM_RX, ELM_TX);

#define pid_request_list_size_max 40
uint8_t pid_request_list_size = pid_request_list_size_max;
uint8_t pid_request_list[pid_request_list_size_max] = {0x0C, 0x0D, 0x10, 0x11, 0x05, 0x42}; // Original PIDs in hex
uint8_t pid_request_list_index = 0;
float pid_values[pid_request_list_size_max];

float lpkm_max, rpmn_max, kmph_max;
bool engine_on;
bool engine_on_prev = false;

float engine_temp;
float consum_l_s, kms, lpkm, gfps, lbsps, rpmn, kmph, maf, fuel_level;
float throttle_pos = 0.0f;
float battery_voltage = 0.0f;

bool can_ready = false, can_connected = false;
bool elm_ready = false, elm_connected = false;
int obd_use = 1;

void update_lpg_state()
{
    if (!engine_on)
    {
        engine_start_ms = 0;
        lpg_eligible = false;
        lpg_likely = false;
        return;
    }
    if (engine_start_ms == 0)
    {
        engine_start_ms = millis();
    }
    if (!lpg_eligible)
    {
        if (engine_temp > lpg_min_coolant_c && millis() - engine_start_ms >= lpg_min_run_ms)
        {
            lpg_eligible = true;
        }
    }
    if (lpg_eligible && rpmn > lpg_rpm_threshold)
    {
        lpg_likely = true;
    }
    else if (rpmn < (lpg_rpm_threshold * 0.8f))
    {
        lpg_likely = false;
    }
}

String statusToString(int id)
{
    switch (id)
    {
    case 0:
        return "ELM_SUCCESS";
    case 1:
        return "ELM_NO_RESPONSE";
    case 2:
        return "ELM_BUFFER_OVERFLOW";
    case 3:
        return "ELM_GARBAGE";
    case 4:
        return "ELM_UNABLE_TO_CONNECT";
    case 5:
        return "ELM_NO_DATA";
    case 6:
        return "ELM_STOPPED";
    case 7:
        return "ELM_TIMEOUT";
    case 8:
        return "ELM_GETTING_MSG";
    case 9:
        return "ELM_MSG_RXD";
    case 10:
        return "ELM_GENERAL_ERROR";
    default:
        return String(id);
    }
}

String protocolNameFromId(char id)
{
    switch (id)
    {
    case 'A':
        return "AUTO";
    case '1':
        return "SAE J1850 PWM 41k";
    case '2':
        return "SAE J1850 PWM 10k";
    case '3':
        return "ISO 9141-2";
    case '4':
        return "ISO 14230-4 (5 baud)";
    case '5':
        return "ISO 14230-4 (fast)";
    case '6':
        return "ISO 15765-4 (11/500)";
    case '7':
        return "ISO 15765-4 (29/500)";
    case '8':
        return "ISO 15765-4 (11/250)";
    case '9':
        return "ISO 15765-4 (29/250)";
    case 'B':
        return "SAE J1939 (29/250)";
    default:
        return String(id);
    }
}

String fetchCurrentProtocol()
{
    if (demo_mode)
    {
        return "demo";
    }

    if (elm_connected)
    {
        return "Kline";
    }
    return "unknown";
}

#define TALK_TO_CAN digitalWrite(TALK_TO_CAN_PIN, LOW);
#define TALK_TO_KLINE digitalWrite(TALK_TO_CAN_PIN, HIGH);

void setup_elm()
{

    pinMode(TALK_TO_CAN_PIN, OUTPUT);
    TALK_TO_KLINE;

    if (obd_use == 1)
    {
        elm_ready = false;
        elm_connected = false;

        Serial.begin(115200);

        // KLine.setDebug(Serial);
        KLine.setProtocol("Automatic");
        KLine.setChecksumType(2);

        elm_ready = true;
        Serial.println("ELM began");
    }
    else if (obd_use == 2)
    {
        can_ready = false;
        can_connected = false;
    }
}

byte pid_num = 0;
#define ELM_ERROR_TM 2000
uint32_t tm_elm_err;
String ELMprotocol;
float responce_pre;
uint32_t responce_int;
int nodata_count = 0;

#define convertion_time 1000
uint32_t tm_convertion = 0, tm_obdpull = 0, tm_t_consum = 0;
void update_distance_from_speed();
void convertion()
{
    if (millis() - tm_convertion > convertion_time)
    {
        tm_convertion = millis();
        if (log_started)
            update_distance_from_speed();

        if (rpmn > 500)
        {
            engine_on = 1;
        }
        else
        {
            engine_on = 0;
        }
        if (engine_on != engine_on_prev)
        {
            engine_on_prev = engine_on;
            if (!engine_on)
            {
                lpg_eligible = false;
                lpg_likely = false;
                engine_start_ms = 0;
            }
            else
            {
                engine_start_ms = millis();
            }
        }
        update_lpg_state();

        if (kmph > (float)1.0)
        {
            kms = kmph * (1.0 / 3600.0);
            lpkm = (float)(3600.0 * maf) / (9069.90 * kms);
        }
        else
        {
            lpkm = 0;
        }

        if (engine_on)
        {
            consum_l_s = maf / 14.7;
            float lt_dts = (consum_l_s / 1000.0) * (millis() - tm_t_consum);
            tm_t_consum = millis();
            current_consumption_l = lt_dts;
            lts_trip += lt_dts;
        }
    }
}
bool request_supported_pids = 0;
bool supported_pids_run = 0;
uint32_t resp;
uint32_t supported_PIDs_1_20 = 0, supported_PIDs_21_40 = 0, supported_PIDs_41_60 = 0, supported_PIDs_61_80 = 0;
uint8_t i_supportred_pids;
uint8_t supported_pids_status = 0;

int get_supported_pids(bool run_again = 0)
{
    int status = 1;
    if (elm_connected && elm_ready)
    {
        if (run_again)
        {
            supported_pids_run = 0;
            supported_PIDs_1_20 = 0;
            supported_PIDs_21_40 = 0;
            supported_PIDs_41_60 = 0;
            supported_PIDs_61_80 = 0;
        }

        if (!supported_pids_run)
        {
            supported_pids_run = 1; // KLine doesn't support PID checking
            status = 2;             // Already run
        }
        else
        {
            status = 2;
        }
    }
    else
    {
        status = 4;
    }

    return status;
}

void loop_elm()
{
    if (elm_ready && !elm_connected)
    {
        rpmn = 0;
        kmph = 0;
        maf = 0;
        throttle_pos = 0;
        engine_temp = 0;
        battery_voltage = 0;

        if (KLine.initOBD2())
        {
            elm_connected = true;
            // ELMprotocol = KLine.getCurrentProtocol();
        }
        else
        {
            nodata_count++;
            if (nodata_count > 10)
            {
                setup_elm();
                nodata_count = 0;
            }
        }
    }

    if (elm_ready && elm_connected && !supported_pids_run)
    {
        supported_pids_status = get_supported_pids(request_supported_pids);
    }

    if (pid_request_list_index < pid_request_list_size)
    {
        if (pid_request_list[pid_request_list_index] != 0)
        {
            uint8_t requested_pid_A = pid_request_list[pid_request_list_index];
            if (requested_pid_A == 0)
            {
                pid_request_list_index++;
            } // pid 0 means disabled, nothing to monitor
            else if (requested_pid_A > 0xc8)
            { // outside of the obd2 pids, livethe onboard adaptor sensors
                switch (requested_pid_A)
                {
                case 0xc9: // adapter temperature
                    pid_values[pid_request_list_index] = temperaturas;
                    break;
                case 0xcA: // adapter humidity
                    pid_values[pid_request_list_index] = humidituras;
                    break;
                case 0xcB:                                  // elevation
                    pid_values[pid_request_list_index] = 0; // fix.altitude();
                    break;
                case 0xcc:
                    pid_values[pid_request_list_index] = vin;
                    break;
                case 0xcD:
                    pid_values[pid_request_list_index] = aux1;
                    break;
                case 0xcE:
                    pid_values[pid_request_list_index] = aux2;
                    break;
                case 0xcF:
                    pid_values[pid_request_list_index] = aux3;
                    break;

                default:
                    Serial.print("Requested Sensor ");
                    Serial.print(requested_pid_A, HEX);
                    Serial.println(" Not Supported");
                    break;
                }
                pid_request_list_index++;
            }
            else if (elm_ready && elm_connected && supported_pids_run && !request_supported_pids)
            {
                // Direct KLine reading - no PID support check
                float raw_value = KLine.getLiveData(requested_pid_A);
                if (raw_value == -1)
                {
                    digitalWrite(LEDB, LOW);
                    elm_connected = false;
                    log_e("KLine lost connection %1.0f", raw_value);
                }
                else if (raw_value == -2)
                {
                    log_e("PID %d Unexpected ", requested_pid_A);
                    digitalWrite(LEDB, !digitalRead(LEDB));
                    pid_values[pid_request_list_index] = 0.0;
                }
                else if (raw_value == -4)
                {
                    log_e("PID %d Unsuported ", requested_pid_A);
                    digitalWrite(LEDB, !digitalRead(LEDB));
                    pid_values[pid_request_list_index] = 0.0;
                }
                else
                {
                    digitalWrite(LEDB, !digitalRead(LEDB));
                    pid_values[pid_request_list_index] = raw_value;

                    // Update global variables for existing code
                    switch (requested_pid_A)
                    {
                    case 0x0C:
                        rpmn = raw_value;
                        break;
                    case 0x0D:
                        kmph = raw_value;
                        break;
                    case 0x10:
                        maf = raw_value;
                        break;
                    case 0x11:
                        throttle_pos = raw_value;
                        break;
                    case 0x05:
                        engine_temp = raw_value;
                        break;
                    case 0x42:
                        battery_voltage = raw_value;
                        break;
                    }

                    pid_request_list_index++;
                }
                // else
                // {
                //     Serial.print("Error reading PID 0x");
                //     Serial.println(requested_pid_A, HEX);
                //     pid_request_list_index++;
                // }
            }
            else
            {
                pid_request_list_index++;
            }
        }
        else
        {
            pid_request_list_index++;
        }
    }
    else
    {
        pid_request_list_index = 0;
    }

    convertion();
}