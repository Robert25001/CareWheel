#include <WiFi.h>
const char* ssid = "MIT-Sous-Sol";
const char* password = "!321poiuytreza";
WiFiServer server(5000);
const int IN1 = 18;
const int IN2 = 19;
const int IN3 = 21;
const int IN4 = 22;
const int ENA = 23;
const int ENB = 5;
const int TRIG = 2;
const int ECHO = 4;
void setup() {
  // put your setup code here, to run once:
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
  // put your main code here, to run repeatedly:  digitalWrite(led, LOW);
  connexion();
  /*avancer();
  delay(1000);
  arreter();
  delay(3000);
  gauche();
  delay(1000);
  droite();
  delay(1000);*/
}

void avancer() {

  digitalWrite(IN1,HIGH);
  digitalWrite(IN2,LOW);
  digitalWrite(IN3,HIGH);
  digitalWrite(IN4,LOW);
  vitesse(10);

}

void reculer() {

  digitalWrite(IN1,LOW);
  digitalWrite(IN2,HIGH);
  digitalWrite(IN3,LOW);
  digitalWrite(IN4,HIGH);
  vitesse(10);

}

void gauche() {

  digitalWrite(IN1,LOW);
  digitalWrite(IN2,LOW);
  digitalWrite(IN3,HIGH);
  digitalWrite(IN4,LOW);
  vitesse(10);

}

void droite() {

  digitalWrite(IN1,HIGH);
  digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW);
  digitalWrite(IN4,LOW);
  vitesse(10);

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

  Serial.print("Distance:");
  Serial.print(distance);
  Serial.println("cm");

  return(distance);
}

void evite_obstacle() {
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
    avancer();
  }
}

void connexion() {
  WiFiClient client = server.available();

  if(client) {
    Serial.println("PC connecter");
    while(client.connected())
    {
      if(client.available())
      {
        String commande = client.readStringUntil('\n');
        commande.trim();
        Serial.println(commande);
        client.println("Bonjour depuis ESP32");
        if(commande == "avancer") {
           evite_obstacle();
           avancer();
        }
        else if(commande == "reculer") {
           evite_obstacle();
           reculer();
        }
        else if(commande == "droite") {
           evite_obstacle();
           droite();
        }
        else {
           evite_obstacle();
           gauche();
        }
      }
    }
    client.stop();
    Serial.println("PC deconnecter");
  }
}

