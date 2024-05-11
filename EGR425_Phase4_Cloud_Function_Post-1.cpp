#include <M5Core2.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "WiFi.h"
#include "FS.h"                 // SD Card ESP32
#include <EEPROM.h>             // read and write from flash memory
#include <NTPClient.h>          // Time Protocol Libraries
#include <WiFiUdp.h>            // Time Protocol Libraries
#include <Adafruit_VCNL4040.h>  // Sensor libraries
#include "Adafruit_SHT4x.h"     // Sensor libraries

////////////////////////////////////////////////////////////////////
// TODO 1: Enter your URL addresses
////////////////////////////////////////////////////////////////////
const String URL_GCF_UPLOAD = "https://us-west2-egr425-project4.cloudfunctions.net/gcf-425-demo";
String getURL = "https://us-west2-egr425-project4.cloudfunctions.net/gcf-425-demo-3/getAveragedData";

////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// TODO 2: Enter your WiFi Credentials
////////////////////////////////////////////////////////////////////
//String wifiNetworkName = "CBU-up200";
String wifiNetworkName = "CBU";
String wifiPassword = "";

// Initialize library objects (sensors and Time protocols)
Adafruit_VCNL4040 vcnl4040 = Adafruit_VCNL4040();
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Time variables
unsigned long lastTime = 0;
unsigned long timerDelayMs = 2000; 
unsigned long startMillis; 
unsigned long currentMillis;
const unsigned long period = 5000;

////////////////////////////////////////////////////////////////////
// TODO 3: Device Details Structure
////////////////////////////////////////////////////////////////////
struct deviceDetails {
    int prox;
    int ambientLight;
    int whiteLight;
    double rHum;
    double temp;
    double accX;
    double accY;
    double accZ;
};

////////////////////////////////////////////////////////////////////
// Method header declarations
////////////////////////////////////////////////////////////////////
int httpGetWithHeaders(String serverURL, String *headerKeys, String *headerVals, int numHeaders);
bool gcfGetWithHeader(String serverUrl, String userId, time_t time, deviceDetails *details);
String generateM5DetailsHeader(String userId, time_t time, deviceDetails *details);
int httpPostFile(String serverURL, String *headerKeys, String *headerVals, int numHeaders, String filePath);
bool gcfPostFile(String serverUrl, String filePathOnSD, String userId, time_t time, deviceDetails *details);
String writeDataToFile(byte * fileData, size_t fileSizeInBytes);
int getNextFileNumFromEEPROM();
double convertFintoC(double f);
double convertCintoF(double c);
void drawScreen1();
void drawScreen2();
void drawScreen3();
int getWithParams();
String httpGETRequest(const char *serverURL);

//buttons
Button userId1(10, 20, 150, 40, "user1");
Button userId2(165, 20, 100, 40, "user2");
Button userId3(270, 20, 50, 40, "userall");
Button time1(40, 80, 40, 40, "5s");
Button time2(90, 80, 60, 40, "10s");
Button time3(155, 80, 60, 40, "30s");
Button time4(220, 80, 60, 40, "60s");
Button data1(40, 140, 80, 40, "Temp");
Button data2(125, 140, 80, 40, "rHum");
Button data3(210, 140, 60, 40, "al");
Button getAvg(40, 190, 235, 40, "get average");
bool userId1b = false;
bool userId2b = false;
bool userId3b = false;
bool time1b = false;
bool time2b = false;
bool time3b = false;
bool time4b = false;
bool data1b = false;
bool data2b = false;
bool data3b = false;
bool getAvgb = false;




