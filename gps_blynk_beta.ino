//gps tracking without sim...find and scan free wifi, store in the SPIFF and send to server (actually blynk app with map widget) 
//continuosly scan until connect with free wifi
//remember to flash nodemcu FW to ESP12 (nodemcuflasher press and hold flash and reset button, release reset and start...idem with arduino ide if not use automatic programmer with CH340 driver


#include <ESP8266WiFi.h>
#define BLYNK_PRINT Serial
#include <SPI.h>
#include <BlynkSimpleEsp8266.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>



void(* Riavvia)(void) = 0;  //Dichiarazione di funzione che punta all'indirizzo zero utilizzata per essere chiamata dalla void riavvia
const int EN_GPS = 12;  //GPIO12 enable gps   0 disable 1 enable
const int PPS = 14;     // GPIO14 INPUT  

 

  static const int RXPin = 13, TXPin = 15;   // GPIO 13=D7(conneect Tx of GPS) and GPIO 15=D8(Connect Rx of GPS)  softwareserial
  static const uint32_t GPSBaud = 9600; //if Baud rate 9600 didn't work in your case then use 4800   sofwareserial 


  float lat = 51.5074;   //declaration float variable to store latitude
  float lon = 0.1278;    //declaration float variable to store longitude





  
  char auth[] ="xxxxxxxxxxxxxxxxxxxxx";   // put here your authorization code received by email from blynk
// Your WiFi credentials.
// Set password to "" for open networks.
//char ssid[] = "ssid type here";          //standard declaration of blynk should be different because can't start if is not declare first ...in this way first we attempt SSID and then put in the char so blynk can start
  char pass[] = "";
  WidgetMap myMap(V0);      //declare widget map to add in the app of blynk previous ocnfiguration device ESP8266  V0 INPUT
  TinyGPSPlus gps;          //gps library usually for standard NMEA UART232  good also UBLOX 6M and similar (I use VK2828U7G5LF)
  SoftwareSerial ss(RXPin, TXPin);  // The serial connection to the GPS device different from serial debug or program
  BlynkTimer timer;

/* Serial Baud Rate */
#define SERIAL_BAUD       9600           //serial debug and flash standard 
/* Delay paramter for connection. */
#define WIFI_DELAY        500
/* Max SSID octets. */
#define MAX_SSID_LEN      32
/* Wait this much until device gets IP. */
#define MAX_CONNECT_TIME  30000            //sometimes find open but can't connect so in this way we have a timeout to proceede



/* SSID that to be stored to connect. */
char ssid[MAX_SSID_LEN] = "";
unsigned int move_index = 1;       // fixed location for now, we can use this function to store point on the map but we would schedule data ora and position




/* Scan available networks and sort them in order to their signal strength. */
void scanAndSort() {
  memset(ssid, 0, MAX_SSID_LEN);
  int n = WiFi.scanNetworks();
  Serial.println("Scan complete!");
  if (n == 0) {
    Serial.println("No networks available.");
  } else {
    Serial.print(n);
    Serial.println(" networks discovered.");
    int indices[n];
    for (int i = 0; i < n; i++) {
      indices[i] = i;
    }
    for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
          std::swap(indices[i], indices[j]);
        }
      }
    }
    for (int i = 0; i < n; ++i) {
      Serial.println("The strongest open network is:");
      Serial.print(WiFi.SSID(indices[i]));
      Serial.print(" ");
      Serial.print(WiFi.RSSI(indices[i]));
      Serial.print(" ");
      Serial.print(WiFi.encryptionType(indices[i]));
      Serial.println();
      if(WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE) {
        memset(ssid, 0, MAX_SSID_LEN);
        strncpy(ssid, WiFi.SSID(indices[i]).c_str(), MAX_SSID_LEN);
        break;
      }
    }
  }
}



