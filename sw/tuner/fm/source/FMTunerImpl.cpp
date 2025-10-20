#include "Log.h"
#include "FMTunerImpl.h"

namespace tuner::fm
{

static const char* TAG = "FMTuner";

std::unique_ptr<IFMTuner> IFMTuner::create(devicedriver::IDeviceDriver& deviceDriver)
{
   return std::make_unique<FMTunerImpl>(deviceDriver);
}

const char* IFMTuner::getEventName(FMTunerEvent event)
{
   switch(event)
   {
      case FMTunerEvent::SCAN_STARTED:
         return "SCAN_STARTED";
      case FMTunerEvent::SCAN_STEP:
         return "SCAN_STEP";
      case FMTunerEvent::SCAN_COMPLETED:
         return "SCAN_COMPLETED";
      case FMTunerEvent::AUDIO_PLAYBACK_STARTED:
         return "AUDIO_PLAYBACK_STARTED";
      case FMTunerEvent::AUDIO_PLAYBACK_STOPPED:
         return "AUDIO_PLAYBACK_STOPPED";
      case FMTunerEvent::PROCESSING_REQUIRED:
         return "PROCESSING_REQUIRED";
      default:
         return "UNKNOWN_EVENT";
   }
}

FMTunerImpl::FMTunerImpl(devicedriver::IDeviceDriver& deviceDriver) :
m_deviceDriver(deviceDriver),
m_tunerApi(devicedriver::ITunerApi::create(deviceDriver)),
m_task(nullptr),
m_frequencyKHz(0),
m_scanProgress(0)
{
   AIO_LOGI(TAG, "Creating FMTuner");
}
FMTunerImpl::~FMTunerImpl()
{
}
bool FMTunerImpl::tuneFrequency(uint32_t frequencyKHz)
{
   if (frequencyKHz == m_frequencyKHz)
   {
      AIO_LOGD(TAG, "Already tuned to %u kHz", frequencyKHz);
      return true;
   }

   bool result = m_tunerApi->tuneFrequencyFM(frequencyKHz);
   if (!result)
   {
      AIO_LOGE(TAG, "Tune to %u kHz failed", frequencyKHz);
      return false;
   }
   m_frequencyKHz = frequencyKHz;

   RSQStatus rsqStatus = {};
   if (!m_tunerApi->getRSQStatus(rsqStatus))
   {
      AIO_LOGE(TAG, "Get RSQ status failed after tuning to %u kHz", frequencyKHz);
      return false;
   }
   m_rsqStatus = rsqStatus;
   AIO_LOGI(TAG, "Tuned to %u kHz: RSSI=%d SNR=%d", frequencyKHz,
                                                    rsqStatus.rssi,
                                                    rsqStatus.snr);
   return true;
}
bool FMTunerImpl::initialize()
{
   AIO_LOGI(TAG, "Initializing FMTuner...");
   m_deviceDriver.setMode(TunerType::FM);
   m_tunerApi->setProperty(PropertyId::PIN_CONFIG_ENABLE, PIN_CONFIG_ENABLE_DACOUTEN |
                                                          PIN_CONFIG_ENABLE_INTBOUTEN);
   m_tunerApi->setProperty(PropertyId::INT_CTL_ENABLE, INT_CTL_ENABLE_RDSIEN |
                                                       INT_CTL_ENABLE_CTSIEN |
                                                       INT_CTL_ENABLE_STCIEN |
                                                       INT_CTL_ENABLE_ERR_CMDIEN);
   m_tunerApi->setProperty(tuner::PropertyId::FM_TUNE_FE_VARB,0x01E0);
   m_tunerApi->setProperty(tuner::PropertyId::FM_TUNE_FE_VARM,0xF7A0);
   m_tunerApi->setProperty(tuner::PropertyId::FM_RDS_INTERRUPT_SOURCE, 0x001F);
   m_tunerApi->setProperty(tuner::PropertyId::FM_RDS_INTERRUPT_FIFO_COUNT, 0x0010);
   m_tunerApi->setProperty(tuner::PropertyId::FM_RDS_CONFIG, 0x00A1);

   // TODO: set FM_VALID_SNR_THRESHOLD, FM_VALID_RSSI_THRESHOLD,
   // FM_VALID_MAX_TUNE_ERROR, FM_VALID_HDLEVEL_THRESHOLD
   m_deviceDriver.setEventHandler(EventType::RDSINT, this);
   return true;
}
void FMTunerImpl::finalize()
{
   //TODO: retore properties
   m_tunerApi->setProperty(PropertyId::INT_CTL_ENABLE, 0x0000);
   m_deviceDriver.removeEventHandler(EventType::RDSINT);
}

void FMTunerImpl::process()
{
   std::lock_guard lock(m_eventMutex);
   while (!m_eventQueue.empty())
   {
      EventType type = m_eventQueue.front();
      m_eventQueue.pop();

      AIO_LOGE(TAG, "Processing event %.4x", static_cast<uint32_t>(type));
      if (type == EventType::RDSINT)
      {
         RDSStatus rdsStatus = {};
         if (m_tunerApi->getRDSStatus(rdsStatus))
         {
            AIO_LOGI(TAG, "RDS Status: PI=0x%.4X TP=%d PTY=%d SYNC=%d FIFO_USED=%u BLEA=%u BLEB=%u BLEC=%u BLED=%u BLOCKA=%.2x BLOCKB=%.2x",
                     rdsStatus.piCode,
                     rdsStatus.tp,
                     rdsStatus.pty,
                     rdsStatus.rdsSync,
                     rdsStatus.rdsFifoUsed,
                     rdsStatus.blea,
                     rdsStatus.bleb,
                     rdsStatus.blec,
                     rdsStatus.bled,
                     rdsStatus.rdsBlockA,
                     rdsStatus.rdsBlockB);
            m_rdsStatus = rdsStatus;
         }
         else
         {
            AIO_LOGE(TAG, "Get RDS status failed!");
         }
      }
   }
}

void FMTunerImpl::onEvent(EventType type)
{
   std::lock_guard lock(m_eventMutex);
   m_eventQueue.push(type);
   notifyEvent(FMTunerEvent::PROCESSING_REQUIRED);
}
void FMTunerImpl::scan()
{
   static constexpr uint32_t startFrequencyKHz = 87500;
   static constexpr uint32_t endFrequencyKHz = 108000;
   static constexpr uint32_t stepFrequencyKHz = 100;
   static constexpr uint32_t totalSteps = (endFrequencyKHz - startFrequencyKHz) / stepFrequencyKHz + 1;

   m_scanProgress = 0;
   AIO_LOGE(TAG, "Starting band scan");
   notifyEvent(FMTunerEvent::SCAN_STARTED);

   for (uint32_t freq = startFrequencyKHz; freq <= endFrequencyKHz; freq += stepFrequencyKHz)
   {
      tuneFrequency(freq);
      m_scanProgress = static_cast<uint8_t>(((freq - startFrequencyKHz) / stepFrequencyKHz) * 100 / totalSteps);
      notifyEvent(FMTunerEvent::SCAN_STEP);
   }
   notifyEvent(FMTunerEvent::SCAN_COMPLETED);
   m_scanProgress = 0;
}
int FMTunerImpl::getScanProgress()
{
   return m_scanProgress;
}
void FMTunerImpl::addEventListener(FMTunerEventListener* listener)
{
   std::lock_guard lock(m_eventListenersMutex);
   m_eventListeners.push_back(listener);
}
void FMTunerImpl::removeEventListener(FMTunerEventListener* listener)
{
   std::lock_guard lock(m_eventListenersMutex);
   auto it = std::remove(m_eventListeners.begin(), m_eventListeners.end(), listener);
   m_eventListeners.erase(it, m_eventListeners.end());
}
void FMTunerImpl::notifyEvent(FMTunerEvent event)
{
   std::lock_guard lock(m_eventListenersMutex);
   for (auto& listener : m_eventListeners)
   {
      listener->onEvent(event);
   }
}
CurrentStationInfo FMTunerImpl::getCurrentStationInfo()
{
   CurrentStationInfo result = {};
   result.frequencyKHz = m_frequencyKHz;
   result.snr = m_rsqStatus.snr;
   result.rssi = m_rsqStatus.rssi;
   result.valid = m_rsqStatus.valid;
   result.piCode = m_rdsStatus.piCode;
   return result;
}

} // namespace tuner
