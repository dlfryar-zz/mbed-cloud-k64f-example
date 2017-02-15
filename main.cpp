#include "mbed.h"
#include "BufferedSerial.h"
// OLE42178P Seeed Studio OLED display 96 x 96
#include "SeeedGrayOLED.h"
// FXOS8700Q MotionSensor Accelerometer & Magnetometer
#include "FXOS8700Q.h"
// all the image bitmaps
#include "images.h"


BufferedSerial pc(USBTX, USBRX);
I2C i2c(PTE25, PTE24);

Thread t_oled;
Thread t_mag;
Thread t_acc;

time_t get_current_time() {
  return time(NULL);
}

void print_current_time() {
  time_t seconds = get_current_time();
  pc.printf("Current Time is: %s", ctime(&seconds));
}

void oled() {
  unsigned char *bitmaps[] = { qsg_96X96_mono_bmp };
  i2c.lock();
  // Setup OLED with proper i2c clock and data
  SeeedGrayOLED SeeedGrayOled(PTE25, PTE24);
  SeeedGrayOled.init(); //initialize SEEED OLED display
  i2c.unlock();

  while (true) {
    i2c.lock();

    pc.printf("%s:%d-->%s()\n", __FILE__, __LINE__, __func__);
    // clear screen
    SeeedGrayOled.drawBitmap(black_square_96X96_mono_bmp, 96 * 96 / 8);

    SeeedGrayOled.setTextXY(0, 0); //set Cursor to first line, 0th column
    SeeedGrayOled.clearDisplay(); //Clear Display.
    SeeedGrayOled.setNormalDisplay(); //Set Normal Display Mode
    SeeedGrayOled.setVerticalMode();
    SeeedGrayOled.setGrayLevel(15); //Set Grayscale level

    // clear screen
    SeeedGrayOled.drawBitmap(black_square_96X96_mono_bmp, 96 * 96 / 8);

    SeeedGrayOled.drawBitmap(ARM_logo_96X96_mono_bmp, 96 * 96 / 8);
    wait_ms(500);

    // clear screen
    SeeedGrayOled.drawBitmap(black_square_96X96_mono_bmp, 96 * 96 / 8);

    SeeedGrayOled.setGrayLevel(15); //Set Grayscale level
    for (int j = 0; j < (int) (sizeof(bitmaps) / sizeof(unsigned char*)); j++) {
      SeeedGrayOled.drawBitmap(bitmaps[j], 96 * 96 / 8);
      wait_ms(500);
    }

    // clear screen
    SeeedGrayOled.drawBitmap(black_square_96X96_mono_bmp, 96 * 96 / 8);

    SeeedGrayOled.setGrayLevel(15); //Set Grayscale level 15.
    SeeedGrayOled.drawBitmap(MBED_ENABLED_LOGO_100X100_96X96_mono_bmp,
        96 * 96 / 8);

    // clear screen
    // SeeedGrayOled.drawBitmap(black_square_96X96_mono_bmp, 96 * 96 / 8);
    wait(1);
    i2c.unlock();
  }
}

