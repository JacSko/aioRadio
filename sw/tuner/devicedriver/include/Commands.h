#pragma once
#include <vector>

#include "TunerTypes.h"
#include "StatusChecker.hpp"
#include "Log.h"
#include "CharConverter.h"

namespace tuner::devicedriver
{

static const char* TAG = "CMDParser";

static inline utilities::charconverter::Charset toCommonCharset(CharsetID charsetId)
{
   switch(charsetId)
   {
      case CharsetID::EBU_LATIN:
         return utilities::charconverter::Charset::EBU_LATIN;
      case CharsetID::UCS_2:
         return utilities::charconverter::Charset::ISO_10646_UCS2;
      case CharsetID::UTF_8:
         return utilities::charconverter::Charset::ISO_10646_UTF_8;
      default:
         AIO_LOGE(TAG, "Unknown charset id: %.2X, defaulting to EBU_LATIN", static_cast<uint8_t>(charsetId));
         return utilities::charconverter::Charset::EBU_LATIN;
   }
}

static inline CharsetID toCharset(uint8_t byte)
{
   switch(byte)
   {
      case 0x00:
         return CharsetID::EBU_LATIN;
      case 0x06:
         return CharsetID::UCS_2;
      case 0x0F:
         return CharsetID::UTF_8;
      default:
         AIO_LOGE(TAG, "Unknown charset id: 0x%.2X, defaulting to EBU_LATIN", byte);
         return CharsetID::EBU_LATIN;
   }
}


class CommandBuffer
{
public:
   CommandBuffer(size_t txBufferSize, size_t rxBufferSize):
   m_txBuffer(txBufferSize, 0x00),
   m_rxBuffer(rxBufferSize, 0x00)
   {}

   std::vector<uint8_t> m_txBuffer;
   std::vector<uint8_t> m_rxBuffer;
};

struct SetProperty : public CommandBuffer
{
   SetProperty(PropertyId id, uint16_t data):
   CommandBuffer(6, 5)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::SET_PROPERTY);
      m_txBuffer[1] = 0x00;
      m_txBuffer[2] = static_cast<uint16_t>(id) & 0xFF;
      m_txBuffer[3] = (static_cast<uint16_t>(id) >> 8) & 0xFF;
      m_txBuffer[4] = data & 0xFF;
      m_txBuffer[5] = (data >> 8) & 0xFF;
   }

   bool process()
   {
      return StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
   }
};

struct GetProperty : public CommandBuffer
{
   GetProperty(PropertyId id):
   CommandBuffer(4, 7)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::GET_PROPERTY);
      m_txBuffer[1] = 0x01;
      m_txBuffer[2] = static_cast<uint16_t>(id) & 0xFF;
      m_txBuffer[3] = (static_cast<uint16_t>(id) >> 8) & 0xFF;
   }

   bool process()
   {
      bool result = StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
      if (result)
      {
         propertyValue = m_rxBuffer[5];
         propertyValue |= (m_rxBuffer[6] << 8);
      }
      AIO_LOGE_IF(!result, TAG, "Status error received, property value not decoded!");
      return result;
   }
   uint16_t propertyValue = {};
};

struct FMTuneFrequency : public CommandBuffer
{
   FMTuneFrequency(uint32_t frequencyKHz, bool dir_tune, uint8_t tunemode, uint8_t injection):
   CommandBuffer(7, 6)
   {
      uint16_t apiFrequency = frequencyKHz / 10;
      m_txBuffer[0] = static_cast<uint8_t>(Command::FM_TUNE_FREQ); // FM_TUNE_FREQ
      m_txBuffer[1] = (dir_tune << 5) | (tunemode << 2) | injection;
      m_txBuffer[2] = apiFrequency & 0xFF;
      m_txBuffer[3] = (apiFrequency >> 8) & 0xFF;
      m_txBuffer[4] = 0x00;
      m_txBuffer[5] = 0x00;
      m_txBuffer[6] = 0x00;
   }

