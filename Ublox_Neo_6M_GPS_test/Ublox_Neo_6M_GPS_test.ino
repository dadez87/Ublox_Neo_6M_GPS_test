// Created by Davide Cavaliere
// E-mail: dadez87@gmail.com
// Website: www.monocilindro.com
// 22 February 2016
// Arduino RX = pin 2 (connect to Ublox Neo 6M TX)
// Arduino TX = pin 4 (connect to Ublox Neo 6M RX)

#include "src/COMMmgr/COMMmgr.h" // Serial Ports Communication management
#include "src/GPSmgr/GPSmgr.h" // GPS manager
#include "src/SDmgr/SDmgr.h" // SD memory management

bool first_loop=true; // first time to execute the loop

void fuelino_yield(unsigned long){
  // do nothing
}

void setup() {
  COMM_begin(); // Initializes serial communication ports
}

void loop() {
  
  // This part is executed only at first cycle loop
  if (first_loop == true){
    GPS_initialize(); // Initializes GPS module communication (this function uses "delays"), this is why it is put here and not in "Setup"
    first_loop = false;
  }
  
  GPS_manager(); // GPS communication manager. Manages messages polling and reception
  COMM_receive_check(); // Checks data requests received from PC

  // If GPS packet has been received, and PC requested to continuously send the packet via Serial, once updated
  if ((GPS_SD_writing_request == true) && (COMM_GPS_send_req == true)) { 
    Serial.write(GPS_recv_buffer, GPS_SD_writing_request_size); // Write GPS raw data
  }

  SDmgr.log_SD_data(); // SD card data logging, as last, after checking all data
  delay(10);

}
