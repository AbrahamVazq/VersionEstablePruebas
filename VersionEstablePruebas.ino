////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <SdFat.h>
#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <DS3231_Simple.h>
#include <SPI.h>
#include <Arduino.h>
////////////////////////////////////////////////////////////////////////////////////////////////////////
#define pinTemp 2          // Pin de entrada del sensor de temperatura D18B20
////////////////////////////////////////////////////////////////////////////////////////////////////////
/*Creamos objetos de la clase SdFat */
SdFat sd;                         // Creamos un objeto de la clase SdFat llamado sd
SdFile archivo;                   // Creamos un objeto de la clase SdFile llamado archivo
////////////////////////////////////////////////////////////////////////////////////////////////////////
DS3231_Simple Clock;              // Creamos un objeto de la calse DS3231
////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Variables
int pulsePin = A0;                // Pin donde va conectado el cable de señal del sensor de pulso (Cable purpura)
int tempPin = A2;                 // Definimos la entrada anlogica en pin A0 
int blinkPin = 5;                // Pin donde el LED sera el primero en iniciar el brillo
int fadePin = 6;                  // Pin que simulara el efecto del latido en cada brillo
int fadeRate = 0;                 // Variable usada para opacar el led en un pin con PWM
int ledMemoria = 8;               // Pin de salida del led
int ledBotonParo = 7;             // Pin de salida del led boton de paro
OneWire sensorTemp(pinTemp);      // 
DallasTemperature sensor(&sensorTemp);
int chipSelect = 10;         // Pin para el ChipSelect
int btnPin = 9;             // Pin donde esta conectado un boton de Pausa para separar datos
int btnEstado = 0;                // Este es el estado inicial del boton de pausa
String server = "31.170.164.164";
// Variables del Wi-Fi
/*
 * arduino Rx (pin 3) ---- ESP8266 Tx
 * arduino Tx (pin 4) ---- ESP8266 Rx
 */
SoftwareSerial esp8266(4,3); 
// Variables volatiles, usadas en las rutinas de interrupcion
volatile int BPM;                   // Variable entera que recibe el dato que detecta el Sensor de pulso cada 2mS***VALOR DEL PULSO***
volatile int Signal;                // Variable que mantiene el dato entrante 
volatile int IBI = 600;             // Variable del intervalo de tiempo entre cada beat, Must be seeded!
volatile boolean Pulse = false;     // Variable se vuelve 'true' cuando se ha detectado el latido del usuario "vivo",
                                    // falso cuando el latido no es de un cuerpo vivo*
volatile boolean QS = false;        // Variable se vuelve 'true' cuando el arduino encuentra un pulso 

// Consideraciones para la salida Seral --> ( Configurable a las necesidades )
static boolean serialVisual = false;   // Inicializado en Falso, configuralo en 'true' para ver en la consola de arduino el puslo en ASCII

