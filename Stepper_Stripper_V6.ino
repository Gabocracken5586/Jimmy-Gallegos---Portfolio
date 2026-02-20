
//For use with Adafruit ESP32-S3 TFT Reverse with 2 OMC stepper online iCL Series NEMA 17 motors

#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#define Set_Strip 0
#define Set_Clamp 1
#define Strip 2

// Jimmy Modification
const int gatePin = 5; // Pin to be used needs to be desided
int gateState = 1;
int center_steps = 12400;

//Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x60);

//set up the tft display
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

//edit everything up here so you only have to change things in one place
int clampdist = 1000;     //1mm pitch lead screw, 500 for 2mm pitch lead screw
int pulldist = 600;       //1mm pitch lead screw, 600 for 2mm pitch lead screw
int knifedist = 5;        //step distance to jog the stripper the the other end
int hold1 = 10;
int hold2 = 200; 
int hold3 = 1500;
int x;


void setup() {
  // put your setup code here, to run once:

  //setting up the new stepper motors
  
  //strip motor
  pinMode(10,OUTPUT);     //set Pin10 as DIR
  pinMode(9,OUTPUT);      //set Pin9 as PUL

  //clamp motor
  pinMode(12,OUTPUT);     //set Pin12 as DIR
  pinMode(11,OUTPUT);     //set Pin11 as PUL

  //shared disable pin
  pinMode(6, OUTPUT);     //set Pin8 as Enable

  //setup button presses
  pinMode(Strip, INPUT_PULLDOWN);
  pinMode(Set_Clamp, INPUT_PULLDOWN);
  pinMode(Set_Strip, INPUT_PULLUP);

  //Setting the built in LED
  pinMode(LED_BUILTIN, OUTPUT);

  //set up serial library at 9600 bps
  Serial.begin(9600);

  //Start of display stuff
  //turn on backlite
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  //turn on the TFT/I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  //initialize TFT
  tft.init(135, 240);     //INit ST7789 240x135
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLUE);

  //White MITAS text
  tft.setCursor(85, 15);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("MITAS");
  
  //White ELECTRONICS text
  tft.setCursor(50, 45);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("ELECTRONICS");

  //Yellow code version---------------------------------------------update this text to match the file name-------------------------------------------------------
  tft.setCursor(60, 75);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.println("Stepper_Stripper_V6");

  //White Initializing text
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, 125);
  tft.setTextSize(1);
  tft.println("Initializing");

  delay(2000);

  //these three add the initializing dots
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, 125);
  tft.setTextSize(1);
  tft.println("Initializing.");

  delay(1000)
;

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, 125);
  tft.setTextSize(1);
  tft.println("Initializing..");

  delay(1000);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(15, 125);
  tft.setTextSize(1);
  tft.println("Initializing...");

  delay(1000);

  //sets the background to blue
  tft.setCursor(0,0);
  tft.fillScreen(ST77XX_BLUE);

  //White Set text next to D0 button
  tft.setCursor(10, 5);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("Set");

  //white inner strip text next to D1 button
  tft.setCursor(10, 60);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("Inner Strip");

  //white Outer Strip text next to D2 button
  tft.setCursor(10, 115);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("OuterStrip");

  //Jimmy Modification: Initializing pin for gate:
  pinMode (gatePin, INPUT);

}

/*void enableMotor() {
  digitalWrite(6, LOW);     // Enable mtoor driver
}

void disableMotor() {
  digitalWrite(6, HIGH);     // Disable motor driver 
}*/

int i;

