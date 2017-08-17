#include "PluginProcessor.h"
#include "PluginEditor.h"

const int meter_left = 10;
const int meter_width = 256 + 1;
const int meter_right = meter_left + meter_width;
const int meter_top = 20;
const int meter_height = 15;

const int plugin_width = meter_width + 50;
const int plugin_height = 102;

const int text_top = meter_top + meter_height + 5;
const int text_left0 = meter_left;
const int text_left1 = meter_left + meter_width / 4;
const int text_left2 = text_left1 + (meter_right - text_left1) / 2;
const int text_width0 = text_left1 - text_left0;
const int text_width1 = text_left2 - text_left1;
const int text_width2 = meter_right - text_left2;
const int text_height = 15;

class Repainter {
public:
  Repainter(DropoutspyAudioProcessorEditor* ed) : editor_(ed) {
  }
  void Cancel() {
    editor_ = nullptr;
  }
  void Repaint() {
    if (editor_) editor_->repaint();
    else delete this;
  }
private:
  DropoutspyAudioProcessorEditor* editor_;
};

DropoutspyAudioProcessorEditor::DropoutspyAudioProcessorEditor (DropoutspyAudioProcessor& p) : AudioProcessorEditor(&p), processor(p) {
  emit_button_dropout_.setButtonText("Click on dropout");
  emit_button_dropout_.addListener(this);
  addAndMakeVisible(&emit_button_dropout_);

  emit_button_warning_.setButtonText("Click on warning");
  emit_button_warning_.addListener(this);
  addAndMakeVisible(&emit_button_warning_);

  reset_button_.setButtonText("Reset");
  reset_button_.addListener(this);
  addAndMakeVisible(&reset_button_);

  tulrich_button_.setButtonText("tulrich.com");
  tulrich_button_.setURL(URL("http://tulrich.com/recording/dropoutspy/"));
  addAndMakeVisible(&tulrich_button_);

  setSize(plugin_width, plugin_height);

  repainter_ = new Repainter(this);
}

DropoutspyAudioProcessorEditor::~DropoutspyAudioProcessorEditor() {
  repainter_->Cancel();
}

void DropoutspyAudioProcessorEditor::resized() {
  emit_button_warning_.setBounds(text_left1, text_top + text_height + 5, text_width1, text_height);
  emit_button_dropout_.setBounds(text_left2, text_top + text_height + 5, text_width2, text_height);
  reset_button_.setBounds(text_left0, text_top, 35, text_height);
  //Font font(text_height * 0.6f);
  //int width = font.getStringWidth(tulrich_button_.getButtonText()) + 5;
  tulrich_button_.setBounds(5, plugin_height - text_height - 5,
                           100 /* width */, text_height);
  tulrich_button_.changeWidthToFitText();
}

void DropoutspyAudioProcessorEditor::buttonClicked(Button* b) {
 if (b == &reset_button_) {
    processor.DoReset();
  }
}

void DropoutspyAudioProcessorEditor::buttonStateChanged(Button* b) {
  if (b == &emit_button_dropout_) {
    processor.setEmitClickOnDropout(b->getToggleState());
  }
  if (b == &emit_button_warning_) {
    processor.setEmitClickOnWarning(b->getToggleState());
  }
}

static const char* Printf(const char* fmt, ...) {
  const int BUFSIZE = 200;
  static char buffer[BUFSIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, BUFSIZE, fmt, args);
  va_end(args);
  return buffer;
}

void DropoutspyAudioProcessorEditor::DrawHistoBar(Graphics& g, int i, int first_empty, int bar_count, int total_count, int max_bar) {
  if (i >= first_empty) return;

  int width = meter_width / METER_BUCKETS;
  int x = width * i + meter_left + 1;
  int y0 = meter_top + 1;
  int y2 = y0 + meter_height - 2;
  int ymid = y2;
  int yplus = (ymid - y0) * (bar_count / float(max_bar));
  if (bar_count > 0 && yplus < 1) yplus = 1;
  int y1 = ymid - yplus;

  Colour c0 = Colour(0, 192, 0);
  Colour c1 = Colour(0, 255, 0);
  if (i == METER_BUCKETS - 1) {
    c0 = Colour(255, 40, 40);
    c1 = Colour(255, 128, 128);
  } else if (i >= METER_BUCKETS / 2) {
    c0 = Colour(192, 192, 0);
    c1 = Colour(255, 255, 0);
  }

  g.setColour(c0);
  g.fillRect(x, y0, width - 1, y1 - y0);
  g.setColour(c1);
  g.fillRect(x, y1, width - 1, y2 - y1);
}

void DropoutspyAudioProcessorEditor::paint(Graphics& g) {
  // (Our component is opaque, so we must completely fill the background with a solid colour)
  Colour bg = Colour(20, 80, 20);
  if (processor.getOverflowCount()) {
    bg = Colour(80, 20, 20);
  } else if (processor.getWarningCount()) {
    bg = Colour(80, 80, 20);
  }
  g.fillAll(bg);

  g.setColour(Colours::white);
  g.drawRect(meter_left, meter_top, meter_width, meter_height);

  int histo[METER_BUCKETS];
  processor.getHisto(histo);
  int count = 0;
  int max_bar = 0;
  int first_empty = 0;
  for (int i = 0; i < METER_BUCKETS; i++) {
    count += histo[i];
    max_bar = std::max(max_bar, histo[i]);
    if (histo[i]) first_empty = i + 1;
  }
  for (int i = 0; i < METER_BUCKETS; i++) {
    DrawHistoBar(g, i, first_empty, histo[i], count, max_bar);
  }

  // Draw a tick mark at the last delta value.
  float_t current = processor.getLastDelta();
  if (current > 1.0f) current = 1.0f;
  if (current < 0.0f) current = 0.0f;
  int current_x = meter_left + 1 + (meter_width - 2) * (current);
  g.setColour(Colours::white);
  g.fillRect(current_x - 1, meter_top + meter_height - 3, 1, 2);
  g.fillRect(current_x + 0, meter_top + meter_height - 5, 1, 4);
  g.fillRect(current_x + 1, meter_top + meter_height - 3, 1, 2);

  // Show buffer size.
  g.setFont(meter_height);
  g.drawFittedText(Printf("%d", processor.getSamplesPerBlock()), meter_right + 2, meter_top, 50, meter_height, Justification::topLeft, 0);

  int warnings = int(processor.getWarningCount());
  if (warnings > 999) warnings = 999;
  g.setColour(warnings == 0 ? Colour(192, 192, 0) : Colours::yellow);
  g.drawFittedText(Printf("warnings: %d", warnings), text_left1, text_top, text_width1, text_height, Justification::topRight, 1);

  int dropouts = int(processor.getOverflowCount());
  if (dropouts > 999) dropouts = 999;
  g.setColour(dropouts == 0 ? Colour(192, 0, 0) : Colours::red);
  g.drawFittedText(Printf("dropouts: %d", dropouts), text_left2, text_top, text_width2, text_height, Justification::topRight, 1);

  auto repainter = repainter_;
  Timer::callAfterDelay(16, [repainter]{ repainter->Repaint(); });
}
