//----------------------------------------------------------------------------
// The confidential and proprietary information contained in this file may
// only be used by a person authorised under and to the extent permitted
// by a subsisting licensing agreement from ARM Limited or its affiliates.
//
// (C) COPYRIGHT 2016 ARM Limited or its affiliates.
// ALL RIGHTS RESERVED
//
// This entire notice must be reproduced on all copies of this file
// and copies of this file may only be made by a person if such person is
// permitted to do so under the terms of a subsisting license agreement
// from ARM Limited or its affiliates.
//----------------------------------------------------------------------------

#include "mbed.h"

#define MYCLOUDAPP

#ifdef MYCLOUDAPP
// OLE42178P Seeed Studio OLED display 96 x 96
#include "SeeedGrayOLED.h"
// FXOS8700Q MotionSensor Accelerometer & Magnetometer
#include "FXOS8700Q.h"
// all the image bitmaps
#include "images.h"
// #define TRACE_GROUP  "acc,mac,oled"
#endif

#ifdef MBED_HEAP_STATS_ENABLED
// used by print_heap_stats only
#include "mbed_stats.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "mbed-client/m2mbase.h"
#endif

#include "mbed-cloud-client/MbedCloudClient.h"
#include "mbed-cloud-client/SimpleM2MResource.h"
#include "mbed-client/m2minterface.h"
#include "lib_factory_injection_client.h"
#include "mbed-trace/mbed_trace.h"
#include "mbed_cloud_client_user_config.h"
#include "memory_tests.h"

#define FACTORY_STACK_SIZE (1024 * 7)

#define ETHERNET        1
#define WIFI            2
#define MESH_LOWPAN_ND  3
#define MESH_THREAD     4

#if MBED_CONF_APP_NETWORK_INTERFACE == WIFI
#include "ESP8266Interface.h"
ESP8266Interface esp(D1, D0);
#elif MBED_CONF_APP_NETWORK_INTERFACE == ETHERNET
#include "EthernetInterface.h"
EthernetInterface eth;
#elif MBED_CONF_APP_NETWORK_INTERFACE == MESH_LOWPAN_ND
#define MESH
#include "NanostackInterface.h"
LoWPANNDInterface mesh;
#elif MBED_CONF_APP_NETWORK_INTERFACE == MESH_THREAD
#define MESH
#include "NanostackInterface.h"
ThreadInterface mesh;
#else
#error "No connectivity method chosen. Please add 'config.network-interfaces.value' to your mbed_app.json (see README.md for more information)."
#endif

#ifdef MBED_CLOUD_CLIENT_SUPPORT_UPDATE
#include "update_ui_example.h"
#endif

#ifdef MBED_APPLICATION_SHIELD
#include "C12832.h"
C12832* lcd;
#endif

#ifdef MYCLOUDAPP
I2C i2c(PTE25, PTE24);
RawSerial   pc(USBTX, USBRX);
static Mutex mutex_uart;

Thread t_oled;
Thread t_mag;
Thread t_acc;
#endif

DigitalOut  led1(LED_GREEN);
InterruptIn btn(SW2);
InterruptIn cls(SW3);
Semaphore   updates(0);

MbedCloudClient client;

volatile bool registered = false;
volatile bool error_occured = false;
volatile bool clicked = false;


static Mutex SerialOutMutex;

void serial_out_mutex_wait()
{
    SerialOutMutex.lock();
}

void serial_out_mutex_release()
{
    SerialOutMutex.unlock();
}

void patternUpdated(string v) {
    pc.printf("New pattern: %s\n", v.c_str());
}

// Note: the mbed-os needs to be compiled with MBED_HEAP_STATS_ENABLED to get
// functional heap stats, or the mbed_stats_heap_get() will return just zeroes.
static void print_heap_stats()
{
#ifdef MBED_HEAP_STATS_ENABLED
    mbed_stats_heap_t stats;
    mbed_stats_heap_get(&stats);
    pc.printf("**** current_size: %" PRIu32 "\n", stats.current_size);
    pc.printf("**** max_size    : %" PRIu32 "\n", stats.max_size);
#endif // MBED_HEAP_STATS_ENABLED
}

#ifdef MYCLOUDAPP
void uart_print(const char* str) {
    mutex_uart.lock();
    pc.printf("%s", str);
    mutex_uart.unlock();
}

time_t get_current_time() {
  return time(NULL);
}

