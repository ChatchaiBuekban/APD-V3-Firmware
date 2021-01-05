#include <Arduino.h>

#define APD_ID (uint32_t)0xFFFFFFFF
#define APD_FIRMWARE_VER_MAJOR (uint32_t)0x00
#define APD_FIRMWARE_VER_MINOR (uint32_t)0x01
/* need more info? */

/*APD Command */
#define APD_CMD_GET_STATUS (uint32_t)0x0001F0F0
#define APD_CMD_GET_INFO (uint32_t)0x000100FF
#define APD_CMD_GET_RTC (uint32_t)0x000100AA
#define APD_CMD_SET_RTC (uint32_t)0x000100A0

#define APD_CMD_START_PRIMMING (uint32_t)0x00F0F000
#define APD_CMD_STOP_PRIMMING (uint32_t)0x00F0F001

#define APD_CMD_START_FILL (uint32_t)0x00F0F002
#define APD_CMD_STOP_FILL (uint32_t)0x00F0F003

#define APD_CMD_START_DWELL (uint32_t)0x00F0F004
#define APD_CMD_STOP_DWELL (uint32_t)0x00F0F005

#define APD_CMD_START_DRAIN (uint32_t)0x00F0F006
#define APD_CMD_STOP_DRAIN (uint32_t)0x00F0F007

#define APD_CMD_LOCK_CASSETTE (uint32_t)0x00F0F0A0

#define APD_CMD_GET_PROG_VOLUME (uint32_t)0x00FAF001

uint8_t H0 = 0xF5;
uint8_t H1 = 0xAF;
uint16_t dataLength = 0;
uint8_t dataBuffer[128];

void serialError(uint8_t flag);
void cmdEvent(uint32_t cmd);
void cmdWithDataEvent(uint32_t cmd, uint8_t *data, int dataLen);

void setup()
{
  /*init app here*/
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB
  }

  uint8_t send[3] = {0xFA, 0x0D, 0x0A};
  Serial.write(send, 3);
}

void loop()
{
  // do work here
}

void serialEvent()
{
  //statements
  unsigned long time = 0;
  unsigned long start = 0;
  unsigned long timeOut = 5000;

  if (Serial.available() > 0)
  {
    if (Serial.read() == H0)
    {
      time = 0;
      start = millis();
      while ((Serial.available() < 1) && (time < timeOut))
      {
        time = millis() - start;
      }
      //time out
      if (time >= timeOut)
      {
        serialError(0xFF);
        return;
      }
      if (Serial.read() == H1)
      {
        time = 0;
        start = millis();
        while ((Serial.available() < 2) && (time < timeOut))
        {
          time = millis() - start;
        }
        //time out
        if (time >= timeOut)
        {
          serialError(0xFF);
          return;
        }
        uint8_t lMsb = Serial.read();
        uint8_t lLsb = Serial.read();
        dataLength = ((uint16_t)lMsb << 8) | (uint16_t)lLsb;
        if (dataLength == 0)
        {
          //only command mode
          time = 0;
          start = millis();
          while ((Serial.available() < 3) && (time < timeOut))
          {
            time = millis() - start;
          }
          //time out
          if (time >= timeOut)
          {
            //clear buffer
            delay(1000);
            while (Serial.available() > 0)
            {
              Serial.read();
            }
            uint8_t send[3] = {0xFF, 0x0D, 0x0A};
            Serial.write(send, 3);
            return;
          }
          for (uint16_t i = 0; i < 3; i++)
          {
            dataBuffer[i] = Serial.read();
          }

          time = 0;
          start = millis();
          while ((Serial.available() < 2) && (time < timeOut))
          {
            time = millis() - start;
          }
          //time out
          if (time >= timeOut)
          {
            delay(1000);
            while (Serial.available() > 0)
            {
              Serial.read();
            }
            uint8_t send[3] = {0xFF, 0x0D, 0x0A};
            Serial.write(send, 3);
            return;
          }
          uint8_t returnLine = Serial.read();
          uint8_t newLine = Serial.read();

          if ((returnLine == 0x0d) && (newLine == 0x0a))
          {
            uint32_t cmd = (((uint32_t)dataBuffer[0] << 16) | ((uint32_t)dataBuffer[1] << 8) | (uint32_t)dataBuffer[2]) & 0x00FFFFFF;
            //get command
            //clear buffer
            while (Serial.available() > 0)
            {
              /* code */
              Serial.read();
            }
            cmdEvent(cmd);
          }
          else
          {
            serialError(0xFD);
            return;
          }
        }
        else
        {
          //with data mode
          time = 0;
          start = millis();
          while ((Serial.available() < (int)(dataLength + 3)) && (time < timeOut))
          {
            time = millis() - start;
          }
          //time out
          if (time >= timeOut)
          {
            serialError(0xFF);
            return;
          }
          for (uint16_t i = 0; i < (dataLength + 3); i++)
          {
            dataBuffer[i] = Serial.read();
          }

          time = 0;
          start = millis();
          while ((Serial.available() < 2) && (time < timeOut))
          {
            time = millis() - start;
          }
          //time out
          if (time >= timeOut)
          {
            serialError(0xFF);
            return;
          }
          uint8_t returnLine = Serial.read();
          uint8_t newLine = Serial.read();

          if ((returnLine == 0x0d) && (newLine == 0x0a))
          {
            uint32_t cmd = (((uint32_t)dataBuffer[0] << 16) | ((uint32_t)dataBuffer[1] << 8) | (uint32_t)dataBuffer[2]) & 0x00FFFFFF;

            //clear buffer
            while (Serial.available() > 0)
            {
              /* code */
              Serial.read();
            }
            uint8_t checkSum = dataBuffer[3] + dataBuffer[dataLength + 2];
            uint8_t response[4] = {0xAA, checkSum, 0x0d, 0x0a};
            Serial.write(response, 4);
            cmdWithDataEvent(cmd, &dataBuffer[3], dataLength);
          }
          else
          {
            //clear buffer
            serialError(0xFD);
            return;
          }
        }
      }
      else
      {
        serialError(0xFD);
        return;
      }
    }
    else
    {
      serialError(0xFD);
      return;
    }
  }
}

void serialError(uint8_t flag)
{
  //clear buffer
  delay(1000);
  while (Serial.available() > 0)
  {
    Serial.read();
  }

  uint8_t send[3] = {flag, 0x0D, 0x0A};
  Serial.write(send, 3);

  /*
    0xFD : Wrong Format
    oxFF : Time Out
  */
}

/* Manage Application Here 
* 
*  
*/
void cmdEvent(uint32_t cmd)
{
  if (cmd == APD_CMD_GET_STATUS)
  {
    digitalWrite(LED_BUILTIN, HIGH); // just test cmd
  }
  uint8_t response[4] = {0x00, 0x02, 0x0d, 0x0a};
  Serial.write(response, 4);
}

void cmdWithDataEvent(uint32_t cmd, uint8_t *data, int dataLen)
{
  if (data[0] == 0x01)
  {
    digitalWrite(LED_BUILTIN, HIGH); // test
  }
  else if (data[0] == 0x00)
  {
    digitalWrite(LED_BUILTIN, LOW); // test
  }
}