#include "ELMduino.h"
// #include <esp32_can.h> // the ESP32_OBD2 library depends on the https://github.com/collin80/esp32_can and https://github.com/collin80/can_common CAN libraries
// #include <esp32_obd2.h>

// #define CAN_RX_PIN  35
// #define CAN_TX_PIN  36

// #include "constants.h"

#define ELMSerial Serial1

ELM327 myELM327;

#define pid_request_list_size_max 40
uint8_t pid_request_list_size = pid_request_list_size_max;
uint8_t pid_request_list[pid_request_list_size_max] = {PID_ENGINE_RPM, PID_VEHICLE_SPEED, PID_MAF_AIR_FLOW, PID_THROTTLE_POSITION, PID_ENGINE_COOLANT_TEMP, PID_CONTROL_MODULE_VOLTAGE};
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
    case ELM_SUCCESS:
        return "ELM_SUCCESS";
    case ELM_NO_RESPONSE:
        return "ELM_NO_RESPONSE";
    case ELM_BUFFER_OVERFLOW:
        return "ELM_BUFFER_OVERFLOW";
    case ELM_GARBAGE:
        return "ELM_GARBAGE";
    case ELM_UNABLE_TO_CONNECT:
        return "ELM_UNABLE_TO_CONNECT";
    case ELM_NO_DATA:
        return "ELM_NO_DATA";
    case ELM_STOPPED:
        return "ELM_STOPPED";
    case ELM_TIMEOUT:
        return "ELM_TIMEOUT";
    case ELM_GETTING_MSG:
        return "ELM_GETTING_MSG";
    case ELM_MSG_RXD:
        return "ELM_MSG_RXD";
    case ELM_GENERAL_ERROR:
        return "ELM_GENERAL_ERROR";
    default:
        return String(id);
    }
}

