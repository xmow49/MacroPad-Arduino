#include <Arduino.h>
#include <pins.h>
#define HID_CUSTOM_LAYOUT
#define LAYOUT_FRENCH
#include <HID-Project.h>
#include <EEPROM.h>

#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiAvrI2c.h>

#define FONT2 Cooper19
#define FONT Adafruit5x7

bool textScrolling = 0;
//List of action
/*const String strAction[17] PROGMEM = {"MediaFastForward", "MediaRewind", "MediaNext", "MediaPrevious", "MediaStop", "MediaPlayPause",
                      "MediaVolumeMute", "MediaVolumeUP", "MediaVolumeDOWN",
                      "ConsumerEmailReader", "ConsumerCalculator", "ConsumerExplorer",
                      "ConsumerBrowserHome", "ConsumerBrowserBack", "ConsumerBrowserForward", "ConsumerBrowserRefresh", "ConsumerBrowserBookmarks"};

//List of actions in Hex
const short hexAction[17] PROGMEM = {0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xCD,
                                     0xE2, 0xE9, 0xEA,
                                     0x18A, 0x192, 0x194,
                                     0x223, 0x224, 0x225, 0x227, 0x22A};
*/
//List of key current action

struct EncoderConf
{
  byte mode;
  short value[3];
};

struct KeyConf
{
  byte mode;
  short value[3];
};

EncoderConf encoderConfig[3];
KeyConf keyConf[6];

volatile int encodersPosition[3] = {50, 50, 50};
int encodersLastValue[3] = {50, 50, 50};

String serialMsg;

const byte keysPins[6] = {key0Pin, key1Pin, key2Pin, key3Pin, key4Pin, key5Pin};
const byte encodersPins[9] = {encoderA0Pin, encoderB0Pin, encoderKey0Pin, encoderA1Pin, encoderB1Pin, encoderKey1Pin, encoderA2Pin, encoderB2Pin, encoderKey2Pin};

SSD1306AsciiAvrI2c oled;
TickerState state;

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

void encoder(byte encoder)
{
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  volatile byte pin;
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
  encoder(0);
}

void encoder1()
{
  encoder(1);
}

void encoder2()
{
  encoder(2);
}

void saveToEEPROM()
{
  Serial.println("-----Save to EEPROM-----");

  volatile short eepromAddress = 0;
  for (byte i = 0; i < 6; i++)
  {
    Serial.print("Key");
    Serial.print(i);
    Serial.print(" ");

    Serial.print(keyConf[i].mode);
    EEPROM.put(eepromAddress, keyConf[i].mode);
    eepromAddress += sizeof(keyConf[i].mode);

    Serial.print(":");
    Serial.print(keyConf[i].value[0]);
    EEPROM.put(eepromAddress, keyConf[i].value[0]);
    eepromAddress += sizeof(keyConf[i].value[0]);

    Serial.print(",");
    Serial.print(keyConf[i].value[1]);
    EEPROM.put(eepromAddress, keyConf[i].value[1]);
    eepromAddress += sizeof(keyConf[i].value[1]);

    Serial.print(",");
    Serial.println(keyConf[i].value[2]);
    EEPROM.put(eepromAddress, keyConf[i].value[2]);
    eepromAddress += sizeof(keyConf[i].value[2]);
  }

  for (byte i = 0; i < 3; i++)
  {
    Serial.print("Encoder");
    Serial.print(i);
    Serial.print(" ");

    //Save Mode
    Serial.print(encoderConfig[i].mode);
    EEPROM.put(eepromAddress, encoderConfig[i].mode);
    eepromAddress += sizeof(encoderConfig[i].mode);

    //Save value 1
    Serial.print(":");
    Serial.print(encoderConfig[i].value[0]);
    EEPROM.put(eepromAddress, encoderConfig[i].value[0]);
    eepromAddress += sizeof(encoderConfig[i].value[0]);

    //Save value 2
    Serial.print(",");
    Serial.print(encoderConfig[i].value[1]);
    EEPROM.put(eepromAddress, encoderConfig[i].value[1]);
    eepromAddress += sizeof(encoderConfig[i].value[1]);

    //Save value 3
    Serial.print(",");
    Serial.println(encoderConfig[i].value[2]);
    EEPROM.put(eepromAddress, encoderConfig[i].value[2]);
    eepromAddress += sizeof(encoderConfig[i].value[2]);
  }
}

