#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

#define DebugNP false

#if DebugNP
#include <juce_core/juce_core.h>
#endif

namespace midiNotePriority
{
	enum class Mode
	{
		Lowest,
		Highest,
		NumModes
	};

	using MidiBuffer = juce::MidiBuffer;
	using MidiMessage = juce::MidiMessage;
	static constexpr int NumVoices = 12;
	using Stack = std::array<MidiMessage, NumVoices>;
	
#if DebugNP
	using String = juce::String;
#endif
	
	template<bool Flipped>
	struct NP
	{
		NP() :
			midiOut(),
			stack()
		{
			midiOut.ensureSize(1024);
		}

		void operator()(MidiBuffer& midiIn, Mode mode)
		{
			midiOut.clear();
			
			if(Flipped)
				mode = mode == Mode::Lowest ? Mode::Highest : Mode::Lowest;
			
			auto s = 0;
			auto numVoices = 0;

			for(const auto midi : midiIn)
			{
				const auto msg = midi.getMessage();
				const auto samplePos = midi.samplePosition;
				
				if (!msg.isNoteOnOrOff())
					midiOut.addEvent(msg, samplePos);
				else
				{
					const bool movedOn = s != samplePos;
					if (movedOn)
						processStack(mode, numVoices, samplePos);

					if (numVoices < NumVoices)
					{
						stack[numVoices] = msg;
						++numVoices;
					}

					s = samplePos;
				}	
			}
			
			processStack(mode, numVoices, s);

			midiIn.swapWith(midiOut);
		}

		MidiBuffer midiOut;
		Stack stack;

		void processStack(Mode mode, int& numVoices, int s)
		{
			if(numVoices == 0)
				return;

			if(numVoices > NumVoices)
				numVoices = NumVoices;

			const auto begin = stack.begin();
			const auto end = begin + numVoices;

			switch (mode)
			{
			case Mode::Lowest:
				std::sort(begin, end, [](const MidiMessage& a, const MidiMessage& b)
				{
					return a.getNoteNumber() < b.getNoteNumber();
				});
				break;
			case Mode::Highest:
				std::sort(begin, end, [](const MidiMessage& a, const MidiMessage& b)
				{
					return a.getNoteNumber() > b.getNoteNumber();
				});
				break;
			}

#if DebugNP
			String str;
			for (auto i = 0; i < numVoices; ++i)
				str += String::formatted("%d ", stack[i].getNoteNumber());
			if (numVoices > 1)
				DBG("sorted: " << str);
#endif

			for (auto i = 0; i < numVoices; ++i)
				midiOut.addEvent(stack[i], s);

			numVoices = 0;
		}
	};
}

#undef DebugNP