   bool process()
   {
      return StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
   }
};

struct GetRSQStatus : public CommandBuffer
{
   GetRSQStatus(bool stcAck, bool cancel, bool atTune, bool rsqAck):
   CommandBuffer(2, 23)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::FM_RSQ_STATUS);
      m_txBuffer[1] = (rsqAck << 3) | (atTune << 2) | (cancel << 1) | stcAck;
   }

   bool process()
   {
      bool result = StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
      if (result)
      {
         status.rssiLInt = m_rxBuffer[5] & 0x01;
         status.rssiHInt = (m_rxBuffer[5] >> 1) & 0x01;
         status.snrLInt = (m_rxBuffer[5] >> 2) & 0x01;
         status.snrHInt = (m_rxBuffer[5] >> 3) & 0x01;
         status.valid = m_rxBuffer[6] & 0x01;
         status.rssi = static_cast<int8_t>(m_rxBuffer[10]);
         status.snr = m_rxBuffer[11];
      }
      return result;
   }
   RSQStatus status = {};
};

struct GetRDSStatus : public CommandBuffer
{
   GetRDSStatus(bool statusOnly, bool mtFifo, bool intAck):
   CommandBuffer(2, 17)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::FM_RDS_STATUS);
      m_txBuffer[1] = (statusOnly << 2) | (mtFifo << 1) | intAck;
   }

   bool process()
   {
      bool result = StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
      if (result)
      {
         status.rdsFifoInt = m_rxBuffer[5] & 0x01;
         status.rdsSyncInt = (m_rxBuffer[5] >> 1) & 0x01;
         status.rdsPiInt = (m_rxBuffer[5] >> 3) & 0x01;
         status.rdsTpPtyInt = (m_rxBuffer[5] >> 4) & 0x01;
         status.rdsFifoLost = m_rxBuffer[6] & 0x01;
         status.rdsSync = (m_rxBuffer[6] >> 1) & 0x01;
         status.piValid = (m_rxBuffer[6] >> 3) & 0x01;
         status.tpPtyValid = (m_rxBuffer[6] >> 4) & 0x01;
         status.pty = m_rxBuffer[7] & 0x1F;
         status.tp = (m_rxBuffer[7] >> 5) & 0x01;
         status.piCode = m_rxBuffer[9];
         status.piCode |= (m_rxBuffer[10] << 8);
         status.rdsFifoUsed = m_rxBuffer[11];
         status.blea = (m_rxBuffer[12] >> 6) & 0x03;
         status.bleb = (m_rxBuffer[12] >> 4) & 0x03;
         status.blec = (m_rxBuffer[12] >> 2) & 0x03;
         status.bled = m_rxBuffer[12] & 0x03;
         status.rdsBlockA = m_rxBuffer[13];
         status.rdsBlockA |= (m_rxBuffer[14] << 8);
         status.rdsBlockB = m_rxBuffer[15];
         status.rdsBlockB |= (m_rxBuffer[16] << 8);
      }
      return result;
   }
   RDSStatus status = {};
};

struct DABTuneFrequency : public CommandBuffer
{
   DABTuneFrequency(uint32_t frequencyIndex, uint8_t antcap, uint8_t injection):
   CommandBuffer(6, 6)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::DAB_TUNE_FREQ); // FM_TUNE_FREQ
      m_txBuffer[1] = injection;
      m_txBuffer[2] = frequencyIndex;
      m_txBuffer[3] = 0x00;
      m_txBuffer[4] = antcap & 0xFF;
      m_txBuffer[5] = (antcap >> 8) & 0xFF;
   }

   bool process()
   {
      return StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
   }
};

