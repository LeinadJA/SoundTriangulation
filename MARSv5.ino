
//added Adafruit LED Ring
//added "Timer" library

#include "Timer.h"
#include <Adafruit_NeoPixel.h>

#define LEDPIN      6
#define NUMPIXELS   24

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LEDPIN, NEO_GRB + NEO_KHZ800);

//int bluLEDPin = 13;
#define micPin0 3
#define micPin1 2
#define micPin2 1
#define SoS 34029.0     //speed of sound in cm/sec
#define micDist 25.7    //distance between mics cm

volatile boolean valMic0 = LOW;
volatile boolean valMic1 = LOW;
volatile boolean valMic2 = LOW;
volatile char micState = 0;
volatile int curMicOrder[3];
volatile int micOrder[3];
volatile double soundTime1;
volatile double soundTime2;
volatile char vectUpdateFlag;

//might be obsolete with array version
volatile unsigned long micTime0 = 0;
volatile unsigned long micTime1 = 0;
volatile unsigned long micTime2 = 0;
Timer timer(MICROS);

void micInter0();
void micInter1();
void micInter2();
double normalizedDeg(double);
double soundCalc(double soundTime1, double soundTime2);
void colorControl(double input);
void sorter(int* arr, int n);

void setup() {  
  pinMode(LEDPIN, OUTPUT);
  //pinMode(bluLEDPin, OUTPUT);

  pinMode(micPin0, INPUT);
  pinMode(micPin1, INPUT);
  pinMode(micPin2, INPUT);

  attachInterrupt(digitalPinToInterrupt(micPin0), micInter0, RISING);
  attachInterrupt(digitalPinToInterrupt(micPin1), micInter1, RISING);
  attachInterrupt(digitalPinToInterrupt(micPin2), micInter2, RISING);

  pixels.begin();
  Serial.begin(9600);
}

//---------------------SOUND-CALC--------------------------
//might need to implement vectUpdateFlag
double soundCalc(double soundTime1, double soundTime2) {    
  int max_delay = floor(micDist * 1000000 / SoS);
  double theta;
  char curMicOrder [3];

  while (1) {
      curMicOrder[0]= micOrder[0];
      curMicOrder[1]= micOrder[1];
      curMicOrder[2]= micOrder[2];
      float dt1= soundTime1; //in uSec
      float dt2= soundTime2; //in uSec

      if (dt2 < max_delay) {    //final check for invalid sound
        if ((soundTime2 - soundTime1) > soundTime1){
          Serial.println("sound came from back, compare 0 and 1");
          theta= ( (float) asin ( (double) (( dt1 * (float) SoS ) / ( (float) micDist * (float) 1000000)) ) * ( (float) 180 / (float) M_PI) );
          Serial.println(theta);
          
          switch (curMicOrder[0]){  
            case 1:
            if (curMicOrder[1] == 2) {
              theta = 360.0 - theta;
            }
            else {
            //micOrder[1]==3
              theta = 240.0 + theta;
            }
            break;

            case 2:
            if (curMicOrder[1] == 1) {
              theta = theta;
            }
            else {
            //micOrder[1]==3
              theta = 120.0 - theta;
            }                
            break;

            case 3:
            if (curMicOrder[1] == 1) {
              theta = 240.0 - theta;
            }
            else{
            //micOrder[1]==2
              theta = 120.0 + theta;
            }
            break;

            default:
//            fprintf(stdout,"error");
            break;
          }
        }
        
        else{
          Serial.println("sound came from front, compare 1 and 2");
          theta= ( (float) asin ( (double) (( (dt2-dt1) * (float) SoS ) / ( (float) micDist * (float) 1000000)) ) * ( (float) 180 / (float) M_PI) );
          Serial.println(theta);
          
          switch (curMicOrder[1]){
            case 1:
            if (curMicOrder[2] == 2){
              theta = 180.0 + theta;
            }
            else{
            //micOrder[2]=3
              theta = 60.0 - theta;
            }
            break;

            case 2:
            if (curMicOrder[2] == 1){
              theta = 180.0 - theta;
            }
            else{
            //micOrder[2]=3
              theta = 300.0 + theta;
            }                
            break;

            case 3:
            if (curMicOrder[2] == 1){
              theta = 60.0 + theta;
            }
            else{
            //micOrder[2]=2
              theta = 300.0 - theta;
            }
            break;

            default:
//            fprintf(stdout,"error");
            break;
          }
        }
        return theta;
      }
      Serial.println("invalid sound");
      return theta = 1000;
  }
}


//------------------------SORTER-FUNCTION----------------------
//possibly obsolute in this version of MARS
void sorter(int* arr, int size){
    for (int i = 0; i < size - 1; i++) {
    for (int j = 0; j < size - i - 1; j++) {
      if (arr[j] > arr[j + 1]) {
        // Swap the elements
        int temp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = temp;
      }
    }
  }
}


