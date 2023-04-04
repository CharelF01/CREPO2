/*
   Charel Feil
   BTS-IOT1
   BATTLESHIP PROJECT

    Json format:
    {"Mode":0/1,"Player":true/false,"Posi":hit/array of positions}
*/
//include all necessary libraries
#include <ShiftIn.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

//define pins for LED strip
#define LED 26

//define pins for Joystick
#define BUT 27
#define X 33
#define Y 32

//define MQTT options
//#define mqtt_server "192.168.129.210" //School PC
//#define mqtt_server "192.168.178.72" //Home PC
#define mqtt_server "192.168.131.125"
#define mqtt_port 1883
#define mqtt_clientid "239499ca09294ce29c03c6687bbf4341"
#define mqtt_user "user"
#define mqtt_pass "userpw"

//define Wifi SSID & Password
#define SchoolSSID "BTS-HUB"
#define SchoolPass "S0meth!ngL33t"
#define HomeSSID "RFWLAN"
#define HomePass "10112834590419693859"

//initialize Wificlient and Pubsubclient
WiFiClient wifiClient;
PubSubClient client(wifiClient);

//Number of LEDs, MAX = playing field for the board player (top half)
const int MAX = 100; //0-99 = player0, 100-199=player1
const int LEDs = 200;

//set values for Modes
const int MODE_PLAYING = 0;
const int MODE_SENDING = 1;

//create neopixel strip to control LEDs
Adafruit_NeoPixel strip(LEDs, LED, NEO_GRB + NEO_KHZ800);

//define Pins for Shift registers
ShiftIn<2> shift;
const int latchPin = 19;
const int clockPin = 23;
const int dataPin = 18;

// PL pin 1
const int load = 22;
// CE pin 15
const int clockEnablePin = 34;
// Q7 pin 7
const int dataIn = 35;
// CP pin 2
const int clockIn = 5;

int numOfRegisters = 2;
byte* registerState;

//arrays to hold the positions of the ships of both players
int ennemyPos[18];
int ownPos[18];

//arrays to hold both players hits
int player1Hits[MAX + 1];
int player0Hits[MAX + 1];

//arrays for tha "animations"
int loss[13] = {23, 26, 33, 36, 63, 64, 65, 66, 72, 77, 81, 88};
int won[13] = {23, 26, 33, 36, 61, 68, 72, 77, 83, 84, 85, 86};
int start[15] = {23, 24, 25, 26, 32, 42, 53, 54, 55, 66, 76, 85, 84, 83, 82};

//variables for array positions
int ennemyArPos = 0;
int ownArPos = 0;

int player0HitsArPos = 0;
int player1HitsArPos = 0;


//holds what player is playing and who won
bool player = 0;
bool turn = 0;
int win = 0;
bool correctShips = false;

//position of cursor
int pos = 100;

//temporary array and array position of the board ship positions
int ownTemp[18];
int ownTempPos;
//counter for how many times the ships have been sent
int countSent = 0;

//variable to see if the game started
bool started = false;

//initialize functions
void callback(char *topic, byte *payload, unsigned int length);
void connect();
uint32_t Wheel(byte WheelPos);
void rainbowCycle(uint8_t wait);
void addHit(bool pl, int hit);
void reset();
void randomPositions();
void drawField();

void setup() {
  pinMode(BUT, INPUT_PULLUP);
  shift.begin(load, clockEnablePin, dataIn, clockIn);
  //Initialize array
  registerState = new byte[numOfRegisters];
  for (size_t i = 0; i < numOfRegisters; i++) {
    registerState[i] = 0;
  }

  //set the pins for the registers
  pinMode(load, OUTPUT);
  pinMode(clockEnablePin, OUTPUT);
  pinMode(clockIn, OUTPUT);
  pinMode(dataIn, INPUT);
  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  //to be sure set the first positions of the ships to -1 on setup
  ennemyPos[0] = -1;
  ownPos[0] = -1;

  //set internal pullup for the button on the joystick
  

  //begin the neopixel strip and set brightness
  strip.begin();
  strip.setBrightness(30);

  //clear the strip of any unwanted pixels
  strip.clear();
  strip.show();

  //begin serial connection for debugging
  Serial.begin(115200);

  //connect to wifi
  Serial.print("Wifi connecting ");
  WiFi.begin(SchoolSSID, SchoolPass);
  //WiFi.begin(HomeSSID, HomePass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  // print out the received IP adress
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //flash leds to show that the wifi is connected
  strip.fill(strip.Color(0, 255, 0), 0, LEDs);
  strip.show();
  delay(500);
  strip.clear();
  strip.show();
  //connect to mqtt
  connect();
}

