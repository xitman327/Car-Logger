#include <LovyanGFX.hpp>

class LGFX_ILI9488 : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9488 _panel_instance;
  lgfx::Bus_SPI _bus_instance;

public:
  LGFX_ILI9488() {
    {
      auto cfg = _bus_instance.config();
      
      // ESP32-S3 SPI Pins (Default HSPI)
      cfg.spi_host = SPI2_HOST;  // ESP32-S3 has SPI2, SPI3
      cfg.spi_mode = 0;          // SPI mode (0-3)
      cfg.freq_write = 70000000; // 40MHz (lower if unstable)
      cfg.freq_read = 16000000;  // Read speed (slower)
      cfg.spi_3wire = false;     // Set true if MISO unused
      cfg.use_lock = true;       // Thread safety

      // Pin Config (Customize for your setup)
      cfg.pin_sclk = 5;        // SCK
      cfg.pin_mosi = 7;        // MOSI
      cfg.pin_miso = 6;        // MISO (-1 = unused)
      cfg.pin_dc = 18;          // Data/Command
      
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      
      cfg.pin_cs = 8;          // Chip Select
      cfg.pin_rst = -1;         // Reset (-1 if unused)
      cfg.pin_busy = -1;        // Busy (-1 if unused)
      
      cfg.panel_width = 320;     // ILI9488 width
      cfg.panel_height = 480;    // ILI9488 height
      cfg.offset_x = 0;          // X offset
      cfg.offset_y = 0;          // Y offset
      cfg.offset_rotation = 0;   // Rotation offset (0-7)
      cfg.dummy_read_pixel = 8;  // Dummy read bits
      cfg.dummy_read_bits = 1;   // Dummy read before data
      cfg.readable = true;       // Set false for speed
      cfg.invert = false;        // Invert colors
      cfg.rgb_order = false;     // BGR order (if colors wrong)
      cfg.dlen_16bit = false;    // 16-bit data length
      cfg.bus_shared = true;    // Shared bus (with other SPI devices)
      
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};