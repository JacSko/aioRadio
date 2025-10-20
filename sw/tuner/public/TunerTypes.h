#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace tuner
{

const uint32_t FREQUENCY_ANY = 0xFFFFFFFF;
const uint32_t SID_ANY = 0xFFFFFFFF;
const uint32_t EID_ANY = 0xFFFFFFFF;
const uint32_t CMPID_ANY = 0xFFFFFFFF;

enum class TunerType
{
   FM,
   DAB,
   ONLINE,
};

enum class ActionType
{
   TUNE_SELECT,
   SCAN,
};

enum class EventType : uint32_t
{
   CTS = 0x80000000,
   ERRCMD = 0x40000000,
   DACQINT = 0x20000000,
   DSRVINT = 0x10000000,
   RSQINT = 0x08000000,
   RDSINT = 0x04000000,
   ACFINT = 0x02000000,
   STCINT = 0x01000000,
   DEVNTINT = 0x00200000,
   DACFINT = 0x00010000,
   PUP_STATE = 0x000000C0,
   RFFE_ERR = 0x00000020,
   DSP_ERR = 0x00000010,
   REPOFERR = 0x00000008,
   CMDOFERR = 0x00000004,
   ARBERR = 0x00000002,
   ERRNR = 0x00000001,
};

enum INT_CTL_ENABLE : uint16_t
{
   INT_CTL_ENABLE_STCIEN =     (1 << 0),
   INT_CTL_ENABLE_ACFIEN =     (1 << 1),
   INT_CTL_ENABLE_RDSIEN =     (1 << 2),
   INT_CTL_ENABLE_RSQIEN =     (1 << 3),
   INT_CTL_ENABLE_SRVIEN =     (1 << 4),
   INT_CTL_ENABLE_ACQIEN =     (1 << 5),
   INT_CTL_ENABLE_ERR_CMDIEN = (1 << 6),
   INT_CTL_ENABLE_CTSIEN =     (1 << 7),
   INT_CTL_ENABLE_EVNTIEN =    (1 << 13),
   INT_CTL_ENABLE_DEVNTIEN =   (1 << 13),
};

enum PIN_CONFIG_ENABLE : uint16_t
{
   PIN_CONFIG_ENABLE_DACOUTEN = (1 << 0),
   PIN_CONFIG_ENABLE_I2SOUTEN = (1 << 1),
   PIN_CONFIG_ENABLE_INTBOUTEN = (1 << 15),
};

enum class Command : uint8_t
{
   // FIRMWARE LOADING
   POWER_UP = 0x01,
   HOST_LOAD = 0x04,
   LOAD_INIT = 0x06,
   BOOT = 0x07,

   // COMMON COMMANDS
   SET_PROPERTY = 0x13,
   GET_PROPERTY = 0x14,

   // AMFM COMMANDS
   FM_TUNE_FREQ = 0x30,
   FM_RSQ_STATUS = 0x32,
   FM_RDS_STATUS = 0x34,
   // DAB COMMANDS
   DAB_TUNE_FREQ = 0xB0,
   GET_DIGITAL_SERVICE_LIST = 0x80,
   START_DIGITAL_SERVICE = 0x81,
   STOP_DIGITAL_SERVICE = 0x82,
   GET_DIGITAL_SERVICE_DATA = 0x84,
   DAB_DIGRAD_STATUS = 0xB2,
   DAB_GET_EVENT_STATUS = 0xB3,
   DAB_GET_ENSEMBLE_INFO = 0xB4,
   DAB_GET_COMPONENT_INFO = 0xBB,
   DAB_SET_FREQ_LIST = 0xB8,
   DAB_GET_AUDIO_INFO = 0xBD,
};

enum class PropertyId : uint16_t
{
   /* Common properties */
   AUDIO_ANALOG_VOLUME = 0x0300,
   PIN_CONFIG_ENABLE = 0x0800,
   INT_CTL_ENABLE = 0x0000,
   /* FM properties */
   FM_TUNE_FE_VARM = 0x1710,
   FM_TUNE_FE_VARB = 0x1711,
   FM_RDS_INTERRUPT_SOURCE = 0x3C00,
   FM_RDS_INTERRUPT_FIFO_COUNT = 0x3C01,
   FM_RDS_CONFIG = 0x3C02,
   FM_RDS_CONFIDENCE = 0x3C03,
   /* DAB properties */
   DAB_TUNE_FE_VARM = 0x1710,
   DAB_TUNE_FE_VARB = 0x1711,
   DAB_TUNE_FE_CFG = 0x1712,
   DAB_EVENT_INTERRUPT_SOURCE = 0xB300,
};

enum class ServiceType : uint8_t
{
   AUDIO_SERVICE = 0x00,
   DATA_SERVICE = 0x01,
};

enum class CharsetID : uint8_t
{
   EBU_LATIN = 0x00,
   UCS_2 = 0x06,
   UTF_8 = 0x0F
};

struct RSQStatus
{
   bool rssiLInt;
   bool rssiHInt;
   bool snrLInt;
   bool snrHInt;
   bool valid;
   int8_t rssi;
   uint8_t snr;
};

struct RDSStatus
{
   bool rdsFifoInt;
   bool rdsSyncInt;
   bool rdsPiInt;
   bool rdsTpPtyInt;
   bool rdsFifoLost;
   bool rdsSync;
   bool piValid;
   bool tpPtyValid;
   uint8_t pty;
   bool tp;
   uint16_t piCode;
   uint8_t rdsFifoUsed;
   uint8_t blea;
   uint8_t bleb;
   uint8_t blec;
   uint8_t bled;
   uint16_t rdsBlockA;
   uint16_t rdsBlockB;
};

struct EnsembleInfo
{
   uint16_t eid;
   std::string label;
   uint8_t ecc;
   CharsetID charset;
};

struct DigradStatus
{
   bool rssiLInt;
   bool rssiHInt;
   bool acqInt;
   bool ficerrInt;
   bool valid;
   bool acq;
   bool ficErr;
   int8_t rssi;
   uint8_t snr;
   uint8_t ficQuality;
   uint8_t cnr;
   uint16_t fibErrorCount;
   uint32_t frequency;
   uint8_t frequencyIndex;
   uint8_t fftOffset;
   uint16_t antenaCapTuning;
   uint16_t CULevel;
   uint8_t fastDetect;
};

struct DABEventStatus
{
   bool recfgInt;
   bool recfgwrnInt;
   bool annoInt;
   bool oeservInt;
   bool servlinkInt;
   bool freqinfoInt;
   bool srvlistInt;
   bool anno;
   bool oeserv;
   bool servLink;
   bool freqInfo;
   bool srvlist;
   uint16_t svrlistVer;
};

struct DABComponent
{
   bool isPrimary;
   uint32_t id;
   uint8_t language;
   CharsetID charset;
   std::string label;
};

struct DABService
{
   bool isAudioService;
   uint32_t id;
   std::vector<DABComponent> components;
   std::string serviceLabel;
};

struct DigitalServiceList
{
   uint16_t version;
   std::vector<DABService> services;
};

struct DigitalServiceData
{
   bool dsrvPcktInt;
   bool dsrvOvflInt;
   uint8_t buffCount;
   uint8_t srvState;
   uint8_t dscty;
   uint8_t dataSrc;
   uint32_t serviceId;
   uint32_t componentId;
   uint16_t byteCount;
   uint16_t segNum;
   uint16_t numSegs;
   std::vector<uint8_t> payload;
};

struct AudioInfo
{
   uint16_t bitrate;
   uint16_t sampleRate;
   uint8_t audioMode;
   bool audioSBRFlag;
   bool audioPSFlag;
   uint8_t audioDRCGain;
};

}
