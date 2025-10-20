#pragma once

/**
 * @file IMCP23017.h
 * @brief MCP23017 ESP-IDF-based, interrupt-driven GPIO Expander driver
 *
 * @details
 * This driver is intended to work with the MCP23017 GPIO Expander via I2C bus.
 * It supports interrupts from the mcp23017 device, as well as regular communication without interrupts.
 *
 * mcp23017::i2cConfig is the entry point for module configuration, see mcp23017_types.h for details.
 *
 * Main features:
 * - Effective communication using interrupts-based system
 * - thread-safe methods
 * - Easy to use API
 * - Possibility to use existing I2C bus
 *
 * Limitations:
 * - Supports inperrupt ony in MIRROR mode. It means, that only one interrupt line can be used for both ports (A/B)
 * - Interrupt pin has to be pulled up, with active-low state
 *
 * @author Jacek Skowronek (jacekskowronekk@gmail.com)
 */

#include <cstdint>
#include <optional>
#include <memory>

#include "driver/i2c_master.h"

namespace drivers::gpio::mcp23017
{
   enum class PIN : uint16_t
   {
      GPIOA0,
      GPIOA1,
      GPIOA2,
      GPIOA3,
      GPIOA4,
      GPIOA5,
      GPIOA6,
      GPIOA7,
      GPIOB0,
      GPIOB1,
      GPIOB2,
      GPIOB3,
      GPIOB4,
      GPIOB5,
      GPIOB6,
      GPIOB7,
      GPIO_NUM,
   };

   enum class Register : uint8_t
   {
      IODIRA   = 0x00,
      IODIRB   = 0x01,
      IPOLA    = 0x02,
      IPOLB    = 0x03,
      GPINTENA = 0x04,
      GPINTENB = 0x05,
      DEFVALA  = 0x06,
      DEFVALB  = 0x07,
      INTCONA  = 0x08,
      INTCONB  = 0x09,
      IOCON    = 0x0A,
      GPPUA    = 0x0C,
      GPPUB    = 0x0D,
      INTFA    = 0x0E,
      INTFB    = 0x0F,
      INTCAPA  = 0x10,
      INTCAPB  = 0x11,
      GPIOA    = 0x12,
      GPIOB    = 0x13,
      OLATA    = 0x14,
      OLATB    = 0x15,
   };

   enum class Direction : uint8_t
   {
      INPUT = 1,
      OUTPUT = 0,
   };

   enum class Polarity : uint8_t
   {
      NORMAL = 0,
      INVERTED = 1,
   };

   enum class InterruptOnChange : uint8_t
   {
      DISABLED = 0,
      ENABLED = 1,
   };

   enum class InterruptCompare : uint8_t
   {
      COMPARE_PREVIOUS = 0,
      COMPARE_DEFAULT = 1,
   };

   enum class Pullup : uint8_t
   {
      DISABLED = 0,
      ENABLED = 1,
   };

   enum class InterruptFlag : uint8_t
   {
      NO_INTERRUPT = 0,
      INTERRUPT_OCCURRED = 1,
   };

   enum class InterruptEdge
   {
      RISING,
      FALLING,
      BOTH
   };

   enum class PullMode
   {
      NONE,
      PULL_UP,
      PULL_DOWN
   };

   typedef void (*Listener)(PIN pin, bool state, void* context);

   struct i2cConfig
   {
      uint8_t i2cAddress;
      uint32_t clockSpeedHz;
      uint16_t sclPin;
      uint16_t sdaPin;
      uint16_t resetPin;
      std::optional<uint16_t> interruptPin;
   };

class IMCP23017
{
public:
   virtual ~IMCP23017() = default;
   IMCP23017() = default;

   IMCP23017(IMCP23017&&) = delete;
   IMCP23017(const IMCP23017&) = delete;
   IMCP23017& operator=(IMCP23017&&) = delete;
   IMCP23017& operator=(const IMCP23017&) = delete;

   /**
    * @brief Probes the device on the I2C bus
    * @return true if device is present, false otherwise
    */
   virtual bool probe() = 0;

   /**
    * @brief Resets the device using the reset pin.
    * @details Default configuration is applied after reset.
    * @return None
    */
   virtual void reset() = 0;

   /**
    * @brief Sets the direction of the specified pin
    * @param[in] pin - Pin identifier
    * @param[in] direction - Direction to set (INPUT/OUTPUT)
    * @return true if successful, false otherwise
    */
   virtual bool setDirection(PIN, Direction) = 0;

   /**
    * @brief Sets the polarity of the specified pin
    * @param[in] pin - Pin identifier
    * @param[in] polarity - Polarity to set (NORMAL/INVERTED)
    * @return true if successful, false otherwise
    */
   virtual bool setPolarity(PIN, Polarity) = 0;

   /**
    * @brief Enables or disables pull-up on the specified pin
    * @param[in] pin - Pin identifier
    * @param[in] pullup - Pull-up configuration (ENABLED/DISABLED)
    * @return true if successful, false otherwise
    */
   virtual bool setPullup(PIN, Pullup) = 0;

   /**
    * @brief Sets interrupt comparison logic for the specified pin
    * @param[in] pin - Pin identifier
    * @param[in] type - Interrupt comparison type (COMPARE_PREVIOUS/COMPARE_DEFAULT)
    * @return true if successful, false otherwise
    */
   virtual bool setInterruptCompare(PIN, InterruptCompare) = 0;

   /**
    * @brief Writes a digital value to the specified pin
    * @param[in] pin - Pin identifier
    * @param[in] state - Digital state to write (true/false)
    * @return true if successful, false otherwise
    */
   virtual bool writePin(PIN, bool) = 0;

   /**
    * @brief Reads the digital value from the specified pin
    * @param[in] pin - Pin identifier
    * @return Optional boolean with pin state if successful, std::nullopt otherwise
    */
   virtual std::optional<bool> readPin(PIN) = 0;

   /**
    * @brief Sets a listener callback for the specified pin
    * @detail Only one listener can be set per pin. Setting a new listener overrides the previous one.
    * @param[in] id - Pin identifier
    * @param[in] listener - Callback function to be called on pin state change
    * @param[in] context - User-defined context pointer passed to the listener
    * @return None
    */
   virtual void setListener(PIN id, Listener listener, void* context) = 0;

   /**
    * @brief Removes the listener callback for the specified pin
    * @param[in] id - Pin identifier
    * @return None
    */
   virtual void removeListener(PIN id) = 0;

   /**
    * @brief Method creating and managing its own I2C bus
    * @param[in] bus - I2C bus number
    * @param[in] config - I2C device configuration
    * @return None
    */
   static std::unique_ptr<IMCP23017> create(int bus, const i2cConfig& config);
   /**
    * @brief Method using existing I2C bus
    * @param[in] handle - I2C bus handle
    * @param[in] config - I2C device configuration
    * @return None
    */
   static std::unique_ptr<IMCP23017> create(i2c_master_bus_handle_t* handle, const i2cConfig& config);
};

}
