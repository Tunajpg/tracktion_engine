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
    const auto pointAfter = [this] (TimePosition t)
    {
        return std::upper_bound (points.begin(), points.end(), t, [] (TimePosition x, const Point& p) { return x < p.time; });
    };

    // The whole block inside one flat stretch (the common case): one gain.
    const float startDb = getDbAt (editTime.getStart());
    const auto first = pointAfter (editTime.getStart());
    const bool crossesPoint = first != points.end() && first->time < editTime.getEnd();
    if (! crossesPoint && startDb == getDbAt (editTime.getEnd()))
    {
        buffer.applyGain (gainAt (startDb));
        return;
    }

    // Piece by piece between breakpoints: linear in dB is a constant gain ratio per sample, so each piece
    // is one pow for its start and one for its ratio (samples timed as FadeInOutNode times them).
    const double secondsPerFrame = editTime.getLength().inSeconds() / numFrames;
    for (int i = 0; i < numFrames;)
    {
        const auto t = editTime.getStart() + TimeDuration::fromSeconds (i * secondsPerFrame);
        const auto next = pointAfter (t);
        int end = numFrames;
        if (next != points.end())
            end = juce::jlimit (i + 1, numFrames, (int) std::ceil ((next->time - editTime.getStart()).inSeconds() / secondsPerFrame));
        const float db0 = getDbAt (t);
        const float dbPerFrame = end - i > 1 ? (getDbAt (t + TimeDuration::fromSeconds ((end - 1 - i) * secondsPerFrame)) - db0) / (float) (end - 1 - i) : 0.0f;
        if (db0 <= AudioClipBase::gainEnvelopeMinDb && dbPerFrame <= 0.0f)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.clear (ch, i, end - i);
        }
        else
        {
            const double ratio = std::pow (10.0, dbPerFrame / 20.0);
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                double g = std::pow (10.0, db0 / 20.0); // not floored: a ramp up from -144 dB rises
                auto* d = buffer.getWritePointer (ch);
                for (int k = i; k < end; ++k, g *= ratio)
                    d[k] *= (float) g;
            }
        }
        i = end;
    }
}

} // namespace tracktion::inline engine
