// ======================================================================
// \title  LinuxI2cDriverComponentImpl.cpp
// \author tcanham
// \brief  cpp file for LinuxI2cDriver component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include "Fw/Types/Assert.hpp"
#include <FpConfig.hpp>
#include <Drv/LinuxI2cDriver/LinuxI2cDriver.hpp>
#include <Fw/Logger/Logger.hpp>

#include <unistd.h> // required for I2C device access
#include <fcntl.h>  // required for I2C device configuration
#include <sys/ioctl.h> // required for I2C device usage
#include <linux/i2c.h> // required for struct / constant definitions
#include <linux/i2c-dev.h> // required for constant definitions
#include <cerrno>

#define DEBUG_PRINT 0

namespace Drv {

  // ----------------------------------------------------------------------
  // Construction, initialization, and destruction
  // ----------------------------------------------------------------------

  LinuxI2cDriver ::
    LinuxI2cDriver(
        const char *const compName
    ) : LinuxI2cDriverComponentBase(compName),
        m_fd(-1)
  {

  }

  void LinuxI2cDriver ::
    init(
        const NATIVE_INT_TYPE instance
    )
  {
    LinuxI2cDriverComponentBase::init(instance);
  }

  LinuxI2cDriver::
    ~LinuxI2cDriver()
  {
    if (-1 != this->m_fd) { // check if file is open
      ::close(this->m_fd);
    }
  }

  bool LinuxI2cDriver::open(const char* device) {
      FW_ASSERT(device);
      this->m_fd = ::open(device, O_RDWR);
      return (-1 != this->m_fd);
  }


  // ----------------------------------------------------------------------
  // Handler implementations for user-defined typed input ports
  // ----------------------------------------------------------------------

  // Note this port handler is guarded, so we can make the ioctl call

  Drv::I2cStatus LinuxI2cDriver ::
    write_handler(
        const NATIVE_INT_TYPE portNum,
        U32 addr,
        Fw::Buffer &serBuffer
    )
  {
      // Make sure file has been opened
      if (-1 == this->m_fd) {
          return I2cStatus::I2C_OPEN_ERR;
      }

#if DEBUG_PRINT
      Fw::Logger::log("I2c addr: 0x%02X\n",addr);
      for (U32 byte = 0; byte < serBuffer.getSize(); byte++) {
    	  Fw::Logger::log("0x%02X ",serBuffer.getData()[byte]);

      }
      Fw::Logger::log("\n");
#endif
      // select slave address
      int stat = ioctl(this->m_fd, I2C_SLAVE, addr);
      if (stat == -1) {
#if DEBUG_PRINT
          Fw::Logger::log("Status: %d Errno: %d\n", stat, errno);
#endif
	  return I2cStatus::I2C_ADDRESS_ERR;
      }
      // make sure it isn't a null pointer
      FW_ASSERT(serBuffer.getData());
      // write data
      stat = static_cast<int>(write(this->m_fd, serBuffer.getData(), serBuffer.getSize()));
      if (stat == -1) {
#if DEBUG_PRINT
          Fw::Logger::log("Status: %d Errno: %d\n", stat, errno);
#endif
	  return I2cStatus::I2C_WRITE_ERR;
      }
      return I2cStatus::I2C_OK;
  }

  Drv::I2cStatus LinuxI2cDriver ::
    read_handler(
        const NATIVE_INT_TYPE portNum,
        U32 addr,
        Fw::Buffer &serBuffer
    )
  {
      // Make sure file has been opened
      if (-1 == this->m_fd) {
          return I2cStatus::I2C_OPEN_ERR;
      }

#if DEBUG_PRINT
      Fw::Logger::log("I2c addr: 0x%02X\n",addr);
#endif
      // select slave address
      int stat = ioctl(this->m_fd, I2C_SLAVE, addr);
      if (stat == -1) {
#if DEBUG_PRINT
          Fw::Logger::log("Status: %d Errno: %d\n", stat, errno);
#endif
	  return I2cStatus::I2C_ADDRESS_ERR;
      }
      // make sure it isn't a null pointer
      FW_ASSERT(serBuffer.getData());
      // read data
      stat = static_cast<int>(read(this->m_fd, serBuffer.getData(), serBuffer.getSize()));
      if (stat == -1) {
#if DEBUG_PRINT
          Fw::Logger::log("Status: %d Errno: %d\n", stat, errno);
#endif
	  return I2cStatus::I2C_READ_ERR;
      }
#if DEBUG_PRINT
      for (U32 byte = 0; byte < serBuffer.getSize(); byte++) {
    	  Fw::Logger::log("0x%02X ",serBuffer.getData()[byte]);

      }
      Fw::Logger::log("\n");
#endif
      return I2cStatus::I2C_OK;
  }

  Drv::I2cStatus LinuxI2cDriver ::
    writeRead_handler(
      const NATIVE_INT_TYPE portNum, /*!< The port number*/
      U32 addr,
      Fw::Buffer &writeBuffer,
      Fw::Buffer &readBuffer
  ){

    // Make sure file has been opened
    if (-1 == this->m_fd) {
        return I2cStatus::I2C_OPEN_ERR;
    }
    FW_ASSERT(-1 != this->m_fd);

    // make sure they are not null pointers
    FW_ASSERT(writeBuffer.getData());
    FW_ASSERT(readBuffer.getData());

    #if DEBUG_PRINT
      Fw::Logger::log("I2c addr: 0x%02X\n",addr);
    #endif

    struct i2c_msg rdwr_msgs[2];

    // Start address
    rdwr_msgs[0].addr = static_cast<U16>(addr);
    rdwr_msgs[0].flags = 0; // write
    rdwr_msgs[0].len = static_cast<U16>(writeBuffer.getSize());
    rdwr_msgs[0].buf = writeBuffer.getData();

    // Read buffer
    rdwr_msgs[1].addr = static_cast<U16>(addr);
    rdwr_msgs[1].flags = I2C_M_RD; // read
    rdwr_msgs[1].len = static_cast<U16>(readBuffer.getSize());
    rdwr_msgs[1].buf = readBuffer.getData();

    struct i2c_rdwr_ioctl_data rdwr_data;
    rdwr_data.msgs = rdwr_msgs;
    rdwr_data.nmsgs = 2;

    //Use ioctl to perform the combined write/read transaction
    NATIVE_INT_TYPE stat = ioctl(this->m_fd, I2C_RDWR, &rdwr_data);

    if(stat == -1){
      #if DEBUG_PRINT
        Fw::Logger::log("Status: %d Errno: %d\n", stat, errno);
      #endif
      //Because we're using ioctl to perform the transaction we dont know exactly the type of error that occurred
      return I2cStatus::I2C_OTHER_ERR;
    }

#if DEBUG_PRINT
    Fw::Logger::log("Wrote:\n");
    for (U32 byte = 0; byte < writeBuffer.getSize(); byte++) {
    	  Fw::Logger::log("0x%02X ",writeBuffer.getData()[byte]);

    }
    Fw::Logger::log("\n");
    Fw::Logger::log("Read:\n");
    for (U32 byte = 0; byte < readBuffer.getSize(); byte++) {
    	  Fw::Logger::log("0x%02X ",readBuffer.getData()[byte]);

    }
    Fw::Logger::log("\n");
#endif

    return I2cStatus::I2C_OK;
  }

} // end namespace Drv