void setup() {


   Serial.begin(SERIAL_BAUD);
   Serial.println();    //svuota serial monitor
   ss.begin(GPSBaud);   //inizialize
   pinMode(EN_GPS, OUTPUT);   //
   pinMode(PPS, INPUT);   //   actually not used but read state of gps 0 not fix   blynk voltage fix 
   digitalRead(EN_GPS) == HIGH;  //start enable GPS
   

   timer.setInterval(120000L, checkGPS); // every 120 s check if GPS is connected, only really needs to be done once but in the same routine alternate on and off GPS for power safe mode


 
// Scan, Sort, and Connect to WiFi   
//  Serial.begin(SERIAL_BAUD);
  Serial.println("Scanning for open networksss...");
   if(WiFi.status() != WL_CONNECTED) {
    /* Clear previous modes. */
    WiFi.softAPdisconnect();
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    delay(WIFI_DELAY);
    /* Scan for networks to find open guy. */
    scanAndSort();
    delay(WIFI_DELAY);
    /* Global ssid param need to be filled to connect. */
    if(strlen(ssid) > 0) {
      Serial.print("Connecting to ");
      Serial.println(ssid);
      /* No pass for WiFi. We are looking for non-encrypteds. */
      WiFi.begin(ssid);
      unsigned short try_cnt = 0;
      /* Wait until WiFi connection but do not exceed MAX_CONNECT_TIME */
      while (WiFi.status() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / WIFI_DELAY) {
        delay(WIFI_DELAY);
        Serial.print(".");
        try_cnt++;
      }
      if(WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("Connection Successful!");
        Serial.println("Your device IP address is ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.println("Connection FAILED");  
         Riavvia();   //necessario perchè se non trova le reti deve poter ripetere finche non trova 
      }
    } else {
      Serial.println("No open networks available. :-(");
       Riavvia();   //necessario perchè se non trova le reti deve poter ripetere finche non trova   
    }

      
  }
   Blynk.begin(auth, ssid, pass);      //declare blynk here after acquire all because if not blynk can't start

  // If you want to remove all points:
  //myMap.clear();

//  int index = 1;

  
}



void checkGPS(){                  //diagnosis in case of failure connection or power supply to GPS module
  if (gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
     //Riavvia();   //necessario perchè se non trova le reti deve poter ripetere finche non trova  (verificare xche non dovrebbe servire qui il riavvio)
  }

  // float to x decimal places  



if(digitalRead(EN_GPS) == HIGH){    //power saving mode by timer declaration switch on and off gps module (improve by in PPS INPUT GPIO14)
    Serial.println("disable gps");
    digitalWrite(EN_GPS, LOW);
    
  }  else {
   Serial.println("enable gps");
    digitalWrite(EN_GPS, HIGH);
     
   
  } 

}

void loop ()  {

 while (ss.available() > 0) 
    {
     
      if (gps.encode(ss.read()))
        displayInfo();   // displays information every time a new sentence is correctly encoded.
  }
  Blynk.run();   //start routine of blynk to manage and send data to your map widget (see blynk website how to do)
  timer.run();   //routine of timer that call checkgps by upper declaration
  

    
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("triying to reconnect...");
    /* Clear previous modes. */
    WiFi.softAPdisconnect();
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    delay(WIFI_DELAY);
    Riavvia();  //if lost connection need to restart
  
   }
   
  
 
}



void displayInfo()
{ 
   Serial.println("aspetto dati gps");  // float to x decimal places
  if (gps.location.isValid()) 
  {
      
    float latitude = (gps.location.lat());     //Storing the Lat. and Lon. 
    float longitude = (gps.location.lng()); 
    
    Serial.print("LAT:  ");
    Serial.println(latitude, 6);  // float to x decimal places after comma


    Serial.print("LONG: ");
    Serial.println(longitude, 6);

    myMap.location(move_index, latitude, longitude, "JONNY");   //put name of point that you want in the map...you will see your dot green position and black of device that you are tracking
   
  }
  
  

  Serial.println();   //empty serial monitor
}









  
