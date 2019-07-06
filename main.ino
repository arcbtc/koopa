/**
 *  ESP32/MH-ET-LIVE epaper watch only bitcoin wallet
 *  
 *
 *  Epaper PIN MAP: [VCC - 3.3V, GND - GND, SDI - GPIO23, SCLK - GPIO18, 
 *                   CS - GPIO5, D/C - GPIO17, Reset - GPIO16, Busy - GPI19]
 *                   
 *
 */
 
#include "qrcode.h"
#include "Bitcoin.h"
#include "EEPROM.h"

#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

const int touchPinNext = 15; 
const int touchPinPrev = 4; 
// change with your threshold value
const int threshold = 20;
// variable for storing the touch pin value 
int touchValueNext;
int touchValuePrev;

GxEPD2_BW<GxEPD2_154, GxEPD2_154::HEIGHT> display(GxEPD2_154(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 19));
HDPublicKey pub("YOUR-MASTER-PUBKEY");

#define MAGIC 0x12345678 // some magic number to detect if we already wrote index in memory before
uint32_t ind = 0; // address index

void setup()
{
  Serial.begin(115200);

   // reading address index from memory
    EEPROM.begin(10);
  if(EEPROM.readULong(0) == MAGIC){ // magic, check if we stored index already
    ind = EEPROM.readULong(4);
  }else{ // memory is empty - write magic and index there
    EEPROM.writeULong(0, MAGIC);
    EEPROM.writeULong(4, ind);
    EEPROM.commit();
  }

  delay(100);
  display.init(115200); 
  showAddress();
}

void loop(){
  
   bool newaddr = false; // do we need to update?

  touchValueNext = touchRead(touchPinNext);
  touchValuePrev = touchRead(touchPinPrev);

  if((touchValueNext < 20) && (touchValueNext > 0)){ // next address
    ind++;
    newaddr = true;
  }

    
  if((touchValuePrev < 20) && (touchValuePrev > 0)){ // previous address
    ind--;
    newaddr = true;
  }
  if(newaddr){
    EEPROM.writeULong(4, ind); // write new index value
    EEPROM.commit();
    showAddress();
  }
 
  delay(100);
}

void showAddress(){

  String addr = pub.child(0).child(ind).address();
  
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  
  // auto detect best qr code size
  int qrSize = 10;
  int sizes[17] = { 14, 26, 42, 62, 84, 106, 122, 152, 180, 213, 251, 287, 331, 362, 412, 480, 504 };
  int len = addr.length();
  for(int i=0; i<17; i++){
    if(sizes[i] > len){
      qrSize = i+1;
      break;
    }
  }
  
  // Create the QR code
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrSize)];
  qrcode_initText(&qrcode, qrcodeData, qrSize, 1, (String("bitcoin:")+addr).c_str());

    int width = 17 + 4*qrSize; // ???
  int scale = 150/width;
  int padding = (200 - width*scale)/2;

  display.setFullWindow();
  display.firstPage();

  // align path in the center of the screen
  String path = String("m/84'/1'/0'/0/")+ind;
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(path, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x0 = ((display.width() - tbw) / 2) - tbx;
  uint16_t y0 = 20;

  // update display, copy-paste from display library example
  do{
    display.fillScreen(GxEPD_WHITE);

    display.setCursor(x0, y0);
    display.print(path);

    // for every pixel in QR code we draw a rectangle with size `scale`
    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if(qrcode_getModule(&qrcode, x, y)){
              display.fillRect(padding+scale*x, 10 + padding+scale*y, scale, scale, GxEPD_BLACK);
            }
        }
    }
}while(display.nextPage());

}
