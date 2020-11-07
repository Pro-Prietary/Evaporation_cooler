#include <Arduino.h>
#include <Wire.h>
//#include <dht.h> << not sure why it's not working

volatile unsigned char* port_b = (unsigned char*) 0x25;
volatile unsigned char* ddr_b  = (unsigned char*) 0x24;
volatile unsigned char* pin_b  = (unsigned char*) 0x23;

volatile unsigned char* port_f = (unsigned char*) 0x31;
volatile unsigned char* ddr_f  = (unsigned char*) 0x30;
volatile unsigned char* pin_f  = (unsigned char*) 0x2F;

volatile unsigned char* port_k = (unsigned char*) 0x108;
volatile unsigned char* ddr_k  = (unsigned char*) 0x107;
volatile unsigned char* pin_k  = (unsigned char*) 0x106;

int resval = 0;  // holds the value
int respin = A11; // sensor pin used
int tempval = 0; // temperature value
int tempin = A12; // temperature pin used

void setup() {

  // start the serial console
  Serial.begin(9600);
  *ddr_b = 0xF0;
}

void loop() {

  resval = analogRead(respin); //Read data from analog pin and store it to resval variable
  tempval = analogRead(tempin); //Read data from analong pin and sotre it in tempval

  if (resval<=100)
    {
      Serial.println("Water Level: EMPTY");
      *port_b = 0x40;
    }
  else if (resval>100 && resval<=200)
    {
      Serial.println("Water Level: TOO LOW");
      *port_b = 0x40;
    }
  else if (resval>200 && resval<=250)
    {
      Serial.println("Water Level: IDLE"); // Green
      *port_b = 0x80;
    }
  else if (resval>250)
    {
      Serial.println("Water Level: High");
    }



  delay(1000);
}
