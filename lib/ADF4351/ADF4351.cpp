#include "ADF4351.h"

#define ADF_FREQ_MAX  4294967295UL    ///< Maximum Generated Frequency = value of MAX Unsigned Long
#define ADF_FREQ_MIN  34385000UL      ///< Minimum Generated Frequency
#define ADF_PFD_MAX   32000000.0      ///< Maximum Frequency for Phase Detector
#define ADF_PFD_MIN   125000.0        ///< Minimum Frequency for Phase Detector
#define ADF_REFIN_MAX   250000000UL   ///< Maximum Reference Frequency
#define REF_FREQ_DEFAULT 250000000UL  ///< Default Reference Frequency

uint32_t steps[] = { 1000, 5000, 10000, 50000, 100000 , 500000, 1000000 }; ///< Array of Allowed Step Values (Hz)


/*!
   single register constructor
*/
Reg::Reg()
{
  whole = 0 ;
}

uint32_t Reg::get()
{
  return whole ;
}

void Reg::set(uint32_t value)
{
  whole = value  ;
}

void Reg::setbf(uint8_t start, uint8_t len, uint32_t value)
{
  uint32_t bitmask =  ((1UL  << len) - 1UL) ;
  value &= bitmask  ;
  bitmask <<= start ;
  whole = ( whole & ( ~bitmask)) | ( value << start ) ;
}

uint32_t  Reg::getbf(uint8_t start, uint8_t len)
{
  uint32_t bitmask =  ((1UL  << len) - 1UL) << start ;
  uint32_t result = ( whole & bitmask) >> start  ;
  return ( result ) ;
}

// Constructor function; initializes communication pinouts
ADF4351::ADF4351(int SCLK, int DATA, int LE, int CE){
	_sclk = SCLK;
	_data=DATA;
	_le=LE;
	_ce=CE;

  SPIClass SPI1(HSPI);
  spi_settings = SPISettings(1000000UL, MSBFIRST, SPI_MODE0) ;

	reffreq = REF_FREQ_DEFAULT ;
  	cfreq = 0 ;
  	ChanStep = steps[0] ;
  	RD2refdouble = 0 ;
  	RCounter = 25 ;
  	RD1Rdiv2 = 0 ;
  	BandSelClock = 80 ;
  	ClkDiv = 150 ;
  	Prescaler = 0 ;
  	pwrlevel = 0 ;
}

void ADF4351::begin(void){
	pinMode(_sclk, OUTPUT);
	pinMode(_data, OUTPUT);
	pinMode(_le, OUTPUT);
	pinMode(_ce, OUTPUT); 

  SPI1.begin();
}

