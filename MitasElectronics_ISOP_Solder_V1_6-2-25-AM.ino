#include <Arduino.h>
#include <AccelStepper.h>
#include <HX711.h>
#include <Adafruit_NeoPixel.h>

#define NEOPIXEL_PIN 33 //Set Pixel Pin
#define NUMPIXELS    1  // Only has 1 pixel
#define STEP_PIN 9  // Update with your step pin
#define DIR_PIN 10   // Update with your direction pin
#define ENABLE_PIN 6
#define DT_PIN 12    // HX711 data pin
#define SCK_PIN 11   // HX711 clock pin
#define SLAVE_TX_PIN 39
#define SLAVE_RX_PIN -1  //set to -1 if unused, since we are only sending data

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
HX711 scale;
// Initializes NeoPixel Constructor(number of pixel, pin it uses, type: RGB or GRB, frequency)
Adafruit_NeoPixel pixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

float LOAD_INCREASE_THRESHOLD = 100.0;  // Set your load increase threshold value here
float initialLoad = 0.0;
unsigned long lastReadTime = 0;
const unsigned long readInterval = 100;  // Interval between sensor readings in milliseconds

const int forwardSpeed = 1000;  // Adjust forward speed
const int forwardAcceleration = 1000;  // Adjust forward acceleration
const int reverseSpeed = 60000;  // Adjust reverse speed
const int reverseAcceleration = 55000;  // Adjust reverse acceleration

void setup() {
    Serial.begin(115200);
    Serial.println("Testing COM Port Stability...");

    Serial1.begin(9600, SERIAL_8N1, -1, SLAVE_TX_PIN);
    //Serial.println("Master read.  Sending every 2s...");    //creating a UART echo test for debugging, remove later

    pinMode(ENABLE_PIN, OUTPUT);
    digitalWrite(ENABLE_PIN, LOW);  // Enable the motor driver initially

    scale.begin(DT_PIN, SCK_PIN);
    scale.set_scale(2280.f);  // Adjust this to calibrate your load cell (example factor)
    scale.tare();       // Reset the scale to zero
    //initializes NeoPixel
    pixel.begin();
    
}

void flashPixelGreen()
{
    pixel.setPixelColor(0,pixel.Color(0,255,0));
    pixel.show();
    delay(1000);
    pixel.setPixelColor(0,pixel.Color(0,0,0));
    pixel.show();
}