String protocolNameFromId(char id)
{
    switch (id)
    {
    case AUTOMATIC:
        return "AUTO";
    case SAE_J1850_PWM_41_KBAUD:
        return "SAE J1850 PWM 41k";
    case SAE_J1850_PWM_10_KBAUD:
        return "SAE J1850 PWM 10k";
    case ISO_9141_5_BAUD_INIT:
        return "ISO 9141-2";
    case ISO_14230_5_BAUD_INIT:
        return "ISO 14230-4 (5 baud)";
    case ISO_14230_FAST_INIT:
        return "ISO 14230-4 (fast)";
    case ISO_15765_11_BIT_500_KBAUD:
        return "ISO 15765-4 (11/500)";
    case ISO_15765_29_BIT_500_KBAUD:
        return "ISO 15765-4 (29/500)";
    case ISO_15765_11_BIT_250_KBAUD:
        return "ISO 15765-4 (11/250)";
    case ISO_15765_29_BIT_250_KBAUD:
        return "ISO 15765-4 (29/250)";
    case SAE_J1939_29_BIT_250_KBAUD:
        return "SAE J1939 (29/250)";
    case USER_1_CAN:
        return "USER1 CAN";
    case USER_2_CAN:
        return "USER2 CAN";
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

    int8_t state = myELM327.sendCommand_Blocking(DISP_CURRENT_PROTOCOL_NUM);
    if (state != ELM_SUCCESS)
    {
        myELM327.printError();
        return "unknown";
    }

    String resp = myELM327.payload ? String(myELM327.payload) : "";
    resp.trim();
    if (resp.startsWith("A") || resp.startsWith("a"))
    {
        resp.remove(0, 1); // drop automatic marker
    }
    char proto_id = resp.length() > 0 ? resp.charAt(0) : '?';
    String name = protocolNameFromId(proto_id);
    if (!resp.isEmpty())
    {
        return name + " (" + resp + ")";
    }
    return name;
}

void setup_elm()
{

    if (obd_use == 1)
    {
        elm_ready = false;
        elm_connected = false;

        ELMSerial.begin(38400, SERIAL_8N1, 47, 48);

        if (!myELM327.begin(ELMSerial, false))
        {
            Serial.println("Couldn't connect to OBD scanner");
        }
        else
        {
            elm_ready = true;
            Serial.println("ELM began");
        }
    }
    else if (obd_use == 2)
    {
        can_ready = false;
        can_connected = false;

        // CAN0.setCANPins((gpio_num_t)CAN_RX_PIN, (gpio_num_t)CAN_TX_PIN);

        // if(!OBD2.begin()){
        //     Serial.println("Can begin failled");
        // }else{
        //     can_ready = true;
        //     Serial.println("Can began");
        // }
    }
}
// int pid_list[] = {ENGINE_RPM, VEHICLE_SPEED, MAF_FLOW_RATE, THROTTLE_POSITION, ENGINE_COOLANT_TEMPERATURE};
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

/*

return status;
0 = finished
1 = running
2 = already run
3 = elm msg error
4 = elm not connected or ready

*/
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
            switch (i_supportred_pids)
            {
            case 0:
                resp = myELM327.supportedPIDs_1_20();
                if (myELM327.nb_rx_state == ELM_SUCCESS)
                {
                    i_supportred_pids++;
                    supported_PIDs_1_20 = resp;
                }
                else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
                {
                    myELM327.printError();
                    i_supportred_pids++;
                    status = 3;
                }
                else if (myELM327.nb_rx_state == ELM_TIMEOUT)
                {
                    elm_connected = false;
                }
                break;
            case 1:
                resp = myELM327.supportedPIDs_21_40();
                if (myELM327.nb_rx_state == ELM_SUCCESS)
                {
                    i_supportred_pids++;
                    supported_PIDs_21_40 = resp;
                }
                else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
                {
                    myELM327.printError();
                    i_supportred_pids++;
                    status = 3;
                }
                else if (myELM327.nb_rx_state == ELM_TIMEOUT)
                {
                    elm_connected = false;
                }
                break;
            case 2:
                resp = myELM327.supportedPIDs_41_60();
                if (myELM327.nb_rx_state == ELM_SUCCESS)
                {
                    i_supportred_pids++;
                    supported_PIDs_41_60 = resp;
                }
                else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
                {
                    myELM327.printError();
                    i_supportred_pids++;
                    status = 3;
                }
                else if (myELM327.nb_rx_state == ELM_TIMEOUT)
                {
                    elm_connected = false;
                }
                break;
            case 3:
                resp = myELM327.supportedPIDs_61_80();
                if (myELM327.nb_rx_state == ELM_SUCCESS)
                {
                    i_supportred_pids++;
                    supported_PIDs_61_80 = resp;
                }
                else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
                {
                    myELM327.printError();
                    i_supportred_pids++;
                    status = 3;
                }
                else if (myELM327.nb_rx_state == ELM_TIMEOUT)
                {
                    elm_connected = false;
                }
                break;

            case 4:
            default:
                if (supported_PIDs_1_20 != 0 && supported_PIDs_21_40 != 0 && supported_PIDs_41_60 != 0 && supported_PIDs_61_80 != 0)
                {
                    supported_pids_run = 1;
                    status = 0;
                }
                else
                {
                    i_supportred_pids = 0;
                }
                break;
            }
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
        resp = myELM327.supportedPIDs_1_20();
        if (myELM327.nb_rx_state == ELM_SUCCESS)
        {
            elm_connected = true;
        }
        else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
        {
            myELM327.printError();
            elm_connected = false;
        }
        else if (myELM327.nb_rx_state == ELM_NO_DATA)
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
            // log_i("%d - %d", pid_request_list_index, requested_pid_A);
            if(requested_pid_A == 0){pid_request_list_index++;} // pid 0 means disabled, nothing to monitor
            else if(requested_pid_A > 0xc8){// outside of the obd2 pids, livethe onboard adaptor sensors
                switch (requested_pid_A)
                {
                case 0xc9:// adapter temperature
                    pid_values[pid_request_list_index] = temperaturas;//temperaturas;
                    
                    break;
                case 0xcA:// adapter humidity
                    pid_values[pid_request_list_index] = humidituras;//humidituras;
                    break;
                case 0xcB:// elevation
                    pid_values[pid_request_list_index] = fix.altitude();
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
                    log_e("Requested Sensor %d Not Supported", requested_pid_A);
                    break;
                }
                pid_request_list_index++;
            }
            else if (elm_ready && elm_connected && supported_pids_run && !request_supported_pids)
            {
                if (myELM327.isPidSupported(requested_pid_A)){
                    responce_pre = myELM327.processPID(SERVICE_01, requested_pid_A, 1, obd_pid_len[requested_pid_A]);
                    if (myELM327.nb_rx_state == ELM_SUCCESS)
                    {
                        pid_values[pid_request_list_index] = responce_pre;
                        pid_request_list_index++;
                    }
                    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
                    {
                        myELM327.printError();
                        pid_request_list_index++;
                    }
                    else if (myELM327.nb_rx_state == ELM_TIMEOUT)
                    {
                        elm_connected = false;
                    }
                }
                else
                {
                    log_w("Requested PID %d Not Supported", requested_pid_A);
                }
            }else{
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