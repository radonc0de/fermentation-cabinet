//TEMP SENSORS SETUP

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_TEMP_BUS 2
OneWire oneWireTemp(ONE_WIRE_TEMP_BUS);
DallasTemperature tempSensors(&oneWireTemp);

//LCD SETUP
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

int red_btn = 10;
int yellow_btn = 11;
int green_btn  = 12;
//modes =  possible state of frige with its own rules; 0 = idle, 1 = pre-heat, 2 = dump, 3 = refrigerate, 4 = cook
const char modes[5][12] = {"Idle", "Pre-Heat", "Dump", "Refrigerate", "Cook"};
int modeQueue[5] = {0, 0, 0, 0, 0}; //currentMode[0] is current state; this is setup as an array so modes can be queued; ex. currentMode[1] will occur after currentMode[0] once a certain condition is met
int numSensors = 4; //how many oneWire temp sensors INSIDE THE FRIDGE
float temperatures[4]; //array for storing temperatures, use same number as previous
float cookingTemp = 106.0; 
float fridgeTemp = 37.0;
float dumpTemp = 60.0; //target temperature for dump, you might want to put a temp sensors in thermal reservoir to determine this
float dumpThreshold = 5.0; //how close temperature needs to be to dump temperature to end dump

const long cookingDuration = 129600; //36 hours in seconds
long timeRemaining = 0;

void setup() {
  //initialize serial communication at 9600 baud/sec
  Serial.begin(9600);

  //initialize buttons as input
  pinMode(red_btn, INPUT);
  pinMode(yellow_btn, INPUT);
  pinMode(green_btn, INPUT);

  //start temp sensors
  tempSensors.begin();

  //start lcd
  lcd.begin(16, 2);
  lcd.print("Greetings, Yogurt Master");
  lcd.setBacklight(WHITE);


}

void loop() {
  //1. HANDLE BUTTON INPUT
  
  //pressing all three buttons clears queue
  //then, modes can be appended to the queue individually
  if (red_btn == HIGH && yellow_btn == HIGH && green_btn == HIGH){
    modeQueue[0] = 0;
    modeQueue[1] = 0;
    modeQueue[2] = 0;
    modeQueue[3] = 0;
    modeQueue[4] = 0;
  }else if (red_btn == HIGH && yellow_btn == HIGH){
    enqueueMode(2);
    enqueueMode(1);
  }else if (red_btn == HIGH && green_btn == HIGH){
    //auto ferment
    enqueueMode(2); //dump
    enqueueMode(1); //preheat
    enqueueMode(4); //cook
    enqueueMode(2); //dump
    enqueueMode(3); //refrigerate
  }else if (yellow_btn == HIGH && green_btn == HIGH){
    enqueueMode(2);
    enqueueMode(3);
  }else if (red_btn == HIGH){
    enqueueMode(1);
  }else if (yellow_btn == HIGH){
    enqueueMode(2);
  }else if (green_btn == HIGH){
    enqueueMode(3);
  }

  //2. GET DATA
  tempSensors.requestTemperatures();
  float avgTemp = 0.0;
  for(int i = 0; i < numSensors; i++){
    temperatures[i] = tempSensors.getTempFByIndex(i);
    avgTemp += temperatures[i];
  }
  avgTemp = avgTemp / numSensors;

  //3. HANDLE DATA USING CURRENT MODE'S OPERATING RULES
  
  if(modeQueue[0] == 1){
    //pre-heat
    if(avgTemp < cookingTemp){
      preHeatHeaterControl(avgTemp); //heater control uses current temp to decide which heaters to turn on
    }else{
      nextInQueue();  
    }
  }else if(modeQueue[0] == 2){
    //dump
    if((avgTemp < dumpTemp - dumpThreshold) || (avgTemp > dumpTemp + dumpThreshold)){
      dumpControl();
    }else{
      drainFluid();
      nextInQueue();
    }
  }else if(modeQueue[0] == 3){
    //refrigerate
    refrigeratorControl(); //code turning on and keeping fridge running
  }else if(modeQueue[0] == 4){
    //cook
    if(timeRemaining > 0){
      cookHeaterControl(avgTemp); //code here for keeping heaters at cookTemp
      delay(1000);
      timeRemaining -= 1;
    }else{
      nextInQueue();
    }
  }
  
  //4. DISPLAY DATA
  lcd.clear();
  lcd.setCursor(0,0);
  
  lcd.print('TEMP + ' + avgTemp + 'F');

  lcd.setCursor(0,1);
  if(timeRemaining > 0){
    int hours = timeRemaining / 3600;
    int minutes = (timeRemaining - (hours * 3600)) / 60;
    int seconds = timeRemaining - (hours * 3600) - (minutes * 60);
    
    lcd.print('TIME: ' + hours + ':' + minutes + ':' + seconds);
  }
}

void enqueueMode(int mode){
  for(int i = 0; i < 5; i++){
    if (modeQueue[i] == 0) {
      modeQueue[i] = mode;
      break;
    }
  }
  Serial.println('Added to queue: ' + modes[mode]);
}

void nextInQueue(){
  for(int i = 1; i < 5; i++){
    modeQueue[i-1] == modeQueue[i];
  }
  modeQueue[4] = 0;
  Serial.println('Going to next mode in queue...');

  //begin timer if next mode is cook
  if(modeQueue[0] == 4){
    timeRemaining = cookingDuration;  
  }
}

void preHeatHeaterControl(float temp){
  //use conditional logic to control heaters based on supplied temperature
}

void cookHeaterControl(float temp){
  //code to keep heaters and fans at optimal levels to maintain cook temp  
}

void dumpControl(){
  //turn on pumps, control fan
}

void refrigeratorControl(){
  //turn on fridge, control fans  
}

void drainFluid(){
  //turn off pump  
}
