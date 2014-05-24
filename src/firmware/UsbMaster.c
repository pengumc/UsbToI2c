//name: usbMaster
//Author: Michiel van der Coelen
//contact: Michiel.van.der.coelen at gmail.com
//date: wip
//tabsize: 2
//usb to I2C relay station
//To be used with the I2C servocontroller

// MCU = atmega88
// F_CPU = 20000000 defined in makefile
// hfuse: DF
// lfuse: E6
// efuse: 0

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <avr/pgmspace.h>
#include "usbdrv.h"
#include "requests.h"  // The custom request numbers

#include "default.h" // some useful macros
#include "i2c_header.h" // defenitions for buffer lengths and addresses
#include "controller.h"

//#define SET(x,y) (x|=(1<<y))
//#define CLR(x,y) (x&=(~(1<<y)))
//#define CHK(x,y) (x&(1<<y))
//#define TOG(x,y) (x^=(1<<y))
//defined in default.h

#define USB_MSG_LENGTH BUFLEN_SERVO_DATA +1
#define NUMBER_OF_ADC_CHANNELS 2

// -------------------------------------------------------------Global variables
uint8_t TWS = 0;  // I2C status
uint8_t r_index =0;  // receive buffer inder
uint8_t recv[BUFLEN_SERVO_DATA];  // receive buffer
uint8_t t_index=0;  // transmit buffer index
uint8_t tran[BUFLEN_SERVO_DATA] = SERVO_DATA_EMPTY;  // transmit buffer
uint8_t mode = 0;
// internal state
// 0: do nothing
// 1: write buffer to I2C
// 2: read from I2C

uint8_t reset = 0;  // set to 1 to let the watchdog timer expire and reset.
uint8_t adc[NUMBER_OF_ADC_CHANNELS]; //buffer to store adc values
uint8_t mux = 0;  // mux for adc port
struct CONTROLLER_DATA pscon;  // playstation controller data

// for use in usbFunctionWrite
uint8_t bytes_remaining;
uint8_t buffer_pos;

// -----------------------------------------------------------USB HID Descriptor
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

// -------------------------------------------------------------usbFunctionSetup
usbMsgLen_t usbFunctionSetup(uchar data[8]) {
  register uint8_t i;
  static uchar dataBuffer[USB_MSG_LENGTH];
  usbRequest_t    *rq = (void *)data;
  if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR) {
    switch(rq->bRequest) {
      case CUSTOM_RQ_GET_DATA: {
        dataBuffer[0] = mode;
        dataBuffer[1] = pscon.SS_Dpad;
        dataBuffer[2] = pscon.Shoulder_Shapes;
        dataBuffer[3] = adc[0];
        dataBuffer[4] = adc[1];
        if (HAS_VALID_ANALOG_DATA(&pscon)) {
          dataBuffer[5] = pscon.Rx;
          dataBuffer[6] = pscon.Ry;
          dataBuffer[7] = pscon.Lx;
          dataBuffer[8] = pscon.Ly;
        } else {
          dataBuffer[5] = 128;
          dataBuffer[6] = 128;
          dataBuffer[7] = 128;
          dataBuffer[8] = 128;
        }
        usbMsgPtr = dataBuffer;
        return USB_MSG_LENGTH;
      }
      case CUSTOM_RQ_GET_POS: {
        if (mode==2) {
          for(i = 0; i < BUFLEN_SERVO_DATA; ++i) {
            dataBuffer[i] = CUSTOM_RQ_GET_POS;
            // returning the buffer filled with the request
            // indicates we're busy with reading from I2C
          }
        } else {
          for(i = 0; i < BUFLEN_SERVO_DATA; ++i) {
            dataBuffer[i] = recv[i];
          }
        }
        usbMsgPtr = dataBuffer;
        return USB_MSG_LENGTH;
      }
      case CUSTOM_RQ_LOAD_POS_FROM_I2C: {
        if (mode) { // not ready for next I2C command
          for(i = 0; i < BUFLEN_SERVO_DATA; ++i) {
            dataBuffer[i] = CUSTOM_RQ_LOAD_POS_FROM_I2C;
          }
          dataBuffer[0] = mode;
          usbMsgPtr = dataBuffer;
          return USB_MSG_LENGTH;
        } else {
          mode = 2;
          return 0;
        }
      }
      case CUSTOM_RQ_RESET: {
        reset = 1;
        return 0;
      }
      case CUSTOM_RQ_SET_DATA: {
        buffer_pos = 0;
        bytes_remaining = rq->wLength.word;
        return USB_NO_MSG;
      }
    }
  } else if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
    if (rq->bRequest == USBRQ_HID_SET_REPORT) {
      buffer_pos = 0;
      bytes_remaining = rq->wLength.word;
      return USB_NO_MSG;
    }
  }
  return 0;
}