///////////////////////////////////////////////////////////////
// Put your setup code here, to run once
///////////////////////////////////////////////////////////////
void setup()
{
    ///////////////////////////////////////////////////////////
    // Initialize the device
    ///////////////////////////////////////////////////////////
    M5.begin();
    M5.IMU.Init();

    ///////////////////////////////////////////////////////////
    // Initialize Sensors
    ///////////////////////////////////////////////////////////
    // Initialize VCNL4040
    if (!vcnl4040.begin()) {
        Serial.println("Couldn't find VCNL4040 chip");
        while (1) delay(1);
    }
    Serial.println("Found VCNL4040 chip");

    // Initialize SHT40
    if (!sht4.begin())
    {
        Serial.println("Couldn't find SHT4x");
        while (1) delay(1);
    }
    Serial.println("Found SHT4x sensor");
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);

    ///////////////////////////////////////////////////////////
    // Connect to WiFi
    ///////////////////////////////////////////////////////////
    WiFi.begin(wifiNetworkName.c_str(), wifiPassword.c_str());
    Serial.printf("Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.print("\n\nConnected to WiFi network with IP address: ");
    Serial.println(WiFi.localIP());

    ///////////////////////////////////////////////////////////
    // Init time connection
    ///////////////////////////////////////////////////////////
    timeClient.begin();
    timeClient.setTimeOffset(3600 * -7);
    drawScreen1();
    startMillis = millis();
}

///////////////////////////////////////////////////////////////
// Put your main code here, to run repeatedly
///////////////////////////////////////////////////////////////
String userId = "DanGrissom";
String timeString = "0";
String data = "none";
bool display1 = true;
bool display2 = false;
bool display3 = false;

