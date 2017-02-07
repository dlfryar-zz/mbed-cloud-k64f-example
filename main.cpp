#include "mbed.h"
// OLE42178P Seeed Studio OLED display 96 x 96
#include "SeeedGrayOLED.h"
// FXOS8700Q MotionSensor Accelerometer & Magnetometer
#include "FXOS8700Q.h"
// mbed enabled logo
#include "MBED_ENABLED_LOGO_100X100.h"
// black image
#include "black_square.h"
// arm logo
#include "ARM-logo.h" 
#include "qsg.h"

I2C i2c(PTE25, PTE24);
FXOS8700QAccelerometer acc(i2c, FXOS8700CQ_SLAVE_ADDR1); // Proper Ports and I2C Address for K64F Freedom board
FXOS8700QMagnetometer mag(i2c, FXOS8700CQ_SLAVE_ADDR1); // Proper Ports and I2C Address for K64F Freedom board

// Setup OLED with proper i2c clock and data
SeeedGrayOLED SeeedGrayOled(PTE25, PTE24);

void draw_bitmap_pics();
void draw_mbed_logo();
void draw_arm_logo();
void print_text();
void print_acc_mag();
void cls();

int main() {
  printf("Starting up K64F board in main.cpp/main()\r\n");
  acc.enable(); // enable the FXOS8700Q Accelerometer
  mag.enable(); // enable the FXOS8700Q Magnetometer
  SeeedGrayOled.init(); //initialize SEEED OLED display

  // Application that uses libraries for sensors and a display on K64F with Grove OLED
  // - One i2c display and a couple of onboard sensors
  // - Sensor values read printed on display
  // - One task for each sensor
  // - Wrap an mbed mutex around the LCD before printing sensor data messages
  //
  // FXOS8700CQ - 6-axis combo Sensor Accelerometer and Magnetometer
  // 2 user push-buttons
  // RGB LED
  // OLED display

  while (true) {
    cls();
    draw_arm_logo();
    wait(3);
    draw_bitmap_pics();
    wait(1);
    draw_mbed_logo();
    wait(1);
    cls();
    print_acc_mag();
    wait(1);
    cls();
    print_text();
    wait(1);
  }
}


void print_text() {
  // cls();
  SeeedGrayOled.setTextXY(0,0); //set Cursor to first line, 0th column
  SeeedGrayOled.clearDisplay(); //Clear Display.
  SeeedGrayOled.setNormalDisplay(); //Set Normal Display Mode
  SeeedGrayOled.setVerticalMode();

  for(char i=0; i < 12; i++) {
    SeeedGrayOled.setTextXY(i,0); //set Cursor to first line, 0th column
    SeeedGrayOled.setGrayLevel(i); //Set Grayscale level. Any number between 0 - 15.
    SeeedGrayOled.putString("Hello World"); //Print Hello World
    wait_ms(5.0);
  }
}

void draw_arm_logo() {
  SeeedGrayOled.setGrayLevel(15); //Set Grayscale level
  SeeedGrayOled.drawBitmap(ARM_logo_96X96_mono_bmp,96*96/8);
}

void draw_bitmap_pics() {
  SeeedGrayOled.setGrayLevel(15); //Set Grayscale level
  unsigned char *bitmaps[] = {qsg_96X96_mono_bmp};

  for (int j=0; j<sizeof(bitmaps)/sizeof(unsigned char*); j++) {
    SeeedGrayOled.drawBitmap(bitmaps[j],96*96/8);
    wait_ms(200);
  }
}

void draw_mbed_logo() {
#ifdef LOOP_GRAY_LEVELS
  for(char i=0; i<16; i++) {
    if (i % 2 !=0) {
      SeeedGrayOled.setGrayLevel(i); //Set Grayscale level. Any number between 0 - 15.
      SeeedGrayOled.drawBitmap(MBED_ENABLED_LOGO_100X100_96X96_mono_bmp,96*96/8);
      wait_ms(3.0);
    }
  }
#else
  SeeedGrayOled.setGrayLevel(15); //Set Grayscale level 15.
  SeeedGrayOled.drawBitmap(MBED_ENABLED_LOGO_100X100_96X96_mono_bmp,96*96/8);
#endif
}

