//Author: Michiel van der Coelen
//date: 25-08-2010
//These functions will allow an atmega8/88 to interact with a playstation1/2 controller 
//SPI frequency of 125 KHz works, 250 KHz does not.
/*
todo:
-allow switching between digital and analog red mode
-speed up CONTROLLER_BYTE in digital mode
-write userfriendly functions for button checks
-finetune DelayCom()
-allow more flexibility in selecting SS port
-write documentation
-switch to interrupt based SPI (should free up some cycles)
*/
#ifndef CONTROLLER_H
#define CONTROLLER_H
#include "default.h"
#include "controller_ports.h" //define the ports in this file
#include <avr/interrupt.h>
#include <avr/io.h>
#ifndef C_PORT
  #warning "no C_PORT defined, using PORTC"
  #define C_PORT PORTC
#endif
#ifndef C_DDR
  #warning "no C_DDR defined, using DDRC"
  #define C_DDR DDRC
#endif
#ifndef C_PIN
  #warning "no C_PIN defined, using C_PIN"
  #define C_PIN PINC
#endif
#ifndef CMD
  #define CMD 1 //MOSI 3
#endif
#ifndef ATT
  #define ATT 0 //SS 2
#endif
#ifndef CLK 
  #define CLK 3 //SCK 5
#endif
#ifndef DATA
  #define DATA 2 //MISO 4
#endif

//select, start, up, right, down, left
//L2 R2 L1 R1 Triangle Circle Cross Square
#define ANALOG 0x73
#define DIGITAL 0x41
#define START 3
#define SELECT 0
#define UP 4
#define DOWN 6
#define LEFT 7
#define RIGHT 5
#define TRIANGLE 4
#define CIRCLE 5
#define SQUARE 7
#define CROSS 6
#define L1 2
#define L2 0
#define R1 3
#define R2 1

#define DelaySmall _delay_us(10)
#define DelayByte _delay_us(15)
#define DelayCom _delay_us(10)

//commands
#define C_START 0x01
#define C_ENTER_CONFIG 0x43
#define C_EXIT_CONFIG 0x43
#define C_REQ_DATA 0x42
#define C_IDLE 0x00
#define C_IGNORE 0x5a
#define C_LOCK_MODE 0x03
#define C_SWITCH_MODE 0x44

//the struct for storing controller readout
#define MAX_BUTTON_TIMER 0xFF
struct CONTROLLER_DATA {
  uint8_t _mode;
  uint8_t SS_Dpad;
  uint8_t Shoulder_Shapes;
  uint8_t previous_SS_Dpad; //useful for single push operation
  uint8_t previous_Shoulder_Shapes;
  uint8_t Rx;
  uint8_t Ry;
  uint8_t Lx;
  uint8_t Ly;
  uint8_t Rx_mid;
  uint8_t Ry_mid;
  uint8_t Lx_mid;
  uint8_t Ly_mid;
};

//simple function to initiate SPI transmission
uint8_t SPI_SEND(uint8_t _cmd);

//transmit one command byte to the controller and return it's respons
uint8_t CONTROLLER_BYTE(uint8_t _data);
//Clearing ATT is done automatically, setting isn't
//function is made to be chained, use 20 ms delay after chain.

//communicate with the controller and request it's button/joytisck readout
void POLL_CONTROLLER(struct CONTROLLER_DATA* cdat);

//communicate with the controller and force it to analog red mode (and lock it)
uint8_t SET_ANALOG(struct CONTROLLER_DATA* cdat);

//store the current joytisck readouts as mid position
void CALIBRATE_CONTROLLER(struct CONTROLLER_DATA* cdat);

//set up the required ports and spi registers, also set initial mode
uint8_t INIT_CONTROLLER(struct CONTROLLER_DATA *cdat,uint8_t setmode);
  //returns 0 if controller reports an other mode than requested
  


#endif