void loop()
{
    currentMillis = millis();
    // Read in button data and store
    M5.update();

    ///////////////////////////////////////////////////////////
    // Read Sensor Values
    ///////////////////////////////////////////////////////////
    // Read VCNL4040 Sensors
    if(currentMillis - startMillis >= period && display1){ //&& display1
    Serial.printf("Live/local sensor readings:\n");
    uint16_t prox = vcnl4040.getProximity();
    uint16_t ambientLight = vcnl4040.getLux();
    uint16_t whiteLight = vcnl4040.getWhiteLight();
    Serial.printf("\tProximity: %d\n", prox);
    Serial.printf("\tAmbient light: %d\n", ambientLight);
    Serial.printf("\tRaw white light: %d\n", whiteLight);

    // Read SHT40 Sensors
    sensors_event_t rHum, temp;
    sht4.getEvent(&rHum, &temp); // populate temp and humidity objects with fresh data
    Serial.printf("\tTemperature: %.2fF\n", convertCintoF(temp.temperature));
    Serial.printf("\tHumidity: %.2f %%rH\n", rHum.relative_humidity);

    // Read M5's Internal Accelerometer (MPU 6886)
    float accX;
    float accY;
    float accZ;
    M5.IMU.getAccelData(&accX, &accY, &accZ);
    accX *= 9.8;
    accY *= 9.8;
    accZ *= 9.8;
    Serial.printf("\tAccel X=%.2fm/s^2\n", accX);        
    Serial.printf("\tAccel Y=%.2fm/s^2\n", accY);
    Serial.printf("\tAccel Z=%.2fm/s^2\n", accZ);

    ///////////////////////////////////////////////////////////
    // Setup data to upload to Google Cloud Functions
    ///////////////////////////////////////////////////////////
    // Get current time as timestamp of last update
    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();
    unsigned long long epochMillis = ((unsigned long long)epochTime)*1000;
    struct tm *ptm = gmtime ((time_t *)&epochTime);
    Serial.printf("\nCurrent Time:\n\tEpoch (ms): %llu", epochMillis);
    Serial.printf("\n\tFormatted: %d/%d/%d ", ptm->tm_mon+1, ptm->tm_mday, ptm->tm_year+1900);
    Serial.printf("%02d:%02d:%02d%s\n\n", timeClient.getHours() % 12, timeClient.getMinutes(), timeClient.getSeconds(), timeClient.getHours() < 12 ? "AM" : "PM");
    
    // Device details
    deviceDetails details;
    details.prox = prox;
    details.ambientLight = ambientLight;
    details.whiteLight = whiteLight;
    details.temp = temp.temperature;
    details.rHum = rHum.relative_humidity;
    details.accX = accX;
    details.accY = accY;
    details.accZ = accZ;
    gcfGetWithHeader(URL_GCF_UPLOAD, userId, epochTime, &details);
    startMillis = currentMillis;
    drawScreen1();
    }

    if(M5.BtnA.wasPressed()){
        display1 = true;
        display2 = false;
        drawScreen1();
    }else if(M5.BtnB.wasPressed()){
        display1 = false;
        display2 = true;
        drawScreen2();
    }// }else if(M5.BtnC.wasPressed()){
    //     display1 = false;
    //     display2 = false;
    //     drawScreen3();
    // }
    if(display2 == true){
       if(userId1.isPressed()){
            userId1b = !userId1b;
            if(userId1b && !userId2b && !userId3b){
                userId = "DanGrissom";
                M5.Lcd.setTextSize(2);
            M5.Lcd.fillRect(10, 20, 150, 40, BLACK);
            M5.Lcd.setTextColor(WHITE, BLACK);
            M5.Lcd.setCursor(25,35);
            M5.Lcd.println("DanGrissom");
            }else if(!userId1b){
                M5.Lcd.setTextSize(2);
            M5.Lcd.fillRect(10, 20, 150, 40, WHITE);
            M5.Lcd.setTextColor(BLACK, WHITE);
            M5.Lcd.setCursor(25,35);
            M5.Lcd.println("DanGrissom");
            }

       }
       if(userId2.isPressed()){
            userId2b = !userId2b;
            if(userId2b && !userId1b && !userId3b){
                userId = "JonDoe";
                 M5.Lcd.setTextSize(2);
            M5.Lcd.fillRect(165, 20, 100, 40, BLACK);
            M5.Lcd.setTextColor(WHITE, BLACK);
            M5.Lcd.setCursor(180,35);
            M5.Lcd.println("JonDoe");
            }else if(!userId2b){
                M5.Lcd.setTextSize(2);
            M5.Lcd.fillRect(165, 20, 100, 40, WHITE);
            M5.Lcd.setTextColor(BLACK, WHITE);
            M5.Lcd.setCursor(180,35);
            M5.Lcd.println("JonDoe");
            }
           
       }
       if(userId3.isPressed()){
            userId3b = !userId3b;
            if(userId3b && userId1b == false && userId2b == false){
                userId = "All";
                 M5.Lcd.setTextSize(2);
            M5.Lcd.fillRect(270, 20, 50, 40, BLACK);
            M5.Lcd.setTextColor(WHITE, BLACK);
            M5.Lcd.setCursor(275,35);
            M5.Lcd.println("All");
            }else if(userId3b == false){
                 M5.Lcd.setTextSize(2);
            M5.Lcd.fillRect(270, 20, 50, 40, WHITE);
            M5.Lcd.setTextColor(BLACK, WHITE);
            M5.Lcd.setCursor(275,35);
            M5.Lcd.println("All");
            }
           
       }
       if(time1.isPressed()){
            time1b = !time1b;
            if(time1b && time2b == false && time3b == false && time4b == false){
                timeString = "5000";
                           M5.Lcd.setTextSize(3);
    M5.Lcd.fillRect(40, 80, 45, 40, BLACK);
     M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(45,90);
    M5.Lcd.println("5s");
            }else if(time1b == false){
                           M5.Lcd.setTextSize(3);
    M5.Lcd.fillRect(40, 80, 45, 40, WHITE);
     M5.Lcd.setTextColor(BLACK, WHITE);
    M5.Lcd.setCursor(45,90);
    M5.Lcd.println("5s");
            }
 
       }
       if(time2.isPressed()){
        time2b = !time2b;
        if(time2b && !time1b && !time3b && !time4b){
            timeString = "10000";
            M5.Lcd.setTextSize(3);
         M5.Lcd.fillRect(90, 80, 60, 40, BLACK);
          M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(95,90);
    M5.Lcd.println("10s");
        }else if(!time2b){
            M5.Lcd.setTextSize(3);
         M5.Lcd.fillRect(90, 80, 60, 40, WHITE);
          M5.Lcd.setTextColor(BLACK, WHITE);
    M5.Lcd.setCursor(95,90);
    M5.Lcd.println("10s");
        }
        
       }
       if(time3.isPressed()){
        time3b = !time3b;
        if(time3b && !time1b && !time2b && !time4b){
            timeString = "30000";
 M5.Lcd.setTextSize(3);
        M5.Lcd.fillRect(155, 80, 60, 40, BLACK);
        M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(160,90);
    M5.Lcd.println("30s");
        }else if(!time3b){
              M5.Lcd.setTextSize(3);
        M5.Lcd.fillRect(155, 80, 60, 40, WHITE);
        M5.Lcd.setTextColor(BLACK, WHITE);
    M5.Lcd.setCursor(160,90);
    M5.Lcd.println("30s"); 
        }
      
       }
       if(time4.isPressed()){
        time4b = !time4b;
        if(time4b && !time1b && !time2b && !time3b){
            timeString = "60000";
 M5.Lcd.setTextSize(3);
        M5.Lcd.fillRect(220, 80, 60, 40, BLACK);
        M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(225,90);
    M5.Lcd.println("60s");
        }else if(!time4b){
 M5.Lcd.setTextSize(3);
        M5.Lcd.fillRect(220, 80, 60, 40, WHITE);
        M5.Lcd.setTextColor(BLACK, WHITE);
    M5.Lcd.setCursor(225,90);
    M5.Lcd.println("60s");
        }
       
       }
       if(data1.isPressed()){
        data1b = !data1b;
        if(data1b && !data2b && !data3b){
            data = "temp";
M5.Lcd.setTextSize(3);
        M5.Lcd.fillRect(40, 140, 80, 40, BLACK);
        M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(45,150);
    M5.Lcd.println("Temp");
        }else if(!data1b){
M5.Lcd.setTextSize(3);
        M5.Lcd.fillRect(40, 140, 80, 40, WHITE);
        M5.Lcd.setTextColor(BLACK, WHITE);
    M5.Lcd.setCursor(45,150);
    M5.Lcd.println("Temp");
        }
        
       }
       if(data2.isPressed()){
        data2b = !data2b;
        if(data2b && !data1b && !data3b){
            data = "rHum";
M5.Lcd.setTextSize(3);
        M5.Lcd.fillRect(125, 140, 80, 40, BLACK);
        M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(130,150);
    M5.Lcd.println("rHum");
        }else if(!data2b){
M5.Lcd.setTextSize(3);
        M5.Lcd.fillRect(125, 140, 80, 40, WHITE);
        M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setCursor(130,150);
    M5.Lcd.println("rHum");
        }
        
       }
       if(data3.isPressed()){
        data3b = !data3b;
        if(data3b && !data1b && !data2b){
            data = "al";
        M5.Lcd.setTextSize(3);
        M5.Lcd.fillRect(210, 140, 60, 40, BLACK);
        M5.Lcd.setTextColor(WHITE, BLACK);
        M5.Lcd.setCursor(225,150);
        M5.Lcd.println("al");
        }else if(!data3b){
        M5.Lcd.setTextSize(3);
        M5.Lcd.fillRect(210, 140, 60, 40, WHITE);
        M5.Lcd.setTextColor(BLACK, WHITE);
        M5.Lcd.setCursor(225,150);
        M5.Lcd.println("al");
        }
      
       }
       if(getAvg.isPressed()){
        M5.Lcd.setTextSize(3);
        M5.Lcd.fillRect(40, 190, 235, 40, RED);
        M5.Lcd.setTextColor(BLACK,RED);
        M5.Lcd.setCursor(50,200);
        M5.Lcd.println("Get Average");
        if((!userId1b && !userId2b && !userId3b) || (!time1b && !time2b && !time3b && !time4b) || (!data1b && !data2b && !data3b)){
             M5.Lcd.setTextSize(3);
        M5.Lcd.fillRect(40, 190, 235, 40, GREEN);
        M5.Lcd.setTextColor(BLACK,GREEN);
        M5.Lcd.setCursor(50,200);
        M5.Lcd.println("Get Average");
            
        }else {
           
        getWithParams();
        display2 = false;
        //drawScreen3();
        }
       }
    }
    delay(300);
}