int  ADF4351::setf(uint32_t freq)
{
  //  calculate settings from freq
  if ( freq > ADF_FREQ_MAX ) return 1 ;

  if ( freq < ADF_FREQ_MIN ) return 1 ;

  int localosc_ratio =   2200000000UL / freq ;
  outdiv = 1 ;
  int RfDivSel = 0 ;

  // select the output divider
  while (  outdiv <=  localosc_ratio   && outdiv <= 64 ) {
    outdiv *= 2 ;
    RfDivSel++  ;
  }

  if ( freq > 3600000000UL/outdiv )
    Prescaler = 1 ;
  else
    Prescaler = 0 ;

  PFDFreq = (float) reffreq  * ( (float) ( 1.0 + RD2refdouble) / (float) (RCounter * (1.0 + RD1Rdiv2)));  // find the loop freq
  BigNumber::begin(10) ;
  char tmpstr[20] ;
  // kludge - BigNumber doesn't like leading spaces
  // so you need to make sure the string passed doesnt
  // have leading spaces.
  int cntdigits = 0 ;
  uint32_t num = (uint32_t) ( PFDFreq / 10000 ) ;

  while ( num != 0 )  {
    cntdigits++ ;
    num /= 10 ;
  }

  dtostrf(PFDFreq, cntdigits + 8 , 3, tmpstr) ;
  // end of kludge
  BigNumber BN_PFDFreq = BigNumber(tmpstr) ;
  BigNumber BN_N = ( BigNumber(freq) * BigNumber(outdiv) ) / BN_PFDFreq ;
  N_Int =  (uint16_t) ( (uint32_t)  BN_N ) ;
  BigNumber BN_Mod = BN_PFDFreq / BigNumber(ChanStep) ;
  Mod = BN_Mod ;
  BN_Mod = BigNumber(Mod) ;
  BigNumber BN_Frac = ((BN_N - BigNumber(N_Int)) * BN_Mod)  + BigNumber("0.5")  ;
  Frac = (int) ( (uint32_t) BN_Frac);
  BN_N = BigNumber(N_Int) ;

  if ( Frac != 0  ) {
    uint32_t gcd = gcd_iter(Frac, Mod) ;

    if ( gcd > 1 ) {
      Frac /= gcd ;
      BN_Frac = BigNumber(Frac) ;
      Mod /= gcd ;
      BN_Mod = BigNumber(Mod) ;
    }
  }

  BigNumber BN_cfreq ;

  if ( Frac == 0 ) {
    BN_cfreq = ( BN_PFDFreq  * BN_N) / BigNumber(outdiv) ;

  } else {
    BN_cfreq = ( BN_PFDFreq * ( BN_N + ( BN_Frac /  BN_Mod) ) ) / BigNumber(outdiv) ;
  }

  cfreq = BN_cfreq ;

  if ( cfreq != freq ) Serial.println(F("output freq diff than requested")) ;

  BigNumber::finish() ;

  if ( Mod < 2 || Mod > 4095) {
    Serial.println(F("Mod out of range")) ;
    return 1 ;
  }

  if ( (uint32_t) Frac > (Mod - 1) ) {
    Serial.println(F("Frac out of range")) ;
    return 1 ;
  }

  if ( Prescaler == 0 && ( N_Int < 23  || N_Int > 65535)) {
    Serial.println(F("N_Int out of range")) ;
    return 1;

  } else if ( Prescaler == 1 && ( N_Int < 75 || N_Int > 65535 )) {
    Serial.println(F("N_Int out of range")) ;
    return 1;
  }

  // setting the registers to default values
  // R0
  R[0].set(0UL) ;
  // (0,3,0) control bits
  R[0].setbf(3, 12, Frac) ; // fractonal
  R[0].setbf(15, 16, N_Int) ; // N integer
  // R1
  R[1].set(0UL) ;
  R[1].setbf(0, 3, 1) ; // control bits
  R[1].setbf(3, 12, Mod) ; // Mod
  R[1].setbf(15, 12, 1); // phase
  R[1].setbf(27, 1, Prescaler); //  prescaler
  // (28,1,0) phase adjust
  // R2
  R[2].set(0UL) ;
  R[2].setbf(0, 3, 2) ; // control bits
  // (3,1,0) counter reset
  // (4,1,0) cp3 state
  // (5,1,0) power down
  R[2].setbf(6, 1, 1) ; // pd polarity

  if ( Frac == 0 )  {
    R[2].setbf(7, 1, 1) ; // LDP, int-n mode
    R[2].setbf(8, 1, 1) ; // ldf, int-n mode

  } else {
    R[2].setbf(7, 1, 0) ; // LDP, frac-n mode
    R[2].setbf(8, 1, 0) ; // ldf ,frac-n mode
  }

  R[2].setbf(9, 4, 7) ; // charge pump
  // (13,1,0) dbl buf
  R[2].setbf(14, 10, RCounter) ; //  r counter
  R[2].setbf(24, 1, RD1Rdiv2)  ; // RD1_RDiv2
  R[2].setbf(25, 1, RD2refdouble)  ; // RD2refdouble
  // R[2].setbf(26,3,0) ; //  muxout, not used
  // (29,2,0) low noise and spurs mode
  // R3
  R[3].set(0UL) ;
  R[3].setbf(0, 3, 3) ; // control bits
  R[3].setbf(3, 12, ClkDiv) ; // clock divider

  // (15,2,0) clk div mode
  // (17,1,0) reserved
  // (18,1,0) CSR
  // (19,2,0) reserved
  if ( Frac == 0 )  {
    R[3].setbf(21, 1, 1); //  charge cancel, reduces pfd spurs
    R[3].setbf(22, 1, 1); //  ABP, int-n

  } else  {
    R[3].setbf(21, 1, 0) ; //  charge cancel
    R[3].setbf(22, 1, 0); //  ABP, frac-n
  }

  R[3].setbf(23, 1, 1) ; // Band Select Clock Mode
  // (24,8,0) reserved
  // R4
  R[4].set(0UL) ;
  R[4].setbf(0, 3, 4) ; // control bits
  R[4].setbf(3, 2, pwrlevel) ; // output power 0-3 (-4dbM to 5dbM, 3db steps)
  R[4].setbf(5, 1, 1) ; // rf output enable
  // (6,2,0) aux output power
  // (8,1,0) aux output enable
  // (9,1,0) aux output select
  // (10,1,0) mtld
  // (11,1,0) vco power down
  R[4].setbf(12, 8, BandSelClock) ; // band select clock divider
  R[4].setbf(20, 3, RfDivSel) ; // rf divider select
  R[4].setbf(23, 1, 1) ; // feedback select
  // (24,8,0) reserved
  // R5
  R[5].set(0UL) ;
  R[5].setbf(0, 3, 5) ; // control bits
  // (3,16,0) reserved
  R[5].setbf(19, 2, 3) ; // Reserved field,set to 11
  // (21,1,0) reserved
  R[5].setbf(22, 2, 1) ; // LD Pin Mode
  // (24,8,0) reserved
  int i ;

  SPI1.beginTransaction(spi_settings);
  for (i = 5 ; i > -1 ; i--) {
    WriteRegister(getReg(i)) ;
    //delayMicroseconds(2500) ;
  }
  SPI1.endTransaction();

  return 0 ;  // ok
}

