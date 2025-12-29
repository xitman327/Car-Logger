#include "ELMduino.h"
// #include <esp32_can.h> // the ESP32_OBD2 library depends on the https://github.com/collin80/esp32_can and https://github.com/collin80/can_common CAN libraries
// #include <esp32_obd2.h>

// #define CAN_RX_PIN  35
// #define CAN_TX_PIN  36


#define ELMSerial Serial1

ELM327 myELM327;

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

void update_lpg_state(){
  if(!engine_on){
    engine_start_ms = 0;
    lpg_eligible = false;
    lpg_likely = false;
    return;
  }
  if(engine_start_ms == 0){
    engine_start_ms = millis();
  }
  if(!lpg_eligible){
    if(engine_temp > lpg_min_coolant_c && millis() - engine_start_ms >= lpg_min_run_ms){
      lpg_eligible = true;
    }
  }
  if(lpg_eligible && rpmn > lpg_rpm_threshold){
    lpg_likely = true;
  }else if(rpmn < (lpg_rpm_threshold * 0.8f)){
    lpg_likely = false;
  }
}

String statusToString(int id){
    switch(id){
        case ELM_SUCCESS: return "ELM_SUCCESS";
        case ELM_NO_RESPONSE: return "ELM_NO_RESPONSE";
        case ELM_BUFFER_OVERFLOW: return "ELM_BUFFER_OVERFLOW";
        case ELM_GARBAGE: return "ELM_GARBAGE";
        case ELM_UNABLE_TO_CONNECT: return "ELM_UNABLE_TO_CONNECT";
        case ELM_NO_DATA: return "ELM_NO_DATA";
        case ELM_STOPPED: return "ELM_STOPPED";
        case ELM_TIMEOUT: return "ELM_TIMEOUT";
        case ELM_GETTING_MSG: return "ELM_GETTING_MSG";
        case ELM_MSG_RXD: return "ELM_MSG_RXD";
        case ELM_GENERAL_ERROR: return "ELM_GENERAL_ERROR";
        default: return String(id);
    }
}

String protocolNameFromId(char id) {
    switch (id) {
        case AUTOMATIC: return "AUTO";
        case SAE_J1850_PWM_41_KBAUD: return "SAE J1850 PWM 41k";
        case SAE_J1850_PWM_10_KBAUD: return "SAE J1850 PWM 10k";
        case ISO_9141_5_BAUD_INIT: return "ISO 9141-2";
        case ISO_14230_5_BAUD_INIT: return "ISO 14230-4 (5 baud)";
        case ISO_14230_FAST_INIT: return "ISO 14230-4 (fast)";
        case ISO_15765_11_BIT_500_KBAUD: return "ISO 15765-4 (11/500)";
        case ISO_15765_29_BIT_500_KBAUD: return "ISO 15765-4 (29/500)";
        case ISO_15765_11_BIT_250_KBAUD: return "ISO 15765-4 (11/250)";
        case ISO_15765_29_BIT_250_KBAUD: return "ISO 15765-4 (29/250)";
        case SAE_J1939_29_BIT_250_KBAUD: return "SAE J1939 (29/250)";
        case USER_1_CAN: return "USER1 CAN";
        case USER_2_CAN: return "USER2 CAN";
        default: return String(id);
    }
}

String fetchCurrentProtocol() {
  if (demo_mode) {
    return "demo";
  }

  int8_t state = myELM327.sendCommand_Blocking(DISP_CURRENT_PROTOCOL_NUM);
  if (state != ELM_SUCCESS) {
    myELM327.printError();
    return "unknown";
  }

  String resp = myELM327.payload ? String(myELM327.payload) : "";
  resp.trim();
  if (resp.startsWith("A") || resp.startsWith("a")) {
    resp.remove(0, 1); // drop automatic marker
  }
  char proto_id = resp.length() > 0 ? resp.charAt(0) : '?';
  String name = protocolNameFromId(proto_id);
  if (!resp.isEmpty()) {
    return name + " (" + resp + ")";
  }
  return name;
}