void drawScreen1() {
    M5.Lcd.setCursor(0,0);
     M5.Lcd.fillScreen(TFT_WHITE);
    M5.Lcd.setTextColor(TFT_BLACK);
     M5.Lcd.setTextSize(2);
  

    M5.Lcd.printf("User ID: DanGrissom\n");

     sensors_event_t rHum, temp;
    sht4.getEvent(&rHum, &temp);
    M5.Lcd.printf("\tTemperature: %.2fF\n", convertCintoF(temp.temperature));
    M5.Lcd.printf("\tHumidity: %.2f %%rH\n", rHum.relative_humidity);

     uint16_t prox = vcnl4040.getProximity();
    uint16_t ambientLight = vcnl4040.getLux();
    uint16_t whiteLight = vcnl4040.getWhiteLight();
    M5.Lcd.printf("\tProximity: %d\n", prox);
    M5.Lcd.printf("\tAmbient light: %d\n", ambientLight);
    M5.Lcd.printf("\tRaw white light: %d\n", whiteLight);

    float accX;
    float accY;
    float accZ;
    M5.IMU.getAccelData(&accX, &accY, &accZ);
    accX *= 9.8;
    accY *= 9.8;
    accZ *= 9.8;
    M5.Lcd.printf("\tAccel X=%.2fm/s^2\n", accX);        
    M5.Lcd.printf("\tAccel Y=%.2fm/s^2\n", accY);
    M5.Lcd.printf("\tAccel Z=%.2fm/s^2\n", accZ);

     timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();
    unsigned long long epochMillis = ((unsigned long long)epochTime)*1000;
    struct tm *ptm = gmtime ((time_t *)&epochTime);
    M5.Lcd.printf("\nCurrent Time:\n\tEpoch (ms): %llu", epochMillis);
    M5.Lcd.printf("\n\tFormatted: %d/%d/%d ", ptm->tm_mon+1, ptm->tm_mday, ptm->tm_year+1900);
    M5.Lcd.printf("%02d:%02d:%02d%s\n\n", timeClient.getHours() % 12, timeClient.getMinutes(), timeClient.getSeconds(), timeClient.getHours() < 12 ? "AM" : "PM");

}


