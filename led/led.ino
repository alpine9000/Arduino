#include <Wire.h>
#include "PinChangeInterrupt.h"
#include "IRLremote.h"
#include "DS3231.h"

// Choose a valid PinInterrupt or PinChangeInterrupt* pin of your Arduino board
#define PIN_IR                   4
#define PIN_LED_RED              10
#define PIN_LED_GREEN            11
#define PIN_LED_BLUE             9
#define PALETTE_FIXED_BITS       7
#define FADE_STEP_DEFAULT_MILLIS 10
#define FADE_STEP_SLOW_MILLIS    40

typedef struct {
  int red;
  int green;
  int blue;
} color_t;

typedef struct {
  color_t from;
  color_t to;
  int numSteps;
  int step;
  unsigned long lastStepMillis;
  int stepMillis;
} fade_t;

// Choose the IR protocol of your remote. See the other example for this.
//CNec IRLremote;
//CPanasonic IRLremote;
//CHashIR IRLremote;
//#define IRLremote Sony12

typedef struct { 
  CNec irlRemote;
  RTClib rtc;
  DS3231 clock;
  int brightness;
  int remoteLastCode;
  int modeRandom;
  long modeTime;
  unsigned long remoteLastMilli; 
  color_t current;
  fade_t fade;
} global_t;

global_t Global;
void(* reset) (void) = 0;

bool 
parseDateFromSerial(byte& Year, byte& Month, byte& Day, byte& DoW, byte& Hour, byte& Minute, byte& Second)
{
  boolean gotString = false;
  char inChar;
  byte temp1, temp2;
  char inString[20];
  byte j = 0;
  int timeout = 0;

  while (!gotString) {
    if (Serial.available()) {
      timeout = 0;
      inChar = Serial.read();
      inString[j] = inChar;
      j += 1;
      if (inChar == 'x' || inChar == 10) {
        gotString = true;
      }
    }
  }
  // Read Year first
  temp1 = (byte)inString[0] -48;
  temp2 = (byte)inString[1] -48;
  Year = temp1*10 + temp2;
  // now month
  temp1 = (byte)inString[2] -48;
  temp2 = (byte)inString[3] -48;
  Month = temp1*10 + temp2;
  // now date
  temp1 = (byte)inString[4] -48;
  temp2 = (byte)inString[5] -48;
  Day = temp1*10 + temp2;
  // now Day of Week
  DoW = (byte)inString[6] - 48;
  // now Hour
  temp1 = (byte)inString[7] -48;
  temp2 = (byte)inString[8] -48;
  Hour = temp1*10 + temp2;
  // now Minute
  temp1 = (byte)inString[9] -48;
  temp2 = (byte)inString[10] -48;
  Minute = temp1*10 + temp2;
  // now Second
  temp1 = (byte)inString[11] -48;
  temp2 = (byte)inString[12] -48;
  Second = temp1*10 + temp2;
  return true;
}


void 
setColor(unsigned int red, unsigned int green, unsigned int blue)
{  
  int brightness = Global.brightness;
  
  if (brightness < 0) {
    brightness = 0;
  }
  
  analogWrite(PIN_LED_RED, (red*brightness)/255);
  analogWrite(PIN_LED_GREEN, (green*brightness)/255);
  analogWrite(PIN_LED_BLUE, (blue*brightness)/255);    
  
  Global.current.red = red;
  Global.current.green = green;
  Global.current.blue = blue;
}

void
printCurrent(const char* label)
{  
  Serial.print(label);
  Serial.print(Global.current.red);
  Serial.print(',');
  Serial.print(Global.current.green);
  Serial.print(',');
  Serial.println(Global.current.blue);
}

void
setColorPrint(const char* label, unsigned int red, unsigned int green, unsigned int blue)
{
  setColor(red, green, blue);
  printCurrent(label);
}

int 
readRemote(void)
{
  if (Global.irlRemote.available()) {    
    auto data = Global.irlRemote.read();
    if (data.address != 0xFFFF) {      
      //Serial.print(F("Address: 0x"));
      //Serial.println(data.address, HEX);
      Serial.print(F("Command: 0x"));
      Serial.println(data.command, HEX);     
      Global.remoteLastCode =  data.command;
      return data.command;
    } else {
      return Global.remoteLastCode;
    }
  }
  return -1;
}


