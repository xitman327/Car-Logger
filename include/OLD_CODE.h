// switch(pid_num){
        //         case 0:
        //             responce_pre = myELM327.rpm();
        //             if (myELM327.nb_rx_state == ELM_SUCCESS){
        //                 pid_num++;
        //                 rpmn = responce_pre;
        //             }
        //             else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
        //                 myELM327.printError();
        //                 pid_num++;
        //             }
        //             else if(myELM327.nb_rx_state == ELM_TIMEOUT){
        //                 elm_connected = false;
        //             }
                    
        //         break;
        //         case 1:
        //             responce_int = myELM327.kph();
        //             if (myELM327.nb_rx_state == ELM_SUCCESS){
        //                 pid_num++;
        //                 kmph = responce_int;
        //             }
        //             else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
        //                 myELM327.printError();
        //                 pid_num++;
        //             }
        //         break;
        //         case 2:
        //             responce_pre = myELM327.mafRate();
        //             if (myELM327.nb_rx_state == ELM_SUCCESS){
        //                 pid_num++;
        //                 maf = responce_pre;
        //             }
        //             else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
        //                 myELM327.printError();
        //                 pid_num++;
        //             }
        //         break;
        //         case 3:
        //             responce_pre = myELM327.throttle();
        //             if (myELM327.nb_rx_state == ELM_SUCCESS){
        //                 pid_num++;
        //                 throttle_pos = responce_pre;
        //             }
        //             else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
        //                 myELM327.printError();
        //                 pid_num++;
        //             }
        //         break;
        //         case 4:
        //             responce_pre = myELM327.engineCoolantTemp();
        //             if (myELM327.nb_rx_state == ELM_SUCCESS){
        //                 pid_num++;
        //                 engine_temp = responce_pre;
        //             }
        //             else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
        //                 myELM327.printError();
        //                 pid_num++;
        //             }
        //         break;
        //         case 5:
        //             responce_pre = myELM327.batteryVoltage();
        //             if (myELM327.nb_rx_state == ELM_SUCCESS){
        //                 pid_num++;
        //                 battery_voltage = responce_pre;
        //             }
        //             else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
        //                 myELM327.printError();
        //                 pid_num++;
        //             }
        //         break;
        //         case 6:
        //             ELMprotocol = fetchCurrentProtocol();
        //             pid_num++;
        //         break;
        //         case 7:
        //             uint8_t requested_pid_A = FUEL_PRESSURE;

        //             if(myELM327.isPidSupported(requested_pid_A)){
        //                responce_pre = myELM327.processPID(SERVICE_01, requested_pid_A, 1, obd_pid_len[requested_pid_A]);
        //                 if (myELM327.nb_rx_state == ELM_SUCCESS){
        //                     pid_num++;
        //                     pid_values[requested_pid_A] = responce_pre;
        //                 }
        //                 else if (myELM327.nb_rx_state != ELM_GETTING_MSG){
        //                     myELM327.printError();
        //                     pid_num++;
        //                 } 
        //             }else{
        //                 log_w("Requested PID %d Not Supported", requested_pid_A);
        //             }

        //         break;
        //         default:
        //             pid_num = 0;
        //         break;
        //     }