struct DABSetFrequencyList: public CommandBuffer
{
   DABSetFrequencyList(const std::vector<uint32_t>& frequencyList):
   CommandBuffer(4 + (frequencyList.size() * 4), 6)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::DAB_SET_FREQ_LIST); // DAB_SET_FREQ_LIST
      m_txBuffer[1] = static_cast<uint8_t>(frequencyList.size());
      m_txBuffer[2] = 0x00;
      m_txBuffer[3] = 0x00;
      size_t index = 4;
      for (const auto& frequencyKHz : frequencyList)
      {
         m_txBuffer[index++] = frequencyKHz & 0xFF;
         m_txBuffer[index++] = (frequencyKHz >> 8) & 0xFF;
         m_txBuffer[index++] = (frequencyKHz >> 16) & 0xFF;
         m_txBuffer[index++] = (frequencyKHz >> 24) & 0xFF;
      }
   }
   bool process()
   {
      return StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
   }
};

struct GetDABDigradStatus : public CommandBuffer
{
   GetDABDigradStatus(bool digradAck, bool atTune, bool fiberrAck, bool stcAck):
   CommandBuffer(2, 24)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::DAB_DIGRAD_STATUS);
      m_txBuffer[1] = (digradAck << 3) | (atTune << 2) | (fiberrAck << 1) | stcAck;
   }

   bool process()
   {
      bool result = StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
      if (result)
      {
         digradStatus.rssiLInt = m_rxBuffer[5] & 0x01;
         digradStatus.rssiHInt = (m_rxBuffer[5] >> 1) & 0x01;
         digradStatus.acqInt = (m_rxBuffer[5] >> 2) & 0x01;
         digradStatus.ficerrInt = (m_rxBuffer[5] >> 3) & 0x01;

         digradStatus.valid = m_rxBuffer[6] & 0x01;
         digradStatus.acq = (m_rxBuffer[6] >> 2) & 0x01;
         digradStatus.ficErr = (m_rxBuffer[6] >> 3) & 0x01;

         digradStatus.rssi = static_cast<int8_t>(m_rxBuffer[7]);
         digradStatus.snr = m_rxBuffer[8];
         digradStatus.ficQuality = m_rxBuffer[9];
         digradStatus.cnr = m_rxBuffer[10];

         digradStatus.fibErrorCount = m_rxBuffer[11];
         digradStatus.fibErrorCount |= (m_rxBuffer[12] << 8);

         digradStatus.frequency = m_rxBuffer[13];
         digradStatus.frequency |= (m_rxBuffer[14] << 8);
         digradStatus.frequency |= (m_rxBuffer[15] << 16);
         digradStatus.frequency |= (m_rxBuffer[16] << 24);

         digradStatus.frequencyIndex = m_rxBuffer[17];
         digradStatus.fftOffset = m_rxBuffer[18];

         digradStatus.antenaCapTuning = m_rxBuffer[19];
         digradStatus.antenaCapTuning |= (m_rxBuffer[20] << 8);

         digradStatus.CULevel = m_rxBuffer[21];
         digradStatus.CULevel |= (m_rxBuffer[22] << 8);

         digradStatus.fastDetect = m_rxBuffer[23];
      }
      AIO_LOGE_IF(!result, TAG, "Status error received, DigradStatus not decoded!");
      return result;
   }
   DigradStatus digradStatus = {};
};

struct GetEnsembleInfo : public CommandBuffer
{
   GetEnsembleInfo():
   CommandBuffer(2, 27)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::DAB_GET_ENSEMBLE_INFO);
      m_txBuffer[1] = 0x00;
   }

   bool process()
   {
      bool result = StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
      if (result)
      {
         ensembleInfo.eid = m_rxBuffer[5];
         ensembleInfo.eid |= (m_rxBuffer[6] << 8);
         ensembleInfo.ecc = m_rxBuffer[23];
         ensembleInfo.charset = toCharset(m_rxBuffer[24]);
         utilities::charconverter::RawString buffer = {};
         buffer.charset = toCommonCharset(ensembleInfo.charset);
         for (size_t i = 0; i < 16; ++i)
         {
            buffer.data.push_back(m_rxBuffer[7 + i]);
         }
         ensembleInfo.label = utilities::charconverter::CharConverter::toUtf8(utilities::charconverter::CharConverter::toUtf16(buffer));
      }
      AIO_LOGE_IF(!result, TAG, "Status error received, EnsembleInfo not decoded!");
      return result;
   }
   EnsembleInfo ensembleInfo = {};
};

