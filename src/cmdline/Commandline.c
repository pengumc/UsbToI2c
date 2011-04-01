//commandline utility for controlling/reading the avr I2C master2
/*

this program has some keybindings to execute functions on the avr.
It also reads the data that was recieved via I2C by master2, 
this data is written to "avr.dat" in csv format. Also included in "avr.dat"
is another dataset with the data send through a kalman filter.
keybindings:
  q:    quit/exit
  x:    reset avr (disconnects usb)
  `:    reconnect usb
  r:    send manual data request

compiling: 
build command: make -f Makefile.windows
(make sure all files/paths are correct in the makefiles first)

this code is for win32 compatible systems. The only thing stopping it from
running on linux is the multithreading.
  
*/


#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
//multithreading libraries (win32)
#define CHECKING_INTERVAL 50
#include <windows.h>
#include <process.h>
//usb libraries
#include <usb.h>        /* this is libusb */
#include "opendevice.h" /* common code moved to separate module */ 
//target device information
#include "../requests.h"   /* custom request numbers */
#include "../usbconfig.h"  /* device's VID/PID and names */
#define USBCOMMANDLINE //used by i2c_header to exclude avr libraries
#include "i2c_header.h" //contains buffer lenghts used in microcontrollers
#define SLOW_MINUS 22
#define SLOW_PLUS 33

#define PW_MAX 96
#define PW_MIN 48
#define PUT_BUFFER \
	for(k=0;k<BUFLEN;k++){\
		if (temp_buffer[k] > PW_MAX && temp_buffer[k] != SLOW_PLUS && temp_buffer[k] != SLOW_MINUS) temp_buffer[k] = PW_MAX;\
		if (temp_buffer[k] < PW_MIN && temp_buffer[k] != SLOW_PLUS && temp_buffer[k]!=SLOW_MINUS) temp_buffer[k] = PW_MIN;\
		buffer[k] = temp_buffer[k];\
	}

	
//global
//-----------------------------------------------------------------------------
 #define MUTEXCOUNT 1
HANDLE mutexes[MUTEXCOUNT];
unsigned char input; 
void inputThread(void *args); 

usb_dev_handle      *handle = NULL;
#define BUFLEN BUFLEN_SERVO_DATA
unsigned char       buffer[BUFLEN]; //buffer for recieving data from device
void usbMsg(int request, int reqType, int wval, int wind);
uint8_t connected =0;
  char                vendor[] = {USB_CFG_VENDOR_NAME, 0}, product[] = {USB_CFG_DEVICE_NAME, 0};

//-----------------------------------------------------------------------------
//MAIN
//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
//usb variables and initialization
//-----------------------------------------------------------------------------
  //get vendor ID and device ID from usbconfig.h
  const unsigned char rawVid[2] = {USB_CFG_VENDOR_ID}, rawPid[2] = {USB_CFG_DEVICE_ID};
  int                 vid, pid;

  usb_init();
  vid = rawVid[1] * 256 + rawVid[0];
  pid = rawPid[1] * 256 + rawPid[0];
  //usbOpenDevice is in opendevice.c
  if(usbOpenDevice(&handle, vid, vendor, pid, product, NULL, NULL, NULL) != 0){
    fprintf(stderr, "Could not find USB device \"%s\" with vid=0x%x pid=0x%x\nNOT CONNECTED\n", product, vid, pid);
    connected=0;
  }else {
    connected =3;
  }
	uint8_t temp_buffer[BUFLEN] = SERVO_DATA_EMPTY;
//inputThread and mutex generation
//-----------------------------------------------------------------------------
mutexes[0] = CreateMutex(NULL,0,NULL);
_beginthread(inputThread,0,NULL);


