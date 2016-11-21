//////////
/////////  All Serial Handling Code, 
/////////  It's Changeable with the 'serialVisual' variable
/////////  Set it to 'true' or 'false' when it's declared at start of code.  
/////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
void serialOutput()
{   // Decide How To Output Serial. 
   if (serialVisual == false)
   {  
       arduinoSerialMonitorVisual('-', Signal);   // goes to function that makes Serial Monitor Visualizer
   } else
   {
        sendDataToSerial('S', Signal);     // goes to sendDataToSerial function
   }        
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Decides How To OutPut BPM and IBI Data
void serialOutputWhenBeatHappens()
{    
   if (serialVisual == false)
   {            //  Code to Make the Serial Monitor Visualizer Work
      if(!archivo.open("Lecturas.bin", O_RDWR | O_CREAT | O_AT_END))
        {
          digitalWrite(ledMemoria, LOW);
        }
        else
        {
            digitalWrite(ledMemoria,HIGH);
            Serial.print("*Latido dectectado* ");  //ASCII Art Madness
            archivo.print(BPM);                        // Guarda en el archivo el valor numerico de la variable BPM
            archivo.println(+ ",");     
            Serial.print("  ");
            archivo.close();  
        }
   } 
   else
   {
          sendDataToSerial('B',BPM);   // send heart rate with a 'B' prefix
          sendDataToSerial('Q',IBI);   // send time between beats with a 'Q' prefix
   }   
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Sends Data to Pulse Sensor Processing App, Native Mac App, or Third-party Serial Readers. 
void sendDataToSerial(char symbol, int data )
{
    Serial.print(symbol);
    Serial.println(data);                
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//*  Code to Make the Serial Monitor Visualizer Work
void arduinoSerialMonitorVisual(char symbol, int data )
{    
  const int sensorMin = 0;      // sensor minimum, discovered through experiment
  const int sensorMax = 1024;    // sensor maximum, discovered through experiment

  int sensorReading = data;
  // map the sensor range to a range of 12 options:
  int range = map(sensorReading, sensorMin, sensorMax, 0, 2);

  // do something different depending on the 
  // range value:
  switch (range) 
  {
    case 0:     
      Serial.println("-");     /////ASCII Art Madness
      break;
    case 1:   
      Serial.println("---");
      break;
    case 2:    
      Serial.println("------");
      break;
  } 
}
////////////////////////////////////////////////////////////////////////////////////////////////////////

