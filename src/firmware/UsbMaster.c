//name: slave2
//Author: Michiel van der Coelen
//contact: Michiel.van.der.coelen at gmail.com
//date: 2011-04-01
//tabsize: 2


//MCU = atmega88
//F_CPU = 20000000 defined in makefile
//hfuse: DF 
//lfuse: E6
//efuse: 0
//bootsector starts at 0xC00 (size= 2kb)
//  bootloader code not included in this file
#include <avr/io.h> 
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h> 

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "requests.h"       /* The custom request numbers */


#include "i2c_header.h"

#define SET(x,y) (x|=(1<<y))
#define CLR(x,y) (x&=(~(1<<y)))
#define CHK(x,y) (x&(1<<y)) 
#define TOG(x,y) (x^=(1<<y))


//-----------------------------------------------------------------------------
//Global Variables
//-----------------------------------------------------------------------------
uint8_t TWS=0;
uint8_t r_index =0;
uint8_t recv[1];
uint8_t t_index=0;
uint8_t tran[BUFLEN_SERVO_DATA] = SERVO_DATA_EMPTY;
uint8_t mode = TW_WRITE;
uint8_t new_buffer =1;
uint8_t reset =0;
uint8_t data_acked;
#define NUMBER_OF_ADC_CHANNELS 2
uint8_t adc[NUMBER_OF_ADC_CHANNELS]; //buffer to store adc values
uint8_t mux = 0;
uint8_t ad_count;

void (*jump_to_boot)(void) = 0x0C00;

//-----------------------------------------------------------------------------
//Functions
//-----------------------------------------------------------------------------
PROGMEM char usbHidReportDescriptor[22] = {    /* USB report descriptor */
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x09, 0x00,                    //   USAGE (Undefined)
    0xb2, 0x02, 0x01,              //   FEATURE (Data,Var,Abs,Buf)
    0xc0                           // END_COLLECTION
};
uint8_t bytes_remaining;
uint8_t buffer_pos;
#define USB_MSG_LENGTH BUFLEN_SERVO_DATA +1
usbMsgLen_t usbFunctionSetup(uchar data[8]){
	static uchar dataBuffer[USB_MSG_LENGTH];     /* buffer must stay valid when usbFunctionSetup returns */
	usbRequest_t    *rq = (void *)data;
	if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR){
		switch(rq->bRequest){
		case CUSTOM_RQ_GET_DATA:
			dataBuffer[0] = data_acked;
      dataBuffer[1] = adc[0];
			dataBuffer[2] = adc[1];
			dataBuffer[3] = 0;
			dataBuffer[4] = 0;
			dataBuffer[5] = TWS;
			dataBuffer[6] = tran[0];
			usbMsgPtr = dataBuffer;         /* tell the driver which data to return */
			return USB_MSG_LENGTH;                       /* tell the driver to send 1 byte */
		case CUSTOM_RQ_RESET:
			reset = 1;
			return 0;
		case CUSTOM_RQ_SET_DATA:
			buffer_pos=0;
			bytes_remaining=rq->wLength.word;
			return USB_NO_MSG;
		}
	}
	return 0;   /* default for not implemented requests: return no data back to host */
}

uchar usbFunctionWrite(uchar * data, uchar len){
	uchar b;
	if(len>bytes_remaining) bytes_remaining = len;
	bytes_remaining -= len;
	for(b=0;b<len;b++){
		tran[buffer_pos++] = data[b];
	}
	if(bytes_remaining==0){
		new_buffer=1;
		return 1;
	}	else return 0;
}
void I2Cmaster(){
  //Master
  if(CHK(TWCR,TWINT)){
		TOG(PORTD,PD7);
		TWS = TW_STATUS;
    switch(TWS){
    case 0x10:  //start or rep start send, determine mode and send SLA
    case 0x08: 
			if(mode == TW_WRITE){
				t_index =0;
				TWDR = SLA_W;
			} else{
				r_index =0;
				TWDR = SLA_R;
			}
			TWACK;
			break;
//--------------- MT ----------------------------------------------------------
		case 0x18: // SLA_W acked
			//load first data
			TWDR = tran[0];
			t_index=1;
			TWACK;
			break;
		case 0x20: //SLA_W not acked for some reason (dc?), try again
			TWCR =0;
      TWSTART;
			break;
		case 0x28: //data acked
			if(t_index < BUFLEN_SERVO_DATA){
				TWDR =  tran[t_index];
				t_index++;
				TWACK;
				break;
			} else {
			//don't switch to reciever mode
				//mode = TW_READ;
				new_buffer=0;
				TWSTART;
				break;
			}
    case 0x38: //what!?
		case 0x30: //data nacked, could be faulty buffer, could be dc, restart
      TWCR = 0;
      TWSTART;
      break;
//-------------------------MR -------------------------------------------------
		case 0x40:	//SLA_R acked, get ready for data
			TWACK;
			break;
    case 0x48: //SLA_R not acked, smack some sense into it
      TWSTART;
      break;
		case 0x50: //non-last data acked
			recv[r_index] = TWDR;
			r_index++;
			if(r_index < BUFLEN_ACC_DATA){
				TWACK;
			} else {
				TWNACK;
				r_index =BUFLEN_ACC_DATA;
			}
			break;
		case 0x58: //last data not acked, as it should be
			mode = TW_WRITE;
			TWSTART;
			break;
			
//--------------------- bus error----------------------------------------------
    case 0x00:
      TWRESET;
      ;
      TWSTART;
      break;
    }
  }
}

void setState(uint8_t state){
  /*
  0 = off
  1 = green
  2 = red
  3 = green+red
  */
  switch(state){
    case 0:
      CLR(PORTD,PD7);
      CLR(PORTB,PB0);
      break;
    case 1:
      SET(PORTD,PD7);
      CLR(PORTB,PB0);
      break;
    case 2:
      CLR(PORTD,PD7);
      SET(PORTB,PB0);
      break;
    case 3:
      SET(PORTD,PD7);
      SET(PORTB,PB0);
      break;
  }
}

void reconnectUSB(){
usbDeviceDisconnect();
register uint8_t i=0;
while(--i){
	wdt_reset();
	_delay_ms(1);
}
usbDeviceConnect();

}
//-----------------------------------------------------------------------------
//MAIN
//-----------------------------------------------------------------------------
//usb initialization
int main() {
wdt_enable(WDTO_1S);
usbInit();
reconnectUSB();
//I2C
  TWBR = 10;
  TWCR = (1<<TWEN)|(1<<TWEA)|(1<<TWSTA);
//timer
	TCCR1B |= (1<<WGM12)|(1<<CS11);
	TIMSK1 |= (1<<OCIE1A);
	OCR1A = 31250;
sei();
//led
  SET(DDRD,PD7);
  SET(DDRB,PB0);
  setState(1);

//-----------------------------------------------------------------------------
//MAIN LOOP
//-----------------------------------------------------------------------------
while(1){
	//reset if necessary
	if(!reset) wdt_reset();
	
  //only continue I2C transmission if there's a change to be send
	if(new_buffer) I2Cmaster();
	
	//handle usb requests
	usbPoll();
	
	//jump to bootloader when D6 is pulled high
  if(CHK(PIND,PD6)){
		TCCR1B =0;
		TIMSK1 =0;
		jump_to_boot();
	}

	}//main loop end
}//main end

ISR(TIMER1_COMPA_vect){
  CLR(PORTD,PD7);
	CLR(PORTB,PB0);
}