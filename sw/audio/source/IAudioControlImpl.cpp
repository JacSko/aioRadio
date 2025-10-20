#include "IAudioControlImpl.h"
#include "Log.h"


namespace audio
{

static const char* TAG = "AC";
static const int MAX_VOLUME = 100;
static const int DEFAULT_VOLUME = 50;

std::unique_ptr<IAudioControl> IAudioControl::create(tuner::devicedriver::IDeviceDriver& deviceDriver)
{
   return std::make_unique<IAudioControlImpl>(deviceDriver);
}

IAudioControlImpl::IAudioControlImpl(tuner::devicedriver::IDeviceDriver& deviceDriver):
m_tunerApi(tuner::devicedriver::ITunerApi::create(deviceDriver))
{
}
IAudioControlImpl::~IAudioControlImpl()
{

}
void IAudioControlImpl::volumeUp()
{
   std::lock_guard<std::mutex> lock(m_mutex);
   setVolumeForSource(m_currentSource, std::min<uint8_t>(MAX_VOLUME, m_volume + 1));
}
void IAudioControlImpl::volumeDown()
{
   std::lock_guard<std::mutex> lock(m_mutex);
   setVolumeForSource(m_currentSource, m_volume == 0? 0 : m_volume - 1);
}
void IAudioControlImpl::setVolume(uint8_t volume)
{
   std::lock_guard<std::mutex> lock(m_mutex);
   setVolumeForSource(m_currentSource, volume);
}
void IAudioControlImpl::setVolumeForSource(AudioSource source, uint8_t volume)
{
   if (volume > MAX_VOLUME)
   {
      AIO_LOGE(TAG, "Volume %d exceeds max volume %d", volume, MAX_VOLUME);
      return;
   }

   switch (source)
   {
      case AudioSource::FM_RADIO:
         {
            static const uint16_t TUNER_MAX_VOLUME = 63;
            uint16_t tunerVolume = static_cast<uint16_t>((volume * TUNER_MAX_VOLUME / MAX_VOLUME));
            AIO_LOGI(TAG, "Setting volume of FM_RADIO to %u (%u/%u)", volume, tunerVolume, TUNER_MAX_VOLUME);
            m_tunerApi->setProperty(tuner::PropertyId::AUDIO_ANALOG_VOLUME, tunerVolume);
            break;
         }
      case AudioSource::DAB_RADIO:
         {
            static const uint16_t TUNER_MAX_VOLUME = 63;
            uint16_t tunerVolume = static_cast<uint16_t>((volume * TUNER_MAX_VOLUME / MAX_VOLUME));
            AIO_LOGI(TAG, "Setting volume of DAB_RADIO to %u (%u/%u)", volume, tunerVolume, TUNER_MAX_VOLUME);
            m_tunerApi->setProperty(tuner::PropertyId::AUDIO_ANALOG_VOLUME, tunerVolume);
            break;
         }
      case AudioSource::ONLINE_RADIO:
         break;
      default:
         AIO_LOGE(TAG, "Unknown audio source");
         return;
   }
   m_volume = volume;
}
uint8_t IAudioControlImpl::getVolume() const
{
   std::lock_guard<std::mutex> lock(m_mutex);
   return m_volume;
}
void IAudioControlImpl::switchSource(AudioSource source)
{
   std::lock_guard<std::mutex> lock(m_mutex);
   if (m_currentSource == source)
   {
      return;
   }
   setVolumeForSource(source, DEFAULT_VOLUME);
   m_currentSource = source;
}


}