void loop() {
 
  //check if the game started
  if (started) {
    if (!client.connected())
    {
      connect();
    }
    client.loop();
    //checks if the ship positions have been set
    if ((ennemyPos[0] == -1) || (ownPos[0] == -1)) {
      //check player
      
      if (countSent < 1) {
        //you can correct for any errors of the magnets by pushing the button 
        
        if (digitalRead(BUT) == LOW) {
          correctShips = true;
        }
       
        if (correctShips) {
          //movement of cursor
          DynamicJsonDocument doc(1024);
          char ser[1024];
          int readx = analogRead(X);
          int readYs = analogRead(Y);
          int x = (readx / 4095.0) * 100;
          int y = (readYs / 4095.0) * 100;
          //moving in the x direction
          if (x < 10) {
            strip.setPixelColor(pos, strip.Color(0, 0, 0));
            if (pos == 100) {
              pos = LEDs - 1;
            }
            else {
              pos--;
            }
          }
          else if (x > 90) {
            strip.setPixelColor(pos, strip.Color(0, 0, 0));
            if (pos == LEDs - 1) {
              pos = 100;
            }
            else {
              pos++;
            }
          }

          //moving in the y direction TO BE FIXED
          else if (y < 10) {
            strip.setPixelColor(pos, strip.Color(0, 0, 0));
            pos -= 10;
            if (pos < 100)
              pos = LEDs - (100 - pos);

          }
          else if (y > 90) {
            strip.setPixelColor(pos, strip.Color(0, 0, 0));
            pos += 10;
            if (pos >= LEDs) {
              pos = pos - LEDs + MAX;

            }

          }
          //Serial.print("\n Position: "); Serial.println(pos);
          //set the current position to blue
          strip.setPixelColor(pos, strip.Color(0, 0, 255));

          //show everything
          strip.show();

          //if the button is pressed now the position gets added
          if (digitalRead(BUT) == LOW) {
            strip.setPixelColor(pos, strip.Color(0, 0, 0));
            Serial.println("BUTTON PRESSED");
            bool match = false;
            if (ownTempPos != 0) {
              for (int i = 0; i < ownTempPos; i++) {
                if (ownTemp[i] == pos)
                  match = true;
              }
            }

            if (!match) {

              ownTemp[ownTempPos] = pos;
              Serial.println(ownTemp[ownTempPos]);
              ownTempPos++;
            }

          }
          //if the array is full send the data
          if (ownTempPos == 18) {
            doc["Mode"] = MODE_SENDING;
            doc["Player"] = false;
            for (int i = 0; i < 18; i++) {
              doc["Posi"][i] = ownTemp[i] - 100;
            }

            serializeJson(doc, ser);
            client.publish("Battleship", ser);
            countSent = 1;
            pos = 0;
            player=true;
            turn=true;
            delay(1000);
          }
          if (ownTempPos != 0) {
            for (int i = 0; i < ownTempPos; i++) {
              strip.setPixelColor(ownTemp[i], strip.Color(0, 0, 255));
            }
          }
          delay(100);
        }
        //go through the matrix with the shift registers
        else {
          for (int i = 15; i >= 6; i--) {
            // ST_CP LOW to keep LEDs from changing while reading serial data
            regWrite(i, HIGH);
            delay(100);
            shift.read();
            for (int j = 0; j < 10; j++) {
              int shiftState = shift.state(j);
              if (shiftState == 1) {
                ownTemp[ownTempPos] = (((i - 6) * 10) + j) + 100;
                ownTempPos++;
                strip.setPixelColor((((i - 6) * 10) + j) + 100, strip.Color(0, 0, 255));
                strip.show();
              }
            }
            delay(100);
            //Serial.println(incoming, BIN);
            Serial.println(i - 5);
            regWrite(i, LOW);
            delay(200);
            //sOut=sOut>>1;
          }
        }
      }
    }
    //when all ships have been set
    else {
      //movement of cursor
      DynamicJsonDocument doc(1024);
      char ser[1024];
      int readx = analogRead(X);
      int readYs = analogRead(Y);
      int x = (readx / 4095.0) * 100;
      int y = (readYs / 4095.0) * 100;
      delay(100);
      // check if someone won and display the right animation
      if (win == 1) {

        if (digitalRead(BUT) == LOW) {
          reset();
        }
        else {
          strip.fill(strip.Color(0, 0, 0), 0, MAX);
          strip.show();
          delay(100);
          for (int i = 0; i < 12; i++) {
            strip.setPixelColor(won[i], strip.Color(0, 255, 0));
          }
          strip.show();
        }
        delay(1000);
      }
      else if (win == 2) {
        if (digitalRead(BUT) == LOW) {
          reset();
        }
        else {
          strip.fill(strip.Color(0, 0, 0), 0, MAX);
          strip.show();
          delay(100);
          for (int i = 0; i < 12; i++) {
            strip.setPixelColor(loss[i], strip.Color(255, 0, 0));
          }
          strip.show();
        }
      }
      else if (win == 0) {
        //if the player is on the board check movement
        if (player == 0) {


          //moving in the x direction
          if (x < 10) {
            strip.setPixelColor(pos, strip.Color(0, 0, 0));
            if (pos == 0) {
              pos = MAX - 1;
            }
            else {
              pos--;
            }
          }
          else if (x > 90) {
            strip.setPixelColor(pos, strip.Color(0, 0, 0));
            if (pos == MAX - 1) {
              pos = 0;
            }
            else {
              pos++;
            }
          }

          //moving in the y direction 
          else if (y < 10) {
            strip.setPixelColor(pos, strip.Color(0, 0, 0));
            pos -= 10;
            if (pos < 0)
              pos = MAX + pos;

          }
          else if (y > 90) {
            strip.setPixelColor(pos, strip.Color(0, 0, 0));
            pos += 10;
            if (pos >= MAX) {
              pos = pos - MAX;

            }

          }
          Serial.print("\n Position: "); Serial.println(pos);
          //set the current position to blue
          strip.setPixelColor(pos, strip.Color(0, 0, 255));

          //show everything
          strip.show();

          //shoot the selected position
          if (digitalRead(BUT) == LOW) {
            strip.setPixelColor(pos, strip.Color(0, 0, 0));
            Serial.println("BUTTON PRESSED");
            bool match = false;
            if (player0HitsArPos != 0) {
              for (int i = 0; i < player0HitsArPos; i++) {
                if (player0Hits[i] == pos)
                  match = true;
              }
            }

            if (!match) {

              doc["Mode"] = MODE_PLAYING;
              doc["Player"] = player;
              doc["Posi"] = pos;
              serializeJson(doc, ser);
              client.publish("Battleship", ser);

            }

          }
          

          //if the clicked positions array isn't empty then fill all the clicked positions with a green color
          if (player0HitsArPos != 0) {
            int count = 0;
            for (int i = 0; i < player0HitsArPos; i++) {
              strip.setPixelColor(player0Hits[i], strip.Color(0, 255, 0));
              for (int j = 0; j < 17; j++) {
                if (player0Hits[i] == ennemyPos[j]) {
                  count++;
                  strip.setPixelColor(player0Hits[i], strip.Color(255, 0, 0));
                }
              }
            }
            if (count == 17) {
              win = 1;
            }
          }


          delay(100);

        }

      }
    }
  }
  //when the game hasn't started display the S animation
  else {
    
    if (digitalRead(BUT) == LOW) {
      started = true;
    }
    Serial.println("Not started yet");
    for (int i = 0; i < 15; i++) {
      strip.setPixelColor(start[i], strip.Color(0, 255, 0));
      strip.show();
      //delay(1);
    }
    delay(100);
    strip.fill(strip.Color(0, 0, 0), 0, MAX);
    strip.show();
    
    delay(1000);
  }
  
}

