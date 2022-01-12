#include <Arduino.h>
#include <config.h> //config file
#include <MemoryUsage.h>

#define HID_CUSTOM_LAYOUT
#define LAYOUT_FRENCH
#include <HID-Project.h> //Keyboard ++
#include <EEPROM.h>

// --- Oled Display ---
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans12pt7b.h>

bool textScrolling = false; // Store the state of text scrolling
short textX;                // Store the current X position of the text
String textOnDisplay;       // Store the current scolling text

struct Encoders // Prepare encoders configstructure
{
  byte type;
  short values[3];
};

struct Keys // Prepare keys config structure
{
  byte type;
  short values[3];
};

struct Profile
{
  byte id;       // profile id
  char name[20]; // profile name
  unsigned char icon[LOGO_SIZE];
  byte color[3];
  Encoders encoders[ENCODERS_COUNT];
  Keys keys[KEYS_COUNT];
};

struct MacropadConfig
{
  Profile profile[PROFILES_COUNT];
};

MacropadConfig macropadConfig; // Prepare macropad config structure

int encodersPosition[3] = {50, 50, 50};  // temp virtual position of encoders
int encodersLastValue[3] = {50, 50, 50}; // Value of encoders

String serialMsg;

const byte keysPins[6] = {key0Pin, key1Pin, key2Pin, key3Pin, key4Pin, key5Pin};                                                                                   // Pins of all keys
const byte encodersPins[9] = {encoderA0Pin, encoderB0Pin, encoderKey0Pin, encoderA1Pin, encoderB1Pin, encoderKey1Pin, encoderA2Pin, encoderB2Pin, encoderKey2Pin}; // Pins of all encodes

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // config the display

bool keyPressed[KEYS_COUNT];         // current state of key
bool encoderPressed[ENCODERS_COUNT]; // current state of encoder key
bool selectProfileMode = false;      // If is true, all function are disabled, and keys do the profile selection

// ---- Millis for timer ----
unsigned long encoderMillis;
unsigned long selectProfileMillis; // to blink the profile number

byte currentProfile = 0; // Current selected profile is 0 by default

const byte repeatDelay = 5;  //
unsigned long currentMillis; // ----------------------------a renomer
unsigned long tickTime;      // for the scrolling text

