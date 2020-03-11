#pragma once
using namespace std;
#define FTYPE double
#include "SoundDevice.h"

namespace synthesizer
{
	enum EKnobId
	{
		K1,
		K2,
		K3,
		K4,
		K5,
		K6,
		K7,
		K8,
	};

	//Converting Hz to angular velocity with the use of PI
	FTYPE ConvHz_AngVel(const FTYPE mHertz)
	{
		return mHertz * 2.0 * PI;
	}

	//A basic structure layout for a note can be used with any instrument
	struct SNote
	{
		DWORD id; //the current note rappresenting the position in the chosen scale
		DWORD volume;
		int channel;
		bool active;
		FTYPE on;//activation time	
		FTYPE off;//deactivation time

		SNote()
		{
			id = 0;
			on = 0.0;
			off = 0.0;
			active = false;
			channel = 0;
			volume = 0;
		}
	};

	struct SKnob
	{
		DWORD id;
		int channel;
		bool active;
		FTYPE on;
		FTYPE off;

		SKnob()
		{
			id = 0;
			on = 0.0;
			off = 0.0;
			active = false;
			channel = 0;
		}
	};



	//Oscilator containing all of the various instruments
	//all values are stored in integers and will be returning 
	//values from +1 to  -1
	const int SINE_WAVE = 0;
	const int SQUARE_WAVE = 1;
	const int TRIANGLE_WAVE = 2;
	const int SAW_TOOTH_ANALOG = 3;
	const int SAW_TOOTH_DIGITAL = 4;
	const int NOISE = 5;

	FTYPE COscillator(const FTYPE mTime, const FTYPE mHertz, const int mType = SINE_WAVE,
		const FTYPE mLFOHertz = 0.0f, const FTYPE mAmplitude = 0.0f, FTYPE mCustom = 50.0f)
	{
		FTYPE mFrequency = ConvHz_AngVel(mHertz) * mTime + mAmplitude * mHertz * (sin(ConvHz_AngVel(mLFOHertz) * mTime));

		switch (mType)
		{
		case SINE_WAVE: // Sine wave bewteen -1 and +1
			return sin(mFrequency);

		case SQUARE_WAVE: // Square wave between -1 and +1
			return sin(mFrequency) > 0 ? 1.0 : -1.0;

		case TRIANGLE_WAVE: // Triangle wave between -1 and +1
			return asin(sin(mFrequency)) * (2.0 / PI);

		case SAW_TOOTH_ANALOG: // Saw wave (analogue / warm / slow)
		{
			FTYPE dOutput = 0.0;
			for (FTYPE n = 1.0; n < mCustom; n++)
				dOutput += (sin(n * mFrequency)) / n;
			return dOutput * (2.0 / PI);
		}

		case SAW_TOOTH_DIGITAL:
			return (2.0 / PI) * (mHertz * PI * fmod(mTime, 1.0 / mHertz) - (PI / 2.0));

		case NOISE:
			return 2.0 * ((FTYPE)rand() / (FTYPE)RAND_MAX) - 1.0;

		default:
			return 0.0;
		}
	}

	//it Converts the current note id to it's corrisponding frequency +

	FTYPE CSscaleConvert(const int noteID)
	{
		//return 256 * pow(1.0594630943592952645618252949463, noteID);
		return noteID / 4;
	}

	struct SEnvelope
	{
		virtual FTYPE Amplitude(const FTYPE mTime, const FTYPE mTimeOn, const FTYPE mTimeOff) = 0;
	};

	struct SEnvelope_ASDR : public SEnvelope
	{
		FTYPE mAttackTime;
		FTYPE mDecayTime;
		FTYPE mSustainAmplitude;
		FTYPE mReleaseTime;
		FTYPE mStartingAmplitude;

		SEnvelope_ASDR()
		{
			mAttackTime = 0.1;
			mDecayTime = 0.1;
			mSustainAmplitude = 1.0;
			mReleaseTime = 0.1;
			mStartingAmplitude = 1.0;
		}

		virtual FTYPE Amplitude(const FTYPE mTime, const FTYPE mTimeOn, const FTYPE mTimeOff)
		{
			FTYPE mAmplitude = 0.0f;
			FTYPE mReleaseAmplitude = 0.0f;

			if (mTimeOn > mTimeOff)
			{
				FTYPE mLifeTime = mTime - mTimeOn;

				if (mLifeTime <= mAttackTime)
				{
					mAmplitude = (mLifeTime / mAttackTime) * mStartingAmplitude;
				}
				if (mLifeTime > mAttackTime&& mLifeTime <= (mAttackTime + mDecayTime))
				{
					mAmplitude = ((mLifeTime - mAttackTime) / mDecayTime) * (mSustainAmplitude - mStartingAmplitude) + mStartingAmplitude;
				}
				if (mLifeTime > (mAttackTime + mDecayTime))
				{
					mAmplitude = mSustainAmplitude;
				}
			}
			else
			{
				FTYPE mLifeTime = mTime - mTimeOn;
				if (mLifeTime <= mAttackTime)
				{
					mReleaseAmplitude = (mLifeTime / mAttackTime) * mStartingAmplitude;
				}
				if (mLifeTime > mAttackTime&& mLifeTime <= (mAttackTime + mDecayTime))
				{
					mReleaseAmplitude = ((mLifeTime - mAttackTime) / mDecayTime) * (mSustainAmplitude - mStartingAmplitude) + mStartingAmplitude;
				}
				if (mLifeTime > (mAttackTime + mDecayTime))
				{
					mReleaseAmplitude = mSustainAmplitude;
				}

				mAmplitude = ((mTime - mTimeOff) / mReleaseTime) * (0.0f - mReleaseAmplitude) + mReleaseAmplitude;
			}

			if (mAmplitude <= 0.0000f)
			{
				mAmplitude = 0.0f;
			}
			return mAmplitude;
		} 
	};

	FTYPE UseEnvelope(const FTYPE mTime, SEnvelope& envelope, const FTYPE mTimeOn, const FTYPE mTimeOff)
	{
		return envelope.Amplitude(mTime, mTimeOn, mTimeOff);
	}
}