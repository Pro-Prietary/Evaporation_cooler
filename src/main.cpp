#include <Arduino.h>
#include <Wire.h>
#include <dht_nonblocking.h> 
#include <LiquidCrystal.h>

// port b register values
volatile unsigned char* port_b = (unsigned char*) 0x25;
volatile unsigned char* ddr_b  = (unsigned char*) 0x24;
volatile unsigned char* pin_b  = (unsigned char*) 0x23;
// port f register values
volatile unsigned char* port_f = (unsigned char*) 0x31;
volatile unsigned char* ddr_f  = (unsigned char*) 0x30;
volatile unsigned char* pin_f  = (unsigned char*) 0x2F;
// port k register values
volatile unsigned char* port_k = (unsigned char*) 0x108;
volatile unsigned char* ddr_k  = (unsigned char*) 0x107;
volatile unsigned char* pin_k  = (unsigned char*) 0x106;

// other global values
int resval = 0;  // holds the value
int respin = A11; // water sensor pin used
int tempval = 0; // temperature value
int tempin = A12; // temperature sensor pin used

// function prototypes
void control_lights( int value );

/*******************************************************************************************
 * Function: setup()
 * Description: Runs once in the arduino as a "start-up configruation"
 * Returns: nothing
 */
void setup()
{
  Serial.begin(9600);
  *ddr_b = 0xFF;
  *ddr_k = 0x00;
}

/*******************************************************************************************
 * Function: loop()
 * Description: Runs again and again in the arduino.
 * Returns: nothing
 */
void loop()
{

  resval = analogRead(respin); // Read data from analog pin and store it to resval variable
  tempval = analogRead(tempin); // Read data from analog pin and store it in tempval


  //Serial.println(resval);
  Serial.println(tempval);
  //  Control_Lights( Resval );

  delay(1000);
}

/*******************************************************************************************
 * Function: control_lights( int )
 * Description: takes int value and manipulates LED lights based on value
 * Returns: nothing
 */
void control_lights( int value )
{
  if (value<=100)
    {
      Serial.println("Water Level: EMPTY");
      *port_b = 0x40;
    }
  else if (value>100 && value<=200)
    {
      Serial.println("Water Level: TOO LOW");
      *port_b = 0x40;
    }
  else if (value>200 && value<=250)
    {
      Serial.println("Water Level: IDLE"); // Green
      *port_b = 0x80;
    }
  else if (value>250)
    {
      Serial.println("Water Level: High");
    }

}
