#include <arduino.h>
#include <Wire.h>
#include <display.h>
#include <config.h>
#include <SPI.h>
#include <Ucglib.h>

// Comments out when required software-based SPI.
#define LCD_SPI_HW

// (Optional, drive with LED)
#define WITH_LED

// used for millis loops
const unsigned long flProgressBar = 200; // speed of update progressbar
const unsigned long flWifiBTStarting = 500; //flash speed Of wifi & BT icon on starting 
const unsigned long flWifiSignal = 5000; //frequency updating wifi signal strength icon
unsigned long prevTimerProgressBar = 0;
unsigned long prevTimerWifiStarting = 0;
unsigned long prevTimerBtStarting = 0;
unsigned long prevTimerRuntime = 0;
unsigned long prevTimerWifiSignal = 0;

// Used variables
int progress = 0;
bool btConnected = false;
byte byteWifiMode;
int intWifiSignal;
byte year = 0;
byte hours = 0;
byte minutes = 0;
byte days = 0;
bool enableProgressbar=false;
String strdispIP = "254.254.254.254";
String strdispStatus="NA";
byte prevStateIcons = 0;
byte prevBTStateIcons = 0;



// ESP-WROVER-KIT aasignment       LCD (ILI9341)
#define LCD_RST   18   // RST  --> RESET
#define LCD_SCLK  19   // SCLK --> SCL
#define LCD_DC    21   // DC   --> D/C
#define LCD_CS    22   // CS   --> CS
#define LCD_MOSI  23   // MOSI --> SDA
#define LCD_MISO  25   // MISO <-- SDO
#define LCD_BL    5    // BL   --> LEDK

#if defined(WITH_LED)
#define LED_RED 0     // Onboard Red LED
#define LED_GREEN 2   // Onboard Green LED
#define LED_BLUE 4    // Onboard Blue LED
#endif

#if defined(LCD_SPI_HW)
static Ucglib_ILI9341_18x240x320_HWSPI ucg(LCD_DC, LCD_CS, LCD_RST);
#else
static Ucglib_ILI9341_18x240x320_SWSPI ucg(LCD_SCLK, LCD_MOSI, LCD_DC, LCD_CS, LCD_RST);
#endif

void initDisplay()
{
  delay(1000);

#if defined(WITH_LED)
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);
#endif

  // Enable backlight.
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, LOW);

#if defined(LCD_SPI_HW)
  // Initialize hardware-based SPI, assigned using ESP32 VSPI.
  SPI.begin(LCD_SCLK, LCD_MISO, LCD_MOSI, LCD_CS);
#endif

  // Initialize Ucglib
  ucg.begin(UCG_FONT_MODE_TRANSPARENT);
  ucg.clearScreen();
  ucg.setRotate90();
  ucg.setFont(ucg_font_8x13_tf);


#if defined(WITH_LED)
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_BLUE, LOW);
#endif
      ucg.setColor(255, 0, 0);
      ucg.setPrintPos(50,50);
      ucg.print("Hello World!");

    wrDisp_IP();
    wrDisp_Running();
    wrDisp_Status("Init....");
    btConnected = false;   
    byteWifiMode = 0;
}

