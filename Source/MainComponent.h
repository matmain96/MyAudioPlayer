#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/


enum TransportState
{
    Stopped,
    Starting,
    Playing,
    Stopping,
    Pausing,
    Paused,
};

class MainComponent : public juce::AudioAppComponent,
                      public juce::ChangeListener,
                      public juce::Timer,
                      public juce::Slider::Listener
{
public:

    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // Your private member variables go here...
    juce::TextButton openButton;
    juce::TextButton startButton;
    juce::TextButton stopButton;
    juce::TextButton pausaButton;

    juce::Label timeLabel;
    juce::Slider timeSlider;

    std::atomic<bool> moveSlider{false};

    void openButtonClicked();
    void stopButtonClicked();
    void startButtonClicked();
    void pauseButtonClicked();

    void changeListenerCallback(juce::ChangeBroadcaster* source);

    void changeState(const TransportState& newState);

    void playing();
    void stopped();
    void paused();

    void timerCallback() override;

    void startSong();
    void stopSong();


    void sliderValueChanged(juce::Slider* slider);

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    TransportState state;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
