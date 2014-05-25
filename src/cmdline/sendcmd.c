
#include <stdio.h>
#include <stdint.h>

#include "hidapi.h"

int main(int argc, char** argv) {
  printf("sendcmd\n\n");
  
  int init_result = hid_init();
  if (init_result) {
    printf("hid_init() failed\n");
    return 1;
  }

  uint16_t vid = 0x16c0;
  uint16_t pid = 0x05df;
  hid_device* handle = hid_open(vid, pid, NULL)
  if (!handle) {
    printf("Failed to opendevice vid: 0x%04hX, pid: 0x%04hX\n", vid, pid); 
    return 1;
  }
  printf("opened device vid: 0x%04hX, pid: 0x%04hX\n", vid, pid);
  
  // send command
  // ...
  
  
  hid_close(handle);
  int exit_result = hid_exit();
  if (exit_result) {
    printf("hid_exit() failed\n");
    return 1;
  }
  return 0;
}
