//name: usbMaster
//Author: Michiel van der Coelen
//contact: Michiel.van.der.coelen at gmail.com
//date: wip
//tabsize: 2
//usb to I2C relay station
//To be used with the I2C servocontroller


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

#include <avr/pgmspace.h>   
#include "usbdrv.h"
#include "requests.h"       /* The custom request numbers */

#include "default.h" //some useful macros
#include "i2c_header.h" //defenitions for buffer lengths and addresses
#include "controller.h"

//#define SET(x,y) (x|=(1<<y))
//#define CLR(x,y) (x&=(~(1<<y)))
//#define CHK(x,y) (x&(1<<y)) 
//#define TOG(x,y) (x^=(1<<y))
//defined in default.h

//----------------------------------------------------------------------------
//Global Variables
//----------------------------------------------------------------------------
uint8_t TWS=0;
uint8_t r_index =0;
uint8_t recv[BUFLEN_SERVO_DATA];
uint8_t t_index=0;
uint8_t tran[BUFLEN_SERVO_DATA] = SERVO_DATA_EMPTY;
uint8_t mode = 0; 
//0: do nothing
//1: write buffer to I2C
//2: read from I2C
uint8_t reset =0;
#define NUMBER_OF_ADC_CHANNELS 2
uint8_t adc[NUMBER_OF_ADC_CHANNELS]; //buffer to store adc values
uint8_t mux = 0;
uint8_t ad_count;

void (*jump_to_boot)(void) = 0x0C00;

//controller data
struct CONTROLLER_DATA pscon;

//----------------------------------------------------------------------------
//Functions
//----------------------------------------------------------------------------
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
    register uint8_t i;
    static uchar dataBuffer[USB_MSG_LENGTH];
    usbRequest_t    *rq = (void *)data;
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR){
        switch(rq->bRequest){
        case CUSTOM_RQ_GET_DATA:
            dataBuffer[0] = mode;
            dataBuffer[1] = pscon.SS_Dpad;
            dataBuffer[2] = pscon.Shoulder_Shapes;
            dataBuffer[3] = pscon.previous_SS_Dpad;
            dataBuffer[4] = pscon.previous_Shoulder_Shapes;
            dataBuffer[5] = pscon.Rx;
            dataBuffer[6] = pscon.Ry;
            dataBuffer[7] = pscon.Lx;
            dataBuffer[8] = pscon.Ly;
            dataBuffer[9] = adc[0];
            dataBuffer[10] = adc[1];
            usbMsgPtr = dataBuffer;
            return USB_MSG_LENGTH; 
        case CUSTOM_RQ_GET_POS:
            if(mode==2){
                for(i=0;i<BUFLEN_SERVO_DATA;i++){
                    dataBuffer[i] = CUSTOM_RQ_GET_POS; 
                    //returning the buffer filled with the request
                    //indicates we're busy with reading from I2C
                }
            }else{
                for(i=0;i<BUFLEN_SERVO_DATA;i++){
                    dataBuffer[i] = recv[i];
                }
            }
            usbMsgPtr = dataBuffer;
            return USB_MSG_LENGTH;      
        case CUSTOM_RQ_LOAD_POS_FROM_I2C:
            if(mode){ //not ready for next I2C command
                for(i=0;i<BUFLEN_SERVO_DATA;i++){
                    dataBuffer[i] = CUSTOM_RQ_LOAD_POS_FROM_I2C;
                }
                dataBuffer[0] = mode;
                usbMsgPtr = dataBuffer;
                return USB_MSG_LENGTH;      
            }else{
                mode = 2;
                return 0;
            }
        case CUSTOM_RQ_RESET:
            reset = 1;
            return 0;
        case CUSTOM_RQ_SET_DATA:
            buffer_pos=0;
            bytes_remaining=rq->wLength.word;
            return USB_NO_MSG;
        }
    }
    return 0;   
}