void handleDisplay()
{
    // progress bar
    if (enableProgressbar == true)
    {
        if (millis() - prevTimerProgressBar >= flProgressBar)
        {
            progress = (progress + 5) % 110;
            drawProgressbar(10, 59, 108, 5, progress);
            prevTimerProgressBar = millis();
        }
    }
    // update running time every 5 seconds
    if (millis() - prevTimerRuntime >= 5000)
    {
        wrDisp_Running();
        prevTimerRuntime = millis();
    }
    // blue not connected is blinking
    if (btConnected == false)
    {
        if (millis() - prevTimerBtStarting >= flWifiBTStarting)
        {
            if (prevBTStateIcons == 0)
            {
                // Flash background white
                prevBTStateIcons = 1;
                blueToothSignal(btConnected);
            }
            else
            {
                // flash background Black
                prevBTStateIcons = 0;
                blueToothSignal(btConnected);
            }
            prevTimerBtStarting = millis();
        }
    }
    if (byteWifiMode == 0)
    {
        if (millis() - prevTimerWifiStarting >= flWifiBTStarting)
        {
            if (prevStateIcons == 0)
            {
                // Flash background white
                prevStateIcons = 1;
                wifisignal(0);
            }
            else
            {
                // flash background Black
                prevStateIcons = 0;
                wifisignal(0);
            }
            prevTimerWifiStarting = millis();
        }
    } else
    {
        if (millis() - prevTimerWifiSignal >= flWifiSignal)
        {
            wifisignal(byteWifiMode, intWifiSignal); 
            prevTimerWifiSignal = millis();
        }
    }

}
void wrDisp_IP(String strIP)
{
    ucg.setColor(255, 255, 255);
    ucg.drawBox(0,140,150,12);
    ucg.setColor(0, 0, 255);
    ucg.setPrintPos(0, 150);
    ucg.print("IP:"+ strIP);
    
}
void wrDisp_Running()
{
    ucg.setColor(0, 0, 0);
    ucg.drawBox(0,100,150,12);
    ucg.setColor(0, 255, 0);
    ucg.setPrintPos(0, 110);
    // important: millis resets after 49days - currently not taken into account in the calculations
    // example output: 365d23h60m
    
    if (((millis()/1000) > 60) && (millis()/1000 <3600))
    {
        
        minutes = ((int)((millis()/1000)/60));
    } else if ((millis()/1000 > 3600) && (millis()/1000 <86400))
    {
        
        hours = ((int)(millis()/1000)/3600); 
        minutes = ((int)(((millis()/1000)-(hours*3600))/60));
    } else if ((millis()/1000 > 86400))
    {
        days = ((int)(millis()/1000)/86400);
        hours = ((int)(((millis()/1000)/3600)-((days*24)))); 
        minutes = ((int)(((millis()/1000)-(hours*3600))/60));
    }
    ucg.print("Runtime: "+ String(days)+"d"+String(hours)+"h"+String(minutes)+"m"); //running time will be max 49days until millis is reset
    
}
void wrDisp_Status(String strStatus)
{
    ucg.setColor(0, 0, 0);
    ucg.drawBox(0,120,150,12);
    ucg.setColor(0, 255, 0);
    ucg.setPrintPos(0, 130);
    ucg.print("Status:" + strStatus);    
}
void blueToothSignal(bool blConnected)
{
    ucg.setColor(128, 128, 128);
    ucg.drawBox(115, 18, 13, 13);
    ucg.setColor(0, 0, 255);
    
        if (blConnected == true)
    {
        ucg.drawPixel(118, 21);
        ucg.drawPixel(118, 27);
        ucg.drawPixel(119, 22);
        ucg.drawPixel(119, 26);
        ucg.drawPixel(120, 25);
        ucg.drawPixel(120, 23);
        ucg.drawLine(121, 19, 121, 29);
        ucg.drawPixel(122, 19);
        ucg.drawPixel(122, 24);
        ucg.drawPixel(122, 29);
        ucg.drawPixel(123, 20);
        ucg.drawPixel(123, 23);
        ucg.drawPixel(123, 25);
        ucg.drawPixel(123, 28);
        ucg.drawPixel(124, 21);
        ucg.drawPixel(124, 22);
        ucg.drawPixel(124, 26);
        ucg.drawPixel(124, 27);
        
    }
    else
    {
        ucg.drawPixel(118, 21);
        ucg.drawPixel(118, 27);
        ucg.drawPixel(119, 22);
        ucg.drawPixel(119, 26);
        ucg.drawPixel(120, 25);
        ucg.drawPixel(120, 23);
        ucg.drawLine(121, 19, 121, 29);
        ucg.drawPixel(122, 19);
        ucg.drawPixel(122, 24);
        ucg.drawPixel(122, 29);
        ucg.drawPixel(123, 20);
        ucg.drawPixel(123, 23);
        ucg.drawPixel(123, 25);
        ucg.drawPixel(123, 28);
        ucg.drawPixel(124, 21);
        ucg.drawPixel(124, 22);
        ucg.drawPixel(124, 26);
        ucg.drawPixel(124, 27);        
    }
}
void wifisignal(int intMode, int intSignal)
{
    // intMode:
    // 0, not connected
    // 1, connected
    // 2, AP mode

    // -55 or higher: 4 bars
    // -56 to -66: 3 bars
    // -67 to -77: 2 bars
    // -78 to -88: 1 bar
    // -89 or lower: 0 bars -> not implemented

    ucg.setColor(128, 128, 128);
    ucg.drawBox(115, 0, 13, 13);
    
    ucg.setColor(255, 255, 255);
    byte textColor = 1; // 1 for White and 0 for black
        if (intMode == 1)
    {
        if (textColor == 0)
        {
            // Black on white
            //ucg.drawBox(115, 0, 13, 13);
            
        } else
        {
            // White on black
            //ucg.drawBox(115, 0, 13, 13);
            //display.drawRect(115, 0, 13, 13, 1);
            

        }
        if ((intSignal > -88) && (intSignal <= -78))
        {
            // extreme weak signal 1 bar
            ucg.drawPixel(121, 11);
        } else if ((intSignal > -77) && (intSignal <= -67))
        {
            // 2 bars

            // one bar
            ucg.drawPixel(121, 11);
            // 2 bar
            ucg.drawPixel(119, 9);
            ucg.drawPixel(123, 9);
            ucg.drawPixel(120, 8);
            ucg.drawPixel(121, 8);
            ucg.drawPixel(122, 8);
        }  else if ((intSignal > -66) && (intSignal <= -56))
        {
            // 3 bars

            // one bar
            ucg.drawPixel(121, 11);
            // 2 bar
            ucg.drawPixel(119, 9);
            ucg.drawPixel(123, 9);
            ucg.drawPixel(120, 8);
            ucg.drawPixel(121, 8);
            ucg.drawPixel(122, 8);
            // 3 bar
            ucg.drawPixel(118, 6);
            ucg.drawPixel(118, 6);
            ucg.drawPixel(124, 6);
            ucg.drawPixel(119, 5);
            ucg.drawPixel(120, 5);
            ucg.drawPixel(121, 5);
            ucg.drawPixel(122, 5);
            ucg.drawPixel(123, 5);
        }
        else if (intSignal > -55)
        {
            // 4 bars

            // one bar
            ucg.drawPixel(121, 11);
            // 2 bar
            ucg.drawPixel(119, 9);
            ucg.drawPixel(123, 9);
            ucg.drawPixel(120, 8);
            ucg.drawPixel(121, 8);
            ucg.drawPixel(122, 8);
            // 3 bar
            ucg.drawPixel(118, 6);
            ucg.drawPixel(118, 6);
            ucg.drawPixel(124, 6);
            ucg.drawPixel(119, 5);
            ucg.drawPixel(120, 5);
            ucg.drawPixel(121, 5);
            ucg.drawPixel(122, 5);
            ucg.drawPixel(123, 5);
            // 4 bar
            ucg.drawPixel(116, 4);
            ucg.drawPixel(126, 4);
            ucg.drawPixel(117, 3);
            ucg.drawPixel(125, 3);
            ucg.drawPixel(118, 2);
            ucg.drawPixel(119, 2);
            ucg.drawPixel(120, 2);
            ucg.drawPixel(121, 2);
            ucg.drawPixel(122, 2);
            ucg.drawPixel(123, 2);
            ucg.drawPixel(124, 2);
        }
           
    }
    else if (intMode == 0)
    {
        
        if (prevStateIcons == 0)
        {
            ////ucg.drawBox(115, 0, 13, 13);
        }
        else
        {
            ////ucg.drawBox(115, 0, 13, 13);
        }
        // one bar
        ucg.drawPixel(121, 11);
        // 2 bar
        ucg.drawPixel(119, 9);
        ucg.drawPixel(123, 9);
        ucg.drawPixel(120, 8);
        ucg.drawPixel(121, 8);
        ucg.drawPixel(122, 8);
        // 3 bar
        ucg.drawPixel(118, 6);
        ucg.drawPixel(118, 6);
        ucg.drawPixel(124, 6);
        ucg.drawPixel(119, 5);
        ucg.drawPixel(120, 5);
        ucg.drawPixel(121, 5);
        ucg.drawPixel(122, 5);
        ucg.drawPixel(123, 5);
        // 4 bar
        ucg.drawPixel(116, 4);
        ucg.drawPixel(126, 4);
        ucg.drawPixel(117, 3);
        ucg.drawPixel(125, 3);
        ucg.drawPixel(118, 2);
        ucg.drawPixel(119, 2);
        ucg.drawPixel(120, 2);
        ucg.drawPixel(121, 2);
        ucg.drawPixel(122, 2);
        ucg.drawPixel(123, 2);
        ucg.drawPixel(124, 2);
        // not connected / trying to connect
        // wifi logo should blink, trying to make connection
        
    }
    else if (intMode == 2)
    {
        // AP mode
        // wifi logo should contain AP as text
        //ucg.drawBox(115, 0, 13, 13);
        //display.setTextColor(BLACK, WHITE);
        ucg.setPrintPos(116, 3);
        ucg.print("AP");
        
    }

}
void disp_setWifiSignal(int extWifMode, int extSignal)
{
    intWifiSignal = extSignal;
    byteWifiMode = extWifMode;
    wifisignal(extWifMode,extSignal);
}
void disp_setWifiMode(byte wMode)
{
    byteWifiMode = wMode;
}
void disp_setIP(String strIP)
{
    if (strIP != strdispIP)
    {
        wrDisp_Status(strIP);
        strdispIP = strIP;
    }
}
void disp_setStatus(String strStatus)
{
    if (strStatus != strdispStatus)
    {
        wrDisp_Status(strStatus);
        strdispStatus = strStatus;
    }
}
void disp_setBlueTooth(bool boolBtConn)
{
    btConnected = boolBtConn;
}
void disp_setPrevStateIcon(byte bytePrevState)
{
    prevStateIcons = bytePrevState;
}
void drawProgressbar(int x,int y, int width,int height, int progress)
{

   // clear old data
   //display.drawRect(x, y, width, height, BLACK);
   //ucg.drawBox(x, y, width , height);
   
   
   progress = progress > 100 ? 100 : progress;
   progress = progress < 0 ? 0 :progress;

   float bar = ((float)(width-1) / 100) * progress;
 
   //display.drawRect(x, y, width, height, WHITE);
   //ucg.drawBox(x, y, bar , height);
   
}