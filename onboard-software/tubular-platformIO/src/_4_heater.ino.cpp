/*
 * Name: Heater object
 * Purpose: To read the temperature sensor for sensorbox
 * and valve sensor and accordingly turn on/off heater.
 * Project: Tubular-Bexus.
 * Authors: Tubular-Bexus software group.
*/
#ifndef UNIT_TEST
#include <SD.h>
#include <ArduinoSTL.h>
#include <vector>
#include "heaterLogic.h"
#include "_8_monitor.h"

#include "_4_heater.h"

extern std::vector<float> htrParam;
extern std::vector<float> tempAtHtr;
float htrParameter[4];
//std::vector<char> htr_flag[2];

std::vector<float> processInitialHtrParameters(char htrParameters[])
{
  Serial.println("I'm at processInitialHtrParameters");
  std::vector<float> newHtrParameter(4);
  
  int i = 0; int z = 0;
  //int sizeParam = sizeof(htrParameters)/sizeof(byte);
  while(z<4)
  {
    int k = 0;
    char buf[6] = {0};
    while(1)
    {
      if (htrParameters[i] == ',')
      {
        i++;
        break;
      }
      buf[k] = htrParameters[i];
      
      i++; k++;
    }
    newHtrParameter[z] = atof(buf);
    Serial.println(newHtrParameter[z]);
    z++;
  }
  return newHtrParameter;
}

void initHtrParameters()
{
  Serial.println("I'm at initHtrParameters");
  File dataParam = SD.open("htr.txt");
  if (dataParam)
  {
    char htrParameters[dataParam.size()];
    int i = 0;
    while (dataParam.available())
    {
        htrParameters[i] = dataParam.read();
        i++;
    }
    Serial.println(String(htrParameters));
    float newParameter[4];
    std::vector<float> newParameterV = processInitialHtrParameters(htrParameters);
    for (int scP = 0; scP < 4; scP++)
    {
        newParameter[scP] = newParameterV[scP];
    }
    dataParam.close();
    setHeaterParameter(newParameter);
  }
  else
  {
    Serial.println("Failed to open htr.txt");
  }
  
}


/*
 * END of readingData child object
 */


/* The parameter child object.
 *  int * heaterParameter returns the pointer
 *  to htrParam.
 */
void readHeaterParameter()
{
  int i=0;
  xSemaphoreTake(sem, portMAX_DELAY);
  //htrParam = htrParameter;
  for ( i = 0; i < 3; i++) {
      htrParam[i] = htrParameter[i];
   }
  xSemaphoreGive(sem);    
  //return htrParam;
}

/* setHeaterParameter has an array of four(4) 
 *  elemnets and will replace the htrParameter array
 *  with new values
 */
void setHeaterParameter(float newParameter[])
{
  xSemaphoreTake(sem, portMAX_DELAY);
  htrParameter[0]=(newParameter[0]);//,*newParameter+1,*newParameter+2,*newParameter+3);
  htrParameter[1]=(newParameter[1]);
  htrParameter[2]=(newParameter[2]);
  htrParameter[3]=(newParameter[3]);
  xSemaphoreGive(sem);
}
/*
 * END of parameter child object
 */

/*
 * The heater control child object
 * 
 */
void heaterControl(bool htrOne, bool htrTwo)
{
  xSemaphoreTake(sem, portMAX_DELAY);
  digitalWrite(htr1_pin,htrOne);
  digitalWrite(htr2_pin,htrTwo);
  xSemaphoreGive(sem);    
}



/* readingData child object
 * readingData reads the temperature data at valve center and electronics box.
 * Turn on/off the two heaters accoring to parameters.
 */
void readingData(void *pvParameters)
{
  (void) pvParameters;
  
  //bool htr1_flag;// = htr_flag(0);
  //bool htr2_flag;// = htr_flag(1);
  
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount ();
  
  while(1)
  {
    //Reads the current mode
    static uint8_t currMode = getMode();
    //Reads the temperature at the two sensors
    tempAtHtr  = readData(0);
 //   tempAtHtr[1]  = *readData(0);//Must make sure to get the correct tempSensors.
    struct heater htrflag = heaterCompare();
    heaterControl(htrflag.htr1_flag,htrflag.htr2_flag);

    flagPost(1);
    vTaskDelayUntil(&xLastWakeTime, (5000 / portTICK_PERIOD_MS) ); 
  }
    
}



void initReadingData() {
    Serial.println("I'm at initReadingData");
    xTaskCreate(
    readingData
    ,  (const portCHAR *) "readingData"   // Name
    ,  2048  // This stack size 
    ,  NULL
    ,  2  // Priority
    ,  NULL );

}

void initHeater() {
  Serial.println("Im at initHeater");
  //initHtrParameters();
  float initHeaterParam[] = {15,20,-5,0};
  setHeaterParameter(initHeaterParam);
  pinMode(htr1_pin, OUTPUT);
  pinMode(htr2_pin, OUTPUT);
  initReadingData();
 // int htrParameter[4];

}

/*
 * End of the heaterControl child object
 */
#endif