uint8_t k;
uint8_t servo=0;
//-----------------------------------------------------------------------------
// main loop
//-----------------------------------------------------------------------------
while(1){
  switch(WaitForMultipleObjects(
		MUTEXCOUNT,
		mutexes, 
		0, 
		CHECKING_INTERVAL) 
	){
//-----------------------------------------------------------------------------
//keyboard input
//-----------------------------------------------------------------------------
    case WAIT_OBJECT_0: 
      printf("input: %c == ",input);
      switch(input){
/*quit*/case 'q':     
          printf("quitting...\n");
          if(connected) usb_close(handle);
          exit(0);
/*reco*/case '`':
          printf("(re)connecting \n");
          if(usbOpenDevice(&handle, vid, vendor, pid,product, NULL, NULL, NULL) != 0){
            fprintf(stderr, \
						"Could not find USB device \"%s\" with vid=0x%x pid=0x%x\n",\
						product, vid, pid);
            connected =0;
          }else {
            printf("reconnected to device\n");
            connected =1;
          }
          break;
/*req */case 'r':    
          printf("request data\n");
          usbMsg(CUSTOM_RQ_GET_DATA, USB_ENDPOINT_IN, 0,0);
          break;
/*set */case '+':    
          printf("increase servo %d\n",servo+1);
					temp_buffer[servo] += 4;
					PUT_BUFFER
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
          break;
/*set */case '-':    
          printf("decrease servo %d\n",servo+1);
					temp_buffer[servo] -= 4;
					PUT_BUFFER
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
          break;
/*set */case ']':   //SLOW PLUS
          printf("slow increase servo %d\n",servo+1);
					temp_buffer[servo] = SLOW_PLUS;
					PUT_BUFFER
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
					temp_buffer[servo] = PW_MAX;
          break;
/*set */case '[':   //SLOW MINUS
          printf("slow decrease servo %d\n",servo+1);
					temp_buffer[servo] = SLOW_MINUS;
					PUT_BUFFER
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
					temp_buffer[servo] = PW_MIN;
          break;
/*set */case '!':    
          printf("all legs UP\n");
					temp_buffer[1]+=4;
					temp_buffer[2]-=4;
					temp_buffer[4]+=4;
					temp_buffer[5]-=4;
					temp_buffer[7]+=4;
					temp_buffer[8]-=4;
					temp_buffer[10]+=4;
					temp_buffer[11]-=4;
					PUT_BUFFER
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
          break;
/*set */case '@':    
          printf("all legs DOWN\n");
					temp_buffer[1]-=4;
					temp_buffer[2]+=4;
					temp_buffer[4]-=4;
					temp_buffer[5]+=4;
					temp_buffer[7]-=4;
					temp_buffer[8]+=4;
					temp_buffer[10]-=4;
					temp_buffer[11]+=4;
					PUT_BUFFER
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
          break;
/*set */case '#':    
          printf("FORWARD\n");
					temp_buffer[0]+=4;
					temp_buffer[6]+=4;
					temp_buffer[9]-=4;
					temp_buffer[3]-=4;
					PUT_BUFFER
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
          break;
/*set */case '$':    
          printf("BACKWARD\n");
					temp_buffer[0]-=4;
					temp_buffer[6]-=4;
					temp_buffer[9]+=4;
					temp_buffer[3]+=4;
					PUT_BUFFER
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
          break;
/*set */case '%':    
          printf("A D UP, B C DOWN\n");
					temp_buffer[1]+=4; //A
					temp_buffer[2]-=4;
					temp_buffer[4]-=4; //B
					temp_buffer[5]+=4;
					temp_buffer[7]-=4; //C
					temp_buffer[8]+=4;
					temp_buffer[10]+=4; //D
					temp_buffer[11]-=4;
					PUT_BUFFER
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
          break;
/*res */case 'x':    
          printf("reset\nTry reconnecting in a bit\n");
          usbMsg(CUSTOM_RQ_RESET, USB_ENDPOINT_IN, 0,0);
          break;
//------------------servo select-----------------------------------------------
/*sel*/case '1':
					servo = 0;
					printf("Changed to servo %d\n",servo+1);
					break;
/*sel*/case '2':
					servo = 1;
					printf("Changed to servo %d\n",servo+1);
					break;
/*sel*/case '3':
					servo = 2;
					printf("Changed to servo %d\n",servo+1);
					break;
/*sel*/case '4':
					servo = 3;
					printf("Changed to servo %d\n",servo+1);
					break;
/*sel*/case '5':
					servo = 4;
					printf("Changed to servo %d\n",servo+1);
					break;
/*sel*/case '6':
					servo = 5;
					printf("Changed to servo %d\n",servo+1);
					break;
/*sel*/case '7':
					servo = 6;
					printf("Changed to servo %d\n",servo+1);
					break;
/*sel*/case '8':
					servo = 7;
					printf("Changed to servo %d\n",servo+1);
					break;
/*sel*/case '9':
					servo = 8;
					printf("Changed to servo %d\n",servo+1);
					break;
/*sel*/case '0':
					servo = 9;
					printf("Changed to servo %d\n",servo+1);
					break;
/*sel*/case '.':
					servo = 10;
					printf("Changed to servo %d\n",servo+1);
					break;
/*sel*/case '/':
					servo = 11;
					printf("Changed to servo %d\n",servo+1);
					break;
//-------------------------CYCLES----------------------------------------------
/*strt*/case 'a':
					printf("reset legs\n");
					uint8_t qq[BUFLEN] = SERVO_DATA_EMPTY;
					for(k=0;k<BUFLEN;k++){
						temp_buffer[k] = qq[k];
					}
					PUT_BUFFER
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
          break;
/*cycl*/case 't':
					temp_buffer[0] = 92;
					temp_buffer[1] = 72;
					//temp_buffer[2] = ;
					temp_buffer[3] = 92;
					//temp_buffer[4] = ;
					temp_buffer[5] = 92;
					//temp_buffer[6] = ;
					temp_buffer[7] = 72;
					PUT_BUFFER
					buffer[0] = 90;
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
          break;
/*cycl*/case 'g':
					//temp_buffer[0] = ;
					temp_buffer[1] = 92;
					//temp_buffer[2] = ;
					temp_buffer[3] = 72;
					//temp_buffer[4] = ;
					temp_buffer[5] = 72;
					//temp_buffer[6] = ;
					temp_buffer[7] = 92;
					PUT_BUFFER
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
          break;
/*cycl*/case 'y':
					temp_buffer[2]=86;
					temp_buffer[4]=66;
					temp_buffer[0]=72;
					temp_buffer[6]= 70;
					PUT_BUFFER
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
          break;
/*cycl*/case 'h':
					temp_buffer[2]=66;
					temp_buffer[4]=86;
					temp_buffer[0]=66;
					temp_buffer[6]= 80;
					PUT_BUFFER
          usbMsg(CUSTOM_RQ_SET_DATA, USB_ENDPOINT_OUT, 0,0);
          break;
					
/*def */default:
          printf("not bound.\n");
          break;
      }//switch input
      break; 
//-----------------------------------------------------------------------------
// no input
//-----------------------------------------------------------------------------
  case WAIT_TIMEOUT: //request data from device every CHECKING_INTERVAL ms
		break;
		
		
  }//end mutex switch
 
  ReleaseMutex(mutexes[0]);
//-----------------------------------------------------------------------------
}//end main loop
}//end main

void inputThread(void *args){
	while(1){
		//wait for the main thread to release the mutex
		WaitForSingleObject(mutexes[0], INFINITE);
			//wait for keyboard input
			input = getch();
		ReleaseMutex(mutexes[0]);
	}
	
	_endthread();
} 

//this function sends a request through libusb
// and prints the recieved data, use wind=244 to skip printing
void usbMsg(int request, int endpoint, int wval, int wind){
  int cnt;
  short int i;
  if(handle && connected){
    cnt = usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | endpoint, request, wval-1, wind-1, buffer, 12, 5000);
    if(cnt < 0){
      fprintf(stderr, "USB error: %s\n", usb_strerror());
      connected--;
    }else if(cnt>0 && wind != 244){
      printf("bytes recieved: %d, buffer length: %d\n",cnt, sizeof(buffer));
			connected = 3;
      for(i=0; i<BUFLEN;i++){
        printf("buffer[%d] = %d\n", i, buffer[i]);
      }
    }
  }else{
    printf("no valid usb_dev_handle\nTry reconnecting\n");
  }

}

