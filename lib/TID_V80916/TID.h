#ifndef TID_H
#define TID_H
 
#include <Arduino.h> 

#define tid_delay 180

class TID {
public:
        TID(byte sda, byte scl, byte mrq);

		void display_message(String message, byte roll_speed); // roll_speed = 1 -> min roll_speed = 255 -> max
		void display_sub_message(String message, byte roll_speed,byte start,byte stop); //same as above, but adresses only a subset of the available chars
		// 0 is the leftmost char, 7 the rightmost
		void clear_message();
		void clear_sub_message(byte start,byte stop);
		void display_symbol(byte symbo);
		void clear_symbol(byte symbo);
		void clear_all_symbols();
		void bargraph(byte level);  //uses the symbols on top to create a bargraph effect, from left (minimum) to right


		/*
		message is shown and updated according to roll_speed
		if message it's longer than 8 char will roll
		
		symbols and their correspondant value:

		DolbyC	symbo = 1
		DolbyB	symbo = 2
		AS		symbo = 3
		Cr		symbo = 4
		stereo	symbo = 5
		CD		symbo = 6
		RDS		symbo = 7
		CPS		symbo = 8
		[]		symbo = 9
		TP		symbo = 10
		.		symbo = 11


		*/

		
		
		
private:
		void tid_byte(byte data);
		void tid_data(byte data);
		void tid_address();
		void start_tid();
		void stop_tid();
		void symbol_assign (byte symbo, byte RW);
		void cycle(); //sends a whole writing cycle 
//private:
		byte _sda;
		byte _scl;
		byte _mrq;
		unsigned long _timestamp;
		byte _incr;
		byte _ad;
		byte _SYMBOLS1;
		byte _SYMBOLS2;
		char _display[11];
		byte _old_bars;
};
#endif
