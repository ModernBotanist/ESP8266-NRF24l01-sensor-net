

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

uint8_t data[10] = {0};
char toSendChar[10] = {0};
byte ho = 0;
byte to = 0;

void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  
  
  dht.setup(3); //pin 3
  
  
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
    
   
  
  if( (isnan(to)||isnan(ho)) == false) { //if the numbers are good
      if ((ho != 0) && (to > 3)) { //data good
            Send();
      } else { //humidity is zero, so data bad
      
          delay(dht.getMinimumSamplingPeriod()); //try again
          ho = dht.getHumidity();
          to = dht.getTemperature();
          
          
          if( (isnan(to)||isnan(ho)) == false) { //if second try was still numbers
              if ((ho != 0) && (to > 3)) { //And they are not zero
                  Send();
          }}}
      
      //if the second was also bad, then just give up.
          
  }
    
    //the powersaving step GO TO SLEEP
    driver.setModeIdle(); //turn off Radio
    
    Sleepy::loseSomeTime(180000); //off for 3 min.
    
    
    
}

/*In Send() we send the good data*/

void Send() 
{
    driver.setModeRx(); //wake up radio
    //delay(200);
  
    String toSend = "ho=" + String(ho);
    //Serial.println(toSend);
    toSend.toCharArray(toSendChar,10); //create a char array of the string
  
    memcpy(data, toSendChar, 10); //copy the char array memory space to the data array
  
    // Dont put this on the stack:
    //uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
  
    manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS); //SEND
    delay(500);
    
    
    //Send Temperature data separetly  
    toSend = "to=" + String(to);
    toSend.toCharArray(toSendChar,10); //create a char array of the string
  
    memcpy(data, toSendChar, 10); //copy the char array memory space to the data array
  
    manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS); //SEND"to=" + String(to);
}
