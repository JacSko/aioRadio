#include "FirmwareLoader.h"
#include "StatusChecker.hpp"
#include "TableConverter.hpp"
#include "Log.h"

#include "rom_patch.h"
#include "fmhd_firmware.h"
#include "dab_firmware.h"


namespace tuner::devicedriver
{

static const std::vector<std::pair<TunerType, FirmwareImages>> firmwareTable = {
    {TunerType::FM, {{rom00_patch_016_bin, rom00_patch_016_bin_len},
                     {fmhd_radio_3_0_19_bif, fmhd_radio_3_0_19_bif_len}}},
    {TunerType::DAB, {{rom00_patch_016_bin, rom00_patch_016_bin_len},
                      {dab_radio_3_2_7_bif, dab_radio_3_2_7_bif_len}}}
};

static const char* TAG = "FirmwareLoader";

FirmwareLoader::FirmwareLoader(drivers::spi::SPIDriver& driver):
m_driver(driver)
{

}
FirmwareLoader::~FirmwareLoader()
{

}
bool FirmwareLoader::boot(TunerType type)
{
   AIO_LOGI(TAG, "Booting Si4684 tuner...");
   bool result = false;

   std::optional<FirmwareImages> firmwareFiles = utilities::TableConverter::convert(firmwareTable, type);
   if (!firmwareFiles.has_value())
   {
      AIO_LOGE(TAG, "Cannot find firmware for tuner type %u", static_cast<int>(type));
      return false;
   }

   do
   {
      m_driver.reset();
      if (!powerUp())
      {
         AIO_LOGE(TAG, "Failed to power up device");
         break;
      }
      vTaskDelay(pdMS_TO_TICKS(1));
      AIO_LOGI(TAG, "Sending load init before patch");
      if (!loadInit())
      {
         AIO_LOGE(TAG, "Failed to load init");
         break;
      }
      AIO_LOGI(TAG, "Sending patch");
      if (!sendImage(firmwareFiles->patch.data, firmwareFiles->patch.size))
      {
         AIO_LOGE(TAG, "Failed to send bootloader patch");
         break;
      }
      vTaskDelay(pdMS_TO_TICKS(4));
      AIO_LOGI(TAG, "Sending load init before firmware");
      if (!loadInit())
      {
         AIO_LOGE(TAG, "Failed to load init (2nd time)");
         break;
      }
      AIO_LOGI(TAG, "Sending firmware");
      if (!sendImage(firmwareFiles->firmware.data, firmwareFiles->firmware.size))
      {
         AIO_LOGE(TAG, "Failed to send firmware");
         break;
      }
      if (!bootDevice())
      {
         AIO_LOGE(TAG, "Failed to boot device");
         break;
      }
      AIO_LOGI(TAG, "Si4684 tuner booted successfully");
      result = true;
   } while(0);
   return result;
}

bool FirmwareLoader::powerUp()
{
   m_buffer[0] = static_cast<uint8_t>(Command::POWER_UP);
   m_buffer[1] = 0x00;
   m_buffer[2] = 0x17;
   m_buffer[3] = 0x48;
   m_buffer[4] = 0x00;
   m_buffer[5] = 0xf8;
   m_buffer[6] = 0x24;
   m_buffer[7] = 0x01;
   m_buffer[8] = 0x1F;
   m_buffer[9] = 0x10;
   m_buffer[10] = 0x00;
   m_buffer[11] = 0x00;
   m_buffer[12] = 0x00;
   m_buffer[13] = 0x18;
   m_buffer[14] = 0x00;
   m_buffer[15] = 0x00;
   return m_driver.write(m_buffer.data(), 16) && waitCTS();
}
bool FirmwareLoader::loadInit()
{
   m_buffer[0] = static_cast<uint8_t>(Command::LOAD_INIT);
   m_buffer[1] = 0x00;
   return m_driver.write(m_buffer.data(), 2) && waitCTS();
}
bool FirmwareLoader::sendImage(const uint8_t* image, size_t bytesToSend)
{
   uint32_t bytesSent = 0;
   while (bytesSent < bytesToSend)
   {
      m_buffer[0] = static_cast<uint8_t>(Command::HOST_LOAD);
      m_buffer[1] = 0x00;
      m_buffer[2] = 0x00;
      m_buffer[3] = 0x00;
      uint16_t chunkSize = std::min<uint32_t>(bytesToSend - bytesSent, 4092);
      for (uint16_t i = 0; i < chunkSize; i++)
      {
         m_buffer[4 + i] = image[bytesSent + i];
      }
      if (!m_driver.write(m_buffer.data(), 4 + chunkSize) || !waitCTS())
      {
         AIO_LOGE(TAG, "Failed to send image chunk");
         return false;
      }
      bytesSent += chunkSize;
   }
   AIO_LOGI(TAG, "send image success");
   return true;
}
bool FirmwareLoader::bootDevice()
{
   m_buffer[0] = static_cast<uint8_t>(Command::BOOT);
   m_buffer[1] = 0x00;
   bool result = m_driver.write(m_buffer.data(), 2) && waitCTS();
   if (result)
   {
      bool deviceBooted = (m_buffer[4] & static_cast<uint8_t>(EventType::PUP_STATE)) == 0xC0;
      AIO_LOGI(TAG, "Device boot %s", deviceBooted ? "successful" : "failed");
      return deviceBooted;
   }
   return false;
}
bool FirmwareLoader::waitCTS()
{
   int waitTimeoutMs = 1000;
   do
   {
      bool readResult = m_driver.read(m_buffer.data(), 6);
      if (!readResult)
      {
         AIO_LOGE(TAG, "Failed to read CTS");
         return false;
      }
      StatusChecker::Status status = StatusChecker::check(m_buffer.data());
      if (status == StatusChecker::Status::CTS_OK)
      {
         return true;
      }
      else if (status == StatusChecker::Status::ERROR)
      {
         AIO_LOGE(TAG, "Error during firmware loading! [%.2x %.2x %.2x %.2x %.2x %.2x]",
                        m_buffer[0], m_buffer[1], m_buffer[2],
                        m_buffer[3], m_buffer[4], m_buffer[5]);
         return false;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
      waitTimeoutMs -= 10;
   } while (waitTimeoutMs > 0);
   AIO_LOGE(TAG, "Timeout waiting for CTS");
   return false;
}


}


