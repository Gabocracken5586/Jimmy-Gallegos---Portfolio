#include <Arduino.h>
#include <AccelStepper.h>
#include <HX711.h>
#include <HardwareSerial.h>
//Neo Pixel Turn on:
#include <Adafruit_NeoPixel.h>

// Define motor pins
#define PITCH_MOTOR_PIN1 5
#define PITCH_MOTOR_PIN2 6
#define HEIGHT_MOTOR_PIN1 9
#define HEIGHT_MOTOR_PIN2 10
#define EXTRUDE_MOTOR_PIN1 11
#define EXTRUDE_MOTOR_PIN2 12
#define RX_FROM_MASTER 38
#define SLAVE_ID 1

// HX711 pins
#define DT_PIN A1
#define SCK_PIN A3

//Neo Pixel Pin and Set up
#define NEOPIXEL_PIN 33
#define NUMPIXELS    1  

Adafruit_NeoPixel pixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

AccelStepper stepper1(AccelStepper::DRIVER, HEIGHT_MOTOR_PIN1, HEIGHT_MOTOR_PIN2);
AccelStepper stepper2(AccelStepper::DRIVER, PITCH_MOTOR_PIN1, PITCH_MOTOR_PIN2);
AccelStepper stepper3(AccelStepper::DRIVER, EXTRUDE_MOTOR_PIN1, EXTRUDE_MOTOR_PIN2);
HX711 scale;

HardwareSerial masterSerial(1);

float LOAD_INCREASE_THRESHOLD = 100.0;
float initialLoad = 0.0;
unsigned long lastReadTime = 0;
const unsigned long readInterval = 100;

const int forwardSpeed = 1000;
const int forwardAcceleration = 1000;
const int reverseSpeed = 1000;
const int reverseAcceleration = 1000;
long dynamicSteps = 32000;

long connectorSpeed;          // = 5.25;
const int connectorAcceleration = 1000;
long connectorSteps;          // = 140;

long gbSpeed;          //= 26.4;
const int gbAcceleration = 1000;
long gbSteps;         // = 713.7;



void setup() {
  Serial.begin(115200);
  Serial.println("Slave: Booting...");
  masterSerial.begin(9600, SERIAL_8N1, RX_FROM_MASTER, -1); // RX only

  scale.begin(DT_PIN, SCK_PIN);
  scale.set_scale(2280.f);
  scale.tare();

  pinMode(PITCH_MOTOR_PIN1, OUTPUT);
  pinMode(PITCH_MOTOR_PIN2, OUTPUT);
  pinMode(HEIGHT_MOTOR_PIN1, OUTPUT);
  pinMode(HEIGHT_MOTOR_PIN2, OUTPUT);
  pinMode(EXTRUDE_MOTOR_PIN1, OUTPUT);
  pinMode(EXTRUDE_MOTOR_PIN2, OUTPUT);

  //Neo Pixel Begin
  pixel.begin();

}

