#include <WiFi.h>
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

//wifi 
const char* ssid = "MIT-Grande-Salle";
const char* password = "!321poiuytreza";
WiFiServer server(5000);
WiFiClient client;

MAX30105 particleSensor;
uint32_t irBuffer[100];
uint32_t redBuffer[100];
uint32_t ircourant;
int32_t bufferLength = 100;
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

const int IN1 = 18;
const int IN2 = 19;
const int IN3 = 21;
const int IN4 = 22; 
const int ENA = 23;
const int ENB = 5;
const int TRIG = 12;
const int ECHO = 13;
unsigned long dernierEnvoi = 0;
int compter = 0;
int Vitesse = 80;

void setup() {
 Serial.begin(115200);

  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connecter !");
  Serial.println(WiFi.localIP()); 
  server.begin();
  Serial.println("Server TCP demarre sur le port:");
  Serial.println(5000);

  Wire.begin(2,4);
  Serial.println("Recherche des périphériques I2C...");
   if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
   {
    Serial.println("MAX30102 non detecter");
    while(1) 
    {
      delay(1000);
    }
   }
  Serial.println("MAX30102 detecter");

  particleSensor.setup(
    90,
    4,
    2,
    100,
    411,
    4096
  );

  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeIR(0x0A);

  while (particleSensor.available() == false)
    {
      particleSensor.check();
    } 
  for (int i = 0; i < bufferLength; i++)
    {
      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();

      particleSensor.nextSample();
    }

  maxim_heart_rate_and_oxygen_saturation(
      irBuffer,
      bufferLength,
      redBuffer,
      &spo2,
      &validSPO2,
      &heartRate,
      &validHeartRate
  );

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  ledcAttach(ENA,5000,8);
  ledcAttach(ENB,5000,8);
}

void loop() {
  // put your main code here, to run repeatedly:
  connexion();
  lireCapteur();
  if (millis() - dernierEnvoi >= 500)
  {
    dernierEnvoi = millis();

    envoyerDonnees();
  }
}

void avancer() {

  evite_obstacle(avancer);
  digitalWrite(IN1,HIGH);
  digitalWrite(IN2,LOW);
  digitalWrite(IN3,HIGH);
  digitalWrite(IN4,LOW);
  vitesse(Vitesse);

}

void reculer() {

  digitalWrite(IN1,LOW);
  digitalWrite(IN2,HIGH);
  digitalWrite(IN3,LOW);
  digitalWrite(IN4,HIGH);
  vitesse(Vitesse);

}

void gauche() {

  evite_obstacle(gauche);
  digitalWrite(IN1,LOW);
  digitalWrite(IN2,LOW);
  digitalWrite(IN3,HIGH);
  digitalWrite(IN4,LOW);
  vitesse(Vitesse);

}

void droite() {
  
  evite_obstacle(droite);
  digitalWrite(IN1,HIGH);
  digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW);
  digitalWrite(IN4,LOW);
  vitesse(Vitesse);

}

void arreter() {

  digitalWrite(IN1,LOW);
  digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW);
  digitalWrite(IN4,LOW);
  vitesse(0);

}

void vitesse(int v) {
  ledcWrite(ENA,v);
  ledcWrite(ENB,v);
}

float mesure_distance() {
  digitalWrite(TRIG,LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG,HIGH);
  delayMicroseconds(10); 
  digitalWrite(TRIG,LOW);

  long temps = pulseIn(ECHO,HIGH);
  float distance = temps/58.0;

  /*Serial.print("Distance:");
  Serial.print(distance);
  Serial.println("cm");*/

  return(distance);
}

void evite_obstacle(void (*direction)(void)) {
  float distance = mesure_distance();

  if(distance < 10) {
    
    reculer();
    delay(1000);
    arreter();
    delay(1000);
  }
  else if(distance < 30) {
    arreter();
  }
  else {
    direction();
  }
}

void connexion()
{
  // Chercher un nouveau client seulement
  // si aucun client n'est actuellement connecté

  if (!client || !client.connected())
  {
    client = server.available();

    if (client)
    {
      Serial.println("PC connecte !");
    }
  }

  // Si Qt a envoyé une commande
  if (client && client.connected() && client.available())
  {
    String commande = client.readStringUntil('\n');

    commande.trim();

    Serial.print("Commande reçue : ");
    Serial.println(commande);

    execute_commande(commande);
  }
}

void execute_commande(String commande)
{
  if (commande == "avancer")
  {
    avancer();
  }

  else if (commande == "reculer")
  {
    reculer();
  }

  else if (commande == "droite")
  {
    droite();
  }

  else if (commande == "gauche")
  {
    gauche();
  }

  else if (commande == "arreter")
  {
    arreter();
  }

  else
  {
    Serial.println("Commande inconnue !");
  }
}

void lireCapteur()
{
  particleSensor.check();
  if (particleSensor.available() == true)
  {
    ircourant = particleSensor.getIR(); 
    if(ircourant < 20000){
      validHeartRate = 0;
      validSPO2 = 0;
      particleSensor.nextSample();
      compter = 0;
      return;
    }   
    for (int i = 1; i < bufferLength; i++)
    {
      redBuffer[i - 1] = redBuffer[i];
      irBuffer[i - 1] = irBuffer[i];
    }
  }
    redBuffer[bufferLength - 1] = particleSensor.getRed();
    irBuffer[bufferLength - 1] = ircourant;
    particleSensor.nextSample();
    compter++;
    
    if(compter >= 25)
    {
     compter = 0;
     maxim_heart_rate_and_oxygen_saturation(
       irBuffer,
       bufferLength,
       redBuffer,
       &spo2,
       &validSPO2,
       &heartRate,
       &validHeartRate
     );

     Serial.print("BPM:");

     if (validHeartRate)
       Serial.println(heartRate);
     else
       Serial.println("--");

     Serial.print("SpO2:");

     if (validSPO2)
       Serial.println(spo2);
     else
       Serial.println("--");

     Serial.println("----------------");
    }
}

void envoyerDonnees()
{
  if (!client || !client.connected())
    return;


  if (validHeartRate)
    client.print(heartRate);
  else
    client.print("--");

  client.print(";");

  if (validSPO2)
    client.print(spo2);
  else
    client.print("--");
  
  client.print(";");
  client.print(Vitesse);
  client.println();

}