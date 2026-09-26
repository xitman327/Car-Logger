

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define update_lcd 200
uint32_t tm_lcd;
bool display_ok = false;

void loop_lcd(){
    if(millis() > tm_lcd && display_ok){
        
        display.clearDisplay();
        display.setCursor(0,0);
        gps_location_valid? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
        display.println("GPS");
        log_started? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
        display.println("LOG");
        engine_on? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
        display.println("ENG");
        WiFi.isConnected()? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
        display.println("WiFi");
        // detector.base.valid? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
        // display.println("Base");


        display.setTextColor(WHITE, BLACK);
        display.setCursor(25,0);
        display.printf("Loc%5d %4d Km\n", trip_locations_count, trip_distance_km);
        display.setCursor(25,display.getCursorY());
        display.printf("Kmh  %3.0f  %2.1f Lkm\n", kmph, lpkm);
        display.setCursor(25,display.getCursorY());
        display.printf("RPM %4.0f\n", rpmn);
        display.setCursor(25,display.getCursorY());
        display.printf("Temp%3.0f\n", engine_temp);

        display.display();

        tm_lcd = millis() + update_lcd;
    }

}
// clear display and print a message
// showTime arg is in secconds
void showLcdMessage(String message, uint16_t showTime = 3, bool invert = false){
    if(!display_ok){return;}

    display.clearDisplay();
    display.setCursor(0,0);
    invert? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
    bool tempWrap = display.getTextWrap();
    display.setTextWrap(true);
    display.print(message);
    display.setTextWrap(tempWrap);
    display.display();


    tm_lcd = millis() + (showTime * 1000);
}


void setup_lcd(){
    Wire.begin(SDA_PIN, SCL_PIN);
    display_ok = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, false);
    if(!display_ok){
        log_e("Display not ok");
    }else{
        display.clearDisplay();
        display.setRotation(0);
        display.setCursor(0,0);
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.display();

        showLcdMessage("Welcome to use!", 3);
    }
    
}