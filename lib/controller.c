//Author: Michiel van der Coelen
//date: 25-08-2010
#include "../controller.h"
uint8_t SPI_SEND(uint8_t _cmd){
  SPDR = _cmd;
  while(!CHK(SPSR,SPIF));
  return SPDR;
}

uint8_t CONTROLLER_BYTE(uint8_t _data){
  SET(C_PORT,CMD);
  SET(C_PORT,CLK);
  CLR(C_PORT,ATT);
  DelaySmall;
  return SPI_SEND(_data);
}

void POLL_CONTROLLER(struct CONTROLLER_DATA* cdat){
	cdat->previous_SS_Dpad = cdat->SS_Dpad;
	cdat->previous_Shoulder_Shapes = cdat->Shoulder_Shapes;
	CONTROLLER_BYTE(C_START);
  cdat->_mode = CONTROLLER_BYTE(C_REQ_DATA);
  CONTROLLER_BYTE(C_IDLE);
  cdat->SS_Dpad = CONTROLLER_BYTE(C_IDLE);
  cdat->Shoulder_Shapes = CONTROLLER_BYTE(C_IDLE);
  cdat->Rx = CONTROLLER_BYTE(C_IDLE);
  cdat->Ry = CONTROLLER_BYTE(C_IDLE);
  cdat->Lx = CONTROLLER_BYTE(C_IDLE);
  cdat->Ly = CONTROLLER_BYTE(C_IDLE);
  SET(C_PORT,ATT);
  //DelayCom;
}


uint8_t SET_ANALOG(struct CONTROLLER_DATA* cdat){
		CONTROLLER_BYTE(C_START); 
		CONTROLLER_BYTE(C_ENTER_CONFIG); 
		CONTROLLER_BYTE(C_IDLE);
		CONTROLLER_BYTE(C_START);
		CONTROLLER_BYTE(C_IDLE);
		SET(C_PORT,ATT);
		DelayCom;
		
		//switch modes and lock
		CONTROLLER_BYTE(C_START); //start
		CONTROLLER_BYTE(C_SWITCH_MODE); //switch mode
		CONTROLLER_BYTE(C_IDLE);
		CONTROLLER_BYTE(C_START); //set analog
		CONTROLLER_BYTE(C_LOCK_MODE); //lock mode
		CONTROLLER_BYTE(C_IDLE);
		CONTROLLER_BYTE(C_IDLE);
		CONTROLLER_BYTE(C_IDLE);
		CONTROLLER_BYTE(C_IDLE);
		SET(C_PORT,ATT);
		DelayCom;
	
		//exit config mod
		CONTROLLER_BYTE(C_START); //start
		cdat->_mode = CONTROLLER_BYTE(C_EXIT_CONFIG); //exit config
		CONTROLLER_BYTE(C_IDLE);
		CONTROLLER_BYTE(C_IDLE);
		CONTROLLER_BYTE(C_IGNORE);
		CONTROLLER_BYTE(C_IGNORE);
		CONTROLLER_BYTE(C_IGNORE);
		CONTROLLER_BYTE(C_IGNORE);
		CONTROLLER_BYTE(C_IGNORE);
		SET(C_PORT,ATT);
		DelayCom;
    POLL_CONTROLLER(cdat);
    return cdat->_mode;
}

//store the current joytisck readouts as mid position
void CALIBRATE_CONTROLLER(struct CONTROLLER_DATA* cdat){
  POLL_CONTROLLER(cdat);
  cdat->Rx_mid = cdat->Rx ;
  cdat->Ry_mid = cdat->Ry;
  cdat->Lx_mid = cdat->Lx;
  cdat->Ly_mid = cdat->Ly;
}

//set up the required ports and spi registers, also set initial mode
uint8_t INIT_CONTROLLER(struct CONTROLLER_DATA *cdat,uint8_t setmode){
  //returns 0 if controller reports an other mode than requested
  C_DDR &= ~(1<<DATA);
  C_DDR |= 	(1<<CMD)|(1<<ATT)|(1<<CLK);
  C_PORT &= ~((1<<DATA)|(1<<CMD)|(1<<ATT)|(1<<CLK));
  //C_PORT|=	(0<<DATA)|(0<<CMD)|(0<<ATT)|(0<<CLK);
  SPCR |=   (0<<SPIE)|(1<<SPE)|(1<<DORD)|(1<<MSTR)|(1<<CPOL)|(1<<CPHA)|(1<<SPR1)|(1<<SPR0);
  SPCR &= ~((0<<SPR0)|(0<<SPIE));
  POLL_CONTROLLER(cdat);
  if(cdat->_mode == setmode) return 1;
  else{
    if(SET_ANALOG(cdat)==setmode) return 1;
    else return 0;
  }
}




