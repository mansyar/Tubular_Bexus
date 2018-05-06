/*
 * Name: asc object
 * Purpose: To read the pressure sensor from buffer
 * and accordingly activate sampling logic.
 * Project: Tubular-Bexus.
 * Authors: Tubular-Bexus software group.
*/
#ifndef UNIT_TEST
#include <SD.h>
#include "_5_asc.h"
#include "ascLogic.h"

float ascParameter[16];

std::vector<float> getASCParam(int bag)
{
  std::vector<float> dummyParameter(2);
  xSemaphoreTake(sem, portMAX_DELAY);
  dummyParameter[0] = ascParameter[(bag*2)-2];
  dummyParameter[1] = ascParameter[(bag*2)-1];
  xSemaphoreGive(sem);
  return dummyParameter;
}

std::vector<float> processInitialAscParameters(uint8_t scParameters[])
{
  std::vector<float> newParameter(16);
  int i = 0; int k = 0;
  int sizeParam = sizeof(scParameters)/sizeof(byte);
  while(i < sizeParam)
  {
    if (scParameters[i] != ',')
    {
      newParameter[k] = scParameters[i] - '0';
      i++; k++;
    }
    else
    {
      i++;
    }
  }
  return newParameter;
}

void initAscParameters()
{
  File dataParam = SD.open("ascParameters.txt");
  uint8_t scParameters[] = {dataParam.read()};
  float newParameter[16];
  std::vector<float> newParameterV = {processInitialAscParameters(scParameters)};
  for (int scP = 0; scP < 16; scP++)
  {
    newParameter[scP] = newParameterV[scP];
  }
  dataParam.close();
  setASCParameter(newParameter);
}

void setASCParameter(float newParameter[16])
{
  xSemaphoreTake(sem, portMAX_DELAY);
  for (int i = 0; i < 16; i++)
  {
    ascParameter[i]=newParameter[i];
  }
  xSemaphoreGive(sem);
}

void initValvesControl()
{
  pinMode(valve1, OUTPUT);
  pinMode(valve2, OUTPUT);
  pinMode(valve3, OUTPUT);
  pinMode(valve4, OUTPUT);
  pinMode(valve5, OUTPUT);
  pinMode(valve6, OUTPUT);
  pinMode(valve7, OUTPUT);
  pinMode(valve8, OUTPUT);
  pinMode(valve9, OUTPUT);
  pinMode(valve10, OUTPUT);
  pinMode(flushValve, OUTPUT);
  pinMode(CACvalve, OUTPUT);
}

void samplingLogic(int bagcounter)
{
  valvesControl(11, 1); //delay(10);
  pumpControl(1);
  valvesControl(11, 0); //delay(10);
  valvesControl(bagcounter, 1); //delay(10);  
}

void closeValve(int bagcounter)
{
  pumpControl(0); //delay(100);
  valvesControl(bagcounter, 0); //delay(10);
}

void pumpControl(int cond)
{
  xSemaphoreTake(sem, portMAX_DELAY);
  switch (cond){
    case 0:
    digitalWrite(pumpPin, LOW);
    break;

    case 1:
    digitalWrite(pumpPin, HIGH);
    break;
  }
  xSemaphoreGive(sem);
}