uchar usbFunctionWrite(uchar * data, uchar len){
    uchar b;
    if(len>bytes_remaining) bytes_remaining = len;
    bytes_remaining -= len;
    for(b=0;b<len;b++){
        tran[buffer_pos++] = data[b];
    }
    if(bytes_remaining==0){
        mode=1;
        return 1;
    }   else return 0;
}
void I2Cmaster(){
  //Master
  if(CHK(TWCR,TWINT)){
//        TOG(PORTD,PD7);//green
        TWS = TW_STATUS;
    switch(TWS){
    case 0x10:  //start or rep start send, determine mode and send SLA
    case 0x08: 
            if(mode == 1){
                t_index =0;
                TWDR = SLA_W;
            } 
            if(mode ==2 ){
                r_index =0;
                TWDR = SLA_R;
            }
            TWACK;
            break;
//--------------- MT ---------------------------------------------------------
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
                //reset mode
                mode=0;
                TWSTART;
                break;
            }
    case 0x38: //what!?
        case 0x30: //data nacked, could be faulty buffer, could be dc, restart
      TWCR = 0;
      TWSTART;
      break;
//-------------------------MR ------------------------------------------------
        case 0x40:  //SLA_R acked, get ready for data
            TWACK;
            break;
    case 0x48: //SLA_R not acked, smack some sense into it
      TWSTART;
      break;
        case 0x50: //non-last data acked
            recv[r_index] = TWDR;
            r_index++;
            if(r_index < BUFLEN_SERVO_DATA){
                TWACK;
            } else {
                TWNACK;
                r_index =BUFLEN_SERVO_DATA;
            }
            break;
        case 0x58: //last data not acked, as it should be
            mode = TW_WRITE;
            TWSTART;
            mode =0;
            break;
            
//--------------------- bus error---------------------------------------------
    case 0x00:
      TWRESET;
      ;
      TWSTART;
      break;
    }
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
//----------------------------------------------------------------------------
//MAIN
//----------------------------------------------------------------------------
//usb initialization
int main() {
wdt_enable(WDTO_1S);
usbInit();
reconnectUSB();
//I2C
  TWBR = 10;
  TWCR = (1<<TWEN)|(1<<TWEA)|(1<<TWSTA);
//playstation controller
    INIT_CONTROLLER(&pscon, ANALOG);
//timer
    TCCR1B |= (1<<WGM12)|(1<<CS11);
    TIMSK1 |= (1<<OCIE1A);
    OCR1A = 31250;
//ADC
    ADMUX = (1<<REFS0)|(1<<ADLAR); //use AVcc and adjust left (fill ADCH)
    ADCSRA |= 0x07 | (1<<ADEN)|(1<<ADSC);//prescale clock  by 128    
sei();
//led
  SET(DDRD,PD7);
  SET(DDRB,PB0);
  SET(DDRB, PB0); //red

//----------------------------------------------------------------------------
//MAIN LOOP
//----------------------------------------------------------------------------
while(1){
//reset if necessary
    if(!reset) wdt_reset();

//poll the playstation controller   
   POLL_CONTROLLER(&pscon);
   if(!CHK(pscon.Shoulder_Shapes,TRIANGLE)){
       SET(PORTB,PB0);
       SET_ANALOG(&pscon);
   }else CLR(PORTB,PB0);
    DelaySmall;        

//only continue I2C transmission if there's a change to be send
    if(mode) I2Cmaster();
    if(mode == 2) SET(PORTD,PD7);
    else CLR(PORTD,PD7);

//handle usb requests
    usbPoll();

//store ADC result and 
//change channels if a conversion has been completed
	if(!CHK(ADCSRA,ADSC)) {
		adc[mux] = ADCH;
		mux++;
		if (mux == NUMBER_OF_ADC_CHANNELS) mux=0;
		ADMUX &= 0xF0; //clear mux
		ADMUX |= mux;
		SET(ADCSRA,ADSC);//start next conversion
	}
    

    }//main loop end
}//main end

ISR(TIMER1_COMPA_vect){
    //CLR(PORTD,PD7); //green
    //CLR(PORTB,PB0); //red
}
