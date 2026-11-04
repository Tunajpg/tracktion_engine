/*
    ,--.                     ,--.     ,--.  ,--.
  ,-'  '-.,--.--.,--,--.,---.|  |,-.,-'  '-.`--' ,---. ,--,--,      Copyright 2024
  '-.  .-'|  .--' ,-.  | .--'|     /'-.  .-',--.| .-. ||      \   Tracktion Software
    |  |  |  |  \ '-'  \ `--.|  \  \  |  |  |  |' '-' '|  ||  |       Corporation
    `---' `--'   `--`--'`---'`--'`--' `---' `--' `---' `--''--'    www.tracktion.com

    Tracktion Engine uses a GPL/commercial licence - see LICENCE.md for details.
*/

namespace tracktion::inline engine {

ClipGainNode::ClipGainNode (std::unique_ptr<tracktion::graph::Node> inputNode, ProcessState& ps, std::vector<Point> sortedPoints)
    : TracktionEngineNode (ps),
      input (std::move (inputNode)),
      points (std::move (sortedPoints))
{
    jassert (! points.empty());
    setOptimisations ({ tracktion::graph::ClearBuffers::no,
                        tracktion::graph::AllocateAudioBuffer::yes });
}

float ClipGainNode::getDbAt (TimePosition t) const
{
    if (t <= points.front().time)
        return points.front().db;
    if (t >= points.back().time)
        return points.back().db;
    auto next = std::upper_bound (points.begin(), points.end(), t, [] (TimePosition x, const Point& p) { return x < p.time; });
    auto prev = std::prev (next);
    const auto span = next->time - prev->time;
    if (span <= 0s)
        return next->db;
    return prev->db + (next->db - prev->db) * (float) ((t - prev->time) / span);
}

void ClipGainNode::setDynamicOffsetTime (TimeDuration newOffset)
{
    dynamicOffset = newOffset;
}

tracktion::graph::NodeProperties ClipGainNode::getNodeProperties()
{
    auto props = input->getNodeProperties();

    constexpr size_t magicHash = size_t (0x41524c41436c4761); // "ARLAClGa"

    if (props.nodeID != 0)
        hash_combine (props.nodeID, magicHash);

    return props;
}

std::vector<tracktion::graph::Node*> ClipGainNode::getDirectInputNodes()
{
    return { input.get() };
}

bool ClipGainNode::isReadyToProcess()
{
    return input->hasProcessed();
}

void ClipGainNode::process (ProcessContext& pc)
{
    const auto editTime = getEditTimeRange() - dynamicOffset;
    auto sourceBuffers = input->getProcessedOutput();
    auto dest = pc.buffers.audio;
    pc.buffers.midi.copyFrom (sourceBuffers.midi);

    const auto numFrames = (int) dest.getNumFrames();
    if (numFrames == 0 || editTime.isEmpty())
    {
        setAudioOutput (input.get(), sourceBuffers.audio);
        return;
    }

    graph::copyIfNotAliased (dest, sourceBuffers.audio);
    auto buffer = tracktion::graph::toAudioBuffer (dest);

    const auto gainAt = [] (float db) { return juce::Decibels::decibelsToGain (db, AudioClipBase::gainEnvelopeMinDb); };
    const float startDb = getDbAt (editTime.getStart()), endDb = getDbAt (editTime.getEnd());

    // The whole block inside one flat stretch (the common case): one gain.
    const bool crossesPoint = std::any_of (points.begin(), points.end(), [&] (const Point& p)
                                           { return p.time > editTime.getStart() && p.time < editTime.getEnd(); });
    if (! crossesPoint && startDb == endDb)
    {
        buffer.applyGain (gainAt (startDb));
        return;
    }

    // Sample by sample, at each sample's own time (as FadeInOutNode maps them).
    const double secondsPerFrame = editTime.getLength().inSeconds() / numFrames;
    for (int i = 0; i < numFrames; ++i)
    {
        const float g = gainAt (getDbAt (editTime.getStart() + TimeDuration::fromSeconds (i * secondsPerFrame)));
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.getWritePointer (ch)[i] *= g;
    }
}

} // namespace tracktion::inline engine
