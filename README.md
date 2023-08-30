# VIN_projekt: avtomatski zalivalnik z zaslonom

**Za svoj VIN projekt sem izdelal avtomatski zalivalnik za rastline z zaslonom.**

**Uporabljene komponente:**
  - Arduino Uno
  - EZO-PMP dozirna črpalka prozvajalca Atlas Scientific
  - SSD1306 SPI OLED zaslon
  - merilec vlažnosti zemlje z LM393
  - DHT11 merilec vlage in temperature v prostoru

## Potek:
Za delo sem si izbral mikrokontroler Arduino Uno. Originalno sem sicer želel projekt izvesti na STM32F4, vendar sem se tekom projekta premislil, kajti za ključno komponento, ki sem jo imel že od prej - EZO-PMP črpalko, obstaja proizvajalčeva obsežna dokumentacija zgolj za Arduino in Raspberry PI.

**Senzor za merjenje vlažnosti zemlje**

Senzor je že prišel s povezanim napetostnim primerjalnikom LM393. Na njem so 4 pini, od katerih sem uporabil 3:
- Vcc na 5V,
- Gnd,
- Analog out na A0.

V kodo sem dodal:

```
  int sensorPin = A0;    //izbran input pin senzorja vlaznosti zemlje
  int vlaznost = 0;      //spremenljivka za shranjevanje vrednosti senzorja vlaznosti zemlje
  float odstotek_vlaznost = 0;
```
V glavni loop:
```
  vlaznost = analogRead(sensorPin); //prebere vrednost iz senzorja
  delay(1000);          
  //izracun odstotka vlaznosti
  odstotek_vlaznost = (1.0 - (vlaznost / 1023.0)) * 100.0; //sprememba vrednosti 0-1023 v odstotek, 100% = max vlažnost
```
Privzeto je prebrana vrednost senzorja, priklopljenega na 5V, v rangu od 0 do 1023. Za lažje delo in kasneje prikaz sem vrednost pretvoril na rang od 0 do 100%, kjer 100% pomeni maksimalna saturacija z vodo.

**EZO-PMP črpalka**

Dokumentacija: https://files.atlas-scientific.com/EZO_PMP_Datasheet.pdf

Gre za kompaktno dozirno peristaltično črpalko z večimi načini delovanja. Komunicira preko UART ali I2C. Zahteva dve napajanji: za motor črpalke sem jo povezal na 12V, za kontrolni sistem pa na 5V. Ima tri glavne komponente, kaseto, motor in kontrolno enoto:

