#pragma once
#include "IAudioControl.h"
#include "ITunerApi.h"

namespace audio
{

class IAudioControlImpl : public IAudioControl
{
public:
   IAudioControlImpl(tuner::devicedriver::IDeviceDriver& deviceDriver);
   ~IAudioControlImpl();

   IAudioControlImpl(const IAudioControlImpl&) = delete;
   IAudioControlImpl& operator=(const IAudioControlImpl&) = delete;
   IAudioControlImpl(IAudioControlImpl&&) = delete;
   IAudioControlImpl& operator=(IAudioControlImpl&&) = delete;

private:
   void volumeUp() override;
   void volumeDown() override;
   void setVolume(uint8_t volume) override;
   uint8_t getVolume() const override;
   void switchSource(AudioSource) override;
   void setVolumeForSource(AudioSource source, uint8_t volume);

   std::unique_ptr<tuner::devicedriver::ITunerApi> m_tunerApi;
   mutable std::mutex m_mutex;
   AudioSource m_currentSource;
   uint8_t m_volume;


};

} // namespace audio
