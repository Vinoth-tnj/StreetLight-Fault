//blynk declaration
#define BLYNK_TEMPLATE_ID "TMPL3Njy47hlF"
#define BLYNK_TEMPLATE_NAME "LED Blink"
#define BLYNK_AUTH_TOKEN "0_8n48z1qe7k4hGf5sDKb0en8I9SkmX6"
#define BLYNK_PRINT Serial  /*  include Blynk Serial */

#include <WiFi.h>
#include "ThingSpeak.h"
#include <BlynkSimpleEsp32.h>

//things channel declaration
const int mychannelnumber = 2427565;
const char* myApikey = "1HGZ82IGRAK1P4RA";
const char* server = "api.thingspeak.com";

//blynk
// Enter device Authentication Token
char auth[] = BLYNK_AUTH_TOKEN;
BlynkTimer timer;

//wifi declaration
#define WIFI_SSID "realme 3 Pro"
#define WIFI_PASSWORD "123456890"
WiFiClient  client;

//pin declaration
const int light1 = 14;
const int ldr_common = 33;
const int ldr_light1 = 27;
const int vibrator_light1 = 25;
const int current_light1 = 34;
const int ir_light1 = 4;

//for currentsensor calculation
const float sensitivity = 66; // Sensitivity of ACS712 30A sensor (66 mV/A)
int watt =0;
double voltage=0;
double vrms=0;
double amprms=0;

//flag declaration for email sending
int notification_light_flag = 0;
int notification_sent_flag_light1_wire = 0;
int notification_sent_flag_light1_blub = 0;
int notification_sent_flag_light1_vibrator = 0;
int notification_sent_flag_light1_ir = 0;

//fault status
int fault_status=0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); // Initialize serial communication
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: "+ String(WiFi.localIP()));
  WiFi.mode(WIFI_STA);  
  ThingSpeak.begin(client);  // Initialize ThingSpeak
  Serial.println();
  pinMode(light1, OUTPUT);
  pinMode(ldr_common,INPUT);
  pinMode(ldr_light1,INPUT);
  pinMode(vibrator_light1,INPUT);
  pinMode(current_light1,INPUT);
  pinMode(ir_light1,INPUT);
  Blynk.begin(auth, WIFI_SSID, WIFI_PASSWORD, "blynk.cloud", 80);

}

void loop() {
  //blynk 
  Blynk.run();

  //current status check
  voltage = getVPP();
  vrms = (voltage/2.0)*0.707;
  amprms = ((vrms*1000)/sensitivity)-0.3;
  if(amprms < 0.0){
    amprms = 0.0;
  }
  Serial.print("A = ");
  Serial.println(amprms);
  watt = (amprms*240)/1.2;
  if(watt < 0.0){
    watt = 0;
  }
  Serial.print("Watt = ");
  Serial.println(watt);

  //conversion for data pushing
  amprms=amprms*100;
  int amp = (int)amprms;
  watt=watt*100;
  int watts= (int)watt;

  ThingSpeak.setField(1,amp); //thingspeak field1
  ThingSpeak.setField(2,watts);   //thingspeak field2

  //ldr status check;
  int ldr_common_status = analogRead(ldr_common);
  int ldr_light1_status = digitalRead(ldr_light1);
  int common_light_status;
  int light1_status;
  if(ldr_common_status > 3500){
    digitalWrite(light1,HIGH);
    common_light_status = 1;
    if(notification_light_flag == 0){
      Blynk.logEvent("light_on_notification", String("All the Lights are turned On."));
      notification_light_flag == 1;
    }
    if(ldr_light1_status == 1){
      light1_status = 0;
      Serial.println("Light 1 didn't Glow");
      if(watt == 0){
        Serial.println("Wire Fault");
        if (notification_sent_flag_light1_wire ==0)
        {    
          Blynk.logEvent("light_fault_notification", String("Fault: Wire Fault"));
          fault_status=1;
          notification_sent_flag_light1_wire ==1;
        }
      }
      else{
        Serial.println("Bulb Fault");
        if (notification_sent_flag_light1_blub ==0)
        {    
          Blynk.logEvent("light_fault_notification", String("Fault: Bulb Fault"));
          fault_status=2;
          notification_sent_flag_light1_blub ==1;
        }
      }
    }
    else{
      light1_status = 1;
    }
    //vibration status check
    int vibrator_status = digitalRead(vibrator_light1);
    if(vibrator_status){
      Serial.println("vibration detected");
      if (notification_sent_flag_light1_vibrator ==0)
      {    
        Blynk.logEvent("light_fault_notification", String("Fault: vibration detected"));
        fault_status=3;
        notification_sent_flag_light1_vibrator ==1;
      }
    }else{
      Serial.println("No vibartion");
    }
    //thingspeak field5
    ThingSpeak.setField(5,vibrator_status);

    //ir status check
    int ir_status = digitalRead(ir_light1);
    if(digitalRead(ir_light1) == 1){
      Serial.println("Fixture Position change");
      if (notification_sent_flag_light1_ir ==0)
      {    
        Blynk.logEvent("light_fault_notification", String("Fault: Fixture Position change"));
        fault_status=4;
        notification_sent_flag_light1_ir ==1;
      }
    }
    //thingspeak field6
    ThingSpeak.setField(6,ir_status);
    ThingSpeak.setField(7,fault_status);
    
    //writing data in thingspeak
    int x = ThingSpeak.writeFields(mychannelnumber,myApikey);
    //check if writing occurs
    if(x == 200){
      Serial.println("Data pushed Successfully");
    }else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
  }
  else{
    digitalWrite(light1,LOW);
    common_light_status = 0;
    notification_light_flag = 0;
    notification_sent_flag_light1_wire = 0;
    notification_sent_flag_light1_blub = 0;  
    notification_sent_flag_light1_vibrator = 0;
    notification_sent_flag_light1_ir = 0;
    //thingspeak field3
    ThingSpeak.setField(3,common_light_status);
    //thingspeak field4
    ThingSpeak.setField(4,light1_status);
  }
  
  delay(10000); // Delay for stability

}

float getVPP(){
  float result;
  int read_value;
  int max_value=0;
  int min_value=4096;
  uint32_t start_time = millis();
  while((millis()-start_time) <1000){
    read_value = analogRead(current_light1);
    if(read_value > max_value){
      max_value = read_value;
    }
    if(read_value > 0){
      if(read_value < min_value){
        min_value = read_value;
      }
    }else{
      min_value = 0;
    }
  }

  result = ((max_value - min_value)*3.3)/4096.0;
  return result;
}