//function to check for duplicates
bool checkDuplicate(int s, int arr[]) {
  for (int i = 0; i < sizeof(arr); i++) {
    if (s == arr[i]) {
      return true;
    }
  }
  return false;
}
//Code for random positions used for testing
/*void randomPositions(){
      int shipNum=5;
      int sendArPos=0;
      int shipLength=5;
      for (int i = 0; i < 2; i++) {
         DynamicJsonDocument doc(1024);
        char ser[1024];
        doc["Mode"] = MODE_SENDING;
         doc["Player"] = player;

        for (int j = 0; j < shipNum; j++) {
          if(j==0)shipLength=5;
          else if(j==1)shipLength=4;
          else if((j==2)||(j==3))shipLength=3;
          else if(j==4)shipLength=2;
          Serial.println("sending");
          ennemyPos[i] = (int)(random(0, 100));
            ownPos[i] = (int)(random(0, 100));
            bool rotation = true;(bool)(random(0,1));
            int sPos=(int)(random(0, 100));
            int ship[shipLength];
            while(checkDuplicate(sPos, doc["Posi"][])){
                sPos=(int)(random(0, 100));
              }
            doc["Posi"][sendArPos] = sPos;
            sendArPos++;
            bool matched=false;
            int l=0;
            while(matched==false){

              if(rotation==false){
              for(int k=0; k<shipLength; k++){
                if(checkDuplicate(sPos+k, doc["Posi"][]){
                  sPos=(int)(random(0, 100));
                  break;
                }
              }

              }
              else{
                for(int k=0; k<shipLength*10; k+=10){
                  if(checkDuplicate(sPos+k, doc["Posi"][]){
                    sPos=(int)(random(0, 100));
                    break;
                  }
                }
              }
            }

            for(int k=0; k<shipLength*10; k+=10){
                doc["Posi"][sendArPos] = (sPos+k);
                sendArPos++;
              }
          Serial.print("EnnemyPos: ");Serial.println(ennemyPos[i]);
            Serial.print("ownPos: ");Serial.println(ownPos[i]);
        }
        serializeJson(doc, ser);
        client.publish("Battleship", ser);
        delay(1000);
        player = !player;

      }
      turn=player;
      Serial.print("turn: ");Serial.println(turn);
      Serial.print("player: "); Serial.println(player);


  }
*/
//callback function to handle incoming mqtt messages
void callback(char *topic, byte *payload, unsigned int length)
{
  DynamicJsonDocument doc(1024);
  String data = "";
  for (int i = 0; i < length; i++)
    data += (char)payload[i];
  // simply display the received data on the serial monitor
  Serial.println("Got: " + (String)topic + " value: " + data);
  deserializeJson(doc, data);
  //read mode and player
  int modes = doc["Mode"];
  player = doc["Player"];

//check mode if sending or playing
  if (modes == 0) {
    if (turn == player) {
      //read position, hit the other player, draw the field and change turn and player
      int posi = doc["Posi"];
      addHit(player, posi);
      drawField();
      player = !player;
      turn = !turn;

    }
    else {
      player = turn;
    }
  }
  else if (modes == 1) {
    //read out position array and fill out own/ennemy positions
    turn = player;

    int posi[18];
    for (int i = 0; i < 18; i++) {
      posi[i] = doc["Posi"][i];
    }
    if (player == false) {

      if (ownArPos != 18) {
        /*for(int i =0; i<ownArPos; i++){
          if(posi==ownPos[i])
            match=true;
          }*/



        for (int i = 0; i < 18; i++) {
          ownPos[i] = posi[i];
          //Serial.print("ownPos: "); Serial.println(ownPos[ownArPos]);
          ownArPos++;
        }
      }


    }
    else {

      if (ennemyArPos != 18) {
        /*for(int i =0; i<ennemyArPos; i++){
          if(posi==ennemyPos[i])
            match=true;
          }*/
        for (int i = 0; i < 18; i++) {
          ennemyPos[i] = posi[i];
          //Serial.print("EnnemyPos: "); Serial.println(ennemyPos[ennemyArPos]);
          ennemyArPos++;
        }

      }

    }
    /*for (int j = 0; j < 18; j++) {
      Serial.print("\n ennemyPos["); Serial.print(j); Serial.print("]"); Serial.println(ennemyPos[j]);
      Serial.print("\n ownPos["); Serial.print(j); Serial.print("]"); Serial.println(ownPos[j]);
    }*/

    drawField();
  }
}
//function to draw everything on the leds
void drawField() {
  strip.setPixelColor(pos, strip.Color(0, 0, 0));
  int hitAr = player1HitsArPos;
  for (int i = 0; i < 18; i++) {
    bool match = false;
    for (int j = 0; j < hitAr; j++) {
      if (ownPos[i] == player1Hits[j]) {
        match = true;
      }
    }
    if (match) strip.setPixelColor(ownPos[i] + 100, strip.Color(255, 0, 0));
    else strip.setPixelColor(ownPos[i] + 100, strip.Color(0, 0, 255));
  }
  strip.show();
  int hitArPos = 0;
  if (player == false) {
    //Serial.print("\n Position: "); Serial.println(pos);
    hitArPos = player0HitsArPos;
    strip.setPixelColor(pos, strip.Color(0, 0, 0));
  }
  else hitArPos = player1HitsArPos;
  if (hitArPos != 0) {





    int count = 0;
    for (int i = 0; i < hitArPos; i++) {
      if (player == false) {
        bool match = false;
        for (int j = 0; j < sizeof(ennemyPos); j++) {
          if (player0Hits[i] == ennemyPos[j]) match = true;
        }
        if (match) {
          strip.setPixelColor(player0Hits[i], strip.Color(255, 0, 0));
        }
        else strip.setPixelColor(player0Hits[i], strip.Color(0, 255, 0));
      }
      else {
        bool match = false;
        for (int j = 0; j < sizeof(ownPos); j++) {
          if (player1Hits[i] == ownPos[j]) match = true;
        }
        if (match) {
          strip.setPixelColor(player1Hits[i] + 100, strip.Color(255, 0, 0));
        }
        else strip.setPixelColor(player1Hits[i] + 100, strip.Color(0, 255, 0));
      }
    }



  }
  for (int i = 0; i < player1HitsArPos; i++) {
    Serial.print("\n Player1Hit["); Serial.print(i); Serial.print("] "); Serial.println(player1Hits[i]);
  }
  for (int i = 0; i < player0HitsArPos; i++) {
    Serial.print("\n Player0Hit["); Serial.print(i); Serial.print("] "); Serial.println(player0Hits[i]);
  }

  strip.show();
}
//function to add a hit to the array
void addHit(bool pl, int hit) {
  int hitArPos;
  bool match = false;
  if (pl == false) {
    hitArPos = player0HitsArPos;
    if (hitArPos != 0) {
      for (int i = 0; i < hitArPos; i++) {
        if (player0Hits[i] == hit)
          match = true;
      }
    }
    if (!match) {
      player0Hits[player0HitsArPos] = hit;
      player0HitsArPos++;
    }
  }
  else {
    hitArPos = player1HitsArPos;
    if (hitArPos != 0) {
      for (int i = 0; i < hitArPos; i++) {
        if (player1Hits[i] == hit)
          match = true;
      }
    }
    if (!match) {
      player1Hits[player1HitsArPos] = hit;
      player1HitsArPos++;
    }
  }
  match = false;
}
//function to connect to mqtt
void connect()
{
  // set the server data
  client.setServer(mqtt_server, mqtt_port);
  // define the callback function
  client.setCallback(callback);

  // loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // attempt to connect
    if (client.connect(mqtt_clientid, mqtt_user, mqtt_pass))
    {
      Serial.println("connected");

      client.subscribe("Battleship");
      strip.fill(strip.Color(0, 0, 255), 0, LEDs);
      strip.show();
      delay(500);
      strip.clear();
      strip.show();
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // wait 5 seconds before retrying
      strip.fill(strip.Color(255, 0, 0), 0, LEDs);
      strip.show();
      delay(500);
      strip.clear();
      strip.show();
      delay(5000);
    }
  }
}
//resets the board
void reset() {
  ennemyPos[0] = -1;
  ownPos[0] = -1;
  started = false;
  win = 0;
  player0HitsArPos = 0;

  strip.clear();
  strip.show();
}
//function to write to the registers
void regWrite(int pin, bool state) {
  //Determines register
  int reg = pin / 8;
  //Determines pin for actual register
  int actualPin = pin - (8 * reg);

  //Begin session
  digitalWrite(latchPin, LOW);

  for (int i = 0; i < numOfRegisters; i++) {
    //Get actual states for register
    byte* states = &registerState[i];

    //Update state
    if (i == reg) {
      bitWrite(*states, actualPin, state);
    }

    //Write
    shiftOut(dataPin, clockPin, MSBFIRST, *states);
  }

  //End session
  digitalWrite(latchPin, HIGH);
}
