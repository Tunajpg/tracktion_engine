/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2024
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/

namespace tracktion::inline engine {

class MidiNode; // this engine's clip node, not tracktion::graph::MidiNode

/** An Node that mixes a sequence of clips of other nodes.

    This node takes a set of input Nodes with associated start + end times,
    and mixes together their output.

    It initialises and releases its inputs as required according to its current
    play position.
*/
class CombiningNode final : public tracktion::graph::Node,
                            public TracktionEngineNode
{
public:
    CombiningNode (EditItemID, ProcessState&);
    ~CombiningNode() override;

    //==============================================================================
    /** Adds an input node to be played at a given time range.

        The offset is relative to the combining node's zero-time, so the input node's
        time of 0 is equal to its (start + offset) relative to the combiner node's start.

        Any nodes passed-in will be deleted by this node when required.
    */
    void addInput (std::unique_ptr<Node>, TimeRange);

    /** Adds an input node to be played at a given beat range.

        The offset is relative to the combining node's zero-time, so the input node's
        time of 0 is equal to its (start + offset) relative to the combiner node's start.

        Any nodes passed-in will be deleted by this node when required.
    */
    void addInput (std::unique_ptr<Node>, BeatRange);

    /** Returns the number of inputs added. */
    int getNumInputs() const;

    /** Returns the inputs that have been added.
        N.B. This is a bit of a temporary hack to ensure WaveNodes can access previous
        Nodes that have been added via a CombinngNode. This will be cleaned up in the future.
    */
    std::vector<Node*> getInternalNodes() override;

    //==============================================================================
    std::vector<Node*> getDirectInputNodes() override;
    tracktion::graph::NodeProperties getNodeProperties() override;
    void prepareToPlay (const tracktion::graph::PlaybackInitialisationInfo&) override;
    bool isReadyToProcess() override;
    void prefetchBlock (juce::Range<int64_t> /*referenceSampleRange*/) override;
    void process (ProcessContext&) override;

    size_t getAllocatedBytes() const override;

private:
    const EditItemID itemID;

    struct TimedNode;
    juce::OwnedArray<TimedNode> inputs;
    juce::OwnedArray<juce::Array<TimedNode*>> groups;
    std::atomic<bool> isReadyToProcessBlock { false };
    choc::buffer::ChannelArrayBuffer<float> tempAudioBuffer;
    MidiMessageArray noteOffEventsToSend;

    // ARLA (daw-stuck-notes): carries the notes of this track's MidiNodes over a graph rebuild.
    // All built in prepareToPlay (message thread), then only read on the audio thread.
    struct OrphanedNotes { std::shared_ptr<ActiveNoteList> notes; MPESourceID source; };
    std::vector<MidiNode*> midiNodes;           // every MidiNode input
    std::vector<OrphanedNotes> orphanedNotes;   // notes of MidiNodes the rebuild removed
    bool endStaleNotesOnNextBlock = false;
    void carryOverMidiNodeNotes (const CombiningNode& replaced);

    tracktion::graph::NodeProperties nodeProperties;

    void prefetchGroup (juce::Range<int64_t>, TimeRange, BeatRange);
    void queueNoteOffsForClipsNoLongerPresent (const CombiningNode&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CombiningNode)
};

} // namespace tracktion::inline engine
