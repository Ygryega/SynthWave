#include "Synth.h"
using namespace synthesizer;

#define FTYPE double

struct SBase_Instrument
{
	FTYPE dVolume;
	synthesizer::SEnvelope_ASDR envelope;
	virtual FTYPE sound(const FTYPE dTime, synthesizer::SNote n, bool& bNoteFinished, FTYPE lHZ, FTYPE lAmplitude) = 0;
};

struct SBell : public SBase_Instrument
{
public:
	FTYPE AmplitudeModifier = 0.0f;

	SBell()
	{
		envelope.mAttackTime = 0.01;
		envelope.mDecayTime = 1.0;
		envelope.mSustainAmplitude = 0.0;
		envelope.mReleaseTime = 1.0;

		dVolume = 1.0;	
	}

	virtual FTYPE sound(const FTYPE dTime, synthesizer::SNote n, bool& bNoteFinished, FTYPE lHZ, FTYPE lAmplitude)
	{
		//FTYPE newAmplitude = 0.0;
		FTYPE dAmplitude = synthesizer::UseEnvelope(dTime, envelope, n.on, n.off);
		if (dAmplitude <= 0.0) bNoteFinished = true;

		
		FTYPE dSound =
			+1.0 * synthesizer::COscillator(n.on - dTime, synthesizer::CSscaleConvert(n.id + 12), synthesizer::SINE_WAVE, lHZ, lAmplitude);
			+ 0.50 * synthesizer::COscillator(n.on - dTime, synthesizer::CSscaleConvert(n.id + 24))
			+ 0.25 * synthesizer::COscillator(n.on - dTime, synthesizer::CSscaleConvert(n.id + 36));
		
		return dAmplitude * dSound * dVolume;
	}
};

struct SPiano : public SBase_Instrument
{
public:
	bool SineChoice;

	SPiano()
	{
		envelope.mAttackTime = 0.01;
		envelope.mDecayTime = 1.0;
		envelope.mSustainAmplitude = 0.0;
		envelope.mReleaseTime = 1.0;
		SineChoice = false;
		dVolume = 1.0;
	}

	virtual FTYPE sound(const FTYPE dTime, synthesizer::SNote n, bool& bNoteFinished, FTYPE lHZ , FTYPE lAmplitude)
	{
		//FTYPE newAmplitude = 0.0;
		FTYPE dAmplitude = synthesizer::UseEnvelope(dTime, envelope, n.on, n.off);
		if (dAmplitude <= 0.0) bNoteFinished = true;

		FTYPE dSound =
			+1.0 * synthesizer::COscillator(n.on - dTime, synthesizer::CSscaleConvert(n.id + 12), synthesizer::SAW_TOOTH_ANALOG, lHZ, lAmplitude);
	
		return dAmplitude * dSound * dVolume;
	}

	FTYPE AddSine()
	{
		SineChoice = true;
	}
};
struct STriangle : public SBase_Instrument
{
public:
	bool SineChoice;

	STriangle()
	{
		envelope.mAttackTime = 0.01;
		envelope.mDecayTime = 1.0;
		envelope.mSustainAmplitude = 0.0;
		envelope.mReleaseTime = 1.0;
		SineChoice = false;
		dVolume = 1.0;
	}

	virtual FTYPE sound(const FTYPE dTime, synthesizer::SNote n, bool& bNoteFinished, FTYPE lHZ, FTYPE lAmplitude)
	{
		//FTYPE newAmplitude = 0.0;
		FTYPE dAmplitude = synthesizer::UseEnvelope(dTime, envelope, n.on, n.off);
		if (dAmplitude <= 0.0) bNoteFinished = true;

		FTYPE dSound =
			+1.0 * synthesizer::COscillator(n.on - dTime, synthesizer::CSscaleConvert(n.id + 12), synthesizer::TRIANGLE_WAVE, lHZ, lAmplitude);

		return dAmplitude * dSound * dVolume;
	}

	FTYPE AddSine()
	{
		SineChoice = true;
	}
};
struct SSquare : public SBase_Instrument
{
public:
	bool SineChoice;

	SSquare()
	{
		envelope.mAttackTime = 0.01;
		envelope.mDecayTime = 3.0;
		envelope.mSustainAmplitude = 0.0;
		envelope.mReleaseTime = 1.0;
		SineChoice = false;
		dVolume = 1.0;
	}

	virtual FTYPE sound(const FTYPE dTime, synthesizer::SNote n, bool& bNoteFinished, FTYPE lHZ, FTYPE lAmplitude)
	{
		//FTYPE newAmplitude = 0.0;
		FTYPE dAmplitude = synthesizer::UseEnvelope(dTime, envelope, n.on, n.off);
		if (dAmplitude <= 0.0) bNoteFinished = true;

		FTYPE dSound =
			+1.0 * synthesizer::COscillator(n.on - dTime, synthesizer::CSscaleConvert(n.id + 36), synthesizer::SQUARE_WAVE, lHZ, lAmplitude);

		//inclorporating the velocity of the key as the volume out put of the singular key
		return dAmplitude * dSound * n.volume * 0.01;
	}

	FTYPE AddSine()
	{
		SineChoice = true;
	}
};

vector<synthesizer::SNote> vecNotes;
mutex muxNotes;
SBell instBell;
SPiano instPiano;
STriangle instTriangle;
SSquare instSqaure;
typedef bool(*lambda)(synthesizer::SNote const& item);
template<class T>

void safe_remove(T& v, lambda f)
{
	auto n = v.begin();
	while (n != v.end())
		if (!f(*n))
			n = v.erase(n);
		else
			++n;
}

FTYPE IncreaseAmplitude = 0.001f;
FTYPE IncreaseHZ = 2.0f;

FTYPE MakeNoise(int nChannel, FTYPE dTime)
{
	//unique_lock<mutex> lm(muxNotes);
	FTYPE dMixedOutput = 0.0;
	if (GetAsyncKeyState('D') & 0x8000)
	{
		IncreaseAmplitude += 0.0001;
		// the 'A' key is currently being held down
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		IncreaseAmplitude -= 0.0001;
		// the 'A' key is currently being held down
	}
	for (auto& n : vecNotes)
	{
		bool bNoteFinished = false;
		FTYPE dSound = 0;
		if (n.channel == 4)
			dSound = instBell.sound(dTime, n, bNoteFinished, IncreaseHZ, IncreaseAmplitude);
		if (n.channel == 3)
			dSound = instPiano.sound(dTime, n, bNoteFinished, IncreaseHZ, IncreaseAmplitude);
		if (n.channel == 2)
			dSound = instTriangle.sound(dTime, n, bNoteFinished, IncreaseHZ, IncreaseAmplitude);
		if (n.channel == 1)
			dSound = instSqaure.sound(dTime, n, bNoteFinished, IncreaseHZ, IncreaseAmplitude);
		dMixedOutput += dSound;

		if (bNoteFinished && n.off > n.on)
			n.active = false;
	}

	// Woah! Modern C++ Overload!!!
	safe_remove<vector<synthesizer::SNote>>(vecNotes, [](synthesizer::SNote const& item) { return item.active; });

	return dMixedOutput * 0.2;
}