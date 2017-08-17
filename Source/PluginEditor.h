/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

class Repainter;

class DropoutspyAudioProcessorEditor : public AudioProcessorEditor,
  private Button::Listener {
public:
  DropoutspyAudioProcessorEditor (DropoutspyAudioProcessor&);
  ~DropoutspyAudioProcessorEditor();

  void paint (Graphics&) override;
  void resized() override;

private:
  void buttonClicked(Button*) override;
  void buttonStateChanged(Button* b) override;

  void DrawHistoBar(Graphics& g, int i, int first_empty, int bar_count, int total_count, int max_bar);

  // This reference is provided as a quick way for your editor to
  // access the processor object that created it.
  DropoutspyAudioProcessor& processor;

  ToggleButton emit_button_dropout_;
  ToggleButton emit_button_warning_;
  TextButton reset_button_;
  HyperlinkButton tulrich_button_;

  Repainter* repainter_ = nullptr;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DropoutspyAudioProcessorEditor)
};
