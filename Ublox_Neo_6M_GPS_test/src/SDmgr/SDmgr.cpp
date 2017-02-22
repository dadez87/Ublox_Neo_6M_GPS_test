#ifndef SDmgr_cpp
#define SDmgr_cpp

#include <SDFatYield.h> // SD FAT management (modified SDFat library)
#include "../GPSmgr/GPSmgr.h" // GPS manager. To have access to GPS measurements
#include "SDmgr.h"

#define SD_MODULE_PRESENT 1 // Micro SD card module presence (Catalex Micro SD Card)
#define SD_CS_PIN_NUM A1 // CS pin for SD card (SPI) - Slave Select
#define MAX_WRITE_ERRORS 10 // Maximum consecutive write errors that will cause a re-init. 
#define MAX_DELAY_POWERON 20 // This is used as time delay at power ON before performing an "init"

SdFat SD; // needed to manage SD card

// call back for file timestamps
void dateTime(uint16_t* date, uint16_t* time) {

  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(GPS_year, GPS_month, GPS_day);

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(GPS_hour, GPS_min, GPS_sec);
}


SDmgr_class::SDmgr_class(){
	
	SD_init_OK = false; // NG
	packet_cnt = 0; // packet counter init value
	write_errors_cnt = 0;
	
}


bool SDmgr_class::begin(){
	
#if SD_MODULE_PRESENT

	// Set date time callback function
	SdFile::dateTimeCallback(dateTime);
	
	// SD INITIALIZATION
	if (!SD.begin(SD_CS_PIN_NUM, SPI_HALF_SPEED)) { // CS pin for SD card is pin #10
		SD_init_OK = false; // NG
		return false;
	}
	SD_init_OK = true; // OK

	// Read file name from EEPROM and stores file name as string (8.3 format)
	uint16_t file_number = 89; //EEPROM_SD_file_num_rw();
	file_name="fln";
	if (file_number < 10) file_name+='0';
	if (file_number < 100) file_name+='0';
	if (file_number < 1000) file_name+='0';
	if (file_number < 10000) file_name+='0';
	file_name+=(unsigned int)file_number;
	file_name+=".log";
	
#endif
	
	return true; // Init successful
	
}


bool SDmgr_class::log_SD_data(){

	// Performs SD Initialization, in case it is necessary (in case it was not possible before, or it is the first time to call this function)
	if (SD_init_OK == false) {
		write_errors_cnt++; // this is for delay purposes, to avoid continuous "begin()" calls
		if (write_errors_cnt >= MAX_DELAY_POWERON) {
			begin(); // Initializes SD memory
			write_errors_cnt = 0; // reset counter
		} 
	}

	bool temp_reply = false; // temporary response

	// Preparation of Engine Data array (this is needed also for Serial communication - service protocol) - this is dummy, do not remove
	uint8_t i=0; // packet bytes counter
	SD_writing_buffer[i++] = 'd';
	SD_writing_buffer[i++] = packet_cnt; // packet counter
	packet_cnt++; // increase packet counter
	unsigned long time_stamp_temp = millis(); // read time stamp
	SD_writing_buffer[i++] = (uint8_t)(time_stamp_temp & 0xff); // LSB
    SD_writing_buffer[i++] = (uint8_t)((time_stamp_temp >> 8) & 0xff);
    SD_writing_buffer[i++] = (uint8_t)((time_stamp_temp >> 16) & 0xff);
    SD_writing_buffer[i++] = (uint8_t)((time_stamp_temp >> 24) & 0xff); // MSB
	//buffer_busy = 1; // Locks the buffer access (semaphore)
	SD_writing_buffer[i++] = (uint8_t)(0 & 0xff); // LSB
	SD_writing_buffer[i++] = (uint8_t)((0 >> 8) & 0xff); // MSB
    SD_writing_buffer[i++] = (uint8_t)(0 & 0xff); // LSB
	SD_writing_buffer[i++] = (uint8_t)((0 >> 8) & 0xff); // MSB
    SD_writing_buffer[i++] = (uint8_t)(0 & 0xff); // LSB
    SD_writing_buffer[i++] = (uint8_t)((0 >> 8) & 0xff); // MSB
    SD_writing_buffer[i++] = (uint8_t)(0 & 0xff); // LSB
    SD_writing_buffer[i++] = (uint8_t)((0 >> 8) & 0xff); // MSB
	SD_writing_buffer[i++] = (uint8_t)(0 & 0xff); // LSB
    SD_writing_buffer[i++] = (uint8_t)((0 >> 8) & 0xff); // MSB
	SD_writing_buffer[i++] = (uint8_t)(0 & 0xff); // LSB
	SD_writing_buffer[i++] = (uint8_t)((0 >> 8) & 0xff); // MSB
	//buffer_busy = 0; // Opens the buffer again
	SD_writing_buffer[i++] = (uint8_t)0; // LSB
	SD_writing_buffer[i++] = (uint8_t)0; // LSB
	SD_writing_buffer[i++] = 0; // digital inputs status
    uint16_t CK_SUM = COMM_calculate_checksum(SD_writing_buffer, 0, i);
    SD_writing_buffer[i++] = (uint8_t)(CK_SUM >> 8);
    SD_writing_buffer[i++] = (uint8_t)(CK_SUM & 0xFF);

#if SD_MODULE_PRESENT
	// Writing the data on SD memory
	if (SD_init_OK == true){ // SD card initialized properly
	
		if (1){ // logs only if Battery is ON (or if battery check inhibit config flag is active)
			File dataFile = SD.open(file_name, FILE_WRITE); // Opens the file only if there is battery voltage
			if (dataFile){ // log only when the file is open, and when battery is connected (to avoid memory corruption)
				
				uint8_t data_bytes_written; // Data bytes effectively written on the SD card
				bool error_status = false; // Error status
				
				// Engine info
				// GPS info or Lambda info
				if (GPS_SD_writing_request == true) {
					data_bytes_written = dataFile.write(SD_writing_buffer, SD_WRITE_BUFFER_SIZE); // Writes engine related info (calculated above)
					if (data_bytes_written != SD_WRITE_BUFFER_SIZE) error_status = true; // not all bytes were written
					data_bytes_written = dataFile.write(GPS_recv_buffer, GPS_SD_writing_request_size); // Write GPS data
					if (data_bytes_written != GPS_SD_writing_request_size) error_status = true; // not all bytes were written
					//GPS_SD_writing_request = false; // reset flag (necessary to re-enable filling the buffer from GPS module) --> Moved at the end (just in case SD is disabled)
				}

				// Error status check
				if (error_status == true) { // Could not write completely all data
					write_errors_cnt++; // Increase error counter
				}else{ // Completely written all data
					write_errors_cnt = 0; // No error
				}
				
				if (1) { // Data written only in case battery voltage is present, or if bypass flag is active
					dataFile.close(); // Really write the data on the SD, only if battery voltage is present
					if (error_status == false) { // No error found
						temp_reply = true; // Data log considered completed successfully
					}
				}
				
			}else{ // Could not open the file
				write_errors_cnt++; // Increase error counter
			}
		}
		
		// Errors max check
		if (write_errors_cnt >= MAX_WRITE_ERRORS) {
			SD_init_OK = false; // This will cause the SD card to be re-initialized at next function call
			write_errors_cnt = 0; // Reset error counter
		}
		
	}
#endif
	
	GPS_SD_writing_request = false; // reset flag (necessary to re-enable filling the buffer from GPS module)
	return temp_reply;
	
}

SDmgr_class SDmgr;

#endif