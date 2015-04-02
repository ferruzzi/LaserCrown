/**********************************************************
- Original idea by p3nguin; Modes 1, 2, and 3 and the colorwheel code are his from http://www.instructables.com/id/Laser-Crown/
- Code to process HMC5883 output to a bearing taken directly from the sample sketches in the Adafruit_HMC5883_U library
**********************************************************/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include <Adafruit_NeoPixel.h>

#define PIN             4  // NeoPixels on pin 4
#define switchPin       8  // Reed switch on pin D3
#define switchInterrupt 3  // pin D3  = int3

Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345); // assign a unique ID to mag sensor
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(60, PIN);

/* color codes for the NeoPixels */
uint32_t purple = pixels.Color(100, 0, 100),
         red = pixels.Color(255, 0, 0),
         off = pixels.Color(0, 0, 0),
         white  = pixels.Color(255, 255, 255);

uint32_t debounceDelay = 500,    // Ignore bounces under 1/2 second
         lastDebounce = 0,
         prevTime = 0,
         t = 0, 
         c = 0;

uint8_t counter = 0,
        lastCounter = 0,
        offset = 0,
        i = 0;


void displaySensorDetails(void){     /* used for debugging the compass module */
  sensor_t sensor;
  mag.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); 
  Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); 
  Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); 
  Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); 
  Serial.print(sensor.max_value); 
  Serial.println(" uT");
  Serial.print  ("Min Value:    "); 
  Serial.print(sensor.min_value); 
  Serial.println(" uT");
  Serial.print  ("Resolution:   "); 
  Serial.print(sensor.resolution); 
  Serial.println(" uT");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void setup() {
  pixels.begin();
  pixels.setBrightness(255);                             // Full brightness is 255 
  Serial.begin(9600);                                    // Serial for debuggin 
  pinMode(switchPin, INPUT);                             // Set pin for reed switch to input
  digitalWrite(switchPin, HIGH);                         // Turn on pull-up resistor for reed switch
  attachInterrupt(switchInterrupt, trigger0, FALLING);   // Call trigger0() when reed switch comes low

  if(!mag.begin())
  {
    /* There was a problem detecting the HMC5883 ... check your connections */
    Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
    while(1);
  }

  displaySensorDetails();    //used for debugging in serial monitor

  /* flash lights a couple times to indicate booting since starting mode is all llights off */
  for (i=0; i<3; i++){
    pixels.setPixelColor(0, white);
    pixels.setPixelColor(59, off);
    pixels.show();
    delay(200);
    pixels.setPixelColor(59, white);
    pixels.setPixelColor(0, off);
    pixels.show();
    delay (200);
  }
}

void loop() {
  switch(counter) {
  
    default: // All lights off
      mode0();
      break; 
  
    case 1: // Random sparks - just one LED on at a time!
      mode1();
      break;
  
    case 2: // Spinny wheels (30 LEDs on at a time)
      mode2();
      break;
  
    case 3: // Unicorns and Rainbows
      mode3();
      break;
  
    case 4: // Compass
      mode4();
      break;
  }
}

void mode0(){  //all lighs off
  for (i=0; i<60; i++){
    pixels.setPixelColor(i, off);
  }
  pixels.show();
}

void mode1(){  // Random sparks - just one LED on at a time!
  i = random(60);
  pixels.setPixelColor(i, white);
  pixels.show();
  delay(5);
  pixels.setPixelColor(i, 0);
}

void mode2(){  // Spinny wheels (30 LEDs on at a time)
  for(i=0; i<31; i++) {
    c = off;
    if(((offset + i) & 7) < 2) c = purple; // 4 pixels on...
    pixels.setPixelColor(   i, c); // First side
    pixels.setPixelColor(61-i, c); // Second side (flipped)
  }
  pixels.show();
  offset++;
  delay(50);
}

void mode3(){  // Unicorns and Rainbows
  for(i=0; i<31; i++) {
    c = Wheel(i * 6 + offset*4);
    pixels.setPixelColor(   i, c); // First side
    pixels.setPixelColor(61-i, c); // Second side (flipped)
  }
  pixels.show();
  offset++;
  delay(25);
}

void mode4(){  // Compass heading offset the colorwheel
  sensors_event_t event; 
  mag.getEvent(&event);

  // Calculate heading when the magnetometer is level, then correct for signs of axis.
  float heading = atan2(event.magnetic.y, event.magnetic.x);

  // Vancouver declination = .3 radians
  // Anaheim   declination = .21 radians
  
  float declinationAngle = 0.3;
  heading += declinationAngle;

  // Correct for when signs are reversed.
  if(heading < 0)
    heading += 2*PI;

  // Check for wrap due to addition of declination.
  if(heading > 2*PI)
    heading -= 2*PI;

  // Convert radians to degrees for readability.
  float headingDegrees = heading * 180/M_PI; 
  uint8_t headingPixel = round(headingDegrees/6);

  Serial.print("Heading (degrees): "); 
  Serial.println(headingDegrees);
  Serial.print("headingPixel = "); 
  Serial.println(headingPixel);

  for(i=0; i<31; i++) {
    c = Wheel(i * 6);
//    pixels.setPixelColor(headingPixel, 255, 255, 255);
    pixels.setPixelColor((headingPixel+i)%60, c);
    if (i <= headingPixel) {
      pixels.setPixelColor((headingPixel-i)%60, c);
    }
    else {
      pixels.setPixelColor((60-(i-headingPixel))%60, c);
    }
  }
  pixels.show();
  delay(25);
}


void trigger0() {
  if( (millis() - lastDebounce) > debounceDelay){
    counter++;
    counter = counter % 5;
    lastDebounce = millis();
  }
}

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