void drawScreen2() {
     userId1b = false;
 userId2b = false;
 userId3b = false;
 time1b = false;
 time2b = false;
 time3b = false;
 time4b = false;
 data1b = false;
 data2b = false;
 data3b = false;
 getAvgb = false;
    M5.Lcd.fillScreen(TFT_LIGHTGREY);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(BLACK, WHITE);
    M5.Lcd.fillRect(10, 20, 150, 40, WHITE);
    M5.Lcd.setCursor(25,35);
    M5.Lcd.println("DanGrissom");
    M5.Lcd.fillRect(165, 20, 100, 40, WHITE);
    M5.Lcd.setCursor(180,35);
    M5.Lcd.println("JonDoe");
    M5.Lcd.fillRect(270, 20, 50, 40, WHITE);
    M5.Lcd.setCursor(275,35);
    M5.Lcd.println("All");
    M5.Lcd.setTextSize(3);
    M5.Lcd.fillRect(40, 80, 45, 40, WHITE);
    M5.Lcd.setCursor(45,90);
    M5.Lcd.println("5s");
    M5.Lcd.fillRect(90, 80, 60, 40, WHITE);
    M5.Lcd.setCursor(95,90);
    M5.Lcd.println("10s");
    M5.Lcd.fillRect(155, 80, 60, 40, WHITE);
    M5.Lcd.setCursor(160,90);
    M5.Lcd.println("30s");
    M5.Lcd.fillRect(220, 80, 60, 40, WHITE);
    M5.Lcd.setCursor(225,90);
    M5.Lcd.println("60s");
    M5.Lcd.fillRect(40, 140, 80, 40, WHITE);
    M5.Lcd.setCursor(45,150);
    M5.Lcd.println("Temp");
    M5.Lcd.fillRect(125, 140, 80, 40, WHITE);
    M5.Lcd.setCursor(130,150);
    M5.Lcd.println("rHum");
    M5.Lcd.fillRect(210, 140, 60, 40, WHITE);
    M5.Lcd.setCursor(225,150);
    M5.Lcd.println("al");
    M5.Lcd.fillRect(40, 190, 235, 40, GREEN);
    M5.Lcd.setTextColor(BLACK,GREEN);
    M5.Lcd.setCursor(50,200);
    M5.Lcd.println("Get Average");
}