String getArgs(String data, char separator, short index)
{
  // This Function separate String with special characters
  short found = 0;
  short strIndex[] = {0, -1};
  short maxIndex = data.length() - 1;

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

void encoder(byte encoder) // function for the encoder interrupt
{
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  byte pin = encodersPins[encoder * 3 + 1];

  if (interruptTime - lastInterruptTime > 5)
  {
    if (digitalRead(pin) == 0) // check the rotation direction
    {
      encodersPosition[encoder] = encodersLastValue[encoder] - 10; // anti-clockwise
    }
    else
    {
      encodersPosition[encoder] = encodersLastValue[encoder] + 10; // clockwise
    }
  }
  lastInterruptTime = interruptTime;
}

void saveToEEPROM() // Save all config into the atmega eeprom
{
  EEPROM.put(0, macropadConfig);
  Serial.write(sizeof(macropadConfig));
}

void readEEPROM() // Read all config from the atmega eeprom and store it into the temp config
{
  EEPROM.get(0, macropadConfig);
}

void configFont()
{
  oled.setFont(&FreeSans12pt7b);    // set the font
  oled.setTextSize(1);              // set the font size to 1
  oled.setTextColor(SSD1306_WHITE); // Write text in white
}

void scrollText() // function to scoll the text into the display
{
  if (tickTime <= millis()) // every 30 ms
  {
    tickTime = millis() + 30;

    if (textScrolling)
    {
      short textRtn = -12 * textOnDisplay.length();
      if (textX > textRtn)
        textX -= 1;
      else
        textX = -textRtn;

      oled.clearDisplay();           // clear the display
      oled.setTextWrap(false);       // disable text warp on the right border
      oled.setFont(&FreeSans12pt7b); // set the default font
      oled.setTextSize(1);
      oled.setTextColor(SSD1306_WHITE);
      oled.setCursor(textX, ((SCREEN_HEIGHT / 2) + FONT_HEIGHT)); // Center the text
      oled.println(textOnDisplay);

      oled.display(); // print the text
    }
  }
}

void centerText(String txt) // Display the text in the center of the screen
{
  oled.clearDisplay(); // clear the display
  int16_t x1;
  int16_t y1;
  uint16_t textW;
  uint16_t textH;

  configFont();

  oled.getTextBounds(txt, 0, 0, &x1, &y1, &textW, &textH);                 // get the size of the text and store it into textH and textW
  oled.setCursor((SCREEN_WIDTH - textW) / 2, (SCREEN_HEIGHT + textH) / 2); // set the cursor in the good position

  oled.println(txt); // print the text
  oled.display();    // send to screen
  textScrolling = false;
}

void displayOnScreen(String txt) // display test on screen, check if is center text or scrolling text
{
  int16_t x1;
  int16_t y1;
  uint16_t textW;
  uint16_t textH;
  oled.setFont(&FreeSans12pt7b);
  oled.getTextBounds(txt, 0, 0, &x1, &y1, &textW, &textH); // get the size of the text and store it into textH and textW

  if (textW > SCREEN_WIDTH - 5) // if the text width is larger than the screen, the text is scrolling
  {
    textScrolling = true;
    textOnDisplay = txt;
    textX = 0;
    tickTime = millis();
  }
  else
  {
    centerText(txt);
  }
}

void setRGB(byte r, byte g, byte b) // Set rgb value for LEDs
{
  // ---- Turn On LEDs with the analog value ----
  analogWrite(ledR, r);
  analogWrite(ledG, g);
  analogWrite(ledB, b);
  // ---- Save Value to the conf ----
  macropadConfig.profile[currentProfile].color[0] = r;
  macropadConfig.profile[currentProfile].color[1] = g;
  macropadConfig.profile[currentProfile].color[2] = b;
}
bool profileBlinkState = true;

void printCurrentProfile()
{
  oled.clearDisplay();
  oled.setCursor(31, 25);
  oled.println(F("Profil"));
  oled.setCursor(90, 25);
  oled.println(String((currentProfile + 1)));
  oled.display();
}

void setProfile(byte profile)
{
  currentProfile = profile;
  selectProfileMode = false;
  String txt = macropadConfig.profile[profile].name;
  if (txt.length() == 0)
  {
    txt = "Profil " + String((currentProfile + 1));
  }
  displayOnScreen(txt);
  setRGB(macropadConfig.profile[currentProfile].color[0], macropadConfig.profile[currentProfile].color[1], macropadConfig.profile[currentProfile].color[2]);

  //// oled.drawRoundRect(0, 3, LOGO_HEIGHT + 2, LOGO_HEIGHT + 2, 5, WHITE);
  // oled.drawRamBitmap(1, 4, LOGO_HEIGHT, LOGO_WIDTH, WHITE, macropadConfig.profile[currentProfile].icon, 72);

  // oled.display();
}

void clearProfileNumber()
{
  oled.writeFillRect(90, 9, 12, 18, BLACK);
}

void selectProfile()        // comment this function
{                           // When select profile mode is active
  static String strProfile; // String to store the profile name
  // Profile Number blink:
  if (selectProfileMillis <= millis())
  {                                       // wait 500ms
    oled.setCursor(90, 25);               // set the cursor in the good position
    selectProfileMillis = millis() + 500; // wait 500ms
    clearProfileNumber();                 // clear the profile number
    if (profileBlinkState)
    {
      profileBlinkState = false;
      oled.print(strProfile);
    }
    else
    {
      profileBlinkState = true;
    }
    oled.display();
  }

  for (byte i = 0; i <= 5; i++) // foreach key
  {
    if (digitalRead(keysPins[i]))
    {
      setProfile(i);
    }
  }

  if (encodersPosition[1] != encodersLastValue[1])
  {
    if (encodersPosition[1] < encodersLastValue[1])
    { // clockWise
      if (currentProfile >= 5)
        currentProfile = 0;
      else
        currentProfile++;
    }
    else
    {
      if (currentProfile <= 0)
        currentProfile = 5;
      else
        currentProfile--;
    }
    strProfile = String((currentProfile + 1));
    oled.setCursor(90, 25);
    clearProfileNumber();
    oled.print(strProfile);

    encodersLastValue[1] = encodersPosition[1];
    selectProfileMillis = millis() + 500;
    oled.display();
  }
  if (!digitalRead(encoderKey1Pin))
  {
    setProfile(currentProfile);
  }
}

void setup()
{
  Serial.begin(9600);
  // Serial.setTimeout(500);

  for (byte i = 0; i < 6; i++)
  {
    pinMode(keysPins[i], INPUT);
  }
  // Encoders
  for (byte i = 0; i < 3; i++)
  {
    pinMode(encodersPins[i], INPUT);
  }

  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  delay(500);

  setRGB(255, 0, 0);

  //------------Interrupts----------
  //  attachInterrupt(digitalPinToInterrupt(encoderA0Pin), encoder0, LOW);
  // attachInterrupt(digitalPinToInterrupt(encoderA1Pin), encoder1, LOW);
  // attachInterrupt(digitalPinToInterrupt(encoderA2Pin), encoder2, LOW);

  Consumer.begin(); // Start HID

  Wire.begin();

  // oled.begin(&Adafruit128x32, 0x3C);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  oled.clearDisplay();
  oled.display();

  delay(500);

  setRGB(0, 255, 0);

  displayOnScreen(F("Macropad "));

  delay(500);

  setRGB(0, 0, 255);

  delay(500);

  readEEPROM();

  setProfile(0);
}

bool encoderAState[3];

void loop()
{

  for (byte i = 0; i < 3; i++) // forech encoder, check if thre is a nex position and change call the fonction
  {
    if (digitalRead(encodersPins[i * 3]) == 0)
    {
      if (encoderAState[i])
      {
        // alredy press
      }
      else
      {
        // first press
        encoder(i);
        encoderAState[i] = true;
      }
    }
    else
    {
      encoderAState[i] = false;
    }
  }

  if (selectProfileMode) // when the select profile mode is enable
  {
    selectProfile();
  }
  else // Default mode
  {
    // Foreach Keys check the state, and do the action if yes
    for (byte i = 0; i < 6; i++)
    {
      if (digitalRead(keysPins[i]))
      {
        if (keyPressed[i])
        { // if the key is already pressed
        }
        else
        {
          Serial.println("K" + String(i));

          short keyType = macropadConfig.profile[currentProfile].keys[i].type;
          short keyValues[3];
          memcpy(keyValues, macropadConfig.profile[currentProfile].keys[i].values, 6); // 2bytes * 3 values = 6 bytes
          if (keyType == -1)
          {
            // Disable the key
          }
          else if (keyType == 0) // Key Combination
          {
            for (byte j = 0; j < 3; j++) // for each key
            {
              if (keyValues[j] == 0 && keyValues[j] != 60) // if no key and key 60 (<) because impoved keyboard
              {
              }
              else if (keyValues[j] <= 32 && keyValues[j] >= 8) // function key (spaces, shift, etc)
              {
                Keyboard.press(keyValues[j]);
              }
              else if (keyValues[j] <= 90 && keyValues[j] >= 65) // alaphabet
              {                                                  // ascii normal code exept '<' (60)
                Keyboard.press(keyValues[j] + 32);               // Normal character + 32 (ascii Upercase to lowcase)
              }
              else if (keyValues[j] <= 57 && keyValues[j] >= 48) // numbers
              {
                Keyboard.press(keyValues[j]); // Normal number
              }
              else
              {
                Keyboard.press(KeyboardKeycode(keyValues[j])); // Improved keyboard
              }
            }

            // while (digitalRead(keysPins[i]))
            // {
            //   if (currentMillis + repeatDelay <= millis() / 100)
            //   {
            //     delay(50);
            //   }
            // }
          }
          else if (keyType == 1) // System Action
          {
            Consumer.write(ConsumerKeycode(keyValues[0]));
            currentMillis = millis() / 100;
            // while (digitalRead(keysPins[i]))
            // {
            //   if (currentMillis + repeatDelay <= millis() / 100)
            //   {
            //     Consumer.write(keyConf[i][currentProfile].value[0]);
            //     delay(50);
            //   }
            // }
          }
        }

        keyPressed[i] = true;
      }
      else
      {
        keyPressed[i] = false;
        Keyboard.releaseAll();
      }
    }

    // Check Encoders
    for (byte i = 0; i < 3; i++) // For every encoder
    {
      short encoderType = macropadConfig.profile[currentProfile].encoders[i].type;
      short encoderValues[3];
      memcpy(encoderValues, macropadConfig.profile[currentProfile].encoders[i].values, 6); // 2bytes * 3 values

      if (encodersPosition[i] != encodersLastValue[i]) // If Encoder New Value
      {
        if (encodersPosition[i] < encodersLastValue[i]) // Encoder ClockWise Turn
        {
          if (encoderType == 0 && encoderValues[0] == 0) // If is a System Action AND master volume selected
          {
            Consumer.write(MEDIA_VOL_UP);
          }
          else if (encoderType == 0 && encoderValues[0] == 1) // If is a System Action AND master volume selected
          {
            Consumer.write(HID_CONSUMER_FAST_FORWARD);
          }
          else if (encoderType == 1) // If is a key action
          {
            Keyboard.write(encoderValues[0]); // get the ascii code, and press the key
          }
        }
        else // Encoder Anti-ClockWise Turn
        {
          if (encoderType == 0 && encoderValues[0] == 0)
          {
            Consumer.write(MEDIA_VOL_DOWN);
          }
          else if (encoderType == 0 && encoderValues[0] == 1) // If is a System Action AND master volume selected
          {
            Consumer.write(HID_CONSUMER_REWIND);
          }
          else if (encoderType == 1) // If is a key action
          {
            Keyboard.write(encoderValues[1]); // get the ascii code, and press the key
          }
        }
        encodersLastValue[i] = encodersPosition[i];
      }
    }

    if (!digitalRead(encoderKey1Pin)) // When encoder 1 key is pressed
    {

      if (encoderPressed[1])
      { // if the encoder is already pressed
        // wait
        // Serial.println("Already press");
        if (encoderMillis <= millis() && selectProfileMode == false)
        {
          // 2 second after the begin of the press
          // Serial.println("2S --> profile set");
          // set profile menu
          selectProfileMode = true;
          printCurrentProfile();
          oled.drawBitmap(0, 10, profile_left, PROFILE_H, PROFILE_W, WHITE);
          oled.drawBitmap(119, 10, profile_right, PROFILE_H, PROFILE_W, WHITE);
          oled.display();
          selectProfileMillis = millis();
          while (!digitalRead(encoderKey1Pin))
          {
            /* code */
          }
        }
      }
      else
      { // first press of the encoder
        // Serial.println("First press");
        encoderPressed[1] = true;
        encoderMillis = millis() + 1500;
      }
    }

    if (digitalRead(encoderKey1Pin)) // When encoder 1 key is relese
    {
      // Do the action
      encoderPressed[1] = false;
    }
  }

  if (!digitalRead(encoderKey0Pin) && !digitalRead(encoderKey1Pin) && !digitalRead(encoderKey2Pin)) // install the software
  {
    Consumer.write(ConsumerKeycode(0x223)); // open default web Browser
    delay(1000);
    Keyboard.press(KEY_LEFT_CTRL); // focus the url with CTRL + L
    Keyboard.press(KEY_L);
    delay(10);
    Keyboard.releaseAll();
    delay(100);
    Keyboard.print(F("https://github.com/xmow49/MacroPad-Software/releases")); // enter the url
    delay(100);
    Keyboard.write(KEY_ENTER); // go to the url
  }

  if (Serial.available() > 0)
  {
    serialMsg += (char)Serial.read();
  }
  else if (serialMsg != "")
  {
    bool validCmd = true;
    serialMsg.trim();
    String command = getArgs(serialMsg, ' ', 0);
    // Serial.println("finish command");
    short arg[5];
    for (byte i = 0; i < 5; i++)
    {
      arg[i] = getArgs(serialMsg, ' ', i + 1).toInt();
    }
    // Serial.println("finish args");
    /*
    Serial.println(arg[0]); //n encoder/key
    Serial.println(arg[1]); //mode
    Serial.println(arg[2]); //Value 1
    Serial.println(arg[3]); //Value 2
    Serial.println(arg[4]); //Value 3
    */
    if (command == "P") // Ping
    {
      Serial.println("P"); // Pong
    }
    else if (command == "K") // Set Key: K <profile> <n key> <type> <value> <value optional> <value optional>
    {
      macropadConfig.profile[arg[0]].keys[arg[1]].type = arg[2]; // Set the type of the key
      for (byte i = 0; i < 3; i++)                               // foreach value in the command
      {
        macropadConfig.profile[arg[0]].keys[arg[1]].values[i] = arg[i + 3]; // Set the value of the key
      }
    }

    else if (command == "E") // Set encoder action: E <profile> <n encoder> <type> <value> <value optional> <value optional>
    {
      macropadConfig.profile[arg[0]].encoders[arg[1]].type = arg[2]; // Get the mode and save it in the config
      for (byte i = 0; i < 3; i++)                                   // for all values, save it in the config
      {
        macropadConfig.profile[arg[0]].encoders[arg[1]].values[i] = arg[i + 3]; // Save Values
      }
    }
    else if (command == "T") // Set Text
    {
      String txt = getArgs(serialMsg, '"', 1);
      txt.remove(txt.length() - 1);
      displayOnScreen(txt);
    }
#ifdef VERBOSE
    else if (command == "G") // Get config command
    {
      for (byte profile = 0; profile <= 5; profile++)
      {

        String txt = "---- Profile " + String(profile) + " ----";
        Serial.println(txt);

        for (byte i = 0; i < 6; i++) // for all keys, get the config and display it
        {
          // Serial.print("Key");
          // Serial.print(i);
          // Serial.print(" ");
          // Serial.print(keyConf[i][profile].mode);
          // Serial.print(" ");
          // Serial.print(keyConf[i][profile].value[0]);
          // Serial.print(":");
          // Serial.print(keyConf[i][profile].value[1]);
          // Serial.print(":");
          // Serial.println(keyConf[i][profile].value[2]);
        }
        for (byte i = 0; i < 3; i++) // for all encoders, get the config and display it
        {
          // Serial.print("Encoder");
          // Serial.print(i);
          // Serial.print(" ");
          // Serial.print(encoderConfig[i][profile].mode);
          // Serial.print(" ");
          // Serial.print(encoderConfig[i][profile].value[0]);
          // Serial.print(":");
          // Serial.print(encoderConfig[i][profile].value[1]);
          // Serial.print(":");
          // Serial.println(encoderConfig[i][profile].value[2]);
        }
      }
    }
#endif
    else if (command == "S") // save-config command
    {
      saveToEEPROM();
    }
    else if (command == "R") // read-config command
    {
      readEEPROM();
    }

    else if (command == "C") // C <profile> <Red: 0 - 255> <Green: 0 - 255> <Blue: 0 - 255>
    {
      byte profile = arg[0];
      for (byte i = 0; i < 3; i++)
      {
        macropadConfig.profile[profile].color[i] = arg[i + 1];
      }
      if (profile == currentProfile)
      {
        setRGB(arg[0], arg[1], arg[2]); // Set RGB LED color to rgb value from the command :
      }
    }
    else if (command == "Z") // reset all the config to 0
    {
      for (byte profile = 0; profile <= 5; profile++)
      {
        String txt = "Profile " + String(profile) + " :";
        Serial.println(txt);
        for (byte i = 0; i < 3; i++)
        {
          macropadConfig.profile[currentProfile].encoders[i].type = 0;
          // encoderConfig[i][profile].mode = 0;
          for (byte j = 0; j < 3; j++)
          {
            macropadConfig.profile[currentProfile].encoders[i].values[j] = 0;
          }
        }

        for (byte i = 0; i < 6; i++)
        {
          macropadConfig.profile[currentProfile].keys[i].type = 0;
          for (byte j = 0; j < 3; j++)
          {
            macropadConfig.profile[currentProfile].keys[i].values[j] = 0;
          }
        }
      }
    }
    else if (command == "I") // set icon
    {
    }
    else if (command == "A") // set profile
    {
      setProfile(arg[0]);
    }
    else if (command == "B") // set profile
    {
      byte profile = arg[0];                    // Get the profile number
      String name = getArgs(serialMsg, '"', 1); // Get the name of the profile
      name.remove(name.length() - 1);           // Remove the last "
      if (name.length() > 10)
      {
        name.remove(11);
      }
      name.toCharArray(macropadConfig.profile[profile].name, name.length() + 1); // Save the name of the profile
      Serial.println(macropadConfig.profile[profile].name);                      // Display the name of the profile
      if (profile == currentProfile)
      {
        setProfile(profile);
      }
    }
    else
    {
      validCmd = false;
    }
    if (validCmd)
    {
      Serial.println("1"); // ok
    }
    else
    {
      Serial.println("0"); // error
    }
    // command = "";

    serialMsg = "";
  }

  scrollText();
}
