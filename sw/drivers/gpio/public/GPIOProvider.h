#pragma once
/**
 * @file GPIOProvider.h
 * @author Jacek Skowronek (jacekskowronekk@gmail.com)
 *
 * @brief
 *    Class providing GPIO instances based on SystemGPIO enumeration.
 *
 * @description
 *    Main purpose of this class is to provide GPIO instance basing on provided config (GPIOProviderConfig).
 *    In provides abstraction layer over different GPIO implementations, in this case ot covers
 *    SoC GPIO and the GPIO expander (I2C bus).
 *
 *    To obtain GPIO instance, provide mapping at construction time, then call create() method.
 *    IGPIO interface is returned, which can be used in the application.
*/

#include <memory>

#include "IGPIO.h"
#include "IMCP23017.h"

namespace drivers::gpio {

class GPIOProvider
{
public:
   GPIOProvider(const mcp23017::i2cConfig& config);
   ~GPIOProvider();

   std::unique_ptr<IGPIO> create(GPIO gpio);
   mcp23017::IMCP23017& getDriver();
private:
   const char* toString(GPIO gpio);
   std::unique_ptr<mcp23017::IMCP23017> m_i2cDriver;
};

}
