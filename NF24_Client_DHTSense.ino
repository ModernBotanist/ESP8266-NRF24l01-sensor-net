

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
int ho = 0;
int to = 0;


void setup() 
{
  Serial.begin(9600);
  if (!manager.init())
    Serial.println("init failed");
  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
  
  //keep 2Mbps, but lower power output
  //driver.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPowerm12dBm);
  
  
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
    the DHT11 cannot measure 0% RH. Both H and T are zero
    together, and since T might actually be zero, just test H
    */
  
  if( (isnan(to)||isnan(ho)) == false) { //if the numbers are good
      if (ho != 0) { //data good
            Send();
      } else { //humidity is zero, so data bad
          delay(dht.getMinimumSamplingPeriod()); //try again
          ho = dht.getHumidity();
          to = dht.getTemperature();
      
          if( (isnan(to)||isnan(ho)) == false) { //if second try was still numbers
              if (ho != 0) { //And they are not zero
                  Send();
          }}}
      
      //if the second was also bad, then just give up.
          
  }
    
    //the powersaving step GO TO SLEEP
    driver.setModeIdle(); //turn off Radio
    
    Sleepy::loseSomeTime(180000); //off for 3 min.
    
    
    
}


void Send() 
{
    driver.setModeRx();
    delay(200);
  
    String toSend = "ho=" + String(ho) + "\r\n" + "to=" + String(to);
    //Serial.println(toSend);
    toSend.toCharArray(toSendChar,25); //create a char array of the string
  
    memcpy(data, toSendChar, 25); //copy the char array memory space to the data array
  
    // Dont put this on the stack:
    //uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
  
    manager.sendtoWait(data, sizeof(data), SERVER_ADDRESS); //SEND
}

