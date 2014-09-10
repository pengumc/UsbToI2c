
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif

#include "hidapi.h"

// ----------------------------------------------------------------------usbread
void usbread(hid_device* handle, uint8_t* b, size_t length) {
  int res = hid_read(handle, b, length);
  if (res < 0) {
    printf("read error, res = %d\n", res);
    printf("sys: %ls", hid_error(handle));
  } else {
    printf("read %d bytes:\n  ", res);
    int i;
    for (i = 0; i < res; ++i) {
      printf("%d ", b[i]);
    }
    printf("\n");
  }
}

// ---------------------------------------------------------------------usbwrite
void usbwrite(hid_device* handle, const uint8_t* b, size_t length) {
    int res = hid_write(handle, b, length);
    if (res < 0) {
    printf("write failure, res = %d\n", res);
    printf("system: %ls", hid_error(handle));
  } else {
    printf("written %d bytes:\n  ", res);
    int i;
    for (i = 1; i < res; ++i) {
      printf("%d ", b[i]);
    }
    printf("\n");
    uint8_t buf[8];
    usbread(handle, buf, sizeof(buf));
  }
}

void print_usage() {
  printf(" Usage: \n");
  printf(" sendcmd r|w [cmd] [value for rest of 12 words (12 x 2 bytes)] \n");
  printf(" examples:\n");
  printf("   sendcmd w 3 70 (set all to 70)\n");
  printf("   sendcmd r (read)\n");
}

// -------------------------------------------------------------------------main
int main(int argc, char** argv) {
  printf("sendcmd\n\n");
  if (argc < 2) {
    print_usage();
    return 0;
  }

  int init_result = hid_init();
  if (init_result) {
    printf("hid_init() failed\n");
    return 1;
  }

  uint16_t vid = 0x16c0;
  uint16_t pid = 0x05df;
  hid_device* handle = hid_open(vid, pid, NULL);
  if (!handle) {
    printf("Failed to opendevice vid: 0x%04hX, pid: 0x%04hX\n", vid, pid); 
    return 1;
  }
  printf("opened device vid: 0x%04hX, pid: 0x%04hX\n", vid, pid);

  if (argv[1][0] == 'r') {
    // just read
    uint8_t rbuf[8];
    usbread(handle, rbuf, sizeof(rbuf));
  } else if (argv[1][0] == 'w') {
    if (argc != 4) {
      print_usage();
      hid_close(handle);
      return 1;
    }
    uint8_t cmd = (uint8_t) atoi(argv[2]);
    uint8_t a = atol(argv[3]) >> 8;
    uint8_t b = atol(argv[3]);
    uint8_t buf[26] = {0x00, cmd,
                        a, b, a, b, a, b, a, b, a, b, a, b,
                        a, b, a, b, a, b, a, b, a, b, a, b};
    usbwrite(handle, buf, sizeof(buf));
  }
  
  hid_close(handle);
  
  int exit_result = hid_exit();
  if (exit_result) {
    printf("hid_exit() failed\n");
    return 1;
  }
  return 0;
}