void loop() {
    //Serial1.println("HELLO_SLAVE");
    //Serial.println("Sent: HELLO_SLAVE");

    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        Serial.print("Received command: ");
        Serial.println(command);

        if (command == "solder_tare") {     //------------------------------------------------------Tare load cell button--------------------------
            scale.tare();
            Serial.println("Solder load cell tared.");
        }  else if (command.startsWith("SET_THRESHOLD")) {     //-----------------------------Set Threshold button---------------------------
            float threshold = command.substring(14).toFloat();  // Extract threshold value
            LOAD_INCREASE_THRESHOLD = threshold;
            Serial.print("Load increase threshold set to: ");
            Serial.println(LOAD_INCREASE_THRESHOLD);
        } else if (command == "connector_solder") {     //------------------------------------------Connector button-------------------------------
            Serial.println("Soldering Connector...");
            digitalWrite(ENABLE_PIN, LOW);  // Enable the motor driver
            initialLoad = scale.get_units();  // Take the initial load cell value
            Serial.print("Initial load: ");
            Serial.println(initialLoad);
            stepper.setMaxSpeed(forwardSpeed);  // Set forward speed
            stepper.setAcceleration(forwardAcceleration);  // Set forward acceleration
            stepper.setSpeed(forwardSpeed);  // Ensure speed is set before moving forward
            while (true) {  // Continuously move forward until threshold is reached
                stepper.runSpeed();  // Use runSpeed() for continuous movement
                unsigned long currentTime = millis();
                if (currentTime - lastReadTime >= readInterval) {
                    lastReadTime = currentTime;
                    float load = scale.get_units();
                    if (load >= initialLoad + LOAD_INCREASE_THRESHOLD) {
                        Serial.println("Load increase threshold crossed. Stopping motor.");
                        stepper.stop();
                        digitalWrite(ENABLE_PIN, HIGH);  // Disable the motor driver
                        delay(6500);  // Wait for 6 seconds
                        break;
                    }
                }
            }
            Serial.print("Forward motion complete. Current position: ");
            Serial.println(stepper.currentPosition());
            Serial.println("Starting reverse motion...");
            digitalWrite(ENABLE_PIN, LOW);  // Enable the motor driver
            stepper.setMaxSpeed(reverseSpeed);  // Set reverse speed
            stepper.setAcceleration(reverseAcceleration);  // Set reverse acceleration
            stepper.move(-8000);  // Move backward 3200 steps (adjust as needed)
            while (stepper.distanceToGo() != 0) {
                stepper.run();
            }
            Serial.print("Reverse motion complete. Current position: ");
            Serial.println(stepper.currentPosition());  // Should print 0
            digitalWrite(ENABLE_PIN, HIGH);  // Disable the motor driver
        } else if (command.startsWith("solder_lift")) {     //         ------------------------lift button-----------------------------------------
            Serial.println("Starting reverse motion...");
            digitalWrite(ENABLE_PIN, LOW);  // Enable the motor driver
            int steps = command.substring(8).toInt();  // Extract number of steps
            stepper.setMaxSpeed(reverseSpeed);  // Set reverse speed
            stepper.setAcceleration(reverseAcceleration);  // Set reverse acceleration
            stepper.move(-steps);  // Reverse motion
            while (stepper.distanceToGo() != 0) {
                stepper.run();
            }
            Serial.print("Reverse motion complete. Current position: ");
            Serial.println(stepper.currentPosition());
            digitalWrite(ENABLE_PIN, HIGH);  // Disable the motor driver
        } else if (command == "STOP") {     //            --------------------------------stop button--------------------------------------
            Serial.println("Stopping motor...");
            stepper.moveTo(stepper.currentPosition());  // Stop the motor
            digitalWrite(ENABLE_PIN, HIGH);  // Disable the motor driver
            Serial.print("Motor stopped. Current position: ");
            Serial.println(stepper.currentPosition());
        } else if (command == "ground_bar") {     //      ----------------------------------Ground Bar button--------------------------
            Serial.println("Soldering Ground Bar...");
            digitalWrite(ENABLE_PIN, LOW);  // Enable the motor driver
            initialLoad = scale.get_units();  // Take the initial load cell value
            Serial.print("Initial load: ");
            Serial.println(initialLoad);
            stepper.setMaxSpeed(forwardSpeed);  // Set forward speed
            stepper.setAcceleration(forwardAcceleration);  // Set forward acceleration
            stepper.setSpeed(forwardSpeed);  // Ensure speed is set before moving forward
            while (true) {  // Continuously move forward until threshold is reached
                stepper.runSpeed();  // Use runSpeed() for continuous movement
                unsigned long currentTime = millis();
                if (currentTime - lastReadTime >= readInterval) {
                    lastReadTime = currentTime;
                    float load = scale.get_units();
                    if (load >= initialLoad + LOAD_INCREASE_THRESHOLD) {
                        Serial.println("Load increase threshold crossed. Stopping motor.");
                        stepper.stop();
                        digitalWrite(ENABLE_PIN, HIGH);  // Disable the motor driver
                        delay(11000);  // Wait for 11 seconds --- started at 11000
                        break;
                    }
                }
            }
            Serial.print("Ground bar forward motion complete. Current position: ");
            Serial.println(stepper.currentPosition());
            Serial.println("Starting reverse motion...");
            digitalWrite(ENABLE_PIN, LOW);  // Enable the motor driver
            stepper.setMaxSpeed(reverseSpeed);  // Set reverse speed
            stepper.setAcceleration(reverseAcceleration);  // Set reverse acceleration
            stepper.move(-8000);  // Move backward 3200 steps (adjust as needed)
            while (stepper.distanceToGo() != 0) {
                stepper.run();
            }
            Serial.print("Ground bar reverse motion complete. Current position: ");
            Serial.println(stepper.currentPosition());  // Should print 0
            digitalWrite(ENABLE_PIN, HIGH);  // Disable the motor driver
        } 
        else if (command == "conductor_solder") {     //      ----------------------------------Conductor Button--------------------------
            Serial.println("Soldering Center Conductor...");
            digitalWrite(ENABLE_PIN, LOW);  // Enable the motor driver
            initialLoad = scale.get_units();  // Take the initial load cell value
            Serial.print("Initial load: ");
            Serial.println(initialLoad);
            stepper.setMaxSpeed(forwardSpeed);  // Set forward speed
            stepper.setAcceleration(forwardAcceleration);  // Set forward acceleration
            stepper.setSpeed(forwardSpeed);  // Ensure speed is set before moving forward
            while (true) {  // Continuously move forward until threshold is reached
                stepper.runSpeed();  // Use runSpeed() for continuous movement
                unsigned long currentTime = millis();
                if (currentTime - lastReadTime >= readInterval) {
                    lastReadTime = currentTime;
                    float load = scale.get_units();
                    if (load >= initialLoad + LOAD_INCREASE_THRESHOLD) {
                        Serial.println("Load increase threshold crossed. Stopping motor.");
                        stepper.stop();
                        digitalWrite(ENABLE_PIN, HIGH);  // Disable the motor driver
                        delay(20000);  // Wait for 5 seconds
                        break;
                    }
                }
            }
            Serial.print("Ground bar forward motion complete. Current position: ");
            Serial.println(stepper.currentPosition());
            Serial.println("Starting reverse motion...");
            digitalWrite(ENABLE_PIN, LOW);  // Enable the motor driver
            stepper.setMaxSpeed(reverseSpeed);  // Set reverse speed
            stepper.setAcceleration(reverseAcceleration);  // Set reverse acceleration
            stepper.move(-8000);  // Move backward 3200 steps (adjust as needed)
            while (stepper.distanceToGo() != 0) {
                stepper.run();
            }
            Serial.print("Ground bar reverse motion complete. Current position: ");
            Serial.println(stepper.currentPosition());  // Should print 0
            digitalWrite(ENABLE_PIN, HIGH);  // Disable the motor driver
        } 
        else if (command.startsWith("setStrip")) {      //stripper
            Serial1.println("setStrip");
        } 
        else if (command.startsWith("jogUp")) {     //stripper
            Serial1.println("jogUp");
        } 
        else if (command.startsWith("jogDown")) {       //stripper
            Serial1.println("jogDown");
        } 
        else if (command.startsWith("jogForward")) {        //stripper
            Serial1.println("jogForward");
        }
        else if (command.startsWith("jogBackward")) {       //stripper
            Serial1.println("jogBackward");
        }
        else if (command.startsWith("outerStrip")) {        //stripper
            Serial1.println("outerStrip");
        }
        else if (command.startsWith("innerStrip")) {        //stripper
            Serial1.println("innerStrip");
        }
        else if (command.startsWith("prime_motor ")) {     //paste
            Serial1.println(command);
            // Uses Pixel to Communicate a command was send
            flashPixelGreen();
            
        }
        else if (command.startsWith("retract_motor ")) {       //paste
            Serial1.println(command);
            // Uses Pixel to Communicate a command was send
            flashPixelGreen();
        }
        else if (command.startsWith("ground_bar_paste")) {        //paste
            Serial1.println(command);
            // Uses Pixel to Communicate a command was send
            flashPixelGreen();
        }
        else if (command.startsWith("conductorpaste")) {        //paste
            Serial1.println(command);
            // Uses Pixel to Communicate a command was send
            flashPixelGreen();
        }
        else if (command.startsWith("connectorpaste")) {        //paste
            Serial1.println(command);
            // Uses Pixel to Communicate a command was send
            flashPixelGreen();
        }
        else if (command.startsWith("extrude")){                //paste
            Serial1.println(command);
            // Uses Pixel to Communicate a command was send
            flashPixelGreen();
        }
        else {
            Serial.println("Invalid command received.");
            digitalWrite(ENABLE_PIN, HIGH);  // Disable the motor driver
        }
    }
}