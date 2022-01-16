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

bool textScrolling = false;             // Store the state of text scrolling
short textX;                            // Store the current X position of the text
char textOnDisplay[SCREEN_TEXT_LENGTH]; // Store the text to be displayed

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
  char name[PROFILE_TEXT_LENGTH]; // profile name
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

void getStrArg(const char serialMsg[], unsigned short serialMsgIndex, short argOutput[])
{
  char charArg[10];
  for (unsigned int i = 2, lastArgIndex = 2, argIndex = 0; i < serialMsgIndex; i++) // foreach caracter in serialMsg
  {
    if (serialMsg[i] == ' ' || i == serialMsgIndex - 1) // if we have a space
    {
      for (unsigned int j = lastArgIndex; j <= i; j++) // foreach caracter from last arg to now
      {
        charArg[j - lastArgIndex] = serialMsg[j]; // save the caracter in the array of arguments
      }

      argOutput[argIndex] = atoi(charArg); // convert the char array to int
      lastArgIndex = i + 1;                // skip the space
      argIndex++;                          // next arg
    }
  }
}

char asciiToHex(char ascii)
{
  // This function convert ascii to hex
  if (ascii <= 57 && ascii >= 48) // 0 - 9
  {
    return ascii - 48;
  }
  else if (ascii <= 70 && ascii >= 65) // A - F
  {
    return ascii - 55;
  }
  else if (ascii <= 97 && ascii >= 90) // a - f
  {
    return ascii - 87;
  }
  else
  {
    return 0;
  }
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
  // Serial.println(sizeof(macropadConfig));
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
  if (textScrolling)
  {
    if (tickTime <= millis()) // every 30 ms
    {
      tickTime = millis() + 30;

      short textRtn = -10 * strlen(textOnDisplay);
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

void displayOnScreen(const char txt[]) // display test on screen, check if is center text or scrolling text
{

  int16_t x1;
  int16_t y1;
  uint16_t textW;
  uint16_t textH;

  configFont();

  oled.getTextBounds(txt, 0, 0, &x1, &y1, &textW, &textH); // get the size of the text and store it into textH and textW

  if (strlen(txt) == 0)
  {
    textW = 0;
    textH = 0;
  }

  // Serial.print(F("Display: "));
  // Serial.println(txt);

  // Serial.print(F("txt length: "));
  // Serial.println(strlen(txt));

  // Serial.print(F("textW: "));
  // Serial.println(textW);
  // Serial.print(F("textH: "));
  // Serial.println(textH);

  // Serial.print(F("SCREEN_WIDTH - 5: "));
  // Serial.println(SCREEN_WIDTH - 5);

  if (textW > SCREEN_WIDTH - 10) // if the text width is larger than the screen, the text is scrolling
  {
    // Serial.println(F("Scrolling"));
    textScrolling = true;

    strncpy(textOnDisplay, txt, strlen(txt) + 1); // copy the text into the textOnDisplay (+1 \0)
    textX = 0;
    tickTime = millis();
  }
  else
  {
    // Serial.println(F("Text centre"));
    textScrolling = false;
    oled.clearDisplay();

    // Serial.print(F("Preparing cursor "));
    oled.setCursor((SCREEN_WIDTH - textW) / 2, (SCREEN_HEIGHT + textH) / 2); // set the cursor in the good position
    // Serial.println(F("...Done"));

    // Serial.print(F("Preparing print "));
    oled.println(txt); // print the text
    // Serial.println(F("...Done"));
    oled.display();
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

  displayOnScreen(macropadConfig.profile[profile].name);

  setRGB(macropadConfig.profile[currentProfile].color[0], macropadConfig.profile[currentProfile].color[1], macropadConfig.profile[currentProfile].color[2]);

  //// oled.drawRoundRect(0, 3, LOGO_HEIGHT + 2, LOGO_HEIGHT + 2, 5, WHITE);
  // oled.drawRamBitmap(1, 4, LOGO_HEIGHT, LOGO_WIDTH, WHITE, macropadConfig.profile[currentProfile].icon, 72);

  Serial.println("A" + String(profile)); // send the profile to the software
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
  if (!digitalRead(encoderKey1Pin)) // select the profile when the encoder key is pressed
  {
    setProfile(currentProfile);
    encoderPressed[1] = false;
    encoderMillis = millis(); // reset 2s timer
  }
}

void resetConfig()
{
  for (byte profile = 0; profile <= 5; profile++)
  {
    char strProfile = '0' + profile;                                                       // convert the profile number to char
    char txt[PROFILE_TEXT_LENGTH] = {'P', 'r', 'o', 'f', 'i', 'l', ' ', strProfile, '\0'}; // create the profile text
    strncpy(macropadConfig.profile[profile].name, txt, PROFILE_TEXT_LENGTH);               // copy the profile text in the profile name config

    macropadConfig.profile[profile].color[0] = 0; // reset the color
    macropadConfig.profile[profile].color[1] = 0; // reset the color
    macropadConfig.profile[profile].color[2] = 0; // reset the color

    // memset(macropadConfig.profile[profile].icon, 0, sizeof(macropadConfig.profile[profile].icon)); // reset the icon

    for (byte i = 0; i < 3; i++) // reset all encoders
    {
      macropadConfig.profile[profile].encoders[i].type = -1;
      for (byte j = 0; j < 3; j++)
      {
        macropadConfig.profile[profile].encoders[i].values[j] = -1;
      }
    }

    for (byte i = 0; i < 6; i++) // reset all keys
    {
      macropadConfig.profile[profile].keys[i].type = -1;
      for (byte j = 0; j < 3; j++)
      {
        macropadConfig.profile[profile].keys[i].values[j] = -1;
      }
    }
  }
}

void setup()
{
  Serial.begin(9600);

  for (byte i = 0; i < 6; i++) // set all keys pins as input
  {
    pinMode(keysPins[i], INPUT);
  }
  // Encoders
  for (byte i = 0; i < 9; i++)
  {
    pinMode(encodersPins[i], INPUT);
  }

  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    while (1)
    {
      Serial.println(F("SSD1306 allocation failed"));
      MEMORY_PRINT_FREERAM;
      setRGB(255, 0, 0);
      delay(200);
      setRGB(0, 0, 0);
      delay(200);
    }
  }
  oled.clearDisplay();
  oled.display();

  delay(200);

  setRGB(255, 0, 0);

  displayOnScreen("Macropad");

  //------------Interrupts----------
  // attachInterrupt(digitalPinToInterrupt(encoderA0Pin), encoder0, LOW);
  // attachInterrupt(digitalPinToInterrupt(encoderA1Pin), encoder1, LOW);
  // attachInterrupt(digitalPinToInterrupt(encoderA2Pin), encoder2, LOW);

  Consumer.begin(); // Start HID

  delay(200);

  setRGB(0, 255, 0);

  delay(200);

  setRGB(0, 0, 255);

  delay(200);

  readEEPROM();

  setProfile(0);
}

bool encoderAState[3];

void loop()
{
  //------------------------------------------------Serial --------------------------------------------------
  static char serialMsg[70];
  static unsigned short serialMsgIndex = 0;
  if (Serial.available() > 0) // if incomming bytes
  {
    serialMsg[serialMsgIndex] = (char)Serial.read(); // read the byte and store it in the array
    serialMsgIndex++;                                // increment the index
  }
  else if (serialMsg[0] != 0) // msg not empty --> all serial msg has been read
  {

    //-------------------Serial msg processing-------------------
    for (byte i = 0; i < strlen(serialMsg); i++)
    {
      if (serialMsg[i] == 13 || serialMsg[i] == 10) // if the char is a new line
      {
        serialMsg[i] = 0; // replace the char by a null char
        serialMsgIndex--; // remove char
        break;
      }
    }

    serialMsgIndex++; // increment the index (null caracter)
    
    // displayOnScreen(serialMsg);

    bool validCmd = true;        // command exist ? (default : yes)
    char command = serialMsg[0]; // get the command (first char)

    short arg[6]; // convert the arguments to short
    //-------------------------Processing command-------------------------
    if (command == 'P') // Ping
    {
      Serial.println("P"); // Pong
    }
    else if (command == 'T') // Set Text
    {
      char *txt = &serialMsg[2]; // remove the command and the space (2 first char)
      //  strcpy(textOnDisplay, txt);
      displayOnScreen(txt);
    }
    else if (command == 'S') // save-config command
    {
      saveToEEPROM(); // save the config in the EEPROM
    }
    else if (command == 'R') // read-config command
    {
      readEEPROM(); // read the config from the EEPROM
    }
    else if (command == 'Z') // reset all the config to 0
    {
      resetConfig();
    }

    else if (command == 'K') // Set Key: K <profile> <n key> <type> <value> <value optional> <value optional>
    {
      getStrArg(serialMsg, serialMsgIndex, arg); // get the arguments
      Serial.println("setKey");
      macropadConfig.profile[arg[0]].keys[arg[1]].type = arg[2]; // Set the type of the key
      for (byte i = 0; i < 3; i++)                               // foreach value in the command
      {
        macropadConfig.profile[arg[0]].keys[arg[1]].values[i] = arg[i + 3]; // Set the value of the key
      }
    }

    else if (command == 'E') // Set encoder action: E <profile> <n encoder> <type> <value> <value optional> <value optional>
    {
      getStrArg(serialMsg, serialMsgIndex, arg);                     // get the arguments
      macropadConfig.profile[arg[0]].encoders[arg[1]].type = arg[2]; // Get the mode and save it in the config
      for (byte i = 0; i < 3; i++)                                   // for all values, save it in the config
      {
        macropadConfig.profile[arg[0]].encoders[arg[1]].values[i] = arg[i + 3]; // Save Values
      }
    }
    else if (command == 'C') // C <profile> <Red: 0 - 255> <Green: 0 - 255> <Blue: 0 - 255>
    {
      getStrArg(serialMsg, serialMsgIndex, arg); // get the arguments
      byte profile = arg[0];
      for (byte i = 0; i < 3; i++)
      {
        macropadConfig.profile[profile].color[i] = arg[i + 1];
      }
      if (profile == currentProfile)
      {
        setRGB(arg[1], arg[2], arg[3]); // Set RGB LED color to rgb value from the command :
      }
    }

    else if (command == 'A') // select/get profile
    {
      if (strlen(serialMsg) <= 2)
      {
        // only command --> request the current profile from the software
        Serial.println("A" + String(currentProfile));
      }
      else // set a value
      {
        getStrArg(serialMsg, serialMsgIndex, arg); // get the arguments
        setProfile(arg[0]);                        // set the profile
      }
    }
    else if (command == 'B') // set/get profile Name
    {
      if (strlen(serialMsg) <= 2) // only the command
      {
        Serial.println("B" + String(macropadConfig.profile[currentProfile].name));
      }
      else // set a value
      {
        getStrArg(serialMsg, serialMsgIndex, arg); // get the arguments

        byte profile = arg[0];         // Get the profile number
        char *newName = &serialMsg[4]; // remove the command, space, profile numbre, space (4 first char)

        strcpy(macropadConfig.profile[profile].name, newName); // set the name
        // strncpy(macropadConfig.profile[profile].name, name, PROFILE_TEXT_LENGTH); // copy the profile name in the config

        if (profile == currentProfile) // if its the current profile
        {
          setProfile(profile); // update the screen
        }
      }
    }
    else if (command == 'I') // set icon
    {
      getStrArg(serialMsg, serialMsgIndex, arg); // get the arguments
      bool endOfTransmission = false;
      unsigned short tab = 0;
      char data[2];
      while (endOfTransmission == false)
      {
        if (Serial.available() > 0)
        {
          char c = Serial.read();
          if (c == '\n')
          {
            endOfTransmission = true;
          }
          else
          {
            // macropadConfig.profile[arg[0]].icon[tab] = c;
            if (tab % 2 == 0)
            {
              data[0] = asciiToHex(c);
            }
            else
            {
              // transform the ascii to hex

              data[1] = asciiToHex(c);
              Serial.print(data[0]);
              Serial.println(data[1]);
              // create hex number
              unsigned char hex = data[0] << 8 | data[1];
              Serial.print("Result: ");
              Serial.println(hex);
              macropadConfig.profile[arg[0]].icon[tab] = hex;
            }

            // combine 2 hex digits into a byte

            tab++;
          }
        }
      }

      for (int t = 0; t < LOGO_SIZE; t++)
      {
        Serial.println(macropadConfig.profile[arg[0]].icon[t]);
      }
    }
    else
    {
      validCmd = false; // the command is not valid
    }

    if (validCmd)
    {
      Serial.println(F("OK")); // ok
    }
    else
    {
      Serial.println(F("ERR")); // error
    }

    serialMsgIndex = 0; // reset the index
    serialMsg[0] = 0;
    // memset(serialMsg, 0, sizeof(serialMsg)); // clear the serialMsg buffer

  } // end of Serial

  //------------------------------------------------Encoders --------------------------------------------------
  // rotary Encoder
  for (byte i = 0; i < 3; i++) // forech encoder , check if thre is a next position and change call the fonction
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
    return;
  }
  //-------------------------------------------------Default mode --------------------------------------------------
  //------------------------------------------------KEYS --------------------------------------------------
  for (byte i = 0; i < 6; i++) // Foreach Keys, check the state, and do the action the key is pressed
  {
    if (digitalRead(keysPins[i])) // if key pressed
    {
      if (keyPressed[i])
      {
        // if the key is already pressed
      }
      else
      {
        char key = '0' + i;                // convert the key number to char
        char strKey[3] = {'K', key, '\0'}; // create the command
        Serial.println(strKey);            // send the command to the serial

        short keyType = macropadConfig.profile[currentProfile].keys[i].type; // get the type of the key
        short keyValues[3];                                                  // create the array of values
        // memcpy(keyValues, macropadConfig.profile[currentProfile].keys[i].values, 6); // 2bytes * 3 values = 6 bytes

        if (keyType == -1) // if the type = -1
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
    short encoderType = macropadConfig.profile[currentProfile].encoders[i].type;         // get the type of the encoder from the config
    short encoderValues[3];                                                              // create the array of values from the config
    memcpy(encoderValues, macropadConfig.profile[currentProfile].encoders[i].values, 6); // get the values from the config (2bytes * 3 values)

    if (encodersPosition[i] != encodersLastValue[i]) // If Encoder has been moved from last time
    {
      if (encodersPosition[i] < encodersLastValue[i]) // Encoder ClockWise Turn
      {
        Serial.println("E" + String(i) + "U"); // Send Encoder Up To the software

        if (encoderType == 0) // If is a System Action type
        {
          Consumer.write(MEDIA_VOL_UP);
          // Consumer.write(HID_CONSUMER_FAST_FORWARD);
        }
        else if (encoderType == 2) // If is a key action (key combination)
        {
          Keyboard.write(encoderValues[1]); // get the ascii code, and press the key
        }
      }
      else // Encoder Anti-ClockWise Turn
      {
        Serial.println("E" + String(i) + "D"); // Send Encoder Up To the software

        if (encoderType == 0)
        {
          Consumer.write(MEDIA_VOL_DOWN);
          // Consumer.write(HID_CONSUMER_REWIND);
        }
        else if (encoderType == 1) // If is a key action
        {
          Keyboard.write(encoderValues[0]); // get the ascii code, and press the key
        }
      }
      encodersLastValue[i] = encodersPosition[i]; // save the value for the next time
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
  else // When encoder 1 key is relese
  {
    // Do the action
    encoderPressed[1] = false;
  }

  // download the software
  if (!digitalRead(encoderKey0Pin) && !digitalRead(encoderKey1Pin) && !digitalRead(encoderKey2Pin)) // install the software
  {
    Consumer.write(ConsumerKeycode(0x223)); // open default web Browser
    delay(1000);
    Keyboard.press(KEY_LEFT_CTRL); // focus the url with CTRL + L
    Keyboard.press(KEY_L);
    delay(10);
    Keyboard.releaseAll();
    delay(100);
    //////////////////////////Keyboard.print(F("https://github.com/xmow49/MacroPad-Software/releases")); // enter the url
    delay(100);
    Keyboard.write(KEY_ENTER); // go to the url
  }

  scrollText();
}
