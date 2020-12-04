#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <LiquidCrystal.h>

#define DEFAULT_WATER_LVL 300
#define DEFAULT_TEMP_LVL 25

#define DHTPIN A8 // analog pin A8
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

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
unsigned int resval = 0;  // holds the value
int respin = 11; // water sensor pin used
float tempval = 0; // temperature value
float humid = 0; // ADDED: humidity variable.
bool old_buttonstate = 0;
bool new_buttonstate = 0;
bool disabledstate = 0; // start off disabled
int push_button = 8; // location of pin for push button

// DHT initialization
//static const int DHT_SENSOR_PIN = 2; // A12 same as tempin;
// DHT_nonblocking dht_sensor( tempin, DHT_SENSOR_TYPE ); // DHT_SENSOR_PIN changed to tempin it's the sensor pin.

//LCD initialization
LiquidCrystal lcd(23, 24, 25, 26, 27, 28); //(RS, E, D4, D5, D6, D7 )
// function prototypes
void adc_init();
unsigned int adc_read(unsigned char adc_channel_num);
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
  *ddr_h = 0x00; // 0000 0000: all input
  *port_h = 0xFF; // all have pullup resistor
  lcd.begin(16,2); // size of LCD
  lcd.setCursor(0,0); // set it at top-left most position 
  dht.begin();
  adc_init(); // initialize analog-to-digital conversion
}

/*******************************************************************************************
 * Function: loop()
 * Description: Runs again and again in the arduino.
 * Returns: nothing
 */
void loop() {
  Button_chk();

  if( disabledstate == 1 ) { // enabled state PH6

    humid = dht.readHumidity(); //reads humidity
    tempval = dht.readTemperature(true); //reads temp in F
    
    //unsigned int adc_reading = adc_read(respin); // start getting reading
    resval = adc_read(respin); // Read data from analog pin and store it to resval variable

    Serial.println(resval);
    Serial.println(tempval);

    // change lights based on water level
    control_lights( resval, tempval );

    delay(1000); // delay to see the ERROR message AND the readings properly

    lcd.clear();  // to make sure it always start at the initial position
    lcd.print("Temp: "); // shorten to temp to have more space
    lcd.print(tempval); // print temperature from DHT function
    lcd.print((char)223);
    lcd.print("C");
    lcd.setCursor(0,1); // set cursor to the next line.
    lcd.print("Humidity: "); 
    lcd.print(humid); // print humidity from DHT function
    lcd.print("%");
  }
  else { // disabled state

    // enable PB5, which should be the yellow light
    //*port_b &= 0x00; // turn off all lights
    *port_b = 0x20; // Yellow

    lcd.clear();  // to make sure it always start at the initial position
    lcd.print("    DISABLED"); // no monitoring is happening
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
  //*port_b &= 0x00; // turn off all lights

  if ( waterLevel <= DEFAULT_WATER_LVL )
    {
      Serial.println("Water Level: TOO LOW"); // Red
      *port_b = 0x40;
      // *port_b &= 0x40; turn off motor, need to know which bit is for motor...
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
      // turn ON motor, decide which port and bit for motor
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
