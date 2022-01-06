#ifndef __ADF4351_H__
#define __ADF4351_H__

#include <Arduino.h>
#include <stdint.h>
#include <Wire.h>
#include <SPI.h>
#include <BigNumber.h>

/*!
   @brief Stores a device register value

   This class is used to store and manage a single ADF4351 register value
   and provide bit field manipulations.
*/
class Reg {
  public:
    /*!
       Constructor
    */
    Reg();
    /*!
       get the current register value
       returns same value as getbf(0,32)
       @return unsigned long value of the register
    */
    uint32_t get();
    /*!
       sets the register value
       @param value unsigned long
    */
    void set(uint32_t value);
    /*!
       current register value

       returns same value as getbf(0,32)
    */
    uint32_t whole;
    /*!
       modifies the register value based on a value and bitfield (mask)
       @param start index of the bit to start the modification
       @param len length number of bits to modify
       @param value value to modify (value is truncated if larger than len bits)
    */
    void setbf(uint8_t start, uint8_t len , uint32_t value);
    /*!
       gets the current register bitfield value, based on the start and length mask
       @param start index of the bit of where to start
       @param len length number of bits to get
       @return bitfield value
    */
    uint32_t getbf(uint8_t start, uint8_t len);

};

class ADF4351{
	public:
    ADF4351(int SCLK, int DATA, int LE, int CE);
    void begin(void);

    void setReferenceFrequency(uint32_t reffreq);

    void WriteRegister(uint32_t regData);

    int setf(uint32_t freq) ; // set freq

    int setrf(uint32_t f) ;  // set reference freq

    uint32_t getReg(int n) ;
    
    uint32_t gcd_iter(uint32_t u, uint32_t v) ;
    
    Reg R[6] ;

    SPISettings spi_settings;

    SPIClass SPI1;
    
    uint32_t reffreq;
    
    uint32_t cfreq ;
    
    uint16_t N_Int ;
    
    int Frac ;
    
    uint32_t Mod ;
    
    float PFDFreq ;
    
    uint32_t ChanStep;
   
    int outdiv ;
    
    uint8_t RD2refdouble ;
    
    int RCounter ;
    
    uint8_t RD1Rdiv2 ;
    
    uint8_t BandSelClock ;
    
    int ClkDiv ;
    
    uint8_t Prescaler ;
    
    byte pwrlevel ;

  private:
    int _data, _sclk, _le, _ce;
    long _regData;
};

#endif
