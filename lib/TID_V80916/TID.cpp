#include "TID.h"


TID::TID(byte sda, byte scl, byte mrq) {
	
	_sda = sda;
	_mrq = mrq;
	_scl = scl;
	_timestamp = millis();
	_incr = 0;
	_ad = 0;
	_SYMBOLS1 = 0;
	_SYMBOLS2 = 0;
	_old_bars = 0;
	for (int i=0; i<10; i++) {
		_display[i] = ' ';
	};


	
	pinMode(_mrq, OUTPUT); 
	pinMode(_sda, OUTPUT); 
	pinMode(_scl, OUTPUT); 
	digitalWrite(_mrq, HIGH);
	digitalWrite(_sda, HIGH);
	digitalWrite(_scl, HIGH);
}


void TID::tid_byte(byte data) {
  pinMode(_scl, OUTPUT);
   for (uint8_t m = 0X80; m != 0; m >>= 1) { //2 lines from
    digitalWrite(_sda, m & data);              //William Greiman's I2cMaster Library
    delayMicroseconds(10);
    digitalWrite(_scl, HIGH);
    delayMicroseconds(tid_delay);
    digitalWrite(_scl, LOW);
    delayMicroseconds(tid_delay);
    };
  delayMicroseconds(300);
  pinMode(_sda,INPUT);     //3
  delayMicroseconds(tid_delay);
  pinMode(_scl,INPUT);     //4
  delayMicroseconds(100);
  while(digitalRead(_scl)==0);   //6
  delayMicroseconds(300);
  pinMode(_scl,OUTPUT);
  digitalWrite(_scl, LOW);   //7
  while(digitalRead(_sda)==0);   //8
  pinMode(_sda,OUTPUT);
  }

void TID::tid_data(byte data) {      //bytes of data
   byte val=data;            //this parity check code
   byte pari;                //comes from
   val = val ^ (val >> 4);   //Claas Anders "CaScAdE" Rathje http://www.mylifesucks.de/oss/c-tid/ 
   val = val ^ (val >> 2);   //from a post in the
   val = val ^ (val >> 1);   //mikrocontroller.net forum
   val &= 0x01;              //http://www.mikrocontroller.net/topic/19516
   pari = !val;              
   data = (data<<1)|pari;
   tid_byte(data);
   }

void TID::tid_address() {  
  //tid_byte(0x94);		//the address for the 8 char display is embedded here, for the 10 char you have to use 0x9A
  tid_byte(0x9B);		//the address for the 8 char display is embedded here, for the 10 char you have to use 0x9A 9B?
  digitalWrite(_mrq, LOW); 
  }

void TID::start_tid() {
  digitalWrite(_mrq, LOW);
  delayMicroseconds(2000);
  digitalWrite(_mrq, HIGH);
  delayMicroseconds(700);
  pinMode(_sda,OUTPUT);
  pinMode(_scl,OUTPUT);
  digitalWrite(_sda, LOW);
  delayMicroseconds(500);
  digitalWrite(_scl, LOW);
  delayMicroseconds(200);
  }

void TID::stop_tid() {
  digitalWrite(_sda, LOW);
  delayMicroseconds(1000);
  digitalWrite(_mrq, HIGH);
  delayMicroseconds(500);
  digitalWrite(_scl, HIGH);
  delayMicroseconds(200);
  digitalWrite(_sda, HIGH);
  delayMicroseconds(200);
  }

void TID::display_message(String message, byte roll_speed) {
	
	display_sub_message(message,roll_speed,0,9);
}

void TID::clear_message() {
	clear_sub_message(0,9);
	}


void TID::symbol_assign (byte symbo, byte RW) {
	if (symbo == 9) bitWrite(_SYMBOLS1,0,RW);
	if (symbo == 3) bitWrite(_SYMBOLS1,1,RW);
	if (symbo == 5) bitWrite(_SYMBOLS1,3,RW);
	if (symbo == 10) bitWrite(_SYMBOLS1,4,RW);
	if (symbo == 7) bitWrite(_SYMBOLS1,5,RW);
	if (symbo == 11) bitWrite(_SYMBOLS1,6,RW);

	if (symbo == 8) bitWrite(_SYMBOLS2,2,RW);
	if (symbo == 4) bitWrite(_SYMBOLS2,3,RW);
	if (symbo == 2) bitWrite(_SYMBOLS2,4,RW);
	if (symbo == 1) bitWrite(_SYMBOLS2,5,RW);
	if (symbo == 6) bitWrite(_SYMBOLS2,6,RW);
}
	


void TID::display_symbol(byte symbo) {
	symbol_assign(symbo,1);
	cycle();
}

void TID::clear_symbol(byte symbo) {
	symbol_assign(symbo,0);
	cycle();
}

void TID::clear_all_symbols() {
	for (int i=1; i <= 10; i++) {
		symbol_assign(i,0);
	};
	cycle();
}


void TID::display_sub_message(String message, byte roll_speed,byte start,byte stop) {
	
	int upd_delay = 1000 / roll_speed;
	if ((millis() - _timestamp) > upd_delay) {
			
			for (int i=start; i<=min(stop,(start-1+message.length())) ; i++) {
				_ad = i + (_incr * (message.length()>(stop-start+1))) - start;
				if (_ad >= message.length()) {
					_ad = _ad-message.length();
				};
				_display[i] = message.charAt(_ad);
			};
			if (message.length()>(stop-start+1)) {
				_incr++;
			};
			if (_incr >= message.length()) {
				_incr=0;
			};
		
		cycle();
		_timestamp = millis();
	};

}

void TID::clear_sub_message(byte start,byte stop) {

	for (int i=start; i<=stop; i++) {
		_display[i]=0;
	};
	cycle();
}


void TID::cycle(){
	start_tid();
	tid_address();
	tid_data(_SYMBOLS1);
	tid_data(_SYMBOLS2);
	tid_data(_SYMBOLS2);
	for (int i=0; i<10; i++) {
		tid_data(_display[i]);
	};
	stop_tid();
}

void TID::bargraph(byte level) {
	int bars = int(level/25.5)+1;
	if (bars != _old_bars) {
		
		if (bars==11) bars = 10;
		for (int i=1; i <= 10; i++) {
			symbol_assign(i,(i <= bars));
		};
		cycle();
		_old_bars = bars;
	};
}