void setup()
{
  pinMode(blinkPin,OUTPUT);         // Led en este pin Brillara en un latido
  pinMode(fadePin,OUTPUT);          // Led en este pin se opacara en un latido
  pinMode(ledMemoria,OUTPUT);       // Pin que nos dira si el lector esta en uso
  pinMode(ledBotonParo,OUTPUT);     // Recibe la señal de pausa desde el LED
  pinMode(btnPin,INPUT);            // Boton que envia la señal de pausa 
 
  Serial.begin(9600);               // Baudios del monitor serial.
  esp8266.begin(9600);              // Baudios del modulo  Wi-FiESP8255
  esp8266.setTimeout(2000);
  Clock.begin(); 
  sensor.begin();

  interruptSetup();                 // Inicia la lectura del Sensor de pulso cada 2mS
 
/* Inicializa el mudulo SD por medio de la libreria SdFat, imprimira el error en caso de encontrarlo
     usa la velocidad media como la libreria nativa
     Cambia a SPI_FULL_SPEED si es necesario para mejorar la funcion. */

  /* Inicializa SdFat */
  if (!sd.begin(chipSelect, SPI_HALF_SPEED))   // Si sd.begin no se peude inicializar lanza el error
  {
    sd.initErrorHalt();                        // Funcion que delata el error de detecion y lectura del modulo SD
  }
  /*Abre el archivo para escritura la sintaxis se parece mucho a la libreria Nativa SD.h */
  if (!archivo.open("Lecturas.bin", O_RDWR | O_CREAT | O_AT_END))     // Compara si el archivo se ha abierto si no, lanza el error
  {
    sd.errorHalt("Error! no se puede abrir el archivo intenta de nuevo."); // Imprime un error de incapacidad para abrir el archivo
  }
  /* Si el archivo se abrio correctamente entonces escribe en el */
  Serial.print("Ahora estamos escribiendo en el Archivo.");
  //archivo.println("-= Temperatura Corporal =-");            // Escribe al inicio del archivo la leyenda dentro
  //archivo.println("[");                                     // Escribe al inicio del archivo un corchete abierto
  archivo.close();                                          // Funcion que cierra el Archivo
  //Serial.println("Archivo Cerrado.");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// M A I N //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void loop()
{
    serialOutput() ;

    btnEstado = digitalRead(btnPin);
    
   if(btnEstado == HIGH)
    {
      interrumpe();
    }    
    else
    {
       if (QS == true)  // A Heartbeat Was Found
       {              
                         // BPM and IBI have been Determined
                         // Quantified Self "QS" true when arduino finds a heartbeat
          fadeRate = 255;         // Makes the LED Fade Effect Happen
                                  // Set 'fadeRate' Variable to 255 to fade LED with pulse
          serialOutputWhenBeatHappens();   // A Beat Happened, Output that to serial. 
          temperatura();    
          QS = false;                      // reset the Quantified Self flag for next time    
        }
           
        ledFadeToBeat();                      // Makes the LED Fade Effect Happen 
        delay(10);                        //  take a break
    }        
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
void ledFadeToBeat()
{
    fadeRate -= 15;                         //  set LED fade value
    fadeRate = constrain(fadeRate,0,255);   //  keep LED fade value from going into negative numbers!
    analogWrite(fadePin,fadeRate);          //  fade LED
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Funcion que esta a la espera de que se presione el Boton Pausa/Envio.
 */
void interrumpe()
{
    digitalWrite(ledBotonParo,HIGH);        // Enciende el LED en el pin ledBotonParo
    delay(50);                             // Duerme 500 mS
    digitalWrite(ledMemoria,LOW);
    Serial.println(+ "]");                  // Imprime en consola el caracter de corchete cerrado
    Serial.println("Pausando escritura de Archivo en un bloque e iniciando otro.");
    Serial.println(+ "[\r\n");
    archivo.println(+ "\r\n]");                 // Imprime en el ARCHIVO un corchete Cerrado
    archivo.println(+ "[\r\n");                 // Imprime en el ARCHIVO un corchete Abierto
    delay(10000);                           // Duerme 10 segundos
    digitalWrite(ledBotonParo,LOW);         // Apaga el LED en el pin ledBotonParo
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 *  Funcion que lee la temperatura del cuerpo si un latido fue previamente encontrado. 
 */
void temperatura()
{
   DateTime HoraActual = Clock.read();
   /* Re-abrimos el archivo para lectura */
   if (!archivo.open("Lecturas.bin", O_RDWR | O_CREAT | O_AT_END))     // Compara si el archivo se ha abierto si no, lanza el error
  {
    digitalWrite(ledMemoria, LOW);
  }
  else
  {
    digitalWrite(ledMemoria, HIGH);
  }
    /*Si el archivo se abrio correctamente, escribimos en el.
      Los datos se imprimen en el archivo.*/
    archivo.print(HoraActual.Day);
    archivo.print("/");
    archivo.print(HoraActual.Month);
    archivo.print("/");
    archivo.print(HoraActual.Year);
    archivo.print(",");
    archivo.print(HoraActual.Hour);
    archivo.print(":");
    archivo.print(HoraActual.Minute);
    archivo.print(":");
    archivo.print(HoraActual.Second);
    archivo.print(",");
    sensor.requestTemperatures();
    Serial.println("Temperatura guardada");
    //Serial.println(sensor.getTempCByIndex(0));
    Serial.print(" ");
    Serial.print(HoraActual.Hour);
    Serial.print(":");
    Serial.print(HoraActual.Minute);
    Serial.print(":");
    Serial.println(HoraActual.Second);
    archivo.print(sensor.getTempCByIndex(0));
    archivo.print(+ ",");
    archivo.close();                // Cierra el archivo para poder ser abierto mas adelante
    delay(100);  
}
////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////
