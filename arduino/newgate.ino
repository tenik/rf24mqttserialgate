#include <dht.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include<stdlib.h>
#include "printf.h"
#include <stdio.h>
#include <avr/pgmspace.h>

prog_char string_con[]     PROGMEM = "CON";   // "String 0" etc are strings to store - change to suit.
prog_char string_conack[]  PROGMEM = "CONACK";
prog_char string_pub[]     PROGMEM = "PUB";
prog_char string_sub[]     PROGMEM = "SUB";
prog_char string_ping[]     PROGMEM = "PING";
prog_char string_pingresp[]     PROGMEM = "PINGRESP";

RF24NetworkHeader header_for_send;
// nRF24L01(+) radio attached using Getting Started board 


// Address of our node
const uint16_t this_node = 0;

// Address of the other node
//const uint16_t other_node = 1;


#define DHT22_PIN 2
#define pirPIN 3

//dht DHT;
int chk;

int count = 0;

//xxx/OUT1=1 xxx/IN1 xxx/temp1 xxx/HUM1 xxx/BARO xxx/AOUT1=255 xxx/AIN1
// xxx CON - коннект, регистрация устройства, фиксация адреса в брокере, c устройства к гейту  rf *с
// xxx PING - пинг роутера                                                                      rf *r
// xxx PUB TOPIC DATA - паблиш, в любую сторону                                                 rf/s *p
// xxx SUB TOPIC - сабскрайб, подписка на топик от устройства брокеру                           rf *s
// xxx CONACK - подтверждение коннекта, от гейта к устройству                                   s *a 
// xxx PINGRESP - ответ пинга                                                                  s *q
//
#define MSG_CONNECT 'c'
#define MSG_CONNACK 'a'
#define MSG_PING 'r'
#define MSG_PINGRESP 'q'
#define MSG_PUB 'p'
#define MSG_SUB 's'

// -> CON                     коннект
// <- xxx CONACK
// -> PUB /SW/1|OFF             паблиш выключателя
// -> SUB /SW/1                 сабскрайб на выключатель
// -> PUB /SENS/TEMP 99.9       паблиш датчика температуры
// -> PUB /SENS/HUM 99.9        паблиш датчика влажности  
// <- xxx PUB /SW/1 on         входящее сообщение
// -> PING                     пинг
// <- ххх PINGRESP             ответ пинга
//
//

#define MAX_STRING 60
char stringBuffer[MAX_STRING];
char* getString(const char* str) {
	strcpy_P(stringBuffer, (char*)str);
	return stringBuffer;
}

//=====[ PINS ]=================================================================
int Led = 13;

//=====[ VARIABLES ]============================================================
const int bSize = 64;
const int dSize = 32;
char Buffer[bSize];  // Serial buffer
char readBuffer[bSize];  // Serial buffer
char Device[dSize]; //xxx
char Command[dSize];    // Arbitrary Value for command size
char Topic[dSize];
char Data[dSize];       // ditto for data size
int ByteCount;


RF24 radio(9,10);

// Network uses that radio
RF24Network network(radio);
//=====[ SUBROUTINES ]==========================================================

void RFParser(void)
{
  RF24NetworkHeader header;

  network.read(header,&readBuffer,sizeof(readBuffer));  

  memset(Device, 0, dSize);
  memset(Buffer, 0, dSize);
  memset(Command, 0, dSize);
  memset(Topic, 0, dSize);
  memset(Data, 0, dSize);

  itoa(header.from_node, Buffer, 10);
  strcat(Buffer, " ");
  
  switch(header.type)
  {
  case MSG_CONNECT:
    strcat(Buffer, getString(string_con));
    break;
  case MSG_PUB:
    strcat(Buffer, getString(string_pub));
    strcat(Buffer, " ");
    strcat(Buffer, readBuffer);  
    break;
  case MSG_SUB:
    strcat(Buffer, getString(string_sub));
    strcat(Buffer, " ");    
    strcat(Buffer, readBuffer);  
    break;
  case MSG_PING:
    strcat(Buffer, getString(string_ping));
    break;
  } 
}

bool SerialParser(void) {
  char *p;
  memset(readBuffer, 0, bSize);
  ByteCount =  Serial.readBytesUntil('\n',readBuffer,bSize);  
  memset(Device, 0, dSize);
  memset(Command, 0, dSize);
  memset(Topic, 0, dSize);
  memset(Data, 0, dSize);

  if (ByteCount  > 0) 
  {
    //Serial.println(Buffer);
    p = readBuffer;
    strcpy(Device,strtok(p," "));
    strcpy(Command,strtok(NULL," \n\0"));
    strcpy(Data,strtok(NULL,"\n\0"));
  //Serial.print("data:");
//  Serial.println(Data);
   
    header_for_send.to_node = atoi(Device);
    header_for_send.from_node = 0;

    if (!strcmp(Command,"PUB")) // publish
    {
      header_for_send.type = MSG_PUB;
    }
    else if  (!strcmp(Command,"CONACK")) // sudscribe
    {
      header_for_send.type = MSG_CONNACK;
    }
    else if  (!strcmp(Command,"PINGRESP")) // sudscribe
    {
      header_for_send.type = MSG_PINGRESP;
    }
    return true;
  }
  else
  {
    //memset(Buffer, 0, sizeof(Buffer));   // Clear contents of Buffer
    return false;
  }
  //Serial.flush();
}


void setup()
{
  analogReference(INTERNAL);
  Serial.begin(9600);
  Serial.setTimeout(100);
  printf_begin();

  //pinMode(13, OUTPUT);
  //pinMode(pirPIN, INPUT);
  //digitalWrite(13,1);

  SPI.begin();
  //delay(500);
  radio.begin();
  //delay(500);

  //delay(500);
  //  radio.printDetails();
    radio.setDataRate(RF24_250KBPS);
  network.begin(/*channel*/ 100, /*node address*/ this_node);  

  //Serial.println("Gate started");
}


void loop()
{
  // Pump the network regularly
  network.update();

  if ( network.available() ) // something come for us
  {
    //Serial.println("b p");
    RFParser();
    //Serial.println("after parser");      
    Serial.println(Buffer);
    //Serial.flush();
  }

  if (Serial.available())
  {
    if (SerialParser()) //command parsed and ready for send
    {
        bool ok = network.write(header_for_send,Data,strlen(Data)+1);
        //Serial.print("sended:");
        //Serial.println(Data);
        network.update();
    }
  }
  //Serial.println(".");


}