void print_current_time() {
  time_t seconds = get_current_time();
  // tr_info(, "Current Time is: %s", ctime(&seconds));
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
#endif

/*
 * The Led contains one property (pattern) and a function (blink).
 * When the function blink is executed, the pattern is read, and the LED
 * will blink based on the pattern.
 */
class LedResource : public MbedCloudClientCallback {
public:
    LedResource() {
        // create ObjectID with metadata tag of '3201', which is 'digital output'
        led_object = M2MInterfaceFactory::create_object("3201");
        M2MObjectInstance* led_inst = led_object->create_object_instance();

        // 5853 = Multi-state output
        M2MResource* pattern_res = led_inst->create_dynamic_resource("5853", "Pattern",
            M2MResourceInstance::STRING, false);
        // read and write
        pattern_res->set_operation(M2MBase::GET_PUT_ALLOWED);
        // set initial pattern (toggle every 200ms. 7 toggles in total)
        pattern_res->set_value((const uint8_t*)"500:500:500:500:500:500:500", 27);

        // there's not really an execute LWM2M ID that matches... hmm...
        M2MResource* led_res = led_inst->create_dynamic_resource("5850", "Blink",
            M2MResourceInstance::OPAQUE, false);
        // we allow executing a function here...
        led_res->set_operation(M2MBase::POST_ALLOWED);
        // when a POST comes in, we want to execute the led_execute_callback
        led_res->set_execute_function(execute_callback(this, &LedResource::blink));
    }

    M2MObject* get_object() {
        return led_object;
    }

    // implementation of MbedCloudClientCallback
    virtual void value_updated(M2MBase *base, M2MBase::BaseType type) {
        pc.printf("PUT Request Received!\n");
        pc.printf("\nName :'%s',\nPath : '%s',\nType : '%d' (0 for Object, 1 for Resource), \nType : '%s'\n",
                  base->name(),
                  base->uri_path(),
                  type,
                  base->resource_type());
    }

    void blink(void *) {
        // read the value of 'Pattern'
        M2MObjectInstance* inst = led_object->object_instance();
        M2MResource* res = inst->resource("5853");

        // values in mbed Client are all buffers, and we need a vector of int's
        uint8_t* buffIn = NULL;
        uint32_t sizeIn;
        res->get_value(buffIn, sizeIn);

        // turn the buffer into a string, and initialize a vector<int> on the heap
        std::string s((char*)buffIn, sizeIn);
        std::vector<uint32_t>* v = new std::vector<uint32_t>;

        pc.printf("led_execute_callback pattern=%s\n", s.c_str());

        // our pattern is something like 500:200:500, so parse that
        std::size_t found = s.find_first_of(":");
        while (found!=std::string::npos) {

            v->push_back(atoi((const char*)s.substr(0,found).c_str()));
            s = s.substr(found+1);
            found=s.find_first_of(":");
            if(found == std::string::npos) {
                v->push_back(atoi((const char*)s.c_str()));
            }
        }


        // do_blink is called with the vector, and starting at -1
        do_blink(v);
    }

private:
    M2MObject* led_object;

    void do_blink(std::vector<uint32_t>* pattern) {
        uint16_t position = 0;

        for (;;) {

            // blink the LED
            led1 = !led1;

            // up the position, if we reached the end of the vector
            if (position >= pattern->size()) {
                // free memory, and exit this function
                delete pattern;
                return;
            }

            // how long do we need to wait before the next blink?
            uint32_t delay_ms = pattern->at(position);

            // Wait requested time, then continue prosessing the blink pattern from next position.
            Thread::wait(delay_ms);
            position++;

        }

    }
};

/*
 * The button contains one property (click count).
 * When `handle_button_click` is executed, the counter updates.
 */
class ButtonResource {
public:
    ButtonResource(): counter(0) {
        // create ObjectID with metadata tag of '3200', which is 'digital input'
        btn_object = M2MInterfaceFactory::create_object("3200");
        M2MObjectInstance* btn_inst = btn_object->create_object_instance();
        // create resource with ID '5501', which is digital input counter
        M2MResource* btn_res = btn_inst->create_dynamic_resource("5501", "Button",
            M2MResourceInstance::INTEGER, true /* observable */);
        // we can read this value
        btn_res->set_operation(M2MBase::GET_ALLOWED);
        // set initial value (all values in mbed Client are buffers)
        // to be able to read this data easily in the Connector console, we'll use a string
        btn_res->set_value((uint8_t*)"0", 1);
    }

    ~ButtonResource() {
    }

    M2MObject* get_object() {
        return btn_object;
    }

    /*
     * When you press the button, we read the current value of the click counter
     * from mbed Device Connector, then up the value with one.
     */
    void handle_button_click() {
        M2MObjectInstance* inst = btn_object->object_instance();
        M2MResource* res = inst->resource("5501");

        // up counter
        counter++;
        pc.printf("handle_button_click, new value of counter is %d\n", counter);
        // serialize the value of counter as a string, and tell connector
        char buffer[20];
        int size = sprintf(buffer,"%d",counter);
        res->set_value((uint8_t*)buffer, size);
    }

private:
    M2MObject* btn_object;
    uint16_t counter;
};

void fall() {
    clicked = true;
    updates.release();
}

void unregister() {
    registered = false;
    updates.release();
}

void error(int error_code) {
    registered = false;
    error_occured = true;
    const char *error;
    switch(error_code) {
        case MbedCloudClient::IdentityError:
            error = "MbedCloudClient::IdentityError";
            break;
        case MbedCloudClient::IdentityInvalidParameter:
            error = "MbedCloudClient::IdentityInvalidParameter";
            break;
        case MbedCloudClient::IdentityOutofMemory:
            error = "MbedCloudClient::IdentityOutofMemory";
            break;
        case MbedCloudClient::IdentityProvisioningError:
            error = "MbedCloudClient::IdentityProvisioningError";
            break;
        case MbedCloudClient::IdentityInvalidSessionID:
            error = "MbedCloudClient::IdentityInvalidSessionID";
            break;
        case MbedCloudClient::IdentityNetworkError:
            error = "MbedCloudClient::IdentityNetworkError";
            break;
        case MbedCloudClient::IdentityInvalidMessageType:
            error = "MbedCloudClient::IdentityInvalidMessageType";
            break;
        case MbedCloudClient::IdentityInvalidMessageSize:
            error = "MbedCloudClient::IdentityInvalidMessageSize";
            break;
        case MbedCloudClient::IdentityCertOrKeyNotFound:
            error = "MbedCloudClient::IdentityCertOrKeyNotFound";
            break;
        case MbedCloudClient::IdentityRetransmissionError:
            error = "MbedCloudClient::IdentityRetransmissionError";
            break;
        case MbedCloudClient::ConnectErrorNone:
            error = "MbedCloudClient::ConnectErrorNone";
            break;
        case MbedCloudClient::ConnectAlreadyExists:
            error = "MbedCloudClient::ConnectAlreadyExists";
            break;
        case MbedCloudClient::ConnectBootstrapFailed:
            error = "MbedCloudClient::ConnectBootstrapFailed";
            break;
        case MbedCloudClient::ConnectInvalidParameters:
            error = "MbedCloudClient::ConnectInvalidParameters";
            break;
        case MbedCloudClient::ConnectNotRegistered:
            error = "MbedCloudClient::ConnectNotRegistered";
            break;
        case MbedCloudClient::ConnectTimeout:
            error = "MbedCloudClient::ConnectTimeout";
            break;
        case MbedCloudClient::ConnectNetworkError:
            error = "MbedCloudClient::ConnectNetworkError";
            break;
        case MbedCloudClient::ConnectResponseParseFailed:
            error = "MbedCloudClient::ConnectResponseParseFailed";
            break;
        case MbedCloudClient::ConnectUnknownError:
            error = "MbedCloudClient::ConnectUnknownError";
            break;
        case MbedCloudClient::ConnectMemoryConnectFail:
            error = "MbedCloudClient::ConnectMemoryConnectFail";
            break;
        case MbedCloudClient::ConnectNotAllowed:
            error = "MbedCloudClient::ConnectNotAllowed";
            break;
        case MbedCloudClient::ConnectSecureConnectionFailed:
            error = "MbedCloudClient::ConnectSecureConnectionFailed";
            break;
        case MbedCloudClient::ConnectDnsResolvingFailed:
            error = "MbedCloudClient::ConnectDnsResolvingFailed";
            break;
#ifdef MBED_CLOUD_CLIENT_SUPPORT_UPDATE
        case MbedCloudClient::UpdateWarningCertificateNotFound:
            error = "MbedCloudClient::UpdateWarningCertificateNotFound";
            break;
        case MbedCloudClient::UpdateWarningCertificateInvalid:
            error = "MbedCloudClient::UpdateWarningCertificateInvalid";
            break;
        case MbedCloudClient::UpdateWarningSignatureInvalid:
            error = "MbedCloudClient::UpdateWarningSignatureInvalid";
            break;
        case MbedCloudClient::UpdateWarningVendorMismatch:
            error = "MbedCloudClient::UpdateWarningVendorMismatch";
            break;
        case MbedCloudClient::UpdateWarningClassMismatch:
            error = "MbedCloudClient::UpdateWarningClassMismatch";
            break;
        case MbedCloudClient::UpdateWarningDeviceMismatch:
            error = "MbedCloudClient::UpdateWarningDeviceMismatch";
            break;
        case MbedCloudClient::UpdateWarningURINotFound:
            error = "MbedCloudClient::UpdateWarningURINotFound";
            break;
#endif
        default:
            error = "UNKNOWN";
    }
    pc.printf("\nError occured  : %s\n", error);
    pc.printf("\nError code  : %d\n", error_code);
}

void toggleLed() {
    led1 = !led1;
}

void client_registered() {
    registered = true;
    pc.printf("registered\n");


#ifdef MBED_APPLICATION_SHIELD
    static const ConnectorClientServerInfo* server = NULL;

    if (server == NULL)
    {
        server = client.server_info();
        char endpoint_buffer[26] = { 0 };

        memcpy(endpoint_buffer, server->endpoint_name.c_str(), 25);

        lcd->cls();
        lcd->locate(0, 3);
        lcd->printf("Cloud Client: Ready");

        lcd->locate(0, 15);
        lcd->printf("%s", endpoint_buffer);

        /* Turn off red LED to signal device is ready */
        DigitalOut ext_red(D5, 1);
    }
#endif

    print_heap_stats();
}

void client_unregistered() {
    registered = false;
    pc.printf("unregistered\n");
}

int main() {

    pc.baud(115200);

    mbed_trace_init();
    mbed_trace_mutex_wait_function_set( serial_out_mutex_wait );
    mbed_trace_mutex_release_function_set( serial_out_mutex_release );

#ifdef MYCLOUDAPP
    set_time(1486961386);
    pc.printf("starting thread1 %s:%d-->%s()\n", __FILE__, __LINE__, __func__);
    t_oled.start(oled);
    pc.printf("starting thread2 %s:%d-->%s()\n", __FILE__, __LINE__, __func__);
    t_mag.start(mag);
    pc.printf("starting thread3 %s:%d-->%s()\n", __FILE__, __LINE__, __func__);
    t_acc.start(acc);
#endif

#ifdef MBED_APPLICATION_SHIELD
    /* Initialize the LCD on the mbed App Shield if present */
    lcd = new C12832(D11, D13, D12, D7, D10);

    /* Keep the red LED on */
    DigitalOut ext_red(D5, 0);

    /* Clear screen and write status */
    lcd->cls();
    lcd->locate(0, 3);
    lcd->printf("Cloud Client: Initializing");
#endif

    btn.fall(&fall);
    cls.fall(&unregister);

    // magic string used from testcase too
    pc.printf("Starting example client\n");

    // Print some statistics of the object sizes and heap memory consumption
    // if the MBED_HEAP_STATS_ENABLED is defined.
    // print_m2mobject_stats();

    print_heap_stats();

    int connect_success = -1;
#if MBED_CONF_APP_NETWORK_INTERFACE == WIFI
    pc.printf("WiFi is NOT yet supported..Exiting the application\n");
    return 1;
#elif MBED_CONF_APP_NETWORK_INTERFACE == ETHERNET
    pc.printf("Using Ethernet\n");
    connect_success = eth.connect();
    network_handler = &eth;
#endif
#ifdef MESH
    pc.printf("Mesh is NOT yet supported..Exiting the application\n");
    return 1;
#endif

    const char *ip_addr = network_handler->get_ip_address();
    const char *mac_addr = network_handler->get_mac_address();
    if (ip_addr && mac_addr) {
        pc.printf("IP address %s\n", ip_addr);
        pc.printf("MAC address %s\n", mac_addr);
    } else {
        pc.printf("[main] No IP/MAC address\n");
        connect_success = -1;
    }
    if (connect_success == 0) {
        pc.printf("Connected to Network successfully\n");
    } else {
        pc.printf("Connection to Network Failed %d!\n", connect_success);
        pc.printf("Exiting application\n");
        return 1;
    }

    print_heap_stats();

    Ticker status_ticker;
    status_ticker.attach(&toggleLed, 1.0f);

    pc.printf("Start simple mbed Cloud Client\n");
    print_heap_stats();

#ifndef MBED_CONF_APP_DEVELOPER_MODE
#ifdef MBED_APPLICATION_SHIELD
    lcd->cls();
    lcd->locate(0, 3);
    lcd->printf("Cloud Client: Provisioning");
#endif

    /////////// Device Factory Flow ///////////
    Thread *factoryFlow;
    osStatus rtosStatus;

    // kick the factory flow in a new task
    factoryFlow = new Thread(osPriorityHigh, FACTORY_STACK_SIZE);
    rtosStatus = factoryFlow->start(provision_device);
    if (rtosStatus != osOK) {
        pc.printf("Failed forking device factory thread (rc = %d)\n", rtosStatus);
        return 1;
    }

    /**
     * The main task sits and waits until device is provisioned.
     */
    do {
        Thread::wait(1000);
    } while (provisioned == false);

    rtosStatus = factoryFlow->join();
    if (rtosStatus != osOK) {
        pc.printf("Failed joining device factory thread (rc = %d)\n", rtosStatus);
        return 1;
    }

    if (device_provision_status == DEVICE_PROVISION_FAILED) {
        pc.printf("Provisioning Failed. Exiting application...\n");
        return 1;
    } else if (device_provision_status == DEVICE_PROVISIONED) {
        pc.printf("Your device is provisioned. Continuing.\n");
    } else if (device_provision_status == DEVICE_ALREADY_PROVISIONED) {
        pc.printf("Your device is already provisioned. Continuing.\n");
    }
    print_heap_stats();
#endif

#ifdef MBED_APPLICATION_SHIELD
    lcd->cls();
    lcd->locate(0, 3);
    lcd->printf("Cloud Client: Connecting");
#endif

    print_heap_stats();

    bool setup = false;

    M2MObjectList obj_list;
    // we create our button and LED resources
    ButtonResource button_resource;
    LedResource led_resource;

#ifdef MBED_CLOUD_CLIENT_UPDATE_ID
    /* When this configuration flag is set, the manufacturer, model number
       and serial number is taken from update_default_resources.c
    */
#else
    M2MDevice *device_object = M2MInterfaceFactory::create_device();
    // make sure device object was created successfully
    if (device_object) {
        // add resourceID's to device objectID/ObjectInstance
        device_object->create_resource(M2MDevice::Manufacturer, "Manufacturer");
        device_object->create_resource(M2MDevice::DeviceType, "Type");
        device_object->create_resource(M2MDevice::ModelNumber, "ModelNumber");
        device_object->create_resource(M2MDevice::SerialNumber, "SerialNumber");

        obj_list.push_back(device_object);
    }
#endif

    obj_list.push_back(button_resource.get_object());
    obj_list.push_back(led_resource.get_object());

    // Add some test resources to measure memory consumption.
    // This code is activated only if MBED_HEAP_STATS_ENABLED is defined.
    create_m2mobject_test_set(obj_list);

    client.add_objects(obj_list);
    client.on_registered(&client_registered);
    client.on_unregistered(&client_unregistered);
    client.on_error(&error);
    client.set_update_callback(&led_resource);

    pc.printf("Connecting to %s:%d\n", MBED_CLOUD_CLIENT_CLOUD_ADDRESS, MBED_CLOUD_CLIENT_CLOUD_PORT);
    print_heap_stats();

    setup = client.setup(network_handler);

    if (!setup) {
        pc.printf("Client setup failed\n");
        return 1;
    }

#ifdef MBED_CLOUD_CLIENT_SUPPORT_UPDATE
    /* Set callback functions for authorizing updates and monitoring progress.
       Code is implemented in update_ui_example.cpp

       Both callbacks are completely optional. If no authorization callback
       is set, the update process will procede immediately in each step.
    */
    client.set_update_authorize_handler(update_authorize);
    client.set_update_progress_handler(update_progress);
#endif

    registered = true;

    while (true) {
#ifdef MYCLOUDAPP
        print_current_time();
#endif
        updates.wait(25000);
        if (registered) {
            if (!clicked) {
// In case you are using UDP mode, to overcome NAT firewall issue , this example
// application sends keep alive pings every 25 seconds so that the application doesnt
// lose network connection over UDP. In case of TCP, this is not required as long as
// TCP keepalive is properly configured to a reasonable value, default for mbed Cloud
// Client is 300 seconds.
#if defined(MBED_CLOUD_CLIENT_TRANSPORT_MODE_UDP) || \
    defined(MBED_CLOUD_CLIENT_TRANSPORT_MODE_UDP_QUEUE)
                client.keep_alive();
#endif
            }
        } else {
            break;
        }
        if (clicked) {
            clicked = false;
            button_resource.handle_button_click();
        }
    }
    client.close();
    status_ticker.detach();
}