void
fadeToColorStep(void)
{    
  if (Global.fade.step >= 0) {
    unsigned long now = millis();
    if (now > Global.fade.lastStepMillis + Global.fade.stepMillis) {
      int step = Global.fade.step;    
      int nr = ((Global.fade.from.red<<PALETTE_FIXED_BITS)+((((Global.fade.to.red<<PALETTE_FIXED_BITS)-(Global.fade.from.red<<PALETTE_FIXED_BITS))/Global.fade.numSteps)*step))>>PALETTE_FIXED_BITS;
      int ng = ((Global.fade.from.green<<PALETTE_FIXED_BITS)+((((Global.fade.to.green<<PALETTE_FIXED_BITS)-(Global.fade.from.green<<PALETTE_FIXED_BITS))/Global.fade.numSteps)*step))>>PALETTE_FIXED_BITS;
      int nb = ((Global.fade.from.blue<<PALETTE_FIXED_BITS)+((((Global.fade.to.blue<<PALETTE_FIXED_BITS)-(Global.fade.from.blue<<PALETTE_FIXED_BITS))/Global.fade.numSteps)*step))>>PALETTE_FIXED_BITS;
      setColor(nr, ng, nb);     
      Global.fade.step++;
      Global.fade.lastStepMillis = now;
      if (Global.fade.step > Global.fade.numSteps) {
        Global.fade.step = -1;
      }    
    }
  }
}


void 
fadeToColor(int from_red, int from_green, int from_blue, int to_red, int to_green, int to_blue, int steps, int delay_ms)
{  
  Global.fade.numSteps = steps;
  Global.fade.from.red = from_red;
  Global.fade.from.green = from_green;
  Global.fade.from.blue = from_blue;
  Global.fade.to.red = to_red;
  Global.fade.to.green = to_green;
  Global.fade.to.blue = to_blue;
  Global.fade.lastStepMillis = millis();
  Global.fade.stepMillis = delay_ms;
  Global.fade.step = 0;    
}


void 
targetColor(int r, int g, int b)
{
  fadeToColor(Global.current.red, Global.current.green, Global.current.blue, r, g, b, 32, FADE_STEP_DEFAULT_MILLIS);
}


void 
setDateTime(void)
{
  Serial.println("Enter date in the format: YYMMDDwHHMMSS");
  byte year, month, date, dow, hour, minute, second;

  if (parseDateFromSerial(year, month, date, dow, hour, minute, second)) {
    Global.clock.setClockMode(false);  // set to 24h
    Global.clock.setYear(year);
    Global.clock.setMonth(month);
    Global.clock.setDate(date);
    Global.clock.setDoW(dow);
    Global.clock.setHour(hour);
    Global.clock.setMinute(minute);
    Global.clock.setSecond(second);
    Serial.println("Updated time!");
    printDateTime();
  }
}


void 
printDateTime(void)
{
  DateTime now = Global.rtc.now();

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println("");
}

void
debounce(void (*functor)(void))
{
  if (millis() > Global.remoteLastMilli + 1000) {
    functor();
    Global.remoteLastMilli = millis();    
  }
}

void
toggleRandom(void)
{
   Global.modeRandom = !Global.modeRandom;
   Serial.print("random: "); 
   Serial.print(Global.modeRandom); 
   Serial.print(" current: ");
   Serial.print(Global.current.red); Serial.print(" "); 
   Serial.print(Global.current.green); Serial.print(" "); 
   Serial.print(Global.current.blue); Serial.println(""); 
}


void 
setup(void)
{
  Wire.begin();
  
  while (!Serial);
  Serial.begin(115200);
  Serial.println("LED controller ready!");
  Serial.println("Send 'd' to set the date/time.");
  Serial.println("Send 't' toggle time display.");

  // Start reading the remote. PinInterrupt or PinChangeInterrupt* will automatically be selected
  if (!Global.irlRemote.begin(PIN_IR)) {
    Serial.println("You did not choose a valid pin.");
  }

  Global.fade.step = -1;
  Global.remoteLastCode = -1;
  Global.brightness = -1;
  Global.modeTime = -1;
  
  targetColor(20, 60, 195);  
}