void readEEPROM()
{
  Serial.println("-----Read from EEPROM-----");
  volatile short eepromAddress = 0;
  for (byte i = 0; i < 6; i++)
  {
    Serial.print("Key");
    Serial.print(i);
    Serial.print(" ");

    EEPROM.get(eepromAddress, keyConf[i].mode);
    Serial.print(keyConf[i].mode);
    eepromAddress += sizeof(keyConf[i].mode);

    Serial.print(":");
    EEPROM.get(eepromAddress, keyConf[i].value[0]);
    Serial.print(keyConf[i].value[0]);
    eepromAddress += sizeof(keyConf[i].value[0]);

    Serial.print(",");
    EEPROM.get(eepromAddress, keyConf[i].value[1]);
    Serial.print(keyConf[i].value[1]);
    eepromAddress += sizeof(keyConf[i].value[1]);

    Serial.print(",");
    EEPROM.get(eepromAddress, keyConf[i].value[2]);
    Serial.println(keyConf[i].value[2]);
    eepromAddress += sizeof(keyConf[i].value[2]);
  }
  for (byte i = 0; i < 3; i++)
  {
    Serial.print("Encoder");
    Serial.print(i);
    Serial.print(" ");

    //Get Mode
    EEPROM.get(eepromAddress, encoderConfig[i].mode);
    Serial.print(encoderConfig[i].mode);
    eepromAddress += sizeof(encoderConfig[i].mode);

    //Get Value 1
    Serial.print(":");
    EEPROM.get(eepromAddress, encoderConfig[i].value[0]);
    Serial.print(encoderConfig[i].value[0]);
    eepromAddress += sizeof(encoderConfig[i].value[0]);

    //Get Value 2
    Serial.print(":");
    EEPROM.get(eepromAddress, encoderConfig[i].value[1]);
    Serial.print(encoderConfig[i].value[1]);
    eepromAddress += sizeof(encoderConfig[i].value[1]);

    //Get Value 3
    Serial.print(":");
    EEPROM.get(eepromAddress, encoderConfig[i].value[3]);
    Serial.print(encoderConfig[i].value[3]);
    eepromAddress += sizeof(encoderConfig[i].value[3]);
  }
}

String screenTxt = "MacroPad";

uint32_t tickTime = 0;
void scrollText()
{
  if (tickTime <= millis())
  {
    tickTime = millis() + 30;

    if (textScrolling)
    {
      int8_t rtn = oled.tickerTick(&state);

      if (rtn <= 0)
      {
        oled.tickerText(&state, screenTxt);
      }
    }
  }
}

void centerText(String text)
{
  int str_len = text.length();
  char char_array[str_len];
  text.toCharArray(char_array, str_len);
  size_t size = oled.strWidth(char_array);

  oled.clear();
  oled.setFont(FONT);
  oled.set2X();

  oled.setCursor((oled.displayWidth() - size) / 2, ((oled.displayRows() / 2) - 2)); //Center
  oled.println(screenTxt);
  textScrolling = false;
}

void setRGB(byte r, byte g, byte b)
{
  analogWrite(ledR, r);
  analogWrite(ledG, g);
  analogWrite(ledB, b);
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Start");
  setRGB(255, 0, 0);

  Serial.println("PinMode:");
  //Keys
  for (byte i = 0; i < 6; i++)
  {
    pinMode(keysPins[i], INPUT);
    Serial.print("Key:");
    Serial.println(keysPins[i]);
  }
  //Encoders
  for (byte i = 0; i < 3; i++)
  {
    pinMode(encodersPins[i], INPUT);
    Serial.print("Encoder:");
    Serial.println(encodersPins[i]);
  }
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  delay(1000);

  setRGB(0, 255, 0);

  Serial.println("Setup Interrupts");
  //------------Interrupts----------
  attachInterrupt(digitalPinToInterrupt(encoderA0Pin), encoder0, LOW);
  attachInterrupt(digitalPinToInterrupt(encoderA1Pin), encoder1, LOW);
  attachInterrupt(digitalPinToInterrupt(encoderA2Pin), encoder2, LOW);

  Serial.println("Setup HID");
  Consumer.begin(); //Start HID

  Serial.println("Setup Display");

  Wire.begin();
  oled.begin(&Adafruit128x32, 0x3C);

  Serial.println("Read EEPROM");
  readEEPROM();

  delay(1000);
  setRGB(255, 0, 0);

  centerText("Macropad");
  centerText("Macropad");
  delay(1000);
}

const byte repeatDelay = 5;
unsigned long currentMillis;

