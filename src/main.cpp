#include <Arduino.h>
#include <pins.h>
#define HID_CUSTOM_LAYOUT
#define LAYOUT_FRENCH
#include <HID-Project.h>
#include <EEPROM.h>

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>

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
int keyAction[6] = {0, 0, 0, 0, 0, 0};

String encoderAction[3] = {"", "", ""};
String encoderValue[3] = {"", "", ""};

volatile int encodersPosition[3] = {50, 50, 50};
int encodersLastValue[3] = {50, 50, 50};

String serialMsg;

const int keysPins[6] = {key0, key1, key2, key3, key4, key5};

const int encodersPins[9] = {encoderA0, encoderB0, encoderKey0, encoderA1, encoderB1, encoderKey1, encoderA2, encoderB2, encoderKey2};

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

void encoder(byte encoder)
{
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  volatile int pin;
  switch (encoder)
  {
  case 0:
    pin = encodersPins[1];
    break;
  case 1:
    pin = encodersPins[4];
    break;
  case 2:
    pin = encodersPins[7];
    break;

  default:
    return;
    break;
  }
  if (interruptTime - lastInterruptTime > 5)
  {
    if (digitalRead(pin) == 0)
    {
      encodersPosition[encoder] = encodersLastValue[encoder] - 10;
    }
    else
    {
      encodersPosition[encoder] = encodersLastValue[encoder] + 10;
    }
  }
  lastInterruptTime = interruptTime;
}

void encoder0()
{
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  int i = 0;
  if (interruptTime - lastInterruptTime > 5)
  {
    if (digitalRead(encoderB0) == 0)
    {
      encodersPosition[i] = encodersLastValue[i] - 10;
    }
    else
    {
      encodersPosition[i] = encodersLastValue[i] + 10;
    }
  }
  lastInterruptTime = interruptTime;
}

void encoder1()
{
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  int i = 0;
  if (interruptTime - lastInterruptTime > 5)
  {
    if (digitalRead(encoderB1) == 0)
    {
      encodersPosition[i] = encodersLastValue[i] - 10;
    }
    else
    {
      encodersPosition[i] = encodersLastValue[i] + 10;
    }
  }
  lastInterruptTime = interruptTime;
}

void encoder2()
{
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  int i = 0;
  if (interruptTime - lastInterruptTime > 5)
  {
    if (digitalRead(encoderB2) == 0)
    {
      encodersPosition[i] = encodersLastValue[i] - 10;
    }
    else
    {
      encodersPosition[i] = encodersLastValue[i] + 10;
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
  Serial.println("-----Save to EEPROM-----");
  volatile int key = 0;
  volatile short eepromAddress = 0;
  int sizeKeyAction = sizeof(keyAction) / sizeof(int *);
  for (int i = 0; i < sizeKeyAction; i++)
  {
    Serial.print("Save to EEPROM: Key");
    Serial.print(key);
    Serial.print(": ");
    Serial.println(keyAction[i]);
    EEPROM.put(eepromAddress, keyAction[i]);
    eepromAddress += sizeof(keyAction[i]);
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

void setText(String txt, unsigned short size = 1, GFXfont *f = &FreeSans9pt7b, int x = 0, int y = 0, bool color = 1)
{
  display.setTextSize(size);
  display.setTextColor(color);
  display.setFont(f);
  display.setCursor(x, y);
  display.print(txt);
  display.display();
}

String screenTxt = "MacroPad";
int xTxt = display.width();
int xMin = -11 * screenTxt.length();

void scrollText()
{
  display.setTextSize(1);
  display.setFont(&FreeSans9pt7b);
  display.setTextColor(WHITE);
  display.setTextWrap(false);

  int y = 0;
  if (y < 6 * screenTxt.length() + 6)
  {
    display.fillRect(0, 9, 128, 25, BLACK);
    display.setCursor(xTxt, 23);
    display.print(screenTxt);
    xTxt--;
    if (xTxt < xMin)
      xTxt = display.width();
    y++;
  }
  display.display();
}

void setup()
{

  Serial.begin(9600);
  //Keys
  for (int i = 0; i < sizeof(keysPins) / sizeof(int *); i++)
  {
    pinMode(keysPins[i], INPUT);
  }

  //Encoders
  for (int i = 0; i < sizeof(encodersPins) / sizeof(int *); i++)
  {
    pinMode(encodersPins[i], INPUT);
  }

  //------------Interrupts----------
  attachInterrupt(digitalPinToInterrupt(encoderA0), encoder0, LOW);
  attachInterrupt(digitalPinToInterrupt(encoderA1), encoder1, LOW);
  attachInterrupt(digitalPinToInterrupt(encoderA2), encoder2, LOW);

  Consumer.begin(); //Start HID

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  readEEPROM();
  delay(1000);
  Serial.println("Start");
}

const short repeatDelay = 500;

void loop()
{

  /*  for (int i = 0; i < sizeof(keysPins) / sizeof(int *); i++)
  {
    if (digitalRead(keysPins[i]))
    {
      Serial.print("Key ");
      Serial.print(i);
      Serial.println(" Pressed");
      Consumer.write(keyAction[i]);
      int currentMillis = millis();
      while (digitalRead(keysPins[i]))
      {
        if (currentMillis + repeatDelay <= millis())
        {
          Consumer.write(keyAction[i]);
          delay(50);
        }
      }
    }
  }*/

  for (int i = 0; i < 3 / sizeof(int *); i++)
  {
    if (encodersPosition[i] != encodersLastValue[i])
    {
      if (encodersPosition[i] > encodersLastValue[i])
      {
        Serial.print("Encoder");
        Serial.print(i);
        Serial.println(":UP");
      }
      else
      {
        Serial.print("Encoder");
        Serial.print(i);
        Serial.println(":DOWN");
      }
      encodersLastValue[i] = encodersPosition[i];
    }
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
    String arg4 = getArgs(serialMsg, ' ', 4);
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

    else if (command == "set-encoder")
    {
      if (arg2 == "system-vol")
      {
        int encoder = arg1.toInt();
        encoderAction[encoder] = arg3;
        encoderValue[encoder] = arg4;
      }
    }

    else if (command == "set-text")
    {
      if (arg1 == "text")
      {
        String txt = getArgs(serialMsg, '"', 1);
        txt.remove(txt.length() - 1);
        Serial.println(txt);
        screenTxt = txt;
        xTxt = display.width();
        xMin = -11 * screenTxt.length();
      }
    }

    else if (command == "get-config")
    {
      getstrAction();
    }
    else if (command == "save-config")
    {
      saveToEEPROM();
    }
    else if (command == "read-config")
    {
      readEEPROM();
    }
  }

  scrollText();
}