void drawScreen3() {
    M5.Lcd.fillScreen(TFT_GREEN);
}

int getWithParams(){
    HTTPClient http;

      String serverPath = getURL + "?userId=" + userId + "&timeDuration=" + timeString + "&dataType=" + data;
      
      // Your Domain name with URL path or IP address with path
      http.begin(serverPath.c_str());
      
    // Post the headers (NO FILE)
    int httpResCode = http.GET();

    // Print the response code and message
    //Serial.printf("HTTP%scode: %d\n%s\n\n", httpResCode > 0 ? " " : " error ", httpResCode, http.getString().c_str());

    // Retrieves JSON as an object
    String response = http.getString();
    Serial.printf("HTTP%scode: %d\n%s\n\n", httpResCode > 0 ? " " : " error ", httpResCode, response);
    const size_t jsonCapacity = 768 + 250;
    DynamicJsonDocument objResponse(jsonCapacity);
    DeserializationError error = deserializeJson(objResponse, response);
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
    }

    String dataType = objResponse["dataType"];
    int dataPoints = objResponse["dataPoints"];
    double averageVal = objResponse["average"];
    double avgRate = objResponse["rate"];
    String avgStart = objResponse["range"]["start"];
    String avgEnd = objResponse["range"]["end"];
    // Free resources and return response code
    http.end();
    M5.Lcd.fillScreen(TFT_GREEN);
    M5.Lcd.setTextColor(BLACK, GREEN);
    M5.Lcd.setCursor(5,5);
    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("\tData Type: %s\n", dataType);
    //M5.Lcd.println(dataType);
    M5.Lcd.printf("\t# of Data Points: %d\n", dataPoints);
    //M5.Lcd.println(dataPoints);
    M5.Lcd.printf("\tAverage Value: %.2f\n", averageVal);
    //M5.Lcd.println(averageVal);
    M5.Lcd.printf("\tAverage Rate: %.2f\n", avgRate);
    //M5.Lcd.println(avgRate);
    M5.Lcd.printf("\tStart time:\n");
    M5.Lcd.println(avgStart);
    M5.Lcd.printf("\tEnd time:\n");
    M5.Lcd.println(avgEnd);
    return httpResCode; 
}

////////////////////////////////////////////////////////////////////
// This method takes in a user ID, time and structure describing
// device details and makes a GET request with the data. 
////////////////////////////////////////////////////////////////////
bool gcfGetWithHeader(String serverUrl, String userId, time_t time, deviceDetails *details) {
    // Allocate arrays for headers
	const int numHeaders = 1;
    String headerKeys [numHeaders] = {"M5-Details"};
    String headerVals [numHeaders];

    // Add formatted JSON string to header
    headerVals[0] = generateM5DetailsHeader(userId, time, details);
    
    // Attempt to post the file
    Serial.println("Attempting post data.");
    int resCode = httpGetWithHeaders(serverUrl, headerKeys, headerVals, numHeaders);
    
    // Return true if received 200 (OK) response
    return (resCode == 200);
}

