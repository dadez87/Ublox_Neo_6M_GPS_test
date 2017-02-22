#ifndef COMMmgr_cpp
#define COMMmgr_cpp

#include "COMMmgr.h"
#include "../GPSmgr/GPSmgr.h" // GPS manager

#define COMM_SERIAL_RECV_BYTES_NUM 8 // Buffer size for incoming data from SW and HW serial
#define COMM_SERIAL_STR_LEN_MAX 26 // Temporary buffer for string conversion (26 bytes should be enough for GPS config message)

uint8_t serial_inbyte_HW[COMM_SERIAL_RECV_BYTES_NUM]; // buffer for data received by Serial (HW seriale)
//uint8_t serial_inbyte_SW[COMM_SERIAL_RECV_BYTES_NUM]; // buffer for data received by Serial (SW seriale)
uint8_t serial_byte_cnt_HW = 0; // contatore di numero bytes ricevuti in seriale
//uint8_t serial_byte_cnt_SW = 0; // contatore di numero bytes ricevuti in seriale

bool COMM_GPS_send_req = false;

// Initializes communication ports
void COMM_begin(){
	SWseriale.begin(); // SWseriale pins (RX = 2, TX = 4)
	Serial.begin(57600); // Initializes PC serial (USB port)
}


// CALCULATED THE UBX CHECKSUM OF A CHAR ARRAY
uint16_t COMM_calculate_checksum(uint8_t* array, uint8_t array_start, uint8_t array_length) {
	uint8_t CK_A = 0;
	uint8_t CK_B = 0;
	uint8_t i;
	for (i = 0; i < array_length; i++) {
		CK_A = CK_A + array[array_start + i];
		CK_B = CK_B + CK_A;
	}
	return ((CK_A << 8) | (CK_B));
}


// Adds checksum at the end of the array, and sends it via Serial, on SWseriale
void COMM_Send_Char_Array(COMM_destination_port_enum send_port, uint8_t* array_data, uint8_t array_size, bool checksum_enable){
		
	if (checksum_enable == true){
		uint16_t CK_SUM = COMM_calculate_checksum(array_data, 0, array_size);
		array_data[array_size] = CK_SUM >> 8; // CK_A
		array_data[array_size+1] = CK_SUM & 0xFF; // CK_B
		array_size += 2; // add checksum bytes
	}

	if ((send_port == SW_SERIAL) || (send_port == ALL_SERIAL)) SWseriale.write(array_data, array_size); // Send complete string, including 2 bytes checksum
	if ((send_port == HW_SERIAL) || (send_port == ALL_SERIAL)) Serial.write(array_data, array_size); // Send complete string, including 2 bytes checksum
		
}


// Sends a string using SWseriale
void COMM_Send_String(COMM_destination_port_enum send_port, String input_string, bool end_line) {
	
	if (end_line == true){
		input_string+='\r';
		input_string+='\n';
	}
	uint8_t string_size = input_string.length();
	if (string_size > COMM_SERIAL_STR_LEN_MAX) string_size = COMM_SERIAL_STR_LEN_MAX;
	uint8_t serial_out_bytes[COMM_SERIAL_STR_LEN_MAX]; // stringa data fuori in output per il logger, via serial
	input_string.toCharArray((char*)serial_out_bytes, COMM_SERIAL_STR_LEN_MAX);
	COMM_Send_Char_Array(send_port, serial_out_bytes, string_size, false);
	
}


// Converts a character number (0-9) into an unsigned int number
uint8_t COMM_convert_char_array_to_num(uint8_t* input_array, uint8_t array_start, uint8_t array_len){
	uint16_t output_value_sum = 0;
	for (uint16_t i=0;i<array_len;i++){
		uint16_t cypher_tmp = input_array[array_start+i] - '0'; // transforms cypher into number
		if (cypher_tmp > 9) return 0x00; // not valid number
		for (uint8_t j=0; j<(array_len-i-1); j++){
			cypher_tmp*=10; // 100
		}
		output_value_sum += cypher_tmp;
	}
	if (output_value_sum > 255) return 0x00; // not valid number
	return (uint8_t)(output_value_sum & 0xFF); // converts the result in one byte
}


// Sends NACK
void COMM_send_nack(String nack_code, COMM_destination_port_enum send_port){
	COMM_Send_String(send_port, nack_code, true); // // NACK reply
}


// Sends back a service message to a tester
void COMM_send_service_message(String singleMessageString, COMM_destination_port_enum send_port){
		COMM_Send_String(send_port, singleMessageString, true); //send string to destination port
}


// Evaluates the command received from Serial Port
uint8_t COMM_evaluate_parameter_read_writing_request(uint8_t* data_array, uint8_t data_size, COMM_destination_port_enum recv_port){

	String singleMessageString = ""; // Reply message initialization
	
	// DATA command
	if ((data_array[0] == 'd') && (data_size >= 4) && (data_size <= 8)){ // DATA command (d x yy ...)
		if((data_array[1] == '0') && (data_array[2] == '0') && (data_array[3] == '0')){ // Write date and time (d000...)
			Serial.print(GPS_year);
			Serial.print(F("/"));
			Serial.print(GPS_month);
			Serial.print(F("/"));
			Serial.print(GPS_day);
			Serial.print(F("/,"));
			Serial.print(GPS_hour);
			Serial.print(F(":"));
			Serial.print(GPS_min);
			Serial.print(F(":"));
			Serial.print(GPS_sec);
			Serial.write('\n'); // new line
			Serial.write('\r'); // return
			return 1; // OK
		}
		else if((data_array[1] == '0') && (data_array[2] == '0') && (data_array[3] == '1')){ // Enable/Disable GPS data verbose on Serial (d001...)
			COMM_GPS_send_req = !COMM_GPS_send_req; // toggle status (false/true)
			Serial.print(F("GPSverbose:"));
			Serial.print(COMM_GPS_send_req);
			Serial.write('\n'); // new line
			Serial.write('\r'); // return
			return 1; // OK
		}
	}
	
	COMM_send_nack(F("99999999"), recv_port); // // NACK reply
	return 0; // Error
}


// Checks data coming from PC
void COMM_receive_check(){

	while (Serial.available()) { // carattere ricevuto
		uint8_t temp_char_read = Serial.read();
		if ((temp_char_read == '\n') || (temp_char_read == '\r')){ // End of command
			if ((serial_byte_cnt_HW != 0) && ((serial_inbyte_HW[0] == 'e') || (serial_inbyte_HW[0] == 'c') || (serial_inbyte_HW[0] == 'd'))) COMM_evaluate_parameter_read_writing_request(serial_inbyte_HW, serial_byte_cnt_HW, HW_SERIAL);
			serial_byte_cnt_HW = 0; // restart from zero
		}
		else{ // Store character into buffer
			if (serial_byte_cnt_HW == COMM_SERIAL_RECV_BYTES_NUM) serial_byte_cnt_HW = 0; // buffer is full
			serial_inbyte_HW[serial_byte_cnt_HW] = temp_char_read;
			serial_byte_cnt_HW++;
		}
	}
	
}

#endif