void valvesControl(int valve, int cond)
{
  xSemaphoreTake(sem, portMAX_DELAY);
  if (cond==1)
  {
    switch (valve)
    {
      case 1:
      digitalWrite(valve1, HIGH);
      break;

      case 2:
      digitalWrite(valve2, HIGH);
      break;

      case 3:
      digitalWrite(valve3, HIGH);
      break;

      case 4:
      digitalWrite(valve4, HIGH);
      break;

      case 5:
      digitalWrite(valve5, HIGH);
      break;

      case 6:
      digitalWrite(valve6, HIGH);
      break;

      case 7:
      digitalWrite(valve7, HIGH);
      break;

      case 8:
      digitalWrite(valve8, HIGH);
      break;

      case 9:
      digitalWrite(valve9, HIGH);
      break;

      case 10:
      digitalWrite(valve10, HIGH);
      break;

      case 11:
      digitalWrite(flushValve, HIGH);
      break;
  }
  }
  else {
    switch (valve){
    case 1:
    digitalWrite(valve1, LOW);
    break;

    case 2:
    digitalWrite(valve2, LOW);
    break;

    case 3:
    digitalWrite(valve3, LOW);
    break;

    case 4:
    digitalWrite(valve4, LOW);
    break;

    case 5:
    digitalWrite(valve5, LOW);
    break;

    case 6:
    digitalWrite(valve6, LOW);
    break;

    case 7:
    digitalWrite(valve7, LOW);
    break;

    case 8:
    digitalWrite(valve8, LOW);
    break;

    case 9:
    digitalWrite(valve9, LOW);
    break;

    case 10:
    digitalWrite(valve10, LOW);
    break;

    case 11:
    digitalWrite(flushValve, LOW);
    break;

    default:
    break;
  }
  xSemaphoreGive(sem);
  }
}

int ascentSequence(float meanPressureAmbient, float ascParam[], int bagcounter)
{
  digitalWrite(CACvalve, HIGH);
  if (normalSamplingLogic(meanPressureAmbient, ascParam))
  {
    samplingLogic(bagcounter);
  }
  else
  {
    closeValve(bagcounter);
    if (bagcounter<8 && meanPressureAmbient>=(ascParam[1])){
      bagcounter++; 
    }
  }
  return bagcounter;
}

int descentSequence(float meanPressureAmbient, float ascParam[], int bagcounter)
{
  if (normalSamplingLogic(meanPressureAmbient, ascParam))
  {
    samplingLogic(bagcounter);  
  }
  else
  {
    closeValve(bagcounter);
    if (bagcounter<8 && meanPressureAmbient<= ascParam[0]){
      bagcounter++;
    }
  }
  return bagcounter;
}

void reading(void *pvParameters)
{
   (void) pvParameters;
   std::vector<float> dummyParam(2);
   float ascParam[2];
   int bagcounter = 1;
   std::vector<float> currPressure(nrPressSensors);
   float meanPressureAmbient;
   
   TickType_t xLastWakeTime;
   xLastWakeTime = xTaskGetTickCount ();

   while(1)
   {
      uint8_t currMode = getMode();
     
     dummyParam = getASCParam(bagcounter);
     ascParam[0] = dummyParam[0]; // lower limit
     ascParam[1] = dummyParam[1]; // upper limit
     currPressure = readData(2);

     /*Calculating mean pressure from several pressure sensors*/
     meanPressureAmbient = (currPressure[0]+currPressure[1])/2;

     switch (currMode){
     /*Standby*/
     case standbyMode:
     break;
     
     /*Normal - Ascent*/
     case normalAscent:
     bagcounter = ascentSequence(meanPressureAmbient, ascParam, bagcounter);
     break;
     
     /*Normal - Descent*/
     case normalDescent:
     bagcounter = descentSequence(meanPressureAmbient, ascParam, bagcounter);
     break;
     
     /*SAFE*/
     case safeMode:
     digitalWrite(CACvalve, LOW);
     for (int sd = 1;sd <= 8; sd++)
     {
       pumpControl(0);
       valvesControl(sd, 0);
     }
     break;

     case manual:
     
     break;
   }
   vTaskDelayUntil(&xLastWakeTime, (800 / portTICK_PERIOD_MS) );
   }
}

void initReading()
{
  xTaskCreate(
    reading
    ,  (const portCHAR *) "reading"   // Name
    ,  128  // This stack size 
    ,  NULL
    ,  1  // Priority
    ,  NULL );
}

void initPumpControl()
{
  pinMode(pumpPin, OUTPUT);
}

void initASC()
{
  initReading();
  initPumpControl();
  initValvesControl();
}
#endif