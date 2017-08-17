#include "PluginProcessor.h"
#include "PluginEditor.h"

DropoutspyAudioProcessor::DropoutspyAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
  DoReset();
}

DropoutspyAudioProcessor::~DropoutspyAudioProcessor() {
}

const String DropoutspyAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool DropoutspyAudioProcessor::acceptsMidi() const {
 #if JucePlugin_WantsMidiInput
  return true;
 #else
  return false;
 #endif
}

bool DropoutspyAudioProcessor::producesMidi() const {
 #if JucePlugin_ProducesMidiOutput
  return true;
 #else
  return false;
 #endif
}

double DropoutspyAudioProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int DropoutspyAudioProcessor::getNumPrograms() {
  return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int DropoutspyAudioProcessor::getCurrentProgram() {
  return 0;
}

void DropoutspyAudioProcessor::setCurrentProgram (int index) {
}

const String DropoutspyAudioProcessor::getProgramName (int index) {
  return {};
}

void DropoutspyAudioProcessor::changeProgramName (int index, const String& newName) {
}

void DropoutspyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  sample_rate_ = sampleRate;
  samples_per_block_ = samplesPerBlock;
  DoReset();
}

void DropoutspyAudioProcessor::ResetTrackingState(double sample_rate, int samples_per_block) {
  start_ticks_min_ = 0;
  start_ticks_max_ = 0;
  sample_rate_ = sample_rate;
  samples_per_block_ = samples_per_block;
  total_samples_ = 0;
  spread_ = 0;
  last_delta_ = 0;
  ticks_per_block_ = Time::secondsToHighResolutionTicks(samples_per_block_ / sample_rate_);
}

void DropoutspyAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

void DropoutspyAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) {
  int64 now = Time::getHighResolutionTicks();

  ScopedLock l(lock_);

  // To keep precision within reason if the plugin runs continuously for a long time, we
  // periodically reset the start time & samples.
  const float RESET_START_TIME_INTERVAL = 3600.0f;
  if (total_samples_ == 0 ||
      Time::highResolutionTicksToSeconds(now - start_ticks_min_) > RESET_START_TIME_INTERVAL) {
    // TODO: can we determine ticks at true start of playback?
    ResetTrackingState(sample_rate_, samples_per_block_);
    start_ticks_min_ = now;
    start_ticks_max_ = now;
  }

  double seconds0 = total_samples_ / sample_rate_;
  int64 computed_min = now - Time::secondsToHighResolutionTicks(seconds0);
  total_samples_ += buffer.getNumSamples();

  if (computed_min < start_ticks_min_) {
    start_ticks_min_ = computed_min;
  }
  if (computed_min > start_ticks_max_) {
    start_ticks_max_ = computed_min;
  }

  // Compute spread values.
  spread_ = (start_ticks_max_ - start_ticks_min_) / float(ticks_per_block_);
  last_delta_ = (computed_min - start_ticks_min_) / float(ticks_per_block_);

  // Update histogram.
  int bucket = int(last_delta_ * (METER_BUCKETS - 1));
  if (bucket < 0) bucket = 0;
  if (bucket >= METER_BUCKETS) bucket = METER_BUCKETS - 1;
  delta_histo_[bucket]++;
  if (delta_histo_[bucket] > 10000) {
    // Prune histogram so the buckets don't ever overflow.
    for (int i = 0; i < METER_BUCKETS; i++) {
      if (delta_histo_[i] > 1) delta_histo_[i] = (delta_histo_[i] * 7) >> 3;
    }
  }

  bool overflow_happened = false;
  bool warning_happened = false;
  if (spread_ > 1.0f) {
    // Overflow or underflow!
    overflow_happened = true;
    overflow_count_++;
    last_overflow_ticks_ = now;
    ResetTrackingState(sample_rate_, samples_per_block_);
  } else if (bucket >= METER_BUCKETS / 2) {
    // Warning.
    warning_happened = true;
    warning_count_++;
  }

  const int totalNumInputChannels  = getTotalNumInputChannels();
  const int totalNumOutputChannels = getTotalNumOutputChannels();

  // In case we have more outputs than inputs, this code clears any output
  // channels that didn't contain input data, (because these aren't
  // guaranteed to be empty - they may contain garbage).
  // This is here to avoid people getting screaming feedback
  // when they first compile a plugin, but obviously you don't need to keep
  // this code if your algorithm always overwrites all the output channels.
  for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
    buffer.clear (i, 0, buffer.getNumSamples());
  }

  // Process audio. Leave it untouched, unless a warning or dropout happened
  // and we are configured to emit a click on dropout.
  for (int channel = 0; channel < totalNumInputChannels; ++channel) {
    float* channelData = buffer.getWritePointer(channel);
    if (emit_click_on_dropout_ && overflow_happened) {
      // Emit a positive click.
      channelData[0] = 1.0f;
    } else if (emit_click_on_warning_ && warning_happened) {
      // Emit a negative click.
      channelData[0] = -1.0f;
    }
  }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DropoutspyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
  ignoreUnused (layouts);
  return true;
#else
  // This is the place where you check if the layout is supported.
  // In this template code we only support mono or stereo.
  if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
      && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
    return false;

  // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;
#endif

  return true;
#endif
}
#endif

bool DropoutspyAudioProcessor::hasEditor() const {
  return true;
}

AudioProcessorEditor* DropoutspyAudioProcessor::createEditor() {
  return new DropoutspyAudioProcessorEditor(*this);
}

// Store parameters.
void DropoutspyAudioProcessor::getStateInformation(MemoryBlock& destData) {
  char data = emit_click_on_warning_;
  destData.append(&data, 1);
  data = emit_click_on_dropout_;
  destData.append(&data, 1);
}

// Restore parameters which were stored via getStateInformation.
void DropoutspyAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
  if (sizeInBytes > 1) {
    emit_click_on_warning_ = ((const bool*) data)[0];
    emit_click_on_dropout_ = ((const bool*) data)[1];
  }
}

// This creates new instances of the plugin.
AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new DropoutspyAudioProcessor();
}
