#include <WiFiManager.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Firebase_ESP_Client.h>
#include "OneWire.h"
#include "DallasTemperature.h"

//Provide the token generation process info.
#include <addons/TokenHelper.h>

//Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>


//For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

//For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

/* 2. Define the API Key */
#define API_KEY "AIzaSyBj5nWN6kpJxqyIvAdpltV4fD9mb0GBUaQ"

/* 3. Define the RTDB URL */
#define DATABASE_URL "fish-feeder-01-default-rtdb.firebaseio.com" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "fftest@test.com"
#define USER_PASSWORD "test12345"

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

unsigned long count = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org",19800);

#define SensorPin 5            //pH meter Analog output to Arduino Analog Input 0
#define Offset 41.02740741 
#define ArrayLenth  40    
int pHArray[ArrayLenth];   //Store the average value of the sensor feedback
int pHArrayIndex = 0;

String stimer, stimer1;
String Str[]={"00:00","00:00","00:00"};
int i,feednow=0;

#define ONE_WIRE_BUS 18
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float tempAvg = 0.0;
unsigned long starttime=0, nowtime=0;
const unsigned long period = 60000;

void setup() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    // it is a good practice to make sure your code sets wifi mode how you want it.

    // put your setup code here, to run once:
    Serial.begin(115200);
    
    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    //wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
     res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    //res = wm.autoConnect("AutoConnectAP","password"); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }
    timeClient.begin();
    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
    config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  //Or use legacy authenticate method
  //config.database_url = DATABASE_URL;
  //config.signer.tokens.legacy_token = "<database secret>";

  //To connect without auth in Test Mode, see Authentications/TestMode/TestMode.ino

  //////////////////////////////////////////////////////////////////////////////////////////////
  //Please make sure the device free Heap is not lower than 80 k for ESP32 and 10 k for ESP8266,
  //otherwise the SSL connection will fail.
  //////////////////////////////////////////////////////////////////////////////////////////////

  Firebase.begin(&config, &auth);

  //Comment or pass false value when WiFi reconnection will control by your code or third party library
  Firebase.reconnectWiFi(true);

  Firebase.setDoubleDigits(5);
  pinMode(4, OUTPUT);
  sensors.begin();
  starttime = millis();
}

void loop() {
//  if(Firebase.ready()){
    nowtime = millis();
    if(nowtime - starttime > period){
       tempAvg = tempCal();
       static unsigned long samplingTime = millis();
        static unsigned long printTime = millis();
        static float pHValue, voltage;
  
        pHArray[pHArrayIndex++] = analogRead(SensorPin);
        if (pHArrayIndex == ArrayLenth)pHArrayIndex = 0;
          voltage = avergearray(pHArray, ArrayLenth) * 3.3 / 4096;
          pHValue = -19.18518519 * voltage + Offset;
        timeClient.update();
        unsigned long epochTime = timeClient.getEpochTime();
        Serial.printf("Set ph... %s\n", Firebase.RTDB.setFloat(&fbdo, F("dev1/ph"), pHValue) ? "ok" : fbdo.errorReason().c_str());
        delay(5000);
        Serial.printf("Set temp... %s\n", Firebase.RTDB.setFloat(&fbdo, F("dev1/temp"), tempAvg) ? "ok" : fbdo.errorReason().c_str());
        delay(5000);
        Serial.printf("Set timestamp... %s\n", Firebase.RTDB.setFloat(&fbdo, F("dev1/timestamp"), epochTime) ? "ok" : fbdo.errorReason().c_str());
        
      starttime = nowtime;
    }
    Serial.printf("Get string... %s\n", Firebase.RTDB.getInt(&fbdo, F("dev1/feednow")) ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
    feednow = fbdo.to<int>();
    Serial.printf("Get string... %s\n", Firebase.RTDB.getInt(&fbdo, F("dev1/delay")) ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
    int delay1 = fbdo.to<int>();
    if (feednow==1){
        
       digitalWrite(4, HIGH);
        delay(delay1);
        digitalWrite(4, LOW);
//      servo.writeMicroseconds(1000); // rotate clockwise
//      delay(700); // allow to rotate for n micro seconds, you can change this to your need
//      servo.writeMicroseconds(1500); // stop rotation
      feednow = 0;
      Serial.printf("Set int... %s\n", Firebase.RTDB.setInt(&fbdo, F("dev1/feednow"), feednow) ? "ok" : fbdo.errorReason().c_str());
      Serial.println("Fed");
    }
    
    else{ // Scheduling feed
      String path = "dev1/timers/timer"+String(i);
      Serial.printf("Get string... %s\n", Firebase.RTDB.getString(&fbdo, F("dev1/timers/timer0")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str());
      stimer = fbdo.to<String>();
      Str[0]= stimer.substring(9,14);
    timeClient.update();
    char dateBuffer[12];
    sprintf(dateBuffer,"%02u:%02u",timeClient.getHours(),timeClient.getMinutes());
    Serial.println(Str[0].compareTo(dateBuffer));


    Serial.printf("Get string... %s\n", Firebase.RTDB.getString(&fbdo, F("dev1/timers/timer1")) ? fbdo.to<const char *>() : fbdo.errorReason().c_str());
      stimer1 = fbdo.to<String>();
      Str[1]= stimer1.substring(9,14);
    timeClient.update();
    Serial.println(Str[1].compareTo(dateBuffer));

    
    if (Str[0].equals(dateBuffer) || Str[1].equals(dateBuffer)){
      Serial.println("Entered");
      digitalWrite(4, HIGH);
        delay(delay1);
        digitalWrite(4, LOW);
        //      servo.writeMicroseconds(1000); // rotate clockwise
//      delay(700); // allow to rotate for n micro seconds, you can change this to your need
//      servo.writeMicroseconds(1500); // stop rotation
      Serial.println("Success");
      delay(60000);
    }
  }
  Str[0]="00:00";
  Str[1]="00:00";
//  }
}

float tempCal(){
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  return temperatureC;
}


double avergearray(int* arr, int number) {
  int i;
  int max, min;
  double avg;
  long amount = 0;
  if (number <= 0) {
    Serial.println("Error number for the array to avraging!/n");
    return 0;
  }
  if (number < 5) { //less than 5, calculated directly statistics
    for (i = 0; i < number; i++) {
      amount += arr[i];
    }
    avg = amount / number;
    return avg;
  } else {
    if (arr[0] < arr[1]) {
      min = arr[0]; max = arr[1];
    }
    else {
      min = arr[1]; max = arr[0];
    }
    for (i = 2; i < number; i++) {
      if (arr[i] < min) {
        amount += min;      //arr<min
        min = arr[i];
      } else {
        if (arr[i] > max) {
          amount += max;  //arr>max
          max = arr[i];
        } else {
          amount += arr[i]; //min<=arr<=max
        }
      }//if
    }//for
    avg = (double)amount / (number - 2);
  }//if
  return avg;
}
