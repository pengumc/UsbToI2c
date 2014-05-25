
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


  int exit_result = hid_exit();
  if (exit_result) {
    printf("hid_exit() failed\n");
    return 1;
  }
  return 0;
}
