# Ublox_Neo_6M_GPS_test
This project interfaces Arduino Uno/Nano with Ublox Neo 6M GPS module, using Serial communication protocol

// Created by Davide Cavaliere

// E-mail: dadez87@gmail.com

// Website: www.monocilindro.com (please check in "Electronics" category)

// 22 February 2016

Arduino RX = pin 2 (connect to Ublox Neo 6M TX)

Arduino TX = pin 4 (connect to Ublox Neo 6M RX)

After compiling the sketch and flashing it on Arduino Uno/Nano, you can test the communication by sending the command "d000" followed by CR or NL, or both characters. In case the GPS date (UTC) could be received properly from the Neo 6M, it should be shown correctly on the screen. Please notice that some time is required to the GPS, to connect to satellites (some minutes, in case you are into a building). The date is received using UBX TIMEUTC packet polling. For more info related to the communication protocol implemented in Ublox Neo 6M, please check the official website, and the data-sheet (u-blox6 Receiver Description Protocol Specification).
The same request ("d000") can be also performed using RealTerm. The serial communication speed to be set between Arduino and PC is 57600 baud.
In addition to that, RealTerm allows to see the raw packets coming from the Ublox Neo 6M. First of all, set the visualization mode to "Hex [space]" and then send the command "d001": this command enables/disables the "verbose mode". If such mode is active, Arduino Nano is forwarding all data received from the GPS module, to the PC. The packets can be stopped by sending once again "d001" (this will toggle OFF the "verbose mode"). As visible below, the UBX POSLLH packet starts with HEX characters 0xB5 0x62 0x01 0x02.
The same data is also logged on the SD card, in case you connected a Catalex Micro SD card module (and set the "#define" fields properly). In the SD card, there should be a binary file called "flnXXXXX.log" (in which XXXXX is a number between 00000 and 99999, which can be manually defined inside the file "SDmgr.cpp"). This binary raw file can be converted into a CSV file (ASCII) using Fuelino Log File converter, which is available on Monocilindro website ("Fuelino" category). The tool converts the GPS packets binary data (example: POSLLH, TIMEGPS, TIMEUTC) into a readable format, that can be read using Excel or Notepad.
First of all Arduino keeps polling the GPS module using UBX TIMEUTC queries. Once the data received is reliable ("valid" flag byte = 3) for more than 5 times, the queries are switched to USB POSLLH (longitude, latitude, height). Therefore, at first Arduino checks the date and time, and once it is sure about them, it starts asking for the latitude, longitude, and height. 
The column on the left side of the CSV file, before "iTOW[s]" are not useful in this case, because they are filled with fake data (Fuelino is not acquiring engine data, in this example). About the latitude, longitude and height information, in order to make sure that these info are reliable, you should have a look at the values of hAcc (horizontal accuracy) and vAcc (vertical accuracy). If their value is reasonable, for example less than 100m, you can consider the position values quite reliable.