void mag() {
  i2c.lock();
  FXOS8700QMagnetometer mag(i2c, FXOS8700CQ_SLAVE_ADDR1); // Proper Ports and I2C Address for K64F Freedom board
  mag.enable(); // enable the FXOS8700Q Magnetometer
  i2c.unlock();

  while (true) {
    print_current_time();
    pc.printf("%s:%d-->%s()\n", __FILE__, __LINE__, __func__);
    i2c.lock();
    motion_data_units_t mag_data;
    motion_data_counts_t mag_raw;
    float fmX, fmY, fmZ;
    int16_t rmX, rmY, rmZ;
    char *buf;
    size_t sz;

    mag.getAxis(mag_raw);
    mag.getX(rmX);
    mag.getY(rmY);
    mag.getZ(rmZ);

    mag.getAxis(mag_data);
    mag.getX(fmX);
    mag.getY(fmY);
    mag.getZ(fmZ);

    // SeeedGrayOled.setTextXY(2,0); //set Cursor to third line, 0th column
    // sz = snprintf(NULL, 0, "Who Am I=%X", mag.whoAmI());
    // buf = (char *) malloc(sz + 1); /* make sure you check for != NULL in real code */
    // snprintf(buf, sz + 1, "Who Am I=%X", mag.whoAmI());
    // SeeedGrayOled.putString(buf);
    // pc.printf("buf is %s %s:%d-->%s()\n", buf, __FILE__, __LINE__, __func__);
    // free(buf);

    sz = snprintf(NULL, 0, "MAG:X=%1.4f Y=%1.4f Z=%1.4f  ", mag_data.x,
        mag_data.y, mag_data.z);
    buf = (char *) malloc(sz + 1); /* make sure you check for != NULL in real code */
    snprintf(buf, sz + 1, "MAG:X=%1.4f Y=%1.4f Z=%1.4f  ", mag_data.x,
        mag_data.y, mag_data.z);
    pc.printf("buf is %s %s:%d-->%s()\n", buf, __FILE__, __LINE__, __func__);
    free(buf);
    i2c.unlock();
  }
}

void acc() {
  i2c.lock();
  // Proper Ports and I2C Address for K64F Freedom board
  FXOS8700QAccelerometer acc(i2c, FXOS8700CQ_SLAVE_ADDR1);
  acc.enable(); // enable the FXOS8700Q Magnetometer
  i2c.unlock();

  while (true) {
    print_current_time();
    pc.printf("%s:%d-->%s()\n", __FILE__, __LINE__, __func__);
    i2c.lock();
    motion_data_units_t acc_data;
    motion_data_counts_t acc_raw;
    float faX, faY, faZ;
    int16_t raX, raY, raZ;
    char *buf;
    size_t sz;

    acc.getAxis(acc_raw);
    acc.getX(raX);
    acc.getY(raY);
    acc.getZ(raZ);

    acc.getAxis(acc_data);
    acc.getX(faX);
    acc.getY(faY);
    acc.getZ(faZ);

    // SeeedGrayOled.setTextXY(2,0); //set Cursor to third line, 0th column
    // sz = snprintf(NULL, 0, "Who Am I=%X", acc.whoAmI());
    // buf = (char *) malloc(sz + 1); /* make sure you check for != NULL in real code */
    // snprintf(buf, sz + 1, "Who Am I=%X", acc.whoAmI());
    // SeeedGrayOled.putString(buf);
    // pc.printf("buf is %s %s:%d-->%s()\n", buf, __FILE__, __LINE__, __func__);
    // free(buf);

    sz = snprintf(NULL, 0, "ACC:X=%1.4f Y=%1.4f Z=%1.4f  ", acc_data.x,
        acc_data.y, acc_data.z);
    buf = (char *) malloc(sz + 1); /* make sure you check for != NULL in real code */
    snprintf(buf, sz + 1, "ACC:X=%1.4f Y=%1.4f Z=%1.4f  ", acc_data.x,
        acc_data.y, acc_data.z);
    pc.printf("buf is %s %s:%d-->%s()\n", buf, __FILE__, __LINE__, __func__);
    free(buf);
    i2c.unlock();
  }
}

// main() runs in its own thread in the OS
// (note the calls to wait below for delays)
int main() {
  set_time(1486961386);
  pc.baud(115200);
  pc.printf("starting thread1 %s:%d-->%s()\n", __FILE__, __LINE__, __func__);
  t_oled.start(oled);
  pc.printf("starting thread2 %s:%d-->%s()\n", __FILE__, __LINE__, __func__);
  t_mag.start(mag);
  pc.printf("starting thread3 %s:%d-->%s()\n", __FILE__, __LINE__, __func__);
  t_acc.start(acc);
  while (true) {
    print_current_time();
    wait(5);
  }
}