//------------------------MAIN-LOOP----------------------------
void loop() {
  double passOff;
    if(valMic0==HIGH && valMic1==HIGH && valMic2==HIGH){
      passOff = soundCalc(soundTime1, soundTime2);  
      Serial.println(passOff);
      micState = 0;
      colorControl(passOff);
      valMic0 = LOW;
      valMic1 = LOW;
      valMic2 = LOW;
      Serial.println();
      timer.stop();
    }
    
}



//------------------------MIC-INTERRUPTS----------------------------
//order in which microphones get sound signal is determined in the interups with a switch case that is controlled via
//a global variable "micState".  
//case 0 begins the timer and set what ever mic that recived the signal as the first in the "micOrder[]" array then move to case 1
//case 1 reads the timer and stores it in "soundTime1" store this mic as second in "micOrder[]" array then move to case 2
//case 2 reads and stores the timer value in "soundTime2" store this mic as last in "micOrder[]" array then movs to default state
//micState is reset to 0 in the main function
void micInter0(){
  valMic0 = HIGH;
  //Serial.println("mic0 check");
  switch(micState){
    case 0:
    timer.stop();
    Serial.println(" mic0");
    //Serial.println(timer.read());
    timer.start();                    //reseting timer
    micState = 1;                     //update state of mic
    micOrder[0] = 1;                  //update mic order
    break;

    case 1:
    soundTime1 = timer.read();        //save timer value
    //Serial.println(soundTime1);
    micState = 2;                     //update state
    micOrder[1] = 1;                  //update mic order
    break;

    case 2:
    soundTime2 = timer.read();        //save timer value
    //Serial.println(soundTime2);
    micState = 3;                     //update state
    micOrder[2] = 1;                  //update mic order
    vectUpdateFlag = 1;               //update vector of arival
    timer.stop();
    break;

    default:
    micState = -1;
    timer.stop();
    break;   
  }
}


void micInter1(){
  valMic1 = HIGH;
  //Serial.println("mic1 check");
  switch(micState){
    case 0:
    timer.stop();
    Serial.println(" mic1");
    //Serial.println(timer.read());
    timer.start();                    //reseting timer
    micState = 1;                     //update state of mic
    micOrder[0] = 2;                  //update mic order
    break;

    case 1:
    soundTime1 = timer.read();        //save timer value
    //Serial.println(soundTime1);
    micState = 2;                     //update state
    micOrder[1] = 2;                  //update mic order
    break;

    case 2:
    soundTime2 = timer.read();        //save timer value
    //Serial.println(soundTime2);
    micState = 3;                     //update state
    micOrder[2] = 2;                  //update mic order
    vectUpdateFlag = 1;               //update vector of arival
    timer.stop();
    break;

    default:
    micState = -1;
    timer.stop();
    break;   
  }
}

void micInter2(){
  valMic2 = HIGH;
  //Serial.println("mic2 check");
  switch(micState){
    case 0:
    timer.stop();
    Serial.println("mic2");
    //Serial.println(timer.read());
    timer.start();                    //reseting timer
    micState = 1;                     //update state of mic
    micOrder[0] = 3;                  //update mic order
    break;

    case 1:
    soundTime1 = timer.read();        //save timer value
    //Serial.println(soundTime1);
    micState = 2;                     //update state
    micOrder[1] = 3;                  //update mic order
    break;

    case 2:
    soundTime2 = timer.read();        //save timer value
    //Serial.println(soundTime2);
    micState = 3;                     //update state
    micOrder[2] = 3;                  //update mic order
    vectUpdateFlag = 1;               //update vector of arival
    timer.stop();
    break;

    default:
    micState = -1;
    timer.stop();
    break;   
  }
}


//------------------------NORMALIZE-FUNC----------------------------
//this function is possibly obsolute in this version of MARS
double normalizedDeg(double angle){
  int angleInt = static_cast<int>(angle);
  angleInt = angleInt % 360;
  if(angleInt < 0){
    angleInt += 360;
  }
  return static_cast<double>(angleInt);
}


//------------------------LED-RING-CTRL----------------------------
//controlls the adafruit led ring
void colorControl(double input) {

  if(input == 1000){
    for(int i = 0; i <= 24; i++){
      pixels.setPixelColor(i, pixels.Color(1,0,0));
    }
        pixels.show();
    
        delay(500);
    
        pixels.clear();
        pixels.show();
  }
  else{
  int colorVar = input/15;        //takes 360 degree input and divides by number of LEDs on ring, 15 in this case
  int colorEchoPlus;
  int colorEchoMinus;

  
  if(colorVar + 1 >= 24)          //edge case
    colorEchoPlus = 0;
  colorEchoPlus = colorVar + 1;

  if(colorVar - 1 <= 0)           //edge case
    colorEchoMinus = 24;
  colorEchoMinus = colorVar - 1;
  

  //setting pixel colors, red and dimmed red
  pixels.setPixelColor(colorVar, pixels.Color(255, 0, 0));
  pixels.setPixelColor(colorEchoPlus, pixels.Color(5, 0, 0));
  pixels.setPixelColor(colorEchoMinus, pixels.Color(5, 0, 0));

  pixels.show();
    
  delay(500);
    
  pixels.clear();
  pixels.show();
  }
  
}
