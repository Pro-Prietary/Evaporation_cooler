#include <Arduino.h>
#include <Wire.h>
#include <dht_nonblocking.h> 
#include <LiquidCrystal.h>

#define DEFAULT_WATER_LVL 250
#define DEFAULT_TEMP_LVL 25

// port b register values
volatile unsigned char* port_b = (unsigned char*) 0x25;
volatile unsigned char* ddr_b  = (unsigned char*) 0x24;
volatile unsigned char* pin_b  = (unsigned char*) 0x23;
// port f register values
volatile unsigned char* port_f = (unsigned char*) 0x31;
volatile unsigned char* ddr_f  = (unsigned char*) 0x30;
volatile unsigned char* pin_f  = (unsigned char*) 0x2F;
// port h register values
volatile unsigned char* port_h = (unsigned char*) 0x102;
volatile unsigned char* ddr_h  = (unsigned char*) 0x101;
volatile unsigned char* pin_h  = (unsigned char*) 0x100;
// port k register values
volatile unsigned char* port_k = (unsigned char*) 0x108;
volatile unsigned char* ddr_k  = (unsigned char*) 0x107;
volatile unsigned char* pin_k  = (unsigned char*) 0x106;

// other global values
int resval = 0;  // holds the value
int respin = A11; // water sensor pin used
int tempval = 0; // temperature value
int tempin = A12; // temperature sensor pin used
bool old_buttonstate = 0;
bool new_buttonstate = 0;
bool disabledstate = 0; // start off disabled
int push_button = A8;

//LCD initialization
LiquidCrystal lcd(8,9,4,5,6,7); //(RS, E, D4, D5, D6, D7 )
// function prototypes
void control_lights( int waterLevel, int tempLevel );
void Button_chk ();

/*******************************************************************************************
 * Function: setup()
 * Description: Runs once in the arduino as a "start-up configruation"
 * Returns: nothing
 */
void setup()
{
  Serial.begin(9600);
  *ddr_b = 0xFF;
  *ddr_k = 0x00; // all input
  *ddr_h = 0x00; // all input
  *port_h = 0xFF; // all have pullup resistor
  lcd.begin(16,2); // size of LCD
  lcd.setCursor(0,0); // set it at top-left most position 
}

/*******************************************************************************************
 * Function: loop()
 * Description: Runs again and again in the arduino.
 * Returns: nothing
 */
void loop()
{

  Button_chk();
  if( disabledstate == 1 ) { // enabled state PH6
    Serial.println("enabled");

    //resval = analogRead(respin); // Read data from analog pin and store it to resval variable
    //tempval = analogRead(tempin); // Read data from analog pin and store it in tempval

    Serial.println(resval);
    Serial.println(tempval);

    // change lights based on water level
    control_lights( resval, tempval );

    delay(1000); // delay to see the ERROR message AND the readings properly

    lcd.clear();  // to make sure it always start at the initial position
    lcd.print("Temp: "); // shorten to temp to have more space
    //lcd.print("DHT.temperature"); // print temperature from DHT function
    lcd.print((char)223);
    lcd.print("C");
    lcd.setCursor(0,1); // set cursor to the next line.
    lcd.print("Humidity: "); 
    //lcd.print("DHT.humidity"); // print humidity from DHT function
    lcd.print("%");
    
  }
  else { // disabled state

    // here we print to LCD as disabled
    Serial.println("disabled");

    // enable PB5, which should be the yellow light
    //*port_b &= 0x00; // turn off all lights
    *port_b = 0x20; // Yellow

    lcd.clear();  // to make sure it always start at the initial position
    lcd.print("    DISABLED"); // no monitoring is happening
  }

 delay(1000);
}

/*******************************************************************************************
 * Function: control_lights( waterLevel, tempLevel )
 * Description: takes two int values and manipulates LED lights based on water level and temp level
 * Returns: nothing
 */
void control_lights( int waterLevel, int tempLevel )
{
  //*port_b &= 0x00; // turn off all lights

  if ( waterLevel <= DEFAULT_WATER_LVL )
    {
      Serial.println("Water Level: TOO LOW"); // Red
      *port_b = 0x40;
      lcd.clear();  // to make sure it always start at the initial position
      lcd.print("      ERROR"); // error message 
      lcd.setCursor(0,1); // set cursor to the next line.
      lcd.print("Water is TOO low"); 
    }
  else if ( waterLevel > DEFAULT_WATER_LVL && tempLevel < DEFAULT_TEMP_LVL )
    {
      Serial.println("Water Level: IDLE"); // Green
      *port_b = 0x80;
    }
  else if ( waterLevel > DEFAULT_WATER_LVL && tempLevel > DEFAULT_TEMP_LVL )
    {
      Serial.println("Water Level: RUNNING"); // Blue
      *port_b = 0x10;
    }
}

/**************************************
 * Button Function
 */
void Button_chk ()
{
  new_buttonstate = !analogRead(push_button); //!digitalRead(9); //replace with other pin since I needed pin 9 PWM for LCD
  if ( new_buttonstate != old_buttonstate)
  {
    disabledstate = !disabledstate;
    old_buttonstate = 0;
    new_buttonstate = 0;
    delay(500);
  }
}