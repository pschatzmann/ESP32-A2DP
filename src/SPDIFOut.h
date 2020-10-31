#ifndef _SPDIFOUT_H
#define _SPDIFOUT_H

//#include "AudioOutput.h"

#define SPDIF_OUT_PIN_DEFAULT  22
#define DMA_BUF_COUNT_DEFAULT  32 //8
#define DMA_BUF_SIZE_DEFAULT   256 //256


class SPDIFOut
{
  public:
    SPDIFOut(int dout_pin=SPDIF_OUT_PIN_DEFAULT, int port=0, int dma_buf_count = DMA_BUF_COUNT_DEFAULT);
    virtual ~SPDIFOut();
    bool SetPinout(int bclkPin, int wclkPin, int doutPin);
    virtual bool SetRate(int hz);
    virtual bool SetBitsPerSample(int bits);
    virtual bool SetChannels(int channels);
	virtual bool SetGain(float f) { if (f>4.0) f = 4.0; if (f<0.0) f=0.0; gainF2P6 = (uint8_t)(f*(1<<6)); return true; }
    virtual bool begin();
	typedef enum { LEFTCHANNEL=0, RIGHTCHANNEL=1 } SampleIndex;
    virtual bool ConsumeSample(int16_t sample[2]);
    virtual bool stop();

    bool SetOutputModeMono(bool mono);  // Force mono output no matter the input

    const uint32_t VUCP_PREAMBLE_B = 0xCCE80000; // 11001100 11101000
    const uint32_t VUCP_PREAMBLE_M = 0xCCE20000; // 11001100 11100010
    const uint32_t VUCP_PREAMBLE_W = 0xCCE40000; // 11001100 11100100

  protected:
    void MakeSampleStereo16(int16_t sample[2]) {
      // Mono to "stereo" conversion
      if (channels == 1)
        sample[RIGHTCHANNEL] = sample[LEFTCHANNEL];
      if (bps == 8) {
        // Upsample from unsigned 8 bits to signed 16 bits
        sample[LEFTCHANNEL] = (((int16_t)(sample[LEFTCHANNEL]&0xff)) - 128) << 8;
        sample[RIGHTCHANNEL] = (((int16_t)(sample[RIGHTCHANNEL]&0xff)) - 128) << 8;
      }
    };

    inline int16_t Amplify(int16_t s) {
      int32_t v = (s * gainF2P6)>>6;
      if (v < -32767) return -32767;
      else if (v > 32767) return 32767;
      else return (int16_t)(v&0xffff);
    }
	
  protected:
    virtual inline int AdjustI2SRate(int hz) { return rate_multiplier * hz; }
    uint8_t portNo;
    bool mono;
    bool i2sOn;
    uint8_t frame_num;
    uint8_t rate_multiplier;
	uint16_t hertz;
	uint8_t bps;
	uint8_t channels;
	uint8_t gainF2P6;
};

#endif