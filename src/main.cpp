#include <Arduino.h>
#include <math.h>

#define mixerOff 120
#define maxMix 40
#define minMix -40
#define mixDivider 0.8 // (maxMix / 100) * 2

#define totalPins 9
#define pin1 2  // 0 / 1000 Full A
#define pin2 3  // 220 / 780
#define pin3 4  // 370 / 630
#define pin4 5  // 470 / 530
#define pin5 6  // 500 / 500 MidPoint
#define pin6 7  // 530 / 470
#define pin7 8  // 630 / 370
#define pin8 9  // 780 / 220
#define pin9 10 // 1000 / 0 Full B
uint8_t mixPins[totalPins] = { pin1, pin2, pin3, pin4, pin5, pin6, pin7, pin8, pin9 };

int8_t mixState = 0;
int8_t modifier = 5;
#define mixStepInterval 3000
unsigned long lastMixStep = millis();

#define piezoMaxDefault 20
#define piezoDivider 0.5
#define piezoMaxTimerInterval 30000
const byte piezoPins[2] = { A7, A6 };
uint16_t piezoMax[2] = { piezoMaxDefault, piezoMaxDefault };
const uint8_t piezoErrorVal[2] = { 40, 40 }; //cam
//const uint8_t piezoErrorVal[2] = { 30, 27 }; //tape
unsigned long piezoMaxTimer[2] = { millis(), millis() };


void setMix(int8_t mixAB){
  if (mixAB == mixerOff){
    for (uint8_t i = 0; i < totalPins; i++){
      digitalWrite(mixPins[i], 0);
    }
  } else {
    if (mixAB > maxMix){ mixAB = maxMix; } else if (mixAB < minMix){ mixAB = minMix; }
    mixAB = map(mixAB, minMix, maxMix, 0, 8);
    for (uint8_t i = 0; i < totalPins; i++){
      bool setState = i == mixAB ? 1 : 0;
      digitalWrite(mixPins[i], setState);
    }
    Serial.print("A: ");
    Serial.print(round(((mixState * -1) + maxMix) / mixDivider));
    Serial.print("% B: ");
    Serial.print(round((mixState + maxMix) / mixDivider));
    Serial.println("%");
  }
}

void setup(){
  Serial.begin(9600);

//  analogReference(INTERNAL);

  for (uint8_t i = 0; i < totalPins; i++){
    pinMode(mixPins[i], OUTPUT);
  }

  setMix(mixerOff);
}

void loop(){
  int8_t piezoMix = mixState;
  uint16_t piezoADC[2] = { 0, 0 };

  for (uint8_t i = 0; i < 2; i++){
    uint16_t piezoTemp = analogRead(piezoPins[i]);
    if (piezoTemp > piezoErrorVal[i]){
      piezoADC[i] = piezoTemp - piezoErrorVal[i];

      int16_t piezoMaxAdjusted = ((piezoMax[i] - piezoADC[i]) / 2) + piezoADC[i];
      if (piezoADC[i] > piezoMax[i]){
        piezoMax[i] = piezoADC[i];
        piezoMaxTimer[i] = millis();
      } else if (millis() - piezoMaxTimer[i] >= piezoMaxTimerInterval && piezoMaxAdjusted > piezoMaxDefault){
        piezoMax[i] = piezoMaxAdjusted;
        piezoMaxTimer[i] = millis();
      }
    }
    delay(1);
  }
//  Serial.println(piezoADC[0]);


  if (piezoADC[0] > 0 || piezoADC[1] > 0){
    int8_t piezoMap = piezoADC[0] > piezoADC[1] ? map(piezoADC[0], 0, piezoMax[0], -40, 0) : map(piezoADC[1], 0, piezoMax[1], 0, 40);
    if (piezoMap < 0){
      piezoMix = piezoMap < piezoMix ? piezoMap : piezoMix;
      piezoMix = piezoMap < (piezoMix * piezoDivider) ? piezoMix - modifier : piezoMix;
      piezoMix = piezoMix < minMix ? minMix : piezoMix;
    } else {
      piezoMix = piezoMap > piezoMix ? piezoMap : piezoMix;
      piezoMix = piezoMap > (piezoMix * piezoDivider) ? piezoMix + modifier : piezoMix;
      piezoMix = piezoMix > maxMix ? maxMix : piezoMix;
    }
  }

  if (piezoMix != mixState){
    mixState = piezoMix;
    setMix(mixState);
    lastMixStep = millis();
  } else if (millis() - lastMixStep >= mixStepInterval){
    if (mixState >= modifier || mixState <= (modifier * -1)){
      mixState = mixState > 0 ? mixState - modifier : mixState + modifier;
      setMix(mixState);
    }
    lastMixStep = millis();
  }

  mixState = mixState > maxMix ? maxMix : (mixState < minMix ? minMix : mixState);
}
