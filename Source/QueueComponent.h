#pragma once

#include <JuceHeader.h>
#include "QueueManager.h"

class QueueComponent : public juce::Component, public juce::ListBoxModel, public juce::ChangeListener
{
public:
    QueueComponent (QueueManager& qm) : queueManager (qm)
    {
        queueManager.addChangeListener (this);
        
        listBox.setModel (this);
        addAndMakeVisible (listBox);
        
        titleLabel.setText ("Song Queue", juce::dontSendNotification);
        titleLabel.setFont (juce::Font (18.0f, juce::Font::bold));
        addAndMakeVisible (titleLabel);
    }

    ~QueueComponent() override
    {
        queueManager.removeChangeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff1a1a1a));
        g.setColour (juce::Colours::grey);
        g.drawRect (getLocalBounds(), 1);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        titleLabel.setBounds (area.removeFromTop (30).reduced (5, 0));
        listBox.setBounds (area.reduced (2));
    }

    // ListBoxModel overrides
    int getNumRows() override { return queueManager.getNumItems(); }

    void paintListBoxItem (int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override
    {
        auto queue = queueManager.getQueue();
        if (juce::isPositiveAndBelow (rowNumber, queue.size()))
        {
            if (rowIsSelected)
                g.fillAll (juce::Colours::orange.withAlpha (0.3f));

            g.setColour (juce::Colours::white);
            g.setFont (14.0f);
            
            const auto& song = queue[rowNumber].song;
            g.drawText (juce::String (rowNumber + 1) + ". " + song.title + " - " + song.artist,
                        5, 0, width - 10, height, juce::Justification::centredLeft, true);
        }
    }

    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override
    {
        // Maybe move to top or something?
    }

    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        listBox.updateContent();
        listBox.repaint();
    }

private:
    QueueManager& queueManager;
    juce::ListBox listBox;
    juce::Label titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QueueComponent)
};
