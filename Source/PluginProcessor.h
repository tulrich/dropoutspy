#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

class DropoutspyAudioProcessor  : public AudioProcessor {
public:
  DropoutspyAudioProcessor();
  ~DropoutspyAudioProcessor();

  void prepareToPlay (double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

 #ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
 #endif

  void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

  AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram (int index) override;
  const String getProgramName (int index) override;
  void changeProgramName (int index, const String& newName) override;

  void getStateInformation (MemoryBlock& destData) override;
  void setStateInformation (const void* data, int sizeInBytes) override;

  float getSpread() const {
    return spread_;
  }

  float getLastDelta() const {
    return last_delta_;
  }

  int64 getOverflowCount() const {
    return overflow_count_;
  }

  int64 getWarningCount() const {
    return warning_count_;
  }

  void setEmitClickOnDropout(bool set) {
    emit_click_on_dropout_ = set;
  }

  void setEmitClickOnWarning(bool set) {
    emit_click_on_warning_ = set;
  }

  int getSamplesPerBlock() const {
    return samples_per_block_;
  }

  const void getHisto(int histo[16]) const {
    for (int i = 0; i < 16; i++) histo[i] = delta_histo_[i];
  }

  void DoReset() {
    ScopedLock l(lock_);
    ResetTrackingState(sample_rate_, samples_per_block_);
    warning_count_ = 0;
    overflow_count_ = 0;
    last_overflow_ticks_ = 0;
    for (int i = 0; i < 16; i++) {
      delta_histo_[i] = 0;
    }
  }

private:
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DropoutspyAudioProcessor)

  void ResetTrackingState(double sample_rate, int samples_per_block);

  CriticalSection lock_;

  int64 start_ticks_min_;
  int64 start_ticks_max_;
  double sample_rate_ = 0;
  int samples_per_block_ = 0;
  int64 ticks_per_block_ = 1;
  int64 total_samples_ = 0;
  int64 warning_count_ = 0;
  int64 overflow_count_ = 0;
  int64 last_overflow_ticks_ = 0;
  float spread_ = 0;
  float last_delta_ = 0;
  int delta_histo_[16];

  bool emit_click_on_warning_ = false;
  bool emit_click_on_dropout_ = false;
};
