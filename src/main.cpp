#include <Arduino.h>

#define HID_CUSTOM_LAYOUT
#define LAYOUT_FRENCH
#include <HID-Project.h>

#include <EEPROM.h>

String strAction[] = {"MediaFastForward", "MediaRewind", "MediaNext", "MediaPrevious", "MediaStop", "MediaPlayPause",
                      "MediaVolumeMute", "MediaVolumeUP", "MediaVolumeDOWN",
                      "ConsumerEmailReader", "ConsumerCalculator", "ConsumerExplorer",
                      "ConsumerBrowserHome", "ConsumerBrowserBack", "ConsumerBrowserForward", "ConsumerBrowserRefresh", "ConsumerBrowserBookmarks"};

short hexAction[] = {0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xCD,
                     0xE2, 0xE9, 0xEA,
                     0x18A, 0x192, 0x194,
                     0x223, 0x224, 0x225, 0x227, 0x22A};

short keyAction[6] = {0, 0, 0, 0, 0, 0};

short keyCombination[5*6];

const int PinA = 7;
//const int PinB = 6;

const int PinB1 = 6;
const int PinB2 = 5;
const int PinB3 = 4;

int lastCount = 50;
volatile int virtualPosition = 50;

String serialMsg;

String getArgs(String data, char separator, int index)
{
  //Function for separate String with special characters
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

short getNumberArgs(String data, char separator)
{
  int nArgs = 0;
  int maxIndex = data.length() - 1;
  for (int i = 0; i <= maxIndex; i++)
  {
    if (data.charAt(i) == separator)
    {
      nArgs++;
    }
  }
  return nArgs;
}

void isr()
{
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  // If interrupts come faster than 5ms, assume it's a bounce and ignore
  if (interruptTime - lastInterruptTime > 5)
  {

    if (analogRead(A0) == 0)
    {
      virtualPosition = virtualPosition - 10; // Could be -5 or -10
    }
    else
    {
      virtualPosition = virtualPosition + 10; // Could be +5 or +10
    }

    // Restrict value from 0 to +100
    //virtualPosition = min(100, max(0, virtualPosition));
  }
  // Keep track of when we were here last (no more than every 5ms)
  lastInterruptTime = interruptTime;
}

void getstrAction()
{
  volatile byte key = 0;

  int sizeKeyAction = sizeof(keyAction) / sizeof(int *);
  int sizeHexAction = sizeof(hexAction) / sizeof(int *);
  for (byte i = 0; i < sizeKeyAction; i++)
  {
    for (byte i = 0; i < sizeKeyAction; i++)
    {
      for (byte i = 0; i < sizeHexAction; i++)
      {
        if (hexAction[i] == keyAction[key])
        {
          Serial.print("Key");
          Serial.print(key);
          Serial.print(": ");
          Serial.println(strAction[i]);
          break;
        }
      }
      key++;
    }
  }
}

void saveToEEPROM()
{
  volatile byte key = 0;
  volatile short eepromAddress = 0;
  int sizeKeyAction = sizeof(keyAction) / sizeof(int *);
  int sizeHexAction = sizeof(hexAction) / sizeof(int *);
  for (byte i = 0; i < sizeKeyAction; i++)
  {
    for (byte i = 0; i < sizeHexAction; i++)
    {
      if (hexAction[i] == keyAction[key])
      {
        Serial.print("Save to EEPROM: Key");
        Serial.print(key);
        Serial.print(": ");
        Serial.println(hexAction[i]);

        eepromAddress += sizeof(hexAction[i]);
        break;
      }
    }
    key++;
  }
}

void readEEPROM()
{
  volatile byte key = 0;
  volatile short eepromAddress = 0;
  volatile short action;
  int sizeKeyAction = sizeof(keyAction) / sizeof(int *);
  int sizeHexAction = sizeof(hexAction) / sizeof(int *);
  for (byte i = 0; i < sizeKeyAction; i++)
  {
    EEPROM.get(eepromAddress, action);
    Serial.print("Read from EEPROM: Key");
    Serial.print(key);
    Serial.print(": ");
    Serial.println(action);
    keyAction[i] = action;
    eepromAddress += sizeof(keyAction[i]);
    key++;
  }
}
int getAction(String action)
{
  for (byte i = 0; i < sizeof(strAction); i++)
  {
    if (action == strAction[i])
    {
      Serial.print("Found: ");
      Serial.println(i);
      return hexAction[i];
      break;
    }
  }
  return 0;
}

void setup()
{

  Serial.begin(9600);

  pinMode(PinA, INPUT);

  pinMode(PinB1, INPUT);
  pinMode(PinB2, INPUT);
  pinMode(PinB3, INPUT);

  attachInterrupt(digitalPinToInterrupt(PinA), isr, LOW);
  Serial.println("Start");
  Consumer.begin();
}

void loop()
{

  if (digitalRead(PinB1))
  {
    while (digitalRead(PinB1))
    {
    }
    Consumer.write(keyAction[0]);
  }
  if (digitalRead(PinB2))
  {
    while (digitalRead(PinB2))
    {
    }
    Consumer.write(keyAction[1]);
  }
  if (digitalRead(PinB3))
  {
    while (digitalRead(PinB3))
    {
    }
    Consumer.write(keyAction[2]);
  }
  if (virtualPosition != lastCount)
  {
    //Serial.print(virtualPosition > lastCount ? "Up  :" : "Down:");
    //Serial.println(virtualPosition);

    if (virtualPosition > lastCount)
    {
      Serial.println("Encoder1:UP");
      //Consumer.write(MEDIA_VOL_UP);
    }
    else
    {
      //Consumer.write(MEDIA_VOL_DOWN);
      Serial.println("Encoder1:DOWN");
    }
    lastCount = virtualPosition;
  }

  if (Serial.available() > 0)
  {
    serialMsg = Serial.readString();
    serialMsg.trim();

    if (serialMsg.indexOf("ping") > -1)
    {
      Serial.println("pong");
    }

    if (serialMsg.indexOf("set") > -1)
    {
      if (serialMsg.indexOf("key") > -1)
      {
        if (serialMsg.indexOf("action") > -1)
        {
          int key = getArgs(serialMsg, ' ', 2).toInt();
          String action = getArgs(serialMsg, ' ', 4);
          keyAction[key] = getAction(action);
        }
        else if (serialMsg.indexOf("combination") > -1)
        {
          short numberOfKey = getNumberArgs(serialMsg, ' ') - 3;
          int key = getArgs(serialMsg, ' ', 2).toInt();
          serialMsg.remove(0, 22);

          keyAction[key] = -1;
        }
      }
    }
    else if (serialMsg.indexOf("get") > -1)
    {
      if (serialMsg.indexOf("config") > -1)
      {
        getstrAction();
      }
    }
    else if (serialMsg.indexOf("save") > -1)
    {
      saveToEEPROM();
    }
    else if (serialMsg.indexOf("read") > -1)
    {
      readEEPROM();
    }
  }
}