struct GetComponentInfo : public CommandBuffer
{
   GetComponentInfo(uint32_t serviceId, uint32_t componentId):
   CommandBuffer(12, 56)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::DAB_GET_COMPONENT_INFO);
      m_txBuffer[1] = 0x00;
      m_txBuffer[2] = 0x00;
      m_txBuffer[3] = 0x00;
      m_txBuffer[4] = serviceId & 0xFF;
      m_txBuffer[5] = (serviceId >> 8) & 0xFF;
      m_txBuffer[6] = (serviceId >> 16) & 0xFF;
      m_txBuffer[7] = (serviceId >> 24) & 0xFF;
      m_txBuffer[8] = componentId & 0xFF;
      m_txBuffer[9] = (componentId >> 8) & 0xFF;
      m_txBuffer[10] = (componentId >> 16) & 0xFF;
      m_txBuffer[11] = (componentId >> 24) & 0xFF;
   }

   bool process()
   {
      bool result = StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
      if (result)
      {
         componentInfo.language = m_rxBuffer[7] & 0x3F;
         componentInfo.charset = toCharset(m_rxBuffer[8] & 0x3F);
         utilities::charconverter::RawString buffer = {};
         buffer.charset = toCommonCharset(componentInfo.charset);
         for (size_t i = 0; i < 16; ++i)
         {
            buffer.data.push_back(m_rxBuffer[9 + i]);
         }
         componentInfo.label = utilities::charconverter::CharConverter::toUtf8(utilities::charconverter::CharConverter::toUtf16(buffer));
      }
      AIO_LOGE_IF(!result, TAG, "Status error received, componentInfo not decoded!");
      return result;
   }
   DABComponent componentInfo = {};
};

struct GetDABEventStatus : public CommandBuffer
{
   GetDABEventStatus(bool eventAck):
   CommandBuffer(2, 8)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::DAB_GET_EVENT_STATUS);
      m_txBuffer[1] = eventAck;
   }

   bool process()
   {
      bool result = StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
      if (result)
      {
         eventStatus.srvlistInt= m_rxBuffer[5] & 0x01;
         eventStatus.freqinfoInt= (m_rxBuffer[5] >> 1) & 0x01;
         eventStatus.servlinkInt= (m_rxBuffer[5] >> 2) & 0x01;
         eventStatus.oeservInt= (m_rxBuffer[5] >> 3) & 0x01;
         eventStatus.annoInt= (m_rxBuffer[5] >> 4) & 0x01;
         eventStatus.recfgwrnInt= (m_rxBuffer[5] >> 6) & 0x01;
         eventStatus.recfgInt= (m_rxBuffer[5] >> 7) & 0x01;

         eventStatus.srvlist= m_rxBuffer[6] & 0x01;
         eventStatus.freqInfo= (m_rxBuffer[6] >> 1) & 0x01;
         eventStatus.servLink= (m_rxBuffer[6] >> 2) & 0x01;
         eventStatus.oeserv= (m_rxBuffer[6] >> 3) & 0x01;
         eventStatus.anno= (m_rxBuffer[6] >> 4) & 0x01;

         eventStatus.svrlistVer = m_rxBuffer[7];
         eventStatus.svrlistVer |= (m_rxBuffer[8] << 8);
      }
      AIO_LOGE_IF(!result, TAG, "Status error received, EnsembleInfo not decoded!");
      return result;
   }
   DABEventStatus eventStatus = {};
};

