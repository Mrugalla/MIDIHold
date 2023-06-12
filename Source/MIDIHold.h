#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace midihold
{
	using MidiBuffer = juce::MidiBuffer;
	using MidiMessage = juce::MidiMessage;
	using UInt8 = juce::uint8;

	struct Voice
	{
		Voice(int _ch = 1, int _pitch = 0, UInt8 _velo = 0, bool _noteOn = false) :
			ch(_ch), pitch(_pitch), velo(_velo), noteOn(_noteOn)
		{}

		bool operator==(const MidiMessage& msg) const noexcept
		{
			return msg.getChannel() == ch && msg.getNoteNumber() == pitch;
		}

		void addNoteOff(MidiBuffer& midi, int s)
		{
			midi.addEvent(MidiMessage::noteOff(ch, pitch, velo), s);
		}

		void addNoteOn(MidiBuffer& midi, int s)
		{
			midi.addEvent(MidiMessage::noteOn(ch, pitch, velo), s);
		}

		int ch, pitch;
		UInt8 velo;
		bool noteOn;
	};

	inline Voice makeNoteOn(int ch, int pitch, UInt8 velo) noexcept
	{
		return { ch, pitch, velo, true };
	}

	static constexpr int NumVoices = 12; 
	struct Voices
	{
		Voices() :
			voices(),
			idx(0)
		{}

		void clear(MidiBuffer& midiOut) noexcept
		{
			for (auto& voice : voices)
			{
				voice.addNoteOff(midiOut, 0);
				voice = Voice();
			}
		}

		void processNoteOn(MidiBuffer& midiOut, const MidiMessage& msg, int s)
		{
			voices[idx].addNoteOff(midiOut, s);
			idx = (idx + 1) % NumVoices;
			const auto ch = msg.getChannel();
			const auto pitch = msg.getNoteNumber();
			const auto velo = msg.getVelocity();
			voices[idx] = makeNoteOn(ch, pitch, velo);
			voices[idx].addNoteOn(midiOut, s);
		}

		void processNoteOff(MidiBuffer& midiOut, const MidiMessage& msg, int s)
		{
			const auto noteOffIdx = setNoteOff(msg);
			bool wasActiveNote = idx == noteOffIdx;
			if (!wasActiveNote)
				return;
			if (!noteOnsLeft())
				return;

			if(noteOffIdx != -1)
				voices[noteOffIdx].addNoteOff(midiOut, s);

			for (auto i = 0; i < NumVoices; ++i)
			{
				auto j = idx - i - 1;
				if (j < 0)
					j += NumVoices;

				auto& voice = voices[j];
				if (voice.noteOn)
				{
					if (idx != j)
					{
						idx = j;
						voice.addNoteOn(midiOut, s);
					}
					return;
				}
			}
		}

	protected:
		std::array<Voice, NumVoices> voices;
		int idx;

		int setNoteOff(const MidiMessage& msg) noexcept
		{
			for (auto i = 0; i < NumVoices; ++i)
			{
				auto& voice = voices[i];
				if (voice.noteOn && voice == msg)
				{
					voice.noteOn = false;
					return i;
				}
			}
				
			return -1;
		}

		bool noteOnsLeft() const noexcept
		{
			for (const auto& voice : voices)
				if (voice.noteOn)
					return true;
			return false;
		}
	};

	struct MIDIHold
	{
		MIDIHold() :
			voices(),
			midiOut(),
			isPlaying(false)
		{
			midiOut.ensureSize(1024);
		}
		
		void operator()(MidiBuffer& midiIn, bool _isPlaying)
		{
			synthesizeMIDI(midiIn, _isPlaying);
			midiIn.swapWith(midiOut);
		}

		void synthesizeMIDI(const MidiBuffer& midiIn, bool _isPlaying)
		{
			midiOut.clear();

			if (isPlaying != _isPlaying)
			{
				isPlaying = _isPlaying;
				if (!isPlaying)
					voices.clear(midiOut);
			}

			for (auto midi : midiIn)
			{
				const auto msg = midi.getMessage();
				if (msg.isNoteOn())
					voices.processNoteOn(midiOut, msg, midi.samplePosition);
				else if (msg.isNoteOff())
					voices.processNoteOff(midiOut, msg, midi.samplePosition);
				else
					midiOut.addEvent(msg, midi.samplePosition);
			}
		}

		const MidiBuffer& getMIDI() const noexcept
		{
			return midiOut;
		}

	protected:
		Voices voices;
		MidiBuffer midiOut;
		bool isPlaying;
	};
}

/*

todo:

bug of overlapping notes
add kill parameter that clears and noteOffs all voices

*/