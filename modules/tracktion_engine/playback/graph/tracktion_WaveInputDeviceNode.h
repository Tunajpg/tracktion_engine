/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2024
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/

namespace tracktion::inline engine {

/**
    A Node that intercepts incoming live audio and inserts it in to the playback graph.
*/
class WaveInputDeviceNode final : public tracktion::graph::Node,
                                  public InputDeviceInstance::Consumer
{
public:
    WaveInputDeviceNode (InputDeviceInstance&, WaveInputDevice&,
                         const juce::AudioChannelSet& destChannelsToFill,
                         std::shared_ptr<std::atomic<int>> monitorGate = nullptr); // ARLA: 0 = silent
    ~WaveInputDeviceNode() override;

    tracktion::graph::NodeProperties getNodeProperties() override;
    void prepareToPlay (const tracktion::graph::PlaybackInitialisationInfo&) override;
    bool isReadyToProcess() override;
    void process (ProcessContext&) override;

    void acceptInputBuffer (choc::buffer::ChannelArrayView<float>) override;

private:
    //==============================================================================
    InputDeviceInstance& instance;
    WaveInputDevice& waveInputDevice;
    uint32_t lastCallbackTime = 0;
    tracktion::graph::AudioFifo audioFifo { 1, 32 };
    const juce::AudioChannelSet destChannels;
    const std::shared_ptr<std::atomic<int>> monitorGate;
};

} // namespace tracktion::inline engine
