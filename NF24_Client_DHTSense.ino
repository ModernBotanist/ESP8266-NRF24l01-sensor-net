

// Derived from examples in the RadioHead Library
// Produced to work on Arduino Pro Mini 3.3V/8MHz compiled with Arduino IDE 1.5.7
// with NRF24L01 and DHT11
// By Alex Bilgri
// Requires:
// The RadioHead library:
// http://www.airspayce.com/mikem/arduino/RadioHead/
// The DHT library:
// https://github.com/markruys/arduino-DHT
// And the JeeLib:
// https://github.com/jcw/jeelib

#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>
#include "DHT.h"
#include <JeeLib.h>

#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

// Singleton instance of the radio driver
RH_NRF24 driver;
// RH_NRF24 driver(8, 7);   // For RFM73 on Anarduino Mini

// Class to manage message delivery and receipt, using the driver declared above
RHReliableDatagram manager(driver, CLIENT_ADDRESS);

DHT dht; //initialize DHT for humidity/temp

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // Setup the watchdog for sleeping

uint8_t data[25] = {0};
char toSendChar[25] = {0};
byte ho = 0;
byte to = 0;
byte t[10] = {0}; //array to store last 10 temp points
byte counter = 0; //counter for above, needs to start at zero
int Tave; //storeing rolling temp average
byte range = 6; //degrees new temp measure can be off from average to be considered OK

void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  
  
  dht.setup(3); //pin 3
  
  //populate array with things close to real temp
  byte i = 0;
  while (i<10) {
      delay(dht.getMinimumSamplingPeriod());
      byte k = dht.getTemperature();
      
      if (!(isnan(k) || (k == 0))) { //k is not NaN nor 0
          t[i] = k; //store that temp
          i++;
      }
  }
  
    //Find average
    Tave = 0;
    for (byte i = 0; i<10; i++) {
      Tave += t[i]; //sum into Tave
    }
    Tave /= 10; //holds first average
  
}


void loop()
{
  
  delay(dht.getMinimumSamplingPeriod());
  ho = dht.getHumidity();
  to = dht.getTemperature();
  
  
  if(isnan(to)||isnan(ho)) { //numbers are bad, try once more
    delay(dht.getMinimumSamplingPeriod());
    ho = dht.getHumidity();
    to = dht.getTemperature();
  }
 
  /*Sometimes values are zero and not NaN. These are false,
    the DHT11 cannot measure 0% RH. But sometimes temp can be unusually
    low, 20 degrees below surrounding data points. 
    */
    
    //boolean temp = false; //reset the value at the beginning of loop
    boolean temp = abs(to - Tave)<range; //new measurement not more than 5 degrees off average
    
  
  if( (isnan(to)||isnan(ho)) == false) { //if the numbers are good
      if ((ho != 0)&&temp) { //data good
            Send();
      } else { //humidity is zero, so data bad
      
          delay(dht.getMinimumSamplingPeriod()); //try again
          ho = dht.getHumidity();
          to = dht.getTemperature();
          
          temp = abs(to - Tave)<range; //test again
          
          if( (isnan(to)||isnan(ho)) == false) { //if second try was still numbers
              if ((ho != 0)&&temp) { //And they are not zero
                  Send();
          }}}
      
      //if the second was also bad, then just give up.
          
  }
    
    //the powersaving step GO TO SLEEP
    driver.setModeIdle(); //turn off Radio
    
    Sleepy::loseSomeTime(180000); //off for 3 min.
    
    
    
}

/*In Send() we both send the good data, and store the good data. Since
by the time we've gotten here, we've tested the validity of the data
enough to send it, so we also store it and reclac the new average temp.
*/
void Send() 
{
    driver.setModeRx(); //wake up radio
    
    if (counter >= 9) { //reset to wrap around
        counter = 0;
    } else {
      counter++; //increment
    }
    
    t[counter] = to; //store new temp value
        
    
    Tave = 0; //clear average
    for (byte i = 0; i<10; i++) {
      Tave += t[i]; //sum into Tave
    }
    Tave /= 10; //holds new average
    
    
  
    String toSend = "ho=" + String(ho) + "\r\n" + "to=" + String(to);
    //Serial.println(toSend);
    toSend.toCharArray(toSendChar,25); //create a char array of the string
  
    memcpy(data, toSendChar, 25); //copy the char array memory space to the data array
  
    // Dont put this on the stack:
    //uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
  
    manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS); //SEND
}