struct GetDigitalServiceList : public CommandBuffer
{
   GetDigitalServiceList(uint8_t serType):
   CommandBuffer(2, 2054)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::GET_DIGITAL_SERVICE_LIST);
      m_txBuffer[1] = serType;
   }

   bool process()
   {
      bool result = StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
      if (result)
      {
         static const uint8_t SERVICE_PAYLOAD_SIZE = 24;
         static const uint8_t COMPONENT_PAYLOAD_SIZE = 4;
         static const uint8_t SERVICE_LABEL_LENGTH = 16;

         serviceList.version = m_rxBuffer[7];
         serviceList.version |= (m_rxBuffer[8] << 8);
         uint8_t numServices = m_rxBuffer[9];
         size_t offset = 13;
         for(int i = 0; i < numServices; i++)
         {
            DABService service = {};
            service.id = (m_rxBuffer[offset + 3] << 24);
            service.id |= (m_rxBuffer[offset + 2] << 16);
            service.id |= (m_rxBuffer[offset + 1] << 8);
            service.id |= (m_rxBuffer[offset]);
            service.isAudioService = !(m_rxBuffer[offset + 4] & 0x01);

            auto labelBegin = m_rxBuffer.begin() + offset + 8;
            utilities::charconverter::RawString buffer = {};
            buffer.charset = static_cast<utilities::charconverter::Charset>(m_rxBuffer[offset + 6] & 0x0F);
            buffer.data.assign(labelBegin, labelBegin + SERVICE_LABEL_LENGTH);
            service.serviceLabel = utilities::charconverter::CharConverter::toUtf8(utilities::charconverter::CharConverter::toUtf16(buffer));

            size_t numComponents = m_rxBuffer[offset + 5] & 0x0F;
            offset += SERVICE_PAYLOAD_SIZE;
            for (size_t j = 0; j < numComponents; j++)
            {
               DABComponent component = {};
               component.id |= (m_rxBuffer[offset + 1] << 8);
               component.id |= (m_rxBuffer[offset]);
               component.isPrimary = m_rxBuffer[offset + 2] & 0x02;
               service.components.push_back(component);
               offset += COMPONENT_PAYLOAD_SIZE;
            }
            serviceList.services.push_back(service);
         }
      }
      AIO_LOGE_IF(!result, TAG, "Status error received, GetDigitalServiceList not decoded!");
      return result;
   }
   DigitalServiceList serviceList = {};
};

struct StartDigitalService : public CommandBuffer
{
   StartDigitalService(uint8_t serType, uint32_t serviceId, uint32_t componentId):
   CommandBuffer(12, 6)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::START_DIGITAL_SERVICE);
      m_txBuffer[1] = serType;
      m_txBuffer[2] = 0x00;
      m_txBuffer[3] = 0x00;
      m_txBuffer[4] = serviceId & 0xFF;
      m_txBuffer[5] = (serviceId >> 8) & 0xFF;
      m_txBuffer[6] = (serviceId >> 16) & 0xFF;
      m_txBuffer[7] = (serviceId >> 24) & 0xFF;
      m_txBuffer[8] = componentId & 0xFF;
      m_txBuffer[9] = (componentId >> 8) & 0xFF;
      m_txBuffer[10] = (componentId >> 16) & 0xFF;
      m_txBuffer[11] = (componentId >> 24) & 0xFF;
   }

   bool process()
   {
      return StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
   }
};

struct StopDigitalService : public CommandBuffer
{
   StopDigitalService(uint8_t serType, uint32_t serviceId, uint32_t componentId):
   CommandBuffer(12, 6)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::STOP_DIGITAL_SERVICE);
      m_txBuffer[1] = serType;
      m_txBuffer[2] = 0x00;
      m_txBuffer[3] = 0x00;
      m_txBuffer[4] = serviceId & 0xFF;
      m_txBuffer[5] = (serviceId >> 8) & 0xFF;
      m_txBuffer[6] = (serviceId >> 16) & 0xFF;
      m_txBuffer[7] = (serviceId >> 24) & 0xFF;
      m_txBuffer[8] = componentId & 0xFF;
      m_txBuffer[9] = (componentId >> 8) & 0xFF;
      m_txBuffer[10] = (componentId >> 16) & 0xFF;
      m_txBuffer[11] = (componentId >> 24) & 0xFF;
   }

   bool process()
   {
      return StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
   }
};