void 
loop(void)
{
  DateTime now = Global.rtc.now();
  
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'd') {
      while (Serial.read() != 10) {
        delay(100);
      }
      setDateTime();
    } else if (c == 't') {
      if (Global.modeTime < 0) {
        Global.modeTime = millis();
      } else {
        Global.modeTime = -1;
      }      
      while (Serial.read() != 10) {
        delay(100);
      }
    }
  }



  if (Global.fade.step == -1) {
    if (now.hour() >= 20 || now.hour() < 7) {
      while (Global.brightness >= 0) {
        Global.brightness--;
        setColor(Global.current.red, Global.current.green, Global.current.blue);
        delay(5);
      }
    } else if (now.hour() >= 7 && now.hour() < 20) {
      if (Global.brightness == -1) {
        while (Global.brightness != 55) {
          Global.brightness++;
          setColor(Global.current.red, Global.current.green, Global.current.blue);
          delay(5);
        }
      }
    }
  }

  switch (readRemote()) {
    case 0xB:
      debounce(toggleRandom);
      break;
    case 0: // Brightness UP
      if (Global.brightness <= 255-5) {
        Global.brightness = Global.brightness + 5;
      } else {
        Global.brightness = 255;
      }
      Serial.print("brightness: ");
      Serial.println(Global.brightness);
      setColor(Global.current.red, Global.current.green, Global.current.blue);
      break;
    case 1: // Brightness DOWN
      if (Global.brightness >= 5) {
        Global.brightness = Global.brightness - 5;
      } else {
        Global.brightness = 0;
      }
      Serial.print("brightness: ");
      Serial.println(Global.brightness);
      setColor(Global.current.red, Global.current.green, Global.current.blue);
      break;
    case 2: // OFF
      targetColor(0, 0, 0);
    break;
    case 3: // ON
      targetColor(255, 255, 255);
      printDateTime();
    break;
    case 4: // red
      targetColor(255, 0, 0);
      break;
    case 5: // green
      targetColor(0, 255, 0);
      break;
    case 6: // blue
      targetColor(0, 0, 255);
      break;
    case 0x8: // red up
      Global.current.red += 1;
      if (Global.current.red > 255) {
        Global.current.red = 0;
      }
      setColorPrint("Red: ", Global.current.red, Global.current.green, Global.current.blue);
      break;
    case 0xc: // red down
      Global.current.red -= 1;
      if (Global.current.red < 0) {
        Global.current.red = 255;
      }
      setColorPrint("Red: ", Global.current.red, Global.current.green, Global.current.blue);
      break;
    case 0x9: // green up
      Global.current.green += 1;
      if (Global.current.green > 255) {
        Global.current.green = 0;
      }
      setColorPrint("Green: ", Global.current.red, Global.current.green, Global.current.blue);
      break;
    case 0xd: // green down
      Global.current.green -= 1;
      if (Global.current.green < 0) {
        Global.current.green = 255;
      }
      setColorPrint("Green: ", Global.current.red, Global.current.green, Global.current.blue);
      break;
    case 0xa: // blue up
      Global.current.blue += 1;
      if (Global.current.blue > 255) {
        Global.current.blue = 0;
      }
      setColorPrint("Blue: ", Global.current.red, Global.current.green, Global.current.blue);
      break;
    case 0xe: // blue down
      Global.current.blue -= 1;
      if (Global.current.blue < 0) {
        Global.current.blue = 255;
      }
      setColorPrint("Blue: ", Global.current.red, Global.current.green, Global.current.blue);
     break;
    case 0x17:
      reset();
      break;
  }

  if (Global.modeRandom && Global.fade.step == -1) {
    unsigned int r = random(0, 255);
    unsigned int g = random(0, 255);
    unsigned int b = random(0, 255);
    fadeToColor(Global.current.red, Global.current.green, Global.current.blue, r, g, b, 128, FADE_STEP_SLOW_MILLIS);
    Serial.print("random: "); Serial.print(r); Serial.print(" "); Serial.print(g); Serial.print(" "); Serial.println(b);
  }

  if (Global.modeTime >= 0) {
    long msNow = millis();
    if (msNow > Global.modeTime+1000) {
      printDateTime();
      Global.modeTime = msNow;
    }
  }

  fadeToColorStep();
}
