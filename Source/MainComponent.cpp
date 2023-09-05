#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : Timer()
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (440, 250);
    
    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
    addAndMakeVisible(openButton);
    openButton.setButtonText("Open");
    openButton.onClick = [this] {openButtonClicked(); };

    addAndMakeVisible(startButton);
    startButton.setButtonText("Play");
    startButton.onClick = [this] {startButtonClicked(); };
    startButton.setEnabled(false);
    startButton.setColour(juce::TextButton::buttonColourId, juce::Colours::forestgreen);

    addAndMakeVisible(stopButton);
    stopButton.setButtonText("Stop");
    stopButton.onClick = [this] {stopButtonClicked(); };
    stopButton.setEnabled(false);
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::indianred);

    addAndMakeVisible(pausaButton);
    pausaButton.setButtonText("Pause");
    pausaButton.onClick = [this] {pauseButtonClicked(); };
    pausaButton.setEnabled(false);
    pausaButton.setColour(juce::TextButton::buttonColourId, juce::Colours::yellow);

    addAndMakeVisible(timeLabel);
    juce::String startingTime("00:00");
    timeLabel.setText(startingTime,juce::dontSendNotification);

    addAndMakeVisible(timeSlider);
    timeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    timeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);

    formatManager.registerBasicFormats();
    transportSource.addChangeListener(this);
    timeSlider.addListener(this);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay(samplesPerBlockExpected,sampleRate);
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Your audio-processing code goes here!

    // For more details, see the help for AudioProcessor::getNextAudioBlock()

    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    if (moveSlider.load())
    {
        transportSource.setPosition(timeSlider.getValue());
        moveSlider.store(false);
    }

    if (readerSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    transportSource.releaseResources();
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    auto& bound = getLocalBounds();
    const int padding = 20;
    const int buttonSizeW = 100;
    const int buttonSizeH = 50;
    const int buttonStartY = getHeight()/4 - buttonSizeH / 2;
    openButton.setBounds(padding, buttonStartY,buttonSizeW,buttonSizeH);
    startButton.setBounds(openButton.getRight(), buttonStartY, buttonSizeW, buttonSizeH);
    stopButton.setBounds(startButton.getRight(), buttonStartY, buttonSizeW, buttonSizeH);
    pausaButton.setBounds(stopButton.getRight(), buttonStartY, buttonSizeW, buttonSizeH);

    timeSlider.setBounds(padding, 200, (getWidth() - padding * 2), 25);
    timeLabel.setBounds(getWidth() /2, 185, 100, 25);

}

void MainComponent::openButtonClicked()
{
    chooser = std::make_unique<juce::FileChooser>("Select a file to play", juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
         "*.wav");
    auto chooserFlags = juce::FileBrowserComponent::openMode || juce::FileBrowserComponent::canSelectFiles;
    if(chooser!=nullptr)
    {
        chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file != juce::File{})
                {
                    auto* reader = formatManager.createReaderFor(file);
                    if (reader != nullptr)
                    {
                        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                        timeSlider.setRange(0., transportSource.getLengthInSeconds());
                        stopSong();
                        startButton.setEnabled(true);
                        readerSource.reset(newSource.release());
                    }
                }
            });
    }
}

void MainComponent::stopButtonClicked()
{
    if (state == Playing)
    {
        changeState(Stopping);
    }
    if (state == Paused)
    {
        startButton.setButtonText("Start");
        stopButton.setButtonText("Stop");
        changeState(TransportState::Stopped);
    }

}

void MainComponent::startButtonClicked()
{
    if (state == Paused)
    {
        startButton.setButtonText("Start");
        stopButton.setButtonText("Stop");
    }
    changeState(Starting);
    if (transportSource.hasStreamFinished())
    {
        stopSong();
        startSong();
    }
}

void MainComponent::pauseButtonClicked()
{
    startButton.setButtonText("Resume");
    stopButton.setButtonText("Restart");
    changeState(Pausing);
}


void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &transportSource)
    {
        if (transportSource.isPlaying())
        { 
            changeState(TransportState::Playing);
        }
        else
        {
            switch (state)
            {
                case TransportState::Stopping:
                    changeState(TransportState::Stopped);
                    break;
                case TransportState::Pausing:
                    changeState(TransportState::Paused);
                    break;
                default:
                    break;
            }
        }
        if (transportSource.hasStreamFinished())
        {
            stopButton.setEnabled(false);
            pausaButton.setEnabled(false);
            startButton.setEnabled(true);
        }
    }
}

void MainComponent::changeState(const TransportState& newState)
{
    if (state != newState)
    {
        state = newState;
        switch (state)
        {
            case TransportState::Playing:
                playing();
                break;
            case TransportState::Stopped:
                stopped();
                break;
            case TransportState::Stopping:
                transportSource.stop();
                break;
            case TransportState::Starting:
                transportSource.start();
                break;
            case TransportState::Pausing:
                transportSource.stop();
                break;
            case TransportState::Paused:
                paused();
                break;
            default:
                    break;
        }
    }
}

void MainComponent::playing()
{
    startButton.setEnabled(false);
    stopButton.setEnabled(true);
    pausaButton.setEnabled(true);
    startSong();
}

void MainComponent::paused()
{
    pausaButton.setEnabled(false);
    startButton.setEnabled(true);
    stopButton.setEnabled(true);
    stopTimer();
}



void MainComponent::stopped()
{
    stopSong();
    stopButton.setEnabled(false);
    pausaButton.setEnabled(false);
    startButton.setEnabled(true);
}

void MainComponent::timerCallback()
{
    juce::RelativeTime timeGetter(transportSource.getCurrentPosition());
    juce::String currentime(timeGetter.inSeconds());
    timeLabel.setText(juce::String::formatted("%02d:%02d",static_cast<int>(timeGetter.inMinutes()), static_cast<int>(timeGetter.inSeconds())%60), juce::dontSendNotification);
    timeSlider.setValue(timeSlider.getValue() + 1., juce::dontSendNotification);
}

void MainComponent::startSong()
{
    startTimer(1000);
}

void MainComponent::stopSong()
{
    stopTimer();
    transportSource.setPosition(0.0);
    timeSlider.setValue(0.,juce::dontSendNotification);
    juce::String startingTime("00:00");
    timeLabel.setText(startingTime, juce::dontSendNotification);
}

void MainComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &timeSlider)
    {
        if(!moveSlider.load())
        { 
            moveSlider.store(true);
        }
    }
}