int ADF4351::setrf(uint32_t f)
{
  if ( f > ADF_REFIN_MAX ) return 1 ;

  if ( f < 100000UL ) return 1 ;

  float newfreq  =  (float) f  * ( (float) ( 1.0 + RD2refdouble) / (float) (RCounter * (1.0 + RD1Rdiv2)));  // check the loop freq

  if ( newfreq > ADF_PFD_MAX ) return 1 ;

  if ( newfreq < ADF_PFD_MIN ) return 1 ;

  reffreq = f ;
  return 0 ;
}


// write data into register
void ADF4351::WriteRegister(uint32_t regData){
	/*digitalWrite(_le, LOW);
	_regData = regData;

	for(int i=0; i<32; i++)
	{
		if(((_regData<<i)&0x80000000)==0x80000000)
		{
			digitalWrite(_data,1);
			
		}
		else
		{
			digitalWrite(_data,0) ;
			
		}

		digitalWrite(_sclk, HIGH);
		delay(1);
		digitalWrite(_sclk, LOW);
	}
	
	// load data into register
	digitalWrite(_le, HIGH);
	delay(1);
	digitalWrite(_le, LOW);*/

  byte  txbyte ;
  digitalWrite(_le, LOW) ;

  for (int i = 3 ; i > -1 ; i--) {
    txbyte = (byte) (regData >> (i * 8)) ;
    SPI1.transfer(txbyte) ;
  }

  digitalWrite(_le, HIGH) ;
  delayMicroseconds(5) ;
  digitalWrite(_le, LOW) ;

}

uint32_t   ADF4351::getReg(int n)
{
  return R[n].whole ;
}

uint32_t ADF4351::gcd_iter(uint32_t u, uint32_t v)
{
  uint32_t t;

  while (v) {
    t = u ;
    u = v ;
    v = t % v ;
  }

  return u ;
}
