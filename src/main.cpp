#include <Arduino.h>
#include <pins.h>
#define HID_CUSTOM_LAYOUT
#define LAYOUT_FRENCH
#include <HID-Project.h>
#include <EEPROM.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>

//List of action
String strAction[] = {"MediaFastForward", "MediaRewind", "MediaNext", "MediaPrevious", "MediaStop", "MediaPlayPause",
                      "MediaVolumeMute", "MediaVolumeUP", "MediaVolumeDOWN",
                      "ConsumerEmailReader", "ConsumerCalculator", "ConsumerExplorer",
                      "ConsumerBrowserHome", "ConsumerBrowserBack", "ConsumerBrowserForward", "ConsumerBrowserRefresh", "ConsumerBrowserBookmarks"};

//List of actions in Hex
short hexAction[] = {0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xCD,
                     0xE2, 0xE9, 0xEA,
                     0x18A, 0x192, 0x194,
                     0x223, 0x224, 0x225, 0x227, 0x22A};

//List of key current action
short keyAction[6] = {0, 0, 0, 0, 0, 0};

short keyCombination[5 * 6];

int lastCount0 = 50;
volatile int virtualPosition0 = 50;

int lastCount1 = 50;
volatile int virtualPosition1 = 50;

int lastCount2 = 50;
volatile int virtualPosition2 = 50;

String serialMsg;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

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

void encoder0()
{
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 5)
  {
    if (digitalRead(encoderB0) == 0)
    {
      virtualPosition0 = virtualPosition0 - 10;
    }
    else
    {
      virtualPosition0 = virtualPosition0 + 10;
    }
  }
  lastInterruptTime = interruptTime;
}

void encoder1()
{
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 5)
  {
    if (digitalRead(encoderB1) == 0)
    {
      virtualPosition1 = virtualPosition1 - 10;
    }
    else
    {
      virtualPosition1 = virtualPosition1 + 10;
    }
  }
  lastInterruptTime = interruptTime;
}

void encoder2()
{
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime > 5)
  {
    if (digitalRead(encoderB2) == 0)
    {
      virtualPosition2 = virtualPosition2 - 10;
    }
    else
    {
      virtualPosition2 = virtualPosition2 + 10;
    }
  }
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
        EEPROM.put(eepromAddress, hexAction[i]);
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


void setText(String txt, unsigned short size = 1, GFXfont *f = &FreeSans9pt7b, int x = 0, int y = 0, bool color = 1){
  display.setTextSize(size);
  display.setTextColor(color);
  display.setFont(f);
  display.setCursor(x, y);
  display.print(txt);
}

void setup()
{

  Serial.begin(9600);

  //------------Keys--------------
  pinMode(key0, INPUT);
  pinMode(key1, INPUT);
  pinMode(key2, INPUT);
  pinMode(key3, INPUT);
  pinMode(key4, INPUT);
  pinMode(key5, INPUT);

  //------------Encoders----------
  pinMode(encoderA0, INPUT);
  pinMode(encoderB0, INPUT);
  pinMode(encoder0Key, INPUT);

  pinMode(encoderA1, INPUT);
  pinMode(encoderB1, INPUT);
  pinMode(encoder1Key, INPUT);

  pinMode(encoderA2, INPUT);
  pinMode(encoderA2, INPUT);
  pinMode(encoder2Key, INPUT);

  //------------Interrupts----------
  attachInterrupt(digitalPinToInterrupt(encoderA0), encoder0, LOW);
  attachInterrupt(digitalPinToInterrupt(encoderA1), encoder1, LOW);
  attachInterrupt(digitalPinToInterrupt(encoderA2), encoder2, LOW);

  Serial.println("Start");
  Consumer.begin();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("MacroPad");
  display.setFont(&FreeSans12pt7b);
  display.display();
  delay(2000);
}

const short repeatDelay = 500;

void loop()
{
  int currentMillis = millis();

  //-----------------------Key0------------------------
  if (digitalRead(key0))
  {
    Consumer.write(keyAction[0]);
    while (digitalRead(key0))
    {
      if (currentMillis + repeatDelay <= millis())
      {
        Consumer.write(keyAction[0]);
        delay(50);
      }
    }
  }

  //-----------------------Key1------------------------
  if (digitalRead(key1))
  {
    Consumer.write(keyAction[1]);
    while (digitalRead(key1))
    {
      if (currentMillis + repeatDelay <= millis())
      {
        Consumer.write(keyAction[1]);
        delay(50);
      }
    }
  }

  //-----------------------Key2------------------------
  if (digitalRead(key2))
  {
    Consumer.write(keyAction[2]);
    while (digitalRead(key2))
    {
      if (currentMillis + repeatDelay <= millis())
      {
        Consumer.write(keyAction[2]);
        delay(50);
      }
    }
  }

  if (virtualPosition0 != lastCount0)
  {
    //Serial.print(virtualPosition > lastCount ? "Up  :" : "Down:");
    //Serial.println(virtualPosition);

    if (virtualPosition0 > lastCount0)
    {
      Serial.println("Encoder1:UP");
      //Consumer.write(MEDIA_VOL_UP);
    }
    else
    {
      //Consumer.write(MEDIA_VOL_DOWN);
      Serial.println("Encoder1:DOWN");
    }
    lastCount0 = virtualPosition0;
  }

  if (Serial.available() > 0)
  {
    serialMsg = Serial.readString();
    serialMsg.trim();

    if (serialMsg.indexOf("ping") > -1)
    {
      Serial.println("pong");
    }

    String command = getArgs(serialMsg, ' ', 0);
    String arg1 = getArgs(serialMsg, ' ', 1);
    String arg2 = getArgs(serialMsg, ' ', 2);
    String arg3 = getArgs(serialMsg, ' ', 3);
    if (command == "set-key")
    {
      if (arg2 == "action")
      {
        int key = arg1.toInt();
        keyAction[key] = getAction(arg3);
      }
      else if (arg2 == "combination")
      {
        short numberOfKey = getNumberArgs(serialMsg, ' ') - 3;
        int key = getArgs(serialMsg, ' ', 2).toInt();
        serialMsg.remove(0, 22);

        keyAction[key] = -1;
      }
    }
    else if (command == "get")
    {
      if (serialMsg.indexOf("config") > -1)
      {
        getstrAction();
      }
    }
    else if (arg2 == "set-text")
    {
    }

    else if (command == "save")
    {
      saveToEEPROM();
    }
    else if (command == "read")
    {
      readEEPROM();
    }
  }
}
