//AVTOMATSKI ZALIVALNIK S PRIKAZOM VLAŽNOSTI ZEMLJE, VLAGE V ZRAKU IN TEMPERATURE


//display
#include "U8glib.h"
U8GLIB_SH1106_128X64 u8g(13, 11, 10, 9, 8);  // D0=13, D1=11, CS=10, DC=9, Reset=8
float temp = 12.0;
float pavza = 0.0;


//ezo pump
#include <SoftwareSerial.h>                           //we have to include the SoftwareSerial library, or else we can't use it
#define rx 2                                          //define what pin rx is going to be - receive
#define tx 3                                          //define what pin tx is going to be - transmit
SoftwareSerial myserial(rx, tx);                      


//DHT11 senzor vlage in temperature
#include <dht.h>
dht DHT;
#define DHT11_PIN 7


//spremenljivke
String inputstring = "";                              //a string to hold incoming data from the PC - kar pumpi posiljam
String devicestring = "";                             //a string to hold the data from the Atlas Scientific product - kar mi sporoci nazaj
boolean input_string_complete = false;                //have we received all the data from the PC
boolean device_string_complete = false;               //have we received all the data from the Atlas Scientific product
float ml;                                             //used to hold a floating point number that is the volume 
String pozeni = "";


int sensorPin = A0;    //izbran input pin senzorja vlaznosti zemlje
int vlaznost = 0;      //spremenljivka za shranjevanje vrednosti senzorja vlaznosti zemlje
float odstotek_vlaznost = 0;
//float vlaznost1 = 0;


//za timer (pavza od zadnjega zalitja)
unsigned long startTime = 0; // 
unsigned long duration = 900000;


void setup() {                                        //set up the hardware
  Serial.begin(9600);                                 //set baud rate for the hardware serial port_0 to 9600
  myserial.begin(9600);                               //set baud rate for the software serial port to 9600
  inputstring.reserve(10);                            //set aside some bytes for receiving data from the PC
  devicestring.reserve(30);                           //set aside some bytes for receiving data from the Atlas Scientific product
  
  myserial.print("i");
  myserial.print('\r');

  //display uvodni zaslon
  u8g.firstPage();  
  do {
    u8g.setFont(u8g_font_helvB10);  
    u8g.drawStr(30, 10, " "); 
    u8g.drawStr(50, 30, " Zalivalnik ");
    u8g.drawStr(10, 50, " ");
    u8g.drawStr(10, 60, "Domen Mravinc");
  } while( u8g.nextPage() );
  delay(3000);
}


void serialEvent() {                                  //if the hardware serial port_0 receives a char
  inputstring = Serial.readStringUntil(13);           //read the string until we see a <CR> - beri do carriage returna
  input_string_complete = true;                       //set the flag used to tell if we have received a completed string from the PC
}




void loop() {


  //senzor vlage
  vlaznost = analogRead(sensorPin); //prebere vrednost iz senzorja
  //delay(1000);          

  //izracun odstotka vlaznosti
  odstotek_vlaznost = (1.0 - (vlaznost / 1023.0)) * 100.0; //spremenu vrednost 0-1023 v odstotek, 100% = max vlažnost

  //pogon crpalke EZO-PMP
  if (odstotek_vlaznost <  15.0) { //ce je vlaznost premajhna in od zadnjega pogona crpalke ni minilo n-časa

    if (startTime == 0) {
      startTime = millis(); //zagon casovnika
      //Serial.println("Timer started!");
      myserial.print("d,100"); //zagon crpalke, kontinuiran način do 80 ml načrpane vode
      myserial.print('\r');
    }

    if (millis() - startTime >= duration) { //casovnik se resetira, ce je preteklo 15 minut
      startTime = 0;
      //Serial.println("Timer stopped!");
    }
  }





  //display izpis  
  int chk = DHT.read11(DHT11_PIN); //prebere vrednost senzorja za temo. in vlago okolja

  u8g.firstPage();  
  do {
    u8g.setFont(u8g_font_helvR12);  
    u8g.drawStr(0, 15, "Zemlja:");  
    u8g.setPrintPos(75, 15);
    u8g.print(odstotek_vlaznost, 0);
    //u8g.print((char)176);
    u8g.print("%");
    
    u8g.drawStr(0, 35, "Vlaga:");
    u8g.setPrintPos(75, 35);
    u8g.print(DHT.humidity, 0);
    u8g.print("%");

    u8g.drawStr(0, 55, "Temp.:");
    u8g.setPrintPos(75, 55);
    u8g.print(DHT.temperature, 0);
    u8g.print((char)176);
    u8g.print("C");


  } while( u8g.nextPage() );

  //delay(1000);



  //za posiljanje komand crpalki iz terminala
  if (input_string_complete == true) {                //if a string from the PC has been received in its entirety
    myserial.print(inputstring);                      //send that string to the Atlas Scientific product
    myserial.print('\r');                             //add a <CR> to the end of the string
    inputstring = "";                                 //clear the string
    input_string_complete = false;                    //reset the flag used to tell if we have received a completed string from the PC
  }


  //shranim, kar mi crpalka pove
  if (myserial.available() > 0) {                     //if we see that the Atlas Scientific product has sent a character
    char inchar = (char)myserial.read();              //get the char we just received
    devicestring += inchar;                           //add the char to the var called devicestring
    if (inchar == '\r') {                             //if the incoming character is a <CR>
      device_string_complete = true;                  //set the flag
    }
  }

  // 
  if (device_string_complete == true) {                           //if a string from the Atlas Scientific product has been received in its entirety
    Serial.println(devicestring);                                 //send that string to the PC's serial monitor
    if (isdigit(devicestring[0]) || devicestring[0]== '-') {      //if the first character in the string is a digit or a "-" sign    
      ml = devicestring.toFloat();                                //convert the string to a floating point number so it can be evaluated by the Arduino   
       }                                                          //in this code we do not use "ml", But if you need to evaluate the ml as a float, this is how it’s done     
    devicestring = "";                                            //clear the string
    device_string_complete = false;                               //reset the flag used to tell if we have received a completed string from the Atlas Scientific product
  }
}