struct GetDigitalServiceData : public CommandBuffer
{
   GetDigitalServiceData(bool ack, bool statusOnly):
   CommandBuffer(2, 4120)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::GET_DIGITAL_SERVICE_DATA);
      m_txBuffer[1] = (statusOnly << 4) | ack;
   }

   bool process()
   {
      bool result = StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
      if (result)
      {
         digitalServiceData.dsrvPcktInt = m_rxBuffer[5] & 0x01;
         digitalServiceData.dsrvOvflInt = (m_rxBuffer[5] >> 1) & 0x01;
         digitalServiceData.buffCount = m_rxBuffer[6];
         digitalServiceData.srvState = m_rxBuffer[7];
         digitalServiceData.dscty = m_rxBuffer[8] & 0x3F;
         digitalServiceData.dataSrc = (m_rxBuffer[8] >> 6) & 0x03;
         digitalServiceData.serviceId = m_rxBuffer[9];
         digitalServiceData.serviceId |= (m_rxBuffer[10] << 8);
         digitalServiceData.serviceId |= (m_rxBuffer[11] << 16);
         digitalServiceData.serviceId |= (m_rxBuffer[12] << 24);
         digitalServiceData.componentId = m_rxBuffer[13];
         digitalServiceData.componentId |= (m_rxBuffer[14] << 8);
         digitalServiceData.componentId |= (m_rxBuffer[15] << 16);
         digitalServiceData.componentId |= (m_rxBuffer[16] << 24);
         digitalServiceData.byteCount = m_rxBuffer[19];
         digitalServiceData.byteCount |= (m_rxBuffer[20] << 8);
         digitalServiceData.segNum = m_rxBuffer[21];
         digitalServiceData.segNum |= (m_rxBuffer[22] << 8);
         digitalServiceData.numSegs = m_rxBuffer[23];
         digitalServiceData.numSegs |= (m_rxBuffer[24] << 8);
         size_t offset = 25;
         digitalServiceData.payload.clear();
         for (size_t i = 0; i < digitalServiceData.byteCount; ++i)
         {
            digitalServiceData.payload.push_back(m_rxBuffer[offset + i]);
         }
      }
      AIO_LOGE_IF(!result, TAG, "Status error received, EnsembleInfo not decoded!");
      return result;
   }
   DigitalServiceData digitalServiceData = {};
};

struct DABGetAudioInfo : public CommandBuffer
{
   DABGetAudioInfo():
   CommandBuffer(2, 20)
   {
      m_txBuffer[0] = static_cast<uint8_t>(Command::DAB_GET_AUDIO_INFO);
      m_txBuffer[1] = 0x00;
   }

   bool process()
   {
      bool result = StatusChecker::check(m_rxBuffer.data()) == StatusChecker::Status::CTS_OK;
      if (result)
      {
         audioInfo.bitrate = m_rxBuffer[5];
         audioInfo.bitrate |= (m_rxBuffer[6] << 8);

         audioInfo.sampleRate = m_rxBuffer[7];
         audioInfo.sampleRate |= (m_rxBuffer[8] << 8);

         audioInfo.audioMode = m_rxBuffer[9] & 0x03;
         audioInfo.audioSBRFlag = (m_rxBuffer[9] >> 2) & 0x01;
         audioInfo.audioPSFlag = (m_rxBuffer[9] >> 3) & 0x01;
         audioInfo.audioDRCGain = m_rxBuffer[10];
      }
      AIO_LOGE_IF(!result, TAG, "Status error received, EnsembleInfo not decoded!");
      return result;
   }
   AudioInfo audioInfo = {};
};

}