void setup_elm(){

    if(obd_use == 1){
        elm_ready = false;
        elm_connected = false;

        ELMSerial.begin(38400, SERIAL_8N1, 47, 48, false, 2000);

        if (!myELM327.begin(ELMSerial, false, 2000))
        {
            Serial.println("Couldn't connect to OBD scanner");
            
        }else{
            elm_ready = true;
            Serial.println("ELM began");
        }
    }else if(obd_use == 2){
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
int nodata_count=0;
void loop_elm(){

    // if(can_ready){
    //     if(!can_connected){
    //         if(OBD2.pidSupported(PIDS_SUPPORT_01_20)){
    //             can_connected = true;
    //         }else{
    //             Serial.println("connection dropped");
    //         }
    //     }else{

    //         switch(pid_num){
    //             case 0:
    //                 rpmn = OBD2.pidRead(pid_list[pid_num]);
    //                 if(rpmn == NAN){can_connected = false;}
    //                 pid_num++;
    //             break;
    //             case 1:
    //                 kmph = OBD2.pidRead(pid_list[pid_num]);
    //                 if(kmph == NAN){can_connected = false;}
    //                 pid_num++;
    //             break;
    //             case 2:
    //                 maf = OBD2.pidRead(pid_list[pid_num]);
    //                 if(maf == NAN){can_connected = false;}
    //                 pid_num++;
    //             break;
    //             case 3:
    //                 throttle_pos = OBD2.pidRead(pid_list[pid_num]);
    //                 if(throttle_pos == NAN){can_connected = false;}
    //                 pid_num++;
    //             break;
    //             case 4:
    //                 engine_temp = OBD2.pidRead(pid_list[pid_num]);
    //                 if(engine_temp == NAN){can_connected = false;}
    //                 pid_num++;
    //             break;
    //             default:
    //                 pid_num = 0;
    //             break;
    //         }

    //     }
        

    // }
    if(elm_ready && !elm_connected){
        rpmn = 0;
        kmph = 0;
        maf = 0;
        throttle_pos = 0;
        engine_temp = 0;
        battery_voltage = 0;
        uint32_t resp = myELM327.supportedPIDs_1_20();
        if (myELM327.nb_rx_state == ELM_SUCCESS){
            elm_connected = true;
        }
        else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
            myELM327.printError();
            elm_connected = false;
        } 
        else if(myELM327.nb_rx_state == ELM_NO_DATA){
            nodata_count++;
            if(nodata_count > 10){
                setup_elm();
                nodata_count = 0;
            }
            
        }
    }
    if(elm_ready && elm_connected){
        switch(pid_num){
                case 0:
                    responce_pre = myELM327.rpm();
                    if (myELM327.nb_rx_state == ELM_SUCCESS){
                        pid_num++;
                        rpmn = responce_pre;
                    }
                    else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
                        myELM327.printError();
                        pid_num++;
                    }
                    else if(myELM327.nb_rx_state == ELM_TIMEOUT){
                        elm_connected = false;
                    }
                    
                break;
                case 1:
                    responce_int = myELM327.kph();
                    if (myELM327.nb_rx_state == ELM_SUCCESS){
                        pid_num++;
                        kmph = responce_int;
                    }
                    else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
                        myELM327.printError();
                        pid_num++;
                    }
                break;
                case 2:
                    responce_pre = myELM327.mafRate();
                    if (myELM327.nb_rx_state == ELM_SUCCESS){
                        pid_num++;
                        maf = responce_pre;
                    }
                    else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
                        myELM327.printError();
                        pid_num++;
                    }
                break;
                case 3:
                    responce_pre = myELM327.throttle();
                    if (myELM327.nb_rx_state == ELM_SUCCESS){
                        pid_num++;
                        throttle_pos = responce_pre;
                    }
                    else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
                        myELM327.printError();
                        pid_num++;
                    }
                break;
                case 4:
                    responce_pre = myELM327.engineCoolantTemp();
                    if (myELM327.nb_rx_state == ELM_SUCCESS){
                        pid_num++;
                        engine_temp = responce_pre;
                    }
                    else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
                        myELM327.printError();
                        pid_num++;
                    }
                break;
                case 5:
                    responce_pre = myELM327.batteryVoltage();
                    if (myELM327.nb_rx_state == ELM_SUCCESS){
                        pid_num++;
                        battery_voltage = responce_pre;
                    }
                    else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
                        myELM327.printError();
                        pid_num++;
                    }
                break;
                case 6:
                    ELMprotocol = fetchCurrentProtocol();
                    pid_num++;
                break;
                default:
                    pid_num = 0;
                break;
            }
    }

}