////////////////////////////////////////////////////////////////////
// TODO 4: Implement function
// Generates the JSON header with all the sensor details and user
// data and serializes to a String.
////////////////////////////////////////////////////////////////////
String generateM5DetailsHeader(String userId, time_t time, deviceDetails *details) {
    // Allocate M5-Details Header JSON object
    StaticJsonDocument<650> objHeaderM5Details; //DynamicJsonDocument  objHeaderGD(600);
    
    // Add VCNL details
    JsonObject objVcnlDetails = objHeaderM5Details.createNestedObject("vcnlDetails");
    objVcnlDetails["prox"] = details->prox;
    objVcnlDetails["al"] = details->ambientLight;
    objVcnlDetails["rwl"] = details->whiteLight;

    // Add SHT details
    JsonObject objShtDetails = objHeaderM5Details.createNestedObject("shtDetails");
    objShtDetails["temp"] = details->temp;
    objShtDetails["rHum"] = details->rHum;

    // Add M5 Sensor details
    JsonObject objM5Details = objHeaderM5Details.createNestedObject("m5Details");
    objM5Details["ax"] = details->accX;
    objM5Details["ay"] = details->accY;
    objM5Details["az"] = details->accZ;

    // Add Other details
    JsonObject objOtherDetails = objHeaderM5Details.createNestedObject("otherDetails");
    objOtherDetails["timeCaptured"] = time;
    objOtherDetails["userId"] = userId;

    // Convert JSON object to a String which can be sent in the header
    size_t jsonSize = measureJson(objHeaderM5Details) + 1;
    char cHeaderM5Details [jsonSize];
    serializeJson(objHeaderM5Details, cHeaderM5Details, jsonSize);
    String strHeaderM5Details = cHeaderM5Details;
    //Serial.println(strHeaderM5Details.c_str()); // Debug print

    // Return the header as a String
    return strHeaderM5Details;
}