void loop()
{
  //Check Keys
  for (byte i = 0; i < 6; i++)
  {
    if (digitalRead(keysPins[i]))
    {
      Serial.print("Key");
      Serial.println(i);
      if (keyConf[i].mode == 0) //System Action
      {
        Consumer.write(keyConf[i].value[0]);
        Serial.println(keyConf[i].value[0]);
        currentMillis = millis() / 100;
        while (digitalRead(keysPins[i]))
        {
          if (currentMillis + repeatDelay <= millis() / 100)
          {
            Consumer.write(keyConf[i].value[0]);
            delay(50);
          }
        }
      }
    }
  }

  //Check Encoders
  for (byte i = 0; i < 3; i++) //For every encoder
  {
    if (encodersPosition[i] != encodersLastValue[i]) //If Encoder New Value
    {
      if (encodersPosition[i] < encodersLastValue[i]) //Encoder ClockWise Turn
      {
        Serial.print("Encoder");
        Serial.print(i);
        Serial.println(":UP");

        if (encoderConfig[i].mode == 0 && encoderConfig[i].value[0] == 0) //If is a System Action AND master volume selected
        {
          Consumer.write(MEDIA_VOL_UP);
        }
        else if (encoderConfig[i].mode == 0 && encoderConfig[i].value[0] == 1) //If is a System Action AND master volume selected
        {
          Consumer.write(HID_CONSUMER_FAST_FORWARD);
        }
        else if (encoderConfig[i].mode == 1) //If is a key action
        {
          Keyboard.write(encoderConfig[i].value[0]); //get the ascii code, and press the key
        }
      }
      else //Encoder Anti-ClockWise Turn
      {
        Serial.print("Encoder");
        Serial.print(i);
        Serial.println(":DOWN");

        if (encoderConfig[i].mode == 0 && encoderConfig[i].value[0] == 0)
        {
          Consumer.write(MEDIA_VOL_DOWN);
        }
        else if (encoderConfig[i].mode == 0 && encoderConfig[i].value[0] == 1) //If is a System Action AND master volume selected
        {
          Consumer.write(HID_CONSUMER_REWIND);
        }
        else if (encoderConfig[i].mode == 1) //If is a key action
        {
          Keyboard.write(encoderConfig[i].value[1]); //get the ascii code, and press the key
        }
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
    short arg[5];
    for (byte i = 0; i < 5; i++)
    {
      arg[i] = getArgs(serialMsg, ' ', i + 1).toInt();
    }
    /*
    Serial.println(arg[0]); //n encoder/key
    Serial.println(arg[1]); //mode
    Serial.println(arg[2]); //Value 1
    Serial.println(arg[3]); //Value 2
    Serial.println(arg[4]); //Value 3
    */

    if (command == "set-key")
    {
      keyConf[arg[0]].mode = arg[1];     //Save Mode
      keyConf[arg[0]].value[0] = arg[2]; //Save Value 1
      Serial.println("OK");
    }

    else if (command == "set-encoder") //Set encoder action
    {
      encoderConfig[arg[0]].mode = arg[1]; //Get the mode and save it in the config
      for (byte i = 0; i < 3; i++)         //for all values, save it in the config
      {
        encoderConfig[arg[0]].value[i] = arg[i + 2];
      }
      Serial.println("OK");
    }
    else if (command == "set-text")
    {
      String txt = getArgs(serialMsg, '"', 1);
      txt.remove(txt.length() - 1);
      screenTxt = txt;
      int str_len = screenTxt.length();
      char char_array[str_len];
      screenTxt.toCharArray(char_array, str_len);
      size_t size = oled.strWidth(char_array);

      size = 150;
      if (size > 128)
      {
        oled.clear();
        oled.tickerInit(&state, FONT, 1, true, 0, oled.displayWidth());
        textScrolling = true;
      }
      else
      {
        centerText(screenTxt);
        textScrolling = false;
      }

      Serial.println("OK");
    }

    else if (command == "get-config") //Get config command
    {
      for (byte i = 0; i < 6; i++) //for all keys, get the config and display it
      {
        Serial.print("Key");
        Serial.print(i);
        Serial.print(" ");
        Serial.print(keyConf[i].mode);
        Serial.print(" ");
        Serial.print(keyConf[i].value[0]);
        Serial.print(":");
        Serial.print(keyConf[i].value[1]);
        Serial.print(":");
        Serial.println(keyConf[i].value[2]);
      }
      for (byte i = 0; i < 3; i++) //for all encoders, get the config and display it
      {
        Serial.print("Encoder");
        Serial.print(i);
        Serial.print(" ");
        Serial.print(encoderConfig[i].mode);
        Serial.print(" ");
        Serial.print(encoderConfig[i].value[0]);
        Serial.print(":");
        Serial.print(encoderConfig[i].value[1]);
        Serial.print(":");
        Serial.println(encoderConfig[i].value[2]);
      }
    }
    else if (command == "save-config") //save-config command
    {
      saveToEEPROM();
    }
    else if (command == "read-config") //read-config command
    {
      readEEPROM();
    }

    else if (command == "set-rgb")
    {
      setRGB(arg[1], arg[2], arg[3]);
      Serial.println("OK");
    }
    else if (command == "reset-config")
    {
      for (byte i = 0; i < 3; i++)
      {
        encoderConfig[i].mode = 0;
        for (byte j = 0; j < 3; j++)
        {
          encoderConfig[i].value[j] = 0;
        }
      }

      for (byte i = 0; i < 6; i++)
      {
        keyConf[i].mode = 0;
        for (byte j = 0; j < 3; j++)
        {
          keyConf[i].value[j] = 0;
        }
      }
      Serial.println("OK");
      
    }

    else
    {
      Serial.print(command);
      Serial.println(": command not found");
    }
  }

  scrollText();
  delay(10);
}
