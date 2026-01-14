#define SDA 11
#define SCL 10

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// SSD1306Wire display(0x3c, SDA, SCL, GEOMETRY_128_32);
bool display_ok = false;
void setup_lcd(){
    Wire.begin(SDA, SCL);
    display_ok = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, false);
    if(!display_ok){
        log_e("Display not ok");
    }else{
        display.clearDisplay();
        display.setRotation(0);
        display.setCursor(0,0);
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.print("Welcome to Use!");
        display.display();
    }
    
}
#define update_lcd 200
uint32_t tm_lcd;
void loop_lcd(){
    if(millis() - tm_lcd > update_lcd && display_ok){
        tm_lcd = millis();
        display.clearDisplay();
        display.setCursor(0,0);
        gps_location_valid? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
        display.println("GPS");
        log_started? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
        display.println("LOG");
        engine_on? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
        display.println("ENG");
        detector.base.valid? display.setTextColor(BLACK, WHITE) : display.setTextColor(WHITE, BLACK);
        display.println("Base");


        display.setTextColor(WHITE, BLACK);
        display.setCursor(25,0);
        display.printf("Loc %5d\n", trip_locations_count);
        display.setCursor(25,display.getCursorY());
        display.printf("Kmh   %3.0f\n", kmph);
        display.setCursor(25,display.getCursorY());
        display.printf("RPM %5.0f\n", rpmn);
        display.setCursor(25,display.getCursorY());
        display.printf("Temp %5.0f\n", temperaturas);
        display.setCursor(25,display.getCursorY());
        display.printf("Humd %5.0f\n", humidituras);
        
        
        display.display();
    }

}