// -------------------------------------------------------------usbFunctionWrite
uchar usbFunctionWrite(uchar * data, uchar len) {
  // first pass
  //  13 bytes remaining 
  //  len = 8 (7 useful)
  // 2nd:
  //  5 remaining
  // len = 5 (5 useful)
  // buffer_pos = 7, skip 
  uchar store_start = 0;
  uchar store_end = len;
  if (buffer_pos == 0) {  // if we're at the first byte
    switch (data[0]) {
      case CUSTOM_RQ_RESET: {
        reset = 1;
        return 1;  // no need to read the rest of the data
      }
      case CUSTOM_RQ_GET_DATA: {
        // TODO(michiel): implement
        return 1;
      }
      case CUSTOM_RQ_GET_POS: { 
        // TODO(michiel): implement
        return 1;
      }
      case CUSTOM_RQ_LOAD_POS_FROM_I2C: {
        // TODO(michiel): implement
        return 1;
      }
      case CUSTOM_RQ_SET_DATA: {
        ++store_start; // 1st: 1
        break;
      }
      default: {
        return 1;
      }
    }
  }
  uchar b;
  // 1st: if 8 - 1 > 12
  // 2nd: if 5 - 0 > 5
  if (len - store_start > bytes_remaining) {
    bytes_remaining = len - store_start;
    // 1st: storing data[1] ... data[7] at 0...6
    // 2nd: storing data[0] ... data[4] at 7...11
  }
  for(b = store_start; b < store_end; ++b) {
    tran[buffer_pos++] = data[b];
  }
  // 1st: buffer_pos = 7
  // 2nd: buffer_pos = 12
  bytes_remaining -= len;
  // 1st: remaining = 13 - 8 = 5
  // 2nd: remaining = 5 - 5 = 0
  if (bytes_remaining == 0 || buffer_pos >= BUFLEN_SERVO_DATA) {
    mode = 1;
    return 1;
  } else {
    // more bytes to read
    return 0;
  }
}

// --------------------------------------------------------------------I2Cmaster
void I2Cmaster() {
  if (CHK(TWCR,TWINT)) {
    TWS = TW_STATUS;
    switch (TWS) {
      case 0x10:  // start or rep start send, determine mode and send SLA
      case 0x08: {
        if(mode == 1){
          t_index =0;
          TWDR = SLA_W;
        } else if(mode ==2 ) {
          r_index =0;
          TWDR = SLA_R;
        }
        TWACK;
        break;
      }
// ---------------- master transmitting ----------------------------------------
      case 0x18: {  // SLA_W acked
        // load first data
        TWDR = tran[0];
        t_index=1;
        TWACK;
        break;
      }
      case 0x20: {  // SLA_W not acked for some reason (dc?), try again
        TWCR =0;
        TWSTART;
        break;
      }
      case 0x28: {  // data acked
        if (t_index < BUFLEN_SERVO_DATA) {
          TWDR =  tran[t_index];
          ++t_index;
          TWACK;
          break;
        } else {
          // reset mode
          mode=0;
          TWSTART;
          break;
        }
      }
      case 0x38: // what!?
      case 0x30: {  // data nacked, could be faulty buffer, could be dc, restart
        TWCR = 0;
        TWSTART;
        break;
      }
// --------------------------master receiving ----------------------------------
      case 0x40: {  // SLA_R acked, get ready for data
        TWACK;
        break;
      }
      case 0x48: {  // SLA_R not acked, smack some sense into it
        TWSTART;
        break;
      }
      case 0x50: {  // non-last data acked
        recv[r_index] = TWDR;
        ++r_index;
        if (r_index < BUFLEN_SERVO_DATA) {
          TWACK;
        } else {
          TWNACK;
          r_index =BUFLEN_SERVO_DATA;
        }
        break;
      }
      case 0x58: {  // last data not acked, as it should be
        mode = TW_WRITE;
        TWSTART;
        mode =0;
        break;
      }
// ---------------------- bus error---------------------------------------------
      case 0x00: {
        TWRESET;
        ;
        TWSTART;
        break;
      }
    }
  }
}

// -----------------------------------------------------------------reconnectUSB
void reconnectUSB() {
  usbDeviceDisconnect();
  register uint8_t i=0;
  while(--i) {
    wdt_reset();
    _delay_ms(1);
  }
  usbDeviceConnect();
}

// -------------------------------------------------------------------------main
int main() {
// usb initialization
  wdt_enable(WDTO_1S);
  usbInit();
  reconnectUSB();
  // I2C
  TWBR = 10;
  TWCR = (1<<TWEN)|(1<<TWEA)|(1<<TWSTA);
  // playstation controller
  INIT_CONTROLLER(&pscon, ANALOG);
  // timer
  TCCR1B |= (1<<WGM12)|(1<<CS11);
  TIMSK1 |= (1<<OCIE1A);
  OCR1A = 31250;
  // ADC
  ADMUX = 0x20;//(0<<REFS0)|(1<<ADLAR); //use AREF and adjust left (fill ADCH)
  ADCSRA |= 0x07 | (1<<ADEN)|(1<<ADSC);//prescale clock  by 128
  // led
  SET(DDRD,PD7);
  SET(DDRB,PB0);
  SET(DDRB, PB0); //red

  sei();
  while(1) {
    // reset if necessary
    if(!reset) wdt_reset();
    // poll the playstation controller
    POLL_CONTROLLER(&pscon);
    if (!CHK(pscon.Shoulder_Shapes,TRIANGLE)) {
    SET(PORTB,PB0);
    SET_ANALOG(&pscon);
    } else CLR(PORTB,PB0);
    DelaySmall;

    // only continue I2C transmission if there's a change to be send
    if (mode) I2Cmaster();
    if (mode == 2) SET(PORTD,PD7);
    else CLR(PORTD,PD7);

    // handle usb requests
    usbPoll();

    // store ADC result and
    // change channels if a conversion has been completed
    if (!CHK(ADCSRA,ADSC)) {
      adc[mux] = ADCH;
      mux++;
      if (mux == NUMBER_OF_ADC_CHANNELS) mux=0;
      ADMUX &= 0xF0; //clear mux
      ADMUX |= mux;
      SET(ADCSRA,ADSC);//start next conversion
    }
  }  // main loop end
} // main end

ISR(TIMER1_COMPA_vect){
  //CLR(PORTD,PD7); //green
  //CLR(PORTB,PB0); //red
}