![image](https://github.com/domenFRI/VIN_projekt/assets/76186864/27afdc24-373d-49b0-a403-5da2ef91115b) ![image](https://github.com/domenFRI/VIN_projekt/assets/76186864/5a3590a2-3067-4eb9-9b30-f4faef66b377)


Za protokol komunikacije sem izbral UART. Kontrolna enota ima 5 pinov, uporabil sem 4:
- RX na pin 3, 
- TX na pin 2,
- Gnd,
- Vcc na 5V.

V kodi:
```
//ezo pump
#include <SoftwareSerial.h>                           //knjiznica, ki omogoča serijsko komunikacijo brez UART pinov
#define rx 2                                          //receive rx - bo pin 2
#define tx 3                                          //transmit tx - bo pin 3
SoftwareSerial myserial(rx, tx);

//za timer (pavza od zadnjega zalitja)
unsigned long startTime = 0; // 
unsigned long duration = 900000;
```

S črpalko komuniciramo s pošiljanjem ASCII znakov v stringu, tako tudi sama komunicira nazaj. Za zagon črpanja vode sem uporabil niz "d,80": z "d" sem izbral način delovanja (kontinuirano črpanje), z "80" pa želen volumen vode: črpalka bo torej neprekinjeno izčrpala 80 ml vode. Da se voda dejansko začne črpati, sem dodal še dva pogoja. Prvi je ta, da mora biti vrednost, ki jo prebere senzor vlažnosti zemlje, nižja od 15%. To sem izbral na podlagi testiranja, namreč različno vlažne zemlje (oz. mešanice s posajenimi rastlinami) sem primerjal in določil vrednost 15% kot mejo, ko mora biti rastlina zalita. Ob pogonu črpalke sem hkrati zagnal tudi timer oz. časovnik. Namreč drugi pogoj, ki ga preverjam pred dejanskim pogonom črpalke je tudi čas, kdaj je bila črpalka nazadnje zagnana. Tega sem določil kot 15 minut (oz. 900000 ms). Ob zalivanju namreč ni nujno, da se zemlja takoj saturira z dovedeno vodo - s tem 15 minutnim časovnik sem omogočil, da se voda, ki je bila že dočrpana v rastlino, lahko vpije. V nasprotnem primeru bi lahko prišlo do prekomernega zalitja, kajti voda se še ne bi vpila, nizka vrednost iz senzorja v zemlji pa bi ponovno pognala črpalko.

V glavnem loopu:

```
//pogon crpalke EZO-PMP
  if (odstotek_vlaznost <  15.0) { //ce je vlaznost prenizka in od zadnjega pogona crpalke ni minilo n-časa

    if (startTime == 0) {
      startTime = millis(); //zagon casovnika
      //Serial.println("Timer started!");
      myserial.print("d,80"); //zagon crpalke, kontinuiran način do 80 ml načrpane vode
      myserial.print('\r');
    }

    if (millis() - startTime >= duration) { //casovnik se resetira, ce je preteklo 15 minut
      startTime = 0;
      //Serial.println("Timer stopped!");
    }
  }
```
Dodal Gsem tudi kodo iz proizvajalčeve dokumentacije, ki ob priklopljenem Arduinu na računalnik omogoča komunikacijo s črpalko direktno z vnosom s tipkovnico in serijskim monitorjem.

**DHT11 merilec vlage in temperature v prostoru**

Povezal sem 3 pine: + upor!
- Vcc na 5V,
- Gnd,
- signal na pin 7.

V kodi:
```
//DHT11 senzor vlage in temperature
#include <dht.h>
dht DHT;
#define DHT11_PIN 7 //definiran pin
```
V loopu:
```
  int chk = DHT.read11(DHT11_PIN); //prebere vrednost
```
Vrednost temperature in vlage sem nato uporabil pri izpisu na zaslon.

**SSD1306 SPI OLED zaslon**

Dodal sem zaslon, ki komunicira preko SPI protokola. Na njem je 7 pinov, ki sem jih povezal:
- Vcc na 3.3V,
- Gnd,
- D0 na pin13,
- D1 na 11,
- CS na 10,
- DC na 9,
- Reset na 8.

V kodi:
```
//display
#include "U8glib.h"
U8GLIB_SH1106_128X64 u8g(13, 11, 10, 9, 8);  // D0=13, D1=11, CS=10, DC=9, Reset=8
float temp = 12.0;
float pavza = 0.0;
```
Zaslon sem uporabil za 2 zaslonski maski, in sicer ob prižigu se prikaže moje ime:

```
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
```

Po treh sekundah se prikaže glavni del:

```
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
...
```
Zaslon torej hkrati prikazuje vlažnost zemlje, vlažnost okolice in temperaturo.

**Prikaz delovanja na videu:**

https://video.arnes.si/watch/84rc7gp5l1jd

Ob priklopu zalivalnika se na zaslonu najprej izpiše uvodno sporočilo, po treh sekundah pa informacije o vlažnosti zemlje, zraka in temperatura okolice. Ker je vrednost vlažnosti zemlje <15%, se črpalka takoj zažene in izčrpa 80 ml vode. Med črpanjem vode se odstotek vlažnosti zemlje viša. Črpalka nato v vsakem primeru 15 minut ne bo črpala, ne glede na vlažnost, zato da se že izčrpana voda vpije v zemljo. 

![image](https://github.com/domenFRI/VIN_projekt/assets/76186864/94a8d180-b1cd-474e-aa6d-0c4ac2f7eec3)