void print_acc_mag() {
  motion_data_units_t acc_data, mag_data;
  motion_data_counts_t acc_raw, mag_raw;
  float faX, faY, faZ, fmX, fmY, fmZ;
  int16_t raX, raY, raZ, rmX, rmY, rmZ;
  char *buf;
  size_t sz;

  // counts based results
  acc.getAxis(acc_raw);
  mag.getAxis(mag_raw);
  acc.getX(raX);
  acc.getY(raY);
  acc.getZ(raZ);
  mag.getX(rmX);
  mag.getY(rmY);
  mag.getZ(rmZ);

  // unit based results
  acc.getAxis(acc_data);
  mag.getAxis(mag_data);
  acc.getX(faX);
  acc.getY(faY);
  acc.getZ(faZ);
  mag.getX(fmX);
  mag.getY(fmY);
  mag.getZ(fmZ);

  printf("\r\n\nFXOS8700Q Who Am I = %X\r\n", acc.whoAmI());
  printf("FXOS8700Q.MotionSensor: X=%1.4f Y=%1.4f Z=%1.4f  ", acc_data.x, acc_data.y, acc_data.z);
  printf("    MAG: X=%4.1f Y=%4.1f Z=%4.1f\r\n", mag_data.x, mag_data.y, mag_data.z);

  printf("FXOS8700Q.MotionSensor: X=%1.4f Y=%1.4f Z=%1.4f  ", faX, faY, faZ);
  printf("    MAG: X=%4.1f Y=%4.1f Z=%4.1f\r\n", fmX, fmY, fmZ);

  printf("FXOS8700Q.MotionSensor: X=%d Y=%d Z=%d  ", acc_raw.x, acc_raw.y, acc_raw.z);
  printf("    MAG: X=%d Y=%d Z=%d\r\n", mag_raw.x, mag_raw.y, mag_raw.z);

  printf("FXOS8700Q.MotionSensor: X=%d Y=%d Z=%d  ", raX, raY, raZ);
  printf("    MAG: X=%d Y=%d Z=%d\r\n\n", rmX, rmY, rmZ);

  SeeedGrayOled.clearDisplay(); //Clear Display.
  SeeedGrayOled.setNormalDisplay(); //Set Normal Display Mode
  SeeedGrayOled.setVerticalMode();

  SeeedGrayOled.setGrayLevel(15); //Set Grayscale level

  SeeedGrayOled.setTextXY(0,0); //set Cursor to first line, 0th column
  SeeedGrayOled.putString("FXOS8700Q");

  SeeedGrayOled.setTextXY(1,0); //set Cursor to second line, 0th column
  SeeedGrayOled.putString("MotionSensor");

  SeeedGrayOled.setTextXY(2,0); //set Cursor to third line, 0th column
  sz = snprintf(NULL, 0, "Who Am I=%X", acc.whoAmI());
  buf = (char *)malloc(sz + 1); /* make sure you check for != NULL in real code */
  snprintf(buf, sz+1, "Who Am I=%X", acc.whoAmI());
  SeeedGrayOled.putString(buf);
  free(buf);

  SeeedGrayOled.setTextXY(3,0);  //set Cursor to fourth line, 0th column
  sz = snprintf(NULL, 0, "ACC:X=%1.4f Y=%1.4f Z=%1.4f  ", acc_data.x, acc_data.y, acc_data.z);
  buf = (char *)malloc(sz + 1); /* make sure you check for != NULL in real code */
  snprintf(buf, sz+1, "ACC:X=%1.4f Y=%1.4f Z=%1.4f  ", acc_data.x, acc_data.y, acc_data.z);
  SeeedGrayOled.putString(buf);
  free(buf);
}

void cls() {
  SeeedGrayOled.setNormalDisplay();
  SeeedGrayOled.setTextXY(0,0);
  SeeedGrayOled.clearDisplay();
  SeeedGrayOled.setGrayLevel(0); //Set Grayscale level
  SeeedGrayOled.drawBitmap(black_square_96X96_mono_bmp,96*96/8);
}

