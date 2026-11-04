/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2024
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/

namespace tracktion::inline engine {

class AuxSendPlugin   : public Plugin
{
public:
    AuxSendPlugin (PluginCreationInfo);
    ~AuxSendPlugin() override;

    //==============================================================================
    void setGainDb (float newDb);
    float getGainDb() const                 { return volumeFaderPositionToDB (gain->getCurrentValue()); }

    void setMute (bool m);
    bool isMute();

    int getBusNumber() const                { return busNumber; }
    juce::String getBusName();

    static juce::StringArray getBusNames (Edit&, int maxNumBusses);
    static juce::String getDefaultBusName (int busIndex);

    //==============================================================================
    static const char* getPluginName()              { return NEEDS_TRANS("Aux Send"); }
    static const char* xmlTypeName;
    static juce::ValueTree create();

    juce::String getName() const override;
    juce::String getShortName (int suggestedMaxLength) override;
    juce::String getPluginType() override           { return xmlTypeName; }

    int getNumOutputChannelsGivenInputs (int numInputChannels) override { return numInputChannels; }
    BusLayout getBusses() const override   { return BusLayout::singlePassThrough(); }
    void initialise (const PluginInitialisationInfo&) override;
    void initialiseWithoutStopping (const PluginInitialisationInfo&) override;
    void deinitialise() override;
    void applyToBuffer (const PluginRenderContext&) override;

    juce::String getSelectableDescription() override { return TRANS("Aux Send Plugin"); }

    bool canBeAddedToClip() override                 { return false; }
    bool canBeAddedToRack() override                 { return false; }
    bool needsConstantBufferSize() override          { return true; }

    void restorePluginStateFromValueTree (const juce::ValueTree&) override;

    juce::CachedValue<int> busNumber;
    juce::CachedValue<float> gainLevel;
    juce::CachedValue<bool> invertPhase;

    // DAW patch (daw-send-modes): a send flagged followFader sits before its track's
    // VolumeAndPanPlugin (so it is pre-pan) but is scaled by that fader's gain -- a mixer's
    // "post fader, pre pan" send. Property "appFollowFader"; the gain is 1 when unset.
    juce::CachedValue<bool> followFader;
    float getFollowFaderGain() const;

    AutomatableParameter::Ptr gain;

    /// @internal N.B. used only for testing
    bool isOwnedBy (Track&);

private:
    bool shouldProcess();
    //==============================================================================
    juce::CachedValue<float> lastVolumeBeforeMute;
    float lastGain = 1.0f;

    juce::CriticalSection ownerTrackLock;
    Track* ownerTrack = nullptr;
    Plugin::Ptr ownerFaderRef;                               // keeps ownerFader alive (message thread)
    std::atomic<VolumeAndPanPlugin*> ownerFader { nullptr };   // read on the audio thread

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuxSendPlugin)
};

} // namespace tracktion::inline engine
