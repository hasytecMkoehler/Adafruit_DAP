#include "Adafruit_DAP.h"
#include <SdFat.h>
#include <Adafruit_SPIFlash.h>

#define FILENAME "fw.bin"

#define BUFSIZE 256 //don't change

// Configuration of the flash chip pins and flash fatfs object.
// You don't normally need to change these if using a Feather/Metro
// M0 express board.
#if defined(__SAMD51__) || defined(NRF52840_XXAA)
  Adafruit_FlashTransport_QSPI flashTransport(PIN_QSPI_SCK, PIN_QSPI_CS, PIN_QSPI_IO0, PIN_QSPI_IO1, PIN_QSPI_IO2, PIN_QSPI_IO3);
#else
  #if (SPI_INTERFACES_COUNT == 1 || defined(ADAFRUIT_CIRCUITPLAYGROUND_M0))
    Adafruit_FlashTransport_SPI flashTransport(SS, &SPI);
  #else
    Adafruit_FlashTransport_SPI flashTransport(SS1, &SPI1);
  #endif
#endif

Adafruit_SPIFlash flash(&flashTransport);
// file system object from SdFat
FatFileSystem fatfs;

//create a seesaw with m0 DAP support
dap_m0p dap;

//create the target options
//TODO: set some actual options here
options_t g_target_options;

void setup() {
  pinMode(13, OUTPUT);
  Serial.begin(9600);
  while(!Serial);

  dap.begin(9,10,11);
  
  // Initialize flash library and check its chip ID.
  if (!flash.begin()) {
    Serial.println("Error, failed to initialize flash chip!");
    while(1);
  }
  Serial.print("Flash chip JEDEC ID: 0x"); Serial.println(flash.getJEDECID(), HEX);

  // First call begin to mount the filesystem.  Check that it returns true
  // to make sure the filesystem was mounted.
  if (!fatfs.begin(&flash)) {
    Serial.println("Failed to mount filesystem!");
    Serial.println("Was CircuitPython loaded on the board first to create the filesystem?");
    while(1);
  }
  Serial.println("Mounted filesystem!");

  File dataFile = fatfs.open(FILENAME, FILE_READ);
  uint8_t buf[256];

  if(dataFile){
    dap.dap_disconnect();
    dap.dap_get_debugger_info();
    dap.dap_connect();
    dap.dap_transfer_configure(0, 128, 128);
    dap.dap_swd_configure(0);
    dap.dap_reset_link();
    dap.dap_swj_clock(DAP_FREQ);
    dap.dap_target_prepare();
  
    dap.select(&g_target_options);
  
    Serial.print("Erasing... ");
    dap.erase();
    Serial.println(" done.");
  
    Serial.print("Programming... ");
    uint32_t addr = dap.program_start();

    while (dataFile.available()) {
      dataFile.read(buf, 256);
      addr = dap.program(addr, buf);
    }
    dataFile.close();
    
    Serial.println(" done.");
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening file");
    return;
  }
  dap.deselect();
  dap.dap_disconnect();
}

void loop() {
  //blink led on the host to show we're done
  digitalWrite(13, HIGH);
  delay(500); 
  digitalWrite(13, LOW);
  delay(500);  
}