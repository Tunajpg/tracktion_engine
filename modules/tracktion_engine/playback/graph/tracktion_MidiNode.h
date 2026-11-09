/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2024
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/

namespace tracktion::inline engine {

/** A Node that plays MIDI data from a MidiMessageSequence,
    at a specific MIDI channel.
*/
class MidiNode final    : public tracktion::graph::Node,
                          public TracktionEngineNode
{
public:
    MidiNode (std::vector<juce::MidiMessageSequence> sequences,
              MidiList::TimeBase,
              juce::Range<int> midiChannelNumbers,
              bool useMPE,
              juce::Range<double> editSection,
              LiveClipLevel,
              ProcessState&,
              EditItemID,
              std::function<bool()> shouldBeMutedDelegate = nullptr);

    tracktion::graph::NodeProperties getNodeProperties() override;
    void prepareToPlay (const tracktion::graph::PlaybackInitialisationInfo&) override;
    bool isReadyToProcess() override;
    void process (ProcessContext&) override;

    //==============================================================================
    // ARLA (daw-stuck-notes): the notes this node has started and not yet ended. A graph rebuild
    // replaces every node, so the CombiningNode that holds this one hands the list on to the
    // replacing node (same clip ID) or, if the clip is gone, ends the notes itself. Without it an
    // edit made while a note sounds (delete, move, note edit...) left that note hanging.
    const std::shared_ptr<ActiveNoteList>& getActiveNoteList() const    { return activeNotes; }
    void adoptActiveNotes (const MidiNode& replaced)                    { activeNotes = replaced.activeNotes; }
    MPESourceID getMPESourceID() const                                  { return midiSourceID; }

    /** Audio thread, the first block after a rebuild: adds note-offs for the adopted notes this
        node's (new) sequence no longer holds at the playhead, e.g. the note was deleted, transposed
        or shortened, or the clip moved off the playhead. Notes still held play on. */
    void endNotesNoLongerHeld (MidiMessageArray&);

private:
    //==============================================================================
    std::vector<juce::MidiMessageSequence> ms;
    const MidiList::TimeBase timeBase;
    int64_t lastStart = 0;
    size_t currentSequence = 0;
    juce::Range<int> channelNumbers;
    bool useMPEChannelMode;
    juce::Range<double> editRange;
    LiveClipLevel clipLevel;
    EditItemID editItemID;
    std::function<bool()> shouldBeMutedDelegate = nullptr;

    double sampleRate = 44100.0;
    int currentIndex = 0;
    MPESourceID midiSourceID = createUniqueMPESourceID();
    bool wasMute = false, shouldCreateMessagesForTime = false;

    juce::Array<juce::MidiMessage> controllerMessagesScratchBuffer;
    std::shared_ptr<ActiveNoteList> activeNotes = std::make_shared<ActiveNoteList>();

    //==============================================================================
    void processSection (Node::ProcessContext&,
                         juce::Range<double> sectionEditTime,
                         double secondsPerTimeBase,
                         juce::MidiMessageSequence&);
};

} // namespace tracktion::inline engine