////////////////////////////////////////////////////////////////////
// This method takes in a serverURL and array of headers and makes
// a GET request with the headers attached and then returns the response.
////////////////////////////////////////////////////////////////////
int httpGetWithHeaders(String serverURL, String *headerKeys, String *headerVals, int numHeaders) {
    // Make GET request to serverURL
    HTTPClient http;
    http.begin(serverURL.c_str());
    
	////////////////////////////////////////////////////////////////////
	// TODO 5: Add all the headers supplied via parameter
	////////////////////////////////////////////////////////////////////
    for (int i = 0; i < numHeaders; i++)
        http.addHeader(headerKeys[i].c_str(), headerVals[i].c_str());
    
    // Post the headers (NO FILE)
    int httpResCode = http.GET();

    // Print the response code and message
    Serial.printf("HTTP%scode: %d\n%s\n\n", httpResCode > 0 ? " " : " error ", httpResCode, http.getString().c_str());

    // Free resources and return response code
    http.end();
    return httpResCode;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// TODO 6: Implement method
// Given an array of bytes, writes them out to the SD file system.
////////////////////////////////////////////////////////////////////
String writeDataToFile(byte * fileData, size_t fileSizeInBytes) {
    // Print status
    Serial.println("Attempting to write file to SD Card...");

    // Obtain file system from SD card
    fs::FS &sdFileSys = SD;

    // Generate path where new picture will be saved on SD card and open file
    int fileNumber = getNextFileNumFromEEPROM();
    String path = "/file_" + String(fileNumber) + ".txt";
    File file = sdFileSys.open(path.c_str(), FILE_WRITE);

    // If file was opened successfully
    if (file)
    {
        // Write image bytes to the file
        Serial.printf("\tSTATUS: %s FILE successfully OPENED\n", path.c_str());
        file.write(fileData, fileSizeInBytes); // payload (file), payload length
        Serial.printf("\tSTATUS: %s File successfully WRITTEN (%d bytes)\n\n", path.c_str(), fileSizeInBytes);

        // Update picture number
        EEPROM.write(0, fileNumber);
        EEPROM.commit();
    }
    else {
        Serial.printf("\t***ERROR: %s file FAILED OPEN in writing mode\n***", path.c_str());
        return "";
    }

    // Close file
    file.close();

    // Return file name
    return path;
}

////////////////////////////////////////////////////////////////////
// TODO 7: Implement Method
// Get file number by reading last file number in EEPROM (non-volatile
// memory area).
////////////////////////////////////////////////////////////////////
int getNextFileNumFromEEPROM() {
    #define EEPROM_SIZE 1             // Number of bytes you want to access
    EEPROM.begin(EEPROM_SIZE);
    int fileNumber = 0;               // Init to 0 in case read fails
    fileNumber = EEPROM.read(0) + 1;  // Variable to represent file number
    return fileNumber;
}

////////////////////////////////////////////////////////////////////
// TODO 8: Implement Method
// This method takes in an SD file path, user ID, time and structure
// describing device details and POSTs it. 
////////////////////////////////////////////////////////////////////
bool gcfPostFile(String serverUrl, String filePathOnSD, String userId, time_t time, deviceDetails *details) {
    // Allocate arrays for headers
    const int numHeaders = 3;
    String headerKeys [numHeaders] = { "Content-Type", "Content-Disposition", "M5-Details"};
    String headerVals [numHeaders];

    // Content-Type Header
    headerVals[0] = "text/plain";
    
    // Content-Disposition Header
    String filename = filePathOnSD.substring(filePathOnSD.lastIndexOf("/") + 1);
    String headerCD = "attachment; filename=" + filename;
    headerVals[1] = headerCD;

    // Add formatted JSON string to header
    headerVals[2] = generateM5DetailsHeader(userId, time, details);
    
    // Attempt to post the file
    int numAttempts = 1;
    Serial.printf("Attempting upload of %s...\n", filename.c_str());
    int resCode = httpPostFile(serverUrl, headerKeys, headerVals, numHeaders, filePathOnSD);
    
    // If first attempt failed, retry...
    while (resCode != 200) {
        // ...up to 9 more times (10 tries in total)
        if (++numAttempts >= 10)
            break;

        // Re-attempt
        Serial.printf("*Re-attempting upload (try #%d of 10 max tries) of %s...\n", numAttempts, filename.c_str());
        resCode = httpPostFile(serverUrl, headerKeys, headerVals, numHeaders, filePathOnSD);
    }

    // Return true if received 200 (OK) response
    return (resCode == 200);
}

////////////////////////////////////////////////////////////////////
// TODO 9: Implement Method
// This method takes in a serverURL and file path and makes a 
// POST request with the file (to upload) and then returns the response.
////////////////////////////////////////////////////////////////////
int httpPostFile(String serverURL, String *headerKeys, String *headerVals, int numHeaders, String filePath) {
    // Make GET request to serverURL
    HTTPClient http;
    http.begin(serverURL.c_str());
    
    // Add all the headers supplied via parameter
    for (int i = 0; i < numHeaders; i++)
        http.addHeader(headerKeys[i].c_str(), headerVals[i].c_str());
    
    // Open the file, upload and then close
    fs::FS &sdFileSys = SD;
    File file = sdFileSys.open(filePath.c_str(), FILE_READ);
    int httpResCode = http.sendRequest("POST", &file, file.size());
    file.close();

    // Print the response code and message
    Serial.printf("\tHTTP%scode: %d\n\t%s\n\n", httpResCode > 0 ? " " : " error ", httpResCode, http.getString().c_str());

    // Free resources and return response code
    http.end();
    return httpResCode;
}

/////////////////////////////////////////////////////////////////
// Convert between F and C temperatures
/////////////////////////////////////////////////////////////////
double convertFintoC(double f) { return (f - 32) * 5.0 / 9.0; }
double convertCintoF(double c) { return (c * 9.0 / 5.0) + 32; }
