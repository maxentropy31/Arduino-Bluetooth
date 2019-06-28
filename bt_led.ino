#include <AltSoftSerial.h>

#define rxPin 8 //BT tx --> rxPin
#define txPin 9 //BT rx --> txPin
#define numLeds 3
AltSoftSerial btSerial;

int leds[] = {10, 11, 6};
String ledBrightness[] = {"000", "000", "000"};
bool ledStatus[] = {false, false, false};

const byte length = 32;
char data[length];
bool newData = false;


void setup(){
  for(int i = 0; i < numLeds; i++){
    pinMode(leds[i], OUTPUT);
  }
  Serial.begin(9600);
  btSerial.begin(9600);
  Serial.println("<Arduino Is Ready>");
}

void loop(){
  readWithMarkers();
  if(newData){
    processData();
  }
}

void readWithMarkers(){
  char startMarker = '[';
  char endMarker = ']';
  static bool inProgress = false;
  static byte i = 0;
  char rc;
  if(btSerial.available() > 0){
    rc = btSerial.read();
    if(inProgress){
      if(rc != endMarker){
        data[i] = rc;
        i++;
        if(i >= length){
          i = length - 1;
        }
      }else{
        data[i] = '\0';//terminate the string
        inProgress = false;
        i = 0;
        newData = true;
      }
    }else if(rc == startMarker){
      inProgress = true;
    }
  }
}

void processData(){
  int ledNum = data[1] - 49;// -48 ASCII -> integer, -1 to account for array indexing
  if(data[0] == 'L'){
    staticLedCommand(ledNum);
  }else if(data[0] == 'P'){//update everything
    for(int i = 0; i < numLeds; i++){
      if(ledStatus[i]){
        pingApp("L", String(i), "1");
      }else{
        pingApp("L", String(i), "0");
      }
    }
  }else if(data[0] == 'S'){
    sliderCommand(ledNum);
  }
  data[0] = '\0';
  newData = false;
}

void staticLedCommand(int ledNum){
  data[2] = data[2] - 48;//ASCII -> int
  if(data[2] == 1){
    turnLedOn(ledNum, 255);
  }else if(data[2] == 0){
    turnLedOff(ledNum);
  }
}

void sliderCommand(int ledNum){
  //Example: [S2162]; S-type, 2-ledNum, 162-val
  String val = "";
  //build up the value recieved into a string
  for(int i = 2; i < 5; i++){
    val += data[i] - 48;
  }
  if(val.toInt() == 0){
    turnLedOff(ledNum);
  }else{
    Serial.print("Led ");
    Serial.print(ledNum + 1);
    Serial.print(" brightness: ");
    Serial.println(val);
    turnLedOn(ledNum, val.toInt());
  }
  pingApp("S", String(ledNum), val);//update the slider & button for the led
}

void turnLedOn(int ledNum, int val){
  ledBrightness[ledNum] = formatNumber(val);
  analogWrite(leds[ledNum], val);
  ledStatus[ledNum] = true;
}

void turnLedOff(int ledNum){
  ledBrightness[ledNum] = formatNumber(0);
  analogWrite(leds[ledNum], 0);
  ledStatus[ledNum] = false;
}

String buildCommand(String type, String ledNum, String instruction){
  //example led command: [L,3,1]
  //example slider command: [S,2,163]
  String result = "";
//  Serial.println(ledNum);
  if(type == "L"){
    result = "[" + type + "," + ledNum + "," + instruction + "]";
  }else if(type == "S"){
//    Serial.println(ledNum);
    result = "[" + type + "," + ledNum + "," + formatNumber(instruction.toInt()) + "]";
  }
//  Serial.println(result);
  return result;
}

void pingApp(String type, String ledNum, String instruction){
    int tmp = ledNum.toInt();
    tmp++;//account for array indexing
    String result = buildCommand(type, String(tmp), instruction);//ledNum + 1 to reverse array indexing
    btSerial.print(result);//send the fully built command
}

String formatNumber(int val){
  String result = String(val);
  if(result.length() < 3){
    for(int i = 0; i <= (3-result.length()); i++){
      result = "0" + result;
    }
  }
  return result;
}
