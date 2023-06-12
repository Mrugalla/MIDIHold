#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace mpesplit
{
	using MidiBuffer = juce::MidiBuffer;
	
	static constexpr int NumChannels = 16;
	using MidiBuffers = std::array<MidiBuffer, NumChannels + 1>;

	struct MPESplit
	{
		MPESplit() :
			buffers()
		{
			for(auto& buffer: buffers)
				buffer.ensureSize(1024);
		}

		void operator()(MidiBuffer& midiIn)
		{
			for (auto& buffer : buffers)
				buffer.clear();
			
			for (const auto midi : midiIn)
			{
				auto msg = midi.getMessage();
				const auto ch = msg.getChannel();
				buffers[ch].addEvent(msg, midi.samplePosition);
			}

			midiIn.swapWith(buffers[0]);
		}

		MidiBuffer& operator[](int ch) noexcept
		{
			return buffers[ch];
		}

		const MidiBuffer& operator[](int ch) const noexcept
		{
			return buffers[ch];
		}

	protected:
		MidiBuffers buffers;
	};
}