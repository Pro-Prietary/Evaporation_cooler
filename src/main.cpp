#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <LiquidCrystal.h>
#include <DS3231.h>
#include <Servo.h>

// Thresholds
#define DEFAULT_WATER_LVL 330
#define DEFAULT_TEMP_LVL 73

//LCD initialization
LiquidCrystal lcd(23, 24, 25, 26, 27, 28); //(RS, E, D4, D5, D6, D7 )

// DHT Initialization
#define DHTPIN A8           // analog pin A8
#define DHTTYPE DHT11       // uses DHT11 sensor
DHT dht(DHTPIN, DHTTYPE);

// Real Time Clock Initialization
DS3231 clock;              // work on the how to use DS1307
RTCDateTime dt;

// Servo Initialization
Servo myservo;             // create servo object to control a servo

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

// ADC register values
volatile unsigned char* my_ADMUX    = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB   = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA   = (unsigned char*) 0x7A;
volatile unsigned int*  my_ADC_DATA = (unsigned int*)  0x78;

// other global values
bool old_buttonstate = 0;
bool new_buttonstate = 0;
bool disabledstate = 0;     // start off disabled
unsigned int resval = 0;    // water level value
int respin = 11;            // pin for water sensor
float tempval = 0;          // temperature value
float humval = 0;           // humidity value
int motval = 0;             // motor off, probably bool but oh well
int serval = 0;             // servo value

// Function prototypes
void adc_init();
unsigned int adc_read(unsigned char adc_channel_num);
void control_lights( int waterLevel, int tempLevel );
void Button_chk ();
void RTC_stamps ();

/*******************************************************************************************
 * Function: setup()
 * Description: Runs once in the arduino as a "start-up configruation"
 * Returns: nothing
 */
void setup()
{
  Serial.begin(9600);
  *ddr_b = 0xFF;        // 1111 1111: all output

  *ddr_k = 0x00;        // 0000 0000: all input

  *ddr_h = 0x00;        // 0000 0000: all input
  *port_h = 0xFF;       // 1111 1111: all have pullup resistor

  lcd.begin(16,2);      // size of LCD
  lcd.setCursor(0,0);   // set it at top-left most position 
  dht.begin();

  //clock.begin();        // start the clock
  //clock.setDateTime(__DATE__, __TIME__); // set date and time automatically

  adc_init();           // initialize analog-to-digital conversion
}

/*******************************************************************************************
 * Function: loop()
 * Description: Runs again and again in the arduino.
 * Returns: nothing
 */
void loop() {
  Button_chk();

  if( disabledstate == 1 ) 
  {
    humval = dht.readHumidity();          // reads humidity
    tempval = dht.readTemperature(true);  // reads temp in F
    
    resval = adc_read(respin);            // read data from analog pin and store it to resval variable

    Serial.println(resval);               // testing 
    Serial.println(tempval);              // testing

    // change lights based on water level and temperature
    control_lights( resval, tempval );

    delay(1000);                // delay to see the ERROR message AND the readings properly

    lcd.clear();                // to make sure it always start at the initial position
    lcd.print("Temp: ");        
    lcd.print(tempval);         
    lcd.print((char)223);
    lcd.print("F");
    lcd.setCursor(0,1);         // set cursor to the next line.
    lcd.print("Humidity: "); 
    lcd.print(humval);          
    lcd.print("%");
  }
  else {
    *port_b = 0x20;            // Yellow light: DISABLE
    *port_b |= 0x08;           // | 0000 (10)00: 
    *port_b &= 0xFD;           // & 1111 11(0)1: turn motor off, bit 1 is motor enabler

    Serial.println("disabled");// testing purposes
    lcd.clear();               // to make sure it always start at the initial position
    lcd.print("    DISABLED"); // no monitoring is happening

    if (motval == 1)
   {
    Serial.println("Motor OFF at: ");
    //RTC_stamps (); 
    motval = 0;       
   }
  }
 delay(1000);
}

/*******************************************************************************************
 * Function: adc_init()
 * Description: sets up registers for Analog-to-Digital conversion for the MC to read
 * Returns: nothing
 */
void adc_init() {
  // setup A register
  // set bit 7 to 1 to enable ADC
  *my_ADCSRA |= 0x80;
  // clear bit 5 to disable ADC trigger mode
  *my_ADCSRA &= 0xDF;
  // clear bit 3 to disable ADC interrupt
  *my_ADCSRA &= 0xF7;
  // clear bits 2-0 to set prescalar selection to slow reading
  *my_ADCSRA &= 0xF8;

  // setup B register
  // clear bit 3 to reset the channel and gain bits
  *my_ADCSRB &= 0xF7;
  // clear bit 2-0 to set free running mode
  *my_ADCSRB &= 0xF8;

  // setup MUX register
  // clear bit 7 for AVCC analog ref.
  *my_ADMUX &= 0x7F;
  // set bit 6 to 1 for AVCC analog ref.
  *my_ADMUX |= 0x40;
  // clear bit 5 for right adjust result
  *my_ADMUX &= 0xDF;
  // clear bit 4-0 to reset chanel and gain bits
  *my_ADMUX &= 0xE0;
}

/*******************************************************************************************
 * Function: adc_read()
 * Description: takes in a number, which corresponds to the ANALOG pins on the arduino. It data from that particular pin, then returns the data
 * Returns: unsigned int
 */
unsigned int adc_read(unsigned char adc_channel_num){
  // clear channel selection bits (MUX 4:0), which is bit 4:0 on ADMUX
  *my_ADMUX &= 0xE0;

  // clear channel selection bits (MUX 5), which is bit 3 on ADCSRB
  *my_ADCSRB &= 0xF7;


  // This if statement chooses which port on arduino to use
  // set the channel number
  if(adc_channel_num > 7)
    {
      // set the channel selection bits, but remove the most significant bit (bit 3)
      adc_channel_num -= 8;

      // set MUX5 on ADCSRB to higher port
      *my_ADCSRB |= 0x08;
    }

  // set channel selection bits
  *my_ADMUX |= adc_channel_num;

  // set bit 6 of ADCSRA to 1 and start a conversion
  *my_ADCSRA |= 0x40;

  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);

  // return result in ADC data register
  return *my_ADC_DATA;
}

/*******************************************************************************************
 * Function: control_lights( waterLevel, tempLevel )
 * Description: takes two int values and manipulates LED lights based on water level and temp level
 * Returns: nothing
 */
void control_lights( int waterLevel, int tempLevel ) {
  if ( waterLevel <= DEFAULT_WATER_LVL )
    {
      *port_b = 0x40;
      *port_b |= 0x08;          // | 0000 (10)00: 
      *port_b &= 0xFD;          // & 1111 11(0)1: turn motor off, bit 1 is motor enabler
      
     if (motval == 1)
      {
      Serial.println("Motor OFF at: ");
      //RTC_stamps (); 
      motval = 0;       
      } 
    
      lcd.clear();              // to make sure it always start at the initial position
      lcd.print("      ERROR"); // error message 
      lcd.setCursor(0,1);       // set cursor to the next line.
      lcd.print("Water is TOO low"); 
    }
  else if ( waterLevel > DEFAULT_WATER_LVL && tempLevel < DEFAULT_TEMP_LVL )
    {
      *port_b = 0x80;           // 10000 0000, Green light: IDLE

      // turn motor OFF
      *port_b |= 0x08;          // | 0000 (10)00: 
      *port_b &= 0xFD;          // & 1111 11(0)1: turn motor off, bit 1 is motor enabler

      if (motval == 1)
      {
      Serial.println("Motor OFF at: ");
      //RTC_stamps (); 
      motval = 0;       
      }
    }
  else if ( waterLevel > DEFAULT_WATER_LVL && tempLevel > DEFAULT_TEMP_LVL )
    {
      *port_b = 0x10;          // Blue light: RUNNING
      *port_b |= 0x08;         // | 0000 (10)00: 
      *port_b |= 0x02;         // | 0000 00(1)0 50(PB3)=8 51(PB2)=9 52(PB1)=10(En)

      if (motval == 0)
      {
      Serial.println("Motor ON at: ");
      //RTC_stamps (); 
      motval = 1;       
      }
    }
}

/*******************************************************************************************
 * Function: Button_chk()
 * Description: checks to see if button has been pressed. This makes the button function like a toggle switch
 * Returns: nothing
 */
void Button_chk ()
{
  // do we really want this as adc_read?
  new_buttonstate = !digitalRead(9); 
  if ( new_buttonstate != old_buttonstate)
  {
    disabledstate = !disabledstate;
    old_buttonstate = 0;
    new_buttonstate = 0;
    delay(500);
  }
}

/*******************************************************************************************
 * Function RTC_stamps ()
 * Description: prints date and time when called for when the motor is on or off
 * Returns: nothing (it only prints in the serial monitor)
 */
void RTC_stamps ()
{
  dt = clock.getDateTime();
  
  Serial.print(dt.year);   Serial.print("-");
  Serial.print(dt.month);  Serial.print("-");
  Serial.print(dt.day);    Serial.print(" ");
  Serial.print(dt.hour);   Serial.print(":");
  Serial.print(dt.minute); Serial.print(":");
  Serial.print(dt.second); Serial.println("");
}