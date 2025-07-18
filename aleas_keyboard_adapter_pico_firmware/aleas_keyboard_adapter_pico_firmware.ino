/*********************************************************************
 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 Copyright (c) 2019 Ha Thach for Adafruit Industries
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#include "usbh_helper.h"

//#include <Adafruit_NeoPixel.h>
//#define NEOPIXEL 16
//Adafruit_NeoPixel pixels(1, NEOPIXEL, NEO_GRB + NEO_KHZ800);

#define LANGUAGE_ID 0x0409  // English

// holding device descriptor
tusb_desc_device_t desc_device;

#include "keyboard_data.h"

static uint32_t r2r_mask = 0b1110001111111111  ;
static bool shift_key = 0 ;
static bool ctrl_key = 0 ;
static bool alt_key = 0 ;
static bool keyboard_detect = 0 ;
static bool mouse_detect = 0 ;
static uint8_t keyboard_inst = 0;
static uint8_t keyboard_dev = 0;
static uint8_t mouse_inst = 0;
static uint8_t mouse_dev = 0;
static uint8_t mouse_btn = 0 ;
static uint8_t mouse_x = 0 ;
static uint8_t mouse_y = 0 ;
static uint8_t keycodes[6] = {0,0,0,0,0,0};

// the setup function runs once when you press reset or power the board
void setup()
{
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // wait for native usb
  Serial.println("ALEA's USB Keyboard Adapter for Sinclair QL V 0.1");
  pinMode(10,OUTPUT);
  digitalWrite(10, HIGH);
  pinMode(0,OUTPUT);
  pinMode(1,OUTPUT);
  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(4,OUTPUT);
  pinMode(5,OUTPUT);
  pinMode(6,OUTPUT);
  pinMode(7,OUTPUT);
  pinMode(8,OUTPUT);
  pinMode(9,OUTPUT);
  pinMode(13,OUTPUT);
  pinMode(14,OUTPUT);
  pinMode(15,OUTPUT);
  gpio_put_masked(r2r_mask, 0);
  //pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  //pixels.setPixelColor(0, pixels.Color(128, 0, 0));
  //pixels.show(); 
}

void loop()
{
//  static int c=0;
//  static bool led=0;
  int16_t tecla =0;
  uint32_t out= 0;
  digitalWrite(10, HIGH);
  if ( keyboard_detect == 1 ) {
    if (keycodes[0]>1){
      tecla = keycode2QL[ (keycodes[0]) ][shift_key==0? 0 : 1] ;
      if (keycodes[0]==42) {ctrl_key=1;}
      if (shift_key==1) {tecla+=64;}
      if (ctrl_key==1) {tecla+=128;}
      if (alt_key==1) {tecla+=256;}
      out = (tecla & 0b1111111111) | (tecla & 0b1110000000000)<<3 ;
      gpio_put_masked(r2r_mask, out);
      digitalWrite(10, LOW);
  
      Serial.print("Keycode: "); Serial.print(keycodes[0]);
      Serial.print(" Tecla ") ; Serial.printf("%s",keycode2ST[keycodes[0]] );
      Serial.print(" QL: ") ; Serial.print(keycode2QL[ (keycodes[0]) ][shift_key==0? 0 : 1]);
      if (shift_key==1) {Serial.print("+64");}
      if (ctrl_key==1) {Serial.print("+128");}
      if (alt_key==1) {Serial.print("+256");}
      Serial.print("=");
      Serial.print(tecla);
      Serial.print(" => ");
      Serial.println(out);
    } 
  } 
  //Serial.flush();
  Serial.print(".");

  delay(10); //si no, no salen los serial del core1
/*  c++;
  if (c>10000) {
    c=0;
    if (led == HIGH) { led=LOW;} else {led=HIGH;}
    pixels.setPixelColor(0, pixels.Color(0, led==HIGH?128:0, 0));
    pixels.show(); 
  }
*/
}

// core1's setup
void setup1() {
  //while ( !Serial ) delay(10);   // wait for native usb
  // configure pio-usb: defined in usbh_helper.h
  rp2040_configure_pio_usb();

  // run host stack on controller (rhport) 1
  // Note: For rp2040 pico-pio-usb, calling USBHost.begin() on core1 will have most of the
  // host bit-banging processing works done in core1 to free up core0 for other works
  USBHost.begin(1);
}

// core1's loop
void loop1()
{
  USBHost.task();
}

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+
extern "C" {
// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  (void)desc_report;
  (void)desc_len;

  // Interface protocol (hid_interface_protocol_enum_t)
  const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);

  if (itf_protocol==1) {
    keyboard_detect = 1;
    keyboard_inst = instance;
    keyboard_dev = dev_addr;
    char tempbuf[256];
    int count = sprintf(tempbuf, "[%04x:%04x][%u] HID Interface is %u, Protocol = %s\r\n", vid, pid, dev_addr, instance, protocol_str[itf_protocol]);
    Serial.println((char*)tempbuf);
  }
  if (itf_protocol==2) {
    mouse_detect = 1;
    mouse_inst = instance;
    mouse_dev = dev_addr;
    char tempbuf[256];
    int count = sprintf(tempbuf, "[%04x:%04x][%u] HID Interface is %u, Protocol = %s\r\n", vid, pid, dev_addr, instance, protocol_str[itf_protocol]);
    Serial.println((char*)tempbuf);
  }

  // Receive report from boot keyboard & mouse only
  // tuh_hid_report_received_cb() will be invoked when report is available
  
  if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD || itf_protocol == HID_ITF_PROTOCOL_MOUSE)
  {
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      Serial.println("Error: cannot request report");
    }
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  if ((mouse_inst == instance) && (mouse_dev == dev_addr)){
    mouse_detect = 0;
    Serial.print("Mouse [");
    Serial.print(dev_addr);
    Serial.print("] interface ");
    Serial.print(instance);
    Serial.println(" Disconected ");
  }
  if ((keyboard_inst == instance) && (keyboard_dev = dev_addr)){
    keyboard_detect = 0;
    Serial.print("Keyboard [");
    Serial.print(dev_addr);
    Serial.print("] interface ");
    Serial.print(instance);
    Serial.println(" Disconected ");
  }
}

// convert hid keycode to ascii and print via usb device CDC (ignore non-printable)
void process_kbd_report(uint8_t dev_addr, hid_keyboard_report_t const *report)
{
  for(uint8_t i=0; i<6; i++)
  {
    keycodes[i]=report->keycode[i];
  }
  shift_key = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
  ctrl_key = report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL);
  alt_key = report->modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT);
}

// send mouse report to usb device CDC
void process_mouse_report(uint8_t dev_addr, hid_mouse_report_t const * report)
{
  mouse_btn = report->buttons ;
  mouse_x = report->x;
  mouse_y = report->y; 
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) len;
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  switch(itf_protocol)
  {
    case HID_ITF_PROTOCOL_KEYBOARD:
      process_kbd_report(dev_addr, (hid_keyboard_report_t const*) report );
      break;
    case HID_ITF_PROTOCOL_MOUSE:
      process_mouse_report(dev_addr, (hid_mouse_report_t const*) report );
      break;
    default: 
      break;
  }
   if ( !tuh_hid_receive_report(dev_addr, instance) ) //no idea, but need to mark report as received.
  {
    Serial.println("Error: cannot request report");
  } 
}

} //extern C
