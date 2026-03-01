#pragma once

#include "Core/QueueManager.h"
#include <JuceHeader.h>

class QueueComponent : public juce::Component,
                       public juce::ListBoxModel,
                       public juce::ChangeListener {
public:
  QueueComponent(QueueManager &qm) : queueManager(qm) {
    queueManager.addChangeListener(this);

    listBox.setModel(this);
    listBox.setColour(juce::ListBox::backgroundColourId,
                      juce::Colours::transparentBlack);
    listBox.setOutlineThickness(0);
    addAndMakeVisible(listBox);

    titleLabel.setText("Song Queue", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);
  }

  ~QueueComponent() override { queueManager.removeChangeListener(this); }

  void paint(juce::Graphics &g) override {
    g.setColour(juce::Colour(0xff1E2124).withAlpha(0.95f)); // Match bgBase
    g.fillRect(getLocalBounds().toFloat());

    // Removed outline
  }

  void resized() override {
    auto area = getLocalBounds();
    titleLabel.setBounds(area.removeFromTop(30).reduced(5, 0));
    listBox.setBounds(area.reduced(2));
  }

  // ListBoxModel overrides
  int getNumRows() override { return queueManager.getNumItems(); }

  // Note: ListBoxModel uses paintListBoxItem for everything.
  // There is no paintRowBackground in ListBoxModel (unlike TableListBoxModel).

  void paintListBoxItem(int rowNumber, juce::Graphics &g, int width, int height,
                        bool rowIsSelected) override {
    auto queue = queueManager.getQueue();
    if (juce::isPositiveAndBelow(rowNumber, queue.size())) {
      if (rowIsSelected) {
        auto selectionRect = juce::Rectangle<int>(width, height).reduced(4, 2);
        g.setColour(juce::Colour(0xff85C1E9)
                        .withAlpha(0.2f)); // Match the new accent color
        g.fillRect(selectionRect.toFloat());
      } else if (rowNumber % 2 != 0) {
        g.setColour(juce::Colours::white.withAlpha(0.02f));
        g.fillRect(0, 0, width, height);
      }

      g.setColour(juce::Colours::white);
      g.setFont(14.0f);

      const auto &song = queue[rowNumber].song;
      g.drawText(
          juce::String(rowNumber + 1) + ". " + song.title + " - " + song.artist,
          5, 0, width - 10, height, juce::Justification::centredLeft, true);
    }
  }

  void listBoxItemDoubleClicked(int row, const juce::MouseEvent &) override {
    // Maybe move to top or something?
  }

  void changeListenerCallback(juce::ChangeBroadcaster *) override {
    listBox.updateContent();
    listBox.repaint();
  }

private:
  QueueManager &queueManager;
  juce::ListBox listBox;
  juce::Label titleLabel;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QueueComponent)
};