void loop() {
  stepper1.run();
  stepper2.run();
  stepper3.run();

  if (masterSerial.available()) {
      //Truns on Neo Pixel Green if Command Received
      pixel.setPixelColor(0,pixel.Color(0,255,0));
      pixel.show();
      delay(500);
      pixel.setPixelColor(0,pixel.Color(0,0,0));
      pixel.show();



    String command = masterSerial.readStringUntil('\n');
    command.trim(); // clean up whitespace
    Serial.print("Received command: ");
    Serial.println(command);

    if (command == "TARE") {    //------------------------TARE BUTTON----------------
      scale.tare();
      Serial.println("Load cell tared.");
    }

    else if (command.startsWith("SET_THRESHOLD ")) {    //------Threshold Button------
      float threshold = command.substring(14).toFloat();
      LOAD_INCREASE_THRESHOLD = threshold;
      Serial.print("Load increase threshold set to: ");
      Serial.println(LOAD_INCREASE_THRESHOLD);
    }
    
    else if (command.startsWith("extrude")) {
      float speed;
      int steps;
      if (sscanf(command.c_str(), "extrude %f %d", &speed, &steps) == 2) {
          stepper3.setMaxSpeed(speed);
          stepper3.setAcceleration(1000);  // Optional: tune as needed
          stepper3.move(steps);
      }
    }

    else if (command.startsWith("connectorpaste ")) {   //-------connector Button----------
      long value = command.substring(15).toInt();
      dynamicSteps = value;
      Serial.println("Starting connector paste routine...");
      Serial.print("dynamicSteps: "); Serial.println(dynamicSteps);

      long value1 = command.substring(20).toInt();
      connectorSpeed = value1;
      
      long value2 = command.substring(25).toInt();
      connectorSteps = value2;

      initialLoad = scale.get_units();
      stepper1.setMaxSpeed(forwardSpeed);
      stepper1.setAcceleration(forwardAcceleration);
      stepper1.setSpeed(forwardSpeed);

      while (true) {    //-----move extruder tip down
        stepper1.runSpeed();
        if (millis() - lastReadTime >= readInterval) {
          lastReadTime = millis();
          float load = scale.get_units();
          if (load >= initialLoad + LOAD_INCREASE_THRESHOLD) {
            stepper1.stop();
            delay(50);
            break;
          }
        }
      }

      stepper1.setMaxSpeed(reverseSpeed);   //-----move extruder tip up to application height
      stepper1.setAcceleration(reverseAcceleration);
      stepper1.move(-250);
      while (stepper1.distanceToGo() != 0) stepper1.run();

      stepper3.setMaxSpeed(forwardSpeed);   //-----prime extruder
      stepper3.setAcceleration(forwardAcceleration);
      stepper3.move(1100);
      while (stepper3.distanceToGo() != 0) stepper3.run();

      stepper2.setMaxSpeed(forwardSpeed);   //-----dispense paste 
      stepper2.setAcceleration(forwardAcceleration);
      stepper2.move(dynamicSteps);
      stepper3.setMaxSpeed(connectorSpeed);
      stepper3.setAcceleration(forwardAcceleration);
      stepper3.move(connectorSteps);                //dynamicSteps * 0.00525 * 1.04167
      /*while (stepper2.distanceToGo() != 0 || stepper3.distanceToGo() != 0) {
        stepper2.run();
        stepper3.run();
      }*/
      bool stepper3Done = false;

      while (stepper2.distanceToGo() != 0) {
        stepper2.run();

        if (!stepper3Done) {
          stepper3.run();
          if (stepper3.distanceToGo() == 0) {
            stepper3Done = true;
          }
        }
      }


      stepper3.setMaxSpeed(reverseSpeed * 5);   //-----retract extruder
      stepper3.setAcceleration(reverseAcceleration * 6);
      stepper3.move(-1100);
      stepper1.setMaxSpeed(reverseSpeed * 2);   //-----lift extruder and reset table to starting position
      stepper1.setAcceleration(reverseAcceleration);
      stepper1.move(-7400);
      while (stepper3.distanceToGo() != 0 || stepper1.distanceToGo() != 0) {
        stepper3.run();
        stepper1.run();
      }

      stepper2.setMaxSpeed(reverseSpeed);
      stepper2.setAcceleration(reverseAcceleration);
      stepper2.move(-dynamicSteps);
      while (stepper2.distanceToGo() != 0) stepper2.run();

      Serial.println("Connector paste cycle complete.");
    }

    else if (command.startsWith("conductorpaste ")) {   //-------conductor Button------------------------------------------------------------------------------------
      long value = command.substring(15).toInt();
      dynamicSteps = value;
      Serial.println("Starting connector paste routine...");
      Serial.print("dynamicSteps: "); Serial.println(dynamicSteps);

      long value1 = command.substring(20).toInt();
      connectorSpeed = value1;
      
      long value2 = command.substring(25).toInt();
      connectorSteps = value2;

      initialLoad = scale.get_units();
      stepper1.setMaxSpeed(forwardSpeed);
      stepper1.setAcceleration(forwardAcceleration);
      stepper1.setSpeed(forwardSpeed);

      while (true) {    //-----move extruder tip down
        stepper1.runSpeed();
        if (millis() - lastReadTime >= readInterval) {
          lastReadTime = millis();
          float load = scale.get_units();
          if (load >= initialLoad + LOAD_INCREASE_THRESHOLD) {
            stepper1.stop();
            delay(50);
            break;
          }
        }
      }

      stepper1.setMaxSpeed(reverseSpeed);   //-----move extruder tip up to application height
      stepper1.setAcceleration(reverseAcceleration);
      stepper1.move(-1400);
      while (stepper1.distanceToGo() != 0) stepper1.run();

      stepper3.setMaxSpeed(forwardSpeed);   //-----prime extruder
      stepper3.setAcceleration(forwardAcceleration);
      stepper3.move(1100);
      while (stepper3.distanceToGo() != 0) stepper3.run();

      stepper2.setMaxSpeed(forwardSpeed);   //-----dispense paste 
      stepper2.setAcceleration(forwardAcceleration);
      stepper2.move(dynamicSteps);
      stepper3.setMaxSpeed(connectorSpeed);
      stepper3.setAcceleration(forwardAcceleration);
      stepper3.move(connectorSteps);                //dynamicSteps * 0.00525 * 1.04167
      /*while (stepper2.distanceToGo() != 0 || stepper3.distanceToGo() != 0) {
        stepper2.run();
        stepper3.run();
      }*/
      bool stepper3Done = false;

      while (stepper2.distanceToGo() != 0) {
        stepper2.run();

        if (!stepper3Done) {
          stepper3.run();
          if (stepper3.distanceToGo() == 0) {
            stepper3Done = true;
          }
        }
      }


      stepper3.setMaxSpeed(reverseSpeed * 5);   //-----retract extruder
      stepper3.setAcceleration(reverseAcceleration * 6);
      stepper3.move(-1100);
      stepper1.setMaxSpeed(reverseSpeed * 2);   //-----lift extruder and reset table to starting position
      stepper1.setAcceleration(reverseAcceleration);
      stepper1.move(-7400);
      while (stepper3.distanceToGo() != 0 || stepper1.distanceToGo() != 0) {
        stepper3.run();
        stepper1.run();
      }

      stepper2.setMaxSpeed(reverseSpeed);
      stepper2.setAcceleration(reverseAcceleration);
      stepper2.move(-dynamicSteps);
      while (stepper2.distanceToGo() != 0) stepper2.run();

      Serial.println("Connector paste cycle complete.");
    }

    else if (command.startsWith("ground_bar_paste ")) {   //-------------Ground Bar Button-----------------------------------------------------------------------------------------
      long value = command.substring(17).toInt();
      dynamicSteps = value;
      Serial.println("Starting ground bar routine...");
      Serial.print("dynamicSteps: "); Serial.println(dynamicSteps);

      initialLoad = scale.get_units();
      stepper1.setMaxSpeed(forwardSpeed);
      stepper1.setAcceleration(forwardAcceleration);
      stepper1.setSpeed(forwardSpeed);

      while (true) {
        stepper1.runSpeed();    // height motor down until contact is made
        if (millis() - lastReadTime >= readInterval) {
          lastReadTime = millis();
          float load = scale.get_units();
          if (load >= initialLoad + LOAD_INCREASE_THRESHOLD) {
            stepper1.stop();
            delay(50);
            break;
          }
        }
      }

      stepper1.setMaxSpeed(reverseSpeed);   //-----height motor up to set paste gap
      stepper1.setAcceleration(reverseAcceleration);
      stepper1.move(-800);
      while (stepper1.distanceToGo() != 0) stepper1.run();

      stepper3.setMaxSpeed(forwardSpeed);   //-----extrude motor runs to prime nozzle
      stepper3.setAcceleration(forwardAcceleration);
      stepper3.move(3200);
      while (stepper3.distanceToGo() != 0) stepper3.run();

      stepper2.setMaxSpeed(forwardSpeed);   //-----pitch and extrude run to dispense paste
      stepper2.setAcceleration(forwardAcceleration);
      stepper2.move(dynamicSteps);
      stepper3.setMaxSpeed(gbSpeed);
      stepper3.setAcceleration(forwardAcceleration);
      stepper3.move(gbSteps);
      while (stepper2.distanceToGo() != 0 || stepper3.distanceToGo() != 0) {
        stepper2.run();
        stepper3.run();
      }

      stepper3.setMaxSpeed(reverseSpeed * 5);
      stepper3.setAcceleration(reverseAcceleration * 6);
      stepper3.move(-3200);
      stepper1.setMaxSpeed(reverseSpeed * 2);
      stepper1.setAcceleration(reverseAcceleration);
      stepper1.move(-7400);
      while (stepper3.distanceToGo() != 0 || stepper1.distanceToGo() != 0) {
        stepper3.run();
        stepper1.run();
      }

      stepper2.setMaxSpeed(reverseSpeed);
      stepper2.setAcceleration(reverseAcceleration);
      stepper2.move(-dynamicSteps);
      while (stepper2.distanceToGo() != 0) stepper2.run();

      Serial.println("Ground bar paste cycle complete.");
    }

    else if (command.startsWith("prime_motor ")) {    //---------Prime Button------------------------------------------------------------------------------------------------------------
      long primeSteps = command.substring(12).toInt();
      Serial.print("Priming motor forward: ");
      Serial.println(primeSteps);

      stepper3.setMaxSpeed(forwardSpeed);
      stepper3.setAcceleration(forwardAcceleration);
      stepper3.move(primeSteps);
      while (stepper3.distanceToGo() != 0) stepper3.run();

      Serial.println("Priming complete.");
    }

    else if (command.startsWith("retract_motor ")) {    //----------Retract Button-----------------------------------------------------------------------------------------------------------
      long retractSteps = command.substring(14).toInt();
      Serial.print("Retracting motor: ");
      Serial.println(retractSteps);

      stepper3.setMaxSpeed(reverseSpeed);
      stepper3.setAcceleration(reverseAcceleration);
      stepper3.move(-retractSteps);
      while (stepper3.distanceToGo() != 0) stepper3.run();

      Serial.println("Retraction complete.");
    }

    else {
      Serial.println("Unrecognized command.");
//Sends a red pixel if not command recognized
      pixel.setPixelColor(0,pixel.Color(255,0,0));
      pixel.show();
      delay(500);
      pixel.setPixelColor(0,pixel.Color(0,0,0));
      pixel.show();

    }
  }
}
