/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2024
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/

#pragma once

namespace tracktion::inline engine {

//==============================================================================
//==============================================================================
/**
    ARLA: a Node that applies a clip-gain envelope -- breakpoints in edit time and dB, linear in dB
    between them, held flat before the first and after the last -- sample by sample.
    @see AudioClipBase::getGainEnvelope
*/
class ClipGainNode final : public Node,
                           public TracktionEngineNode,
                           public DynamicallyOffsettableNodeBase
{
public:
    struct Point { TimePosition time; float db = 0.0f; };

    ClipGainNode (std::unique_ptr<tracktion::graph::Node> input, ProcessState&, std::vector<Point> sortedPoints);

    /** The envelope's dB at an edit time (the dynamic offset not applied). */
    float getDbAt (TimePosition) const;

    void setDynamicOffsetTime (TimeDuration) override;

    //==============================================================================
    tracktion::graph::NodeProperties getNodeProperties() override;
    std::vector<Node*> getDirectInputNodes() override;
    bool isReadyToProcess() override;
    void process (ProcessContext&) override;

private:
    std::unique_ptr<tracktion::graph::Node> input;
    std::vector<Point> points;
    TimeDuration dynamicOffset;
};

} // namespace tracktion::inline engine