void loop() 
{
  // put your main code here, to run repeatedly:

  boolean OuterStrip = digitalRead(Strip);
  boolean InnerStrip = digitalRead(Set_Clamp);
  boolean Set = digitalRead(Set_Strip);

  if(Set == LOW && OuterStrip == HIGH && InnerStrip == LOW)     //Set + Outer Strip - sets strip opening-----
  {     
    //enableMotor();        
    //start clamp motor movement
    digitalWrite(12,LOW); // set high level direction
    for(x = 0; x < 20700; x++) //distance traveled
      {
        digitalWrite(11,HIGH); // Output high
        delayMicroseconds(50); // set rotate speed
        digitalWrite(11,LOW); // Output low
        delayMicroseconds(50); // set rotate speed
      }
    delay(50); //short pause
    //disableMotor();
    //end clamp motor movement
  }

  if(OuterStrip == HIGH && Set == HIGH && InnerStrip == LOW)     //Outer Strip Sequence--------
  {            
    //enableMotor();
    delay(100);
    //start clamp motor movement
    digitalWrite(12,LOW); // set high level direction
    for(x = 0; x < 15336; x++) //distance traveled
      {
        digitalWrite(11,HIGH); // Output high
        delayMicroseconds(70); // set rotate speed
        digitalWrite(11,LOW); // Output low
        delayMicroseconds(70); // set rotate speed
      }
    delay(50); //short pause
    //end clamp motor movement

    //start stripper motor movement
    digitalWrite(10,HIGH); // set high level direction
    for(x = 0; x < 7200; x++) //distance traveled
      {
        digitalWrite(9,HIGH); // Output high
        delayMicroseconds(50); // set rotate speed
        digitalWrite(9,LOW); // Output low
        delayMicroseconds(50); // set rotate speed
      }
    delay(50); //short pause
    //end stripper motor movement

    //start clamp motor movement
    digitalWrite(12,HIGH); // set high level direction
    while (gateState != 1)//distance traveled 
      {
        //Jimmy Modification: reads gate sensor
        gateState = digitalRead(gatePin);
        //Jimmy Modification: breaks loop if gate sensor engaged
        digitalWrite(11,HIGH); // Output high
        delayMicroseconds(70); // set rotate speed
        digitalWrite(11,LOW); // Output low
        delayMicroseconds(70); // set rotate speed

      }
    // Adds the additional steps needed to reach the center point
    for(x=0; x < center_steps; x++)
    {
        digitalWrite(11,HIGH);
        delayMicroseconds(50);
        digitalWrite(11,LOW);
        delayMicroseconds(50);
    }

    delay(50); //short pause
    //end clamp motor movement

    digitalWrite(10,LOW); // set high level direction
    for(x = 0; x < 7200; x++)  
      {
        digitalWrite(9,HIGH);
        delayMicroseconds(50);
        digitalWrite(9,LOW);
        delayMicroseconds(50);
      }
    delay(50);
    //disableMotor();

   
  }

  if(Set == LOW && InnerStrip == HIGH && OuterStrip == LOW)     //Jog stripper motor back------------
  {                           //Set for Inner Strip
    //enableMotor();
    delay(100);
    //start stripper motor movement
    digitalWrite(10,HIGH); // set high level direction
    for(x = 0; x < 50; x++) //distance traveled
      {
        digitalWrite(9,HIGH); // Output high
        delayMicroseconds(50); // set rotate speed
        digitalWrite(9,LOW); // Output low
        delayMicroseconds(50); // set rotate speed
      }
    delay(50); //short pause
    //disableMotor();
    //end stripper motor movement
  }

  if(InnerStrip == HIGH && Set == HIGH && OuterStrip == LOW)     //Inner strip-----
  {                          //Inner Strip Sequence
     //enableMotor(); 
     delay(100);
      //start clamp motor movement
    digitalWrite(12,HIGH); // set high level direction
    for(x = 0; x < 20500; x++) //distance traveled 14536, 7-24-25 changed step count from 20700 to 20500
      {
        digitalWrite(11,HIGH); // Output high
        delayMicroseconds(50); // set rotate speed
        digitalWrite(11,LOW); // Output low
        delayMicroseconds(50); // set rotate speed
      }
    delay(50); //short pause
    //end clamp motor movement

    //start stripper motor movement
    digitalWrite(10,HIGH); // set high level direction
    for(x = 0; x < 4800; x++) //distance traveled
      {
        digitalWrite(9,HIGH); // Output high
        delayMicroseconds(50); // set rotate speed
        digitalWrite(9,LOW); // Output low
        delayMicroseconds(50); // set rotate speed
      }
    delay(50); //short pause
    //end stripper motor movement

    //start clamp motor movement
    digitalWrite(12,LOW); // set high level direction
    while(gateState != 1) //distance traveled  14536
      {
        //Jimmy Modification: reads gate sensor
        gateState = digitalRead(gatePin);
        //Jimmy Modification: breaks loop if gate sensor engaged
        digitalWrite(11,HIGH); // Output high
        delayMicroseconds(50); // set rotate speed
        digitalWrite(11,LOW); // Output low
        delayMicroseconds(50); // set rotate speed
      
      }
    //Adds the additional steps needed to be centered
    for (x=0; x < center_steps; x++)
      {
        digitalWrite(11,HIGH);
        delayMicroseconds(50);
        digitalWrite(11,LOW);
        delayMicroseconds(50);
      }

    delay(50); //short pause
    //end clamp motor movement

    digitalWrite(10,LOW); // set high level direction
    for(x = 0; x < 4800; x++)  
      {
        digitalWrite(9,HIGH);
        delayMicroseconds(50);
        digitalWrite(9,LOW);
        delayMicroseconds(50);
      }
    delay(50);
    //disableMotor();
  }
 
  else
  {
    //disableMotor();   
  }



}
