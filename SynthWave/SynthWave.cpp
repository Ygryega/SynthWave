// SynthWave.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define FTYPE double
#include"Instruments.h"
//using namespace synthesizer;

#include<iostream>
#include<cstdlib>
#include<windows.h>
#include<Mmsystem.h>
#include<stdio.h>
using namespace std;


vector<wstring> devices = CSoundDevice<short>::Enumerate();

// Create sound machine!!
CSoundDevice<short> sound(devices[0], 44100, 1, 8, 512);

DWORD tempWord;
DWORD tempVel;


void CALLBACK midiCallback(HMIDIIN handle, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	switch (uMsg)
	{
	case MIM_OPEN:
		wcout << "-----OPENED.-----" << endl;
		break;
	case MIM_CLOSE:
		wcout << "-----EVERYTHING IS CLOSING.-----" << endl;
		break;
	case MIM_DATA:
		wcout << "-----APPARENTLY THERE I5 DATA.-----" << endl;
		break;
	case MIM_LONGDATA:
		wcout << "-----LONGDATA'D.-----" << endl;
		break;
	case MIM_ERROR:
		wcout << "-----ERROR.-----" << endl;
		break;
	case MIM_LONGERROR:
		wcout << "-----LONGERROR.  EVEN WORSE.-----" << endl;
		break;
	}
	//wcout << "dwInstance is " << dwInstance << endl;
	//wcout << "Handle is " << handle << endl;
	//wcout << "Bytes of the midi message is " << dwParam1 << endl; //dwParam1 is the bytes of the MIDI Message packed into an unsigned long
	//wcout << "Velocity is " << HIWORD(dwParam1) << endl; //velocity
	//wcout << "Key ID is " << LOWORD(dwParam1) << endl; //keyID
	//wcout << "Time Stamp of keypress is " << dwParam2 << endl; //dwParam2 is the timestamp of key press
	//wcout << "uMsg is " << uMsg << endl;
	//wcout << "-----" << endl;

	double dTimeNow = sound.GetTime();
	tempWord = LOWORD(dwParam1);
	tempVel = HIWORD(dwParam1);
	DWORD newTemp = tempWord;
	//short nKeyState = LOWORD(dwParam1);

	muxNotes.lock();
	auto noteFound = find_if(vecNotes.begin(), vecNotes.end(), [&newTemp](synthesizer::SNote const& item) { return item.active == true; });
	//auto noteFound = vecNotes.begin();
	
	if (noteFound == vecNotes.begin())//begin
	{
		bool noteActive = false;
		// Note not found in vector
		for (auto it = vecNotes.begin(); it != vecNotes.end(); ++it)
		{
			if (it->id == tempWord)
			{
				noteActive = true;
			}
		}
		if (noteActive == false)
		{

			if (HIWORD(dwParam1) != 0)
			{
				// Key has been pressed so create a new note
				//double temp = double(dwParam1);
				synthesizer::SNote n;
				n.id = tempWord;// LOWORD(dwParam1);
				n.on = dTimeNow;
				n.channel = 1;
				n.active = true;
				wcout << "Velocity is " << HIWORD(dwParam1) << endl; //velocity
				n.volume = tempVel;
				// Add note to vector
				vecNotes.emplace_back(n);
			}
			else
			{
				// Note not in vector, but key has been released...
				// ...nothing to do
				//vecNotes.clear();
				tempWord = NULL;
			}
		}
	}
	else
	{
		// Note exists in vector
		if (HIWORD(dwParam1) != 0)
		{
			// Key is still held, so do nothing
			if (noteFound->off > noteFound->on)
			{
				// Key has been pressed again during release phase
				noteFound->on = dTimeNow;
				//noteFound->active = true;
			}
		}
		else
		{
			// Key has been released, so switch off
			if (noteFound->off < noteFound->on)
			{
				noteFound->off = dTimeNow;
				noteFound->active = false;
			}

		}
	}
	muxNotes.unlock();
}

int main()
{

	MIDIINCAPS     mic;

	unsigned long result;
	HMIDIIN      inHandle;

	unsigned long    iNumDevs, i;
	iNumDevs = midiInGetNumDevs();  /* Get the number of MIDI In devices in this computer */


	for (i = 0; i < iNumDevs; i++)
	{
		/* Get info about the next device */
		if (!midiInGetDevCaps(i, &mic, sizeof(MIDIINCAPS)))
		{
			/* Display its Device ID and name */
			wcout << "Device ID [" << i << "]: " << mic.szPname << endl;
		}
	}
	cout << endl;

	// Link noise function with sound machine
	sound.SetExternalFunction(MakeNoise);

	for (auto d : devices)
	{
		wcout << "Device found: " << d << endl;
	}
	cout << endl;

	char keyboard[129];
	memset(keyboard, ' ', 127);
	keyboard[128] = '\0';

	auto clock_old_time = chrono::high_resolution_clock::now();
	auto clock_real_time = chrono::high_resolution_clock::now();
	double dElapsedTime = 0.0;

	cout << "Press R to Reset and change wave type" << endl;
	cout << "Press Q to Reset the Amplitude" << endl << endl;

	cout << "Press A to decrease aplitude" << endl;
	cout << "Press D to increase apliture" << endl;
	cout << "Press W to Increas HZ" << endl;
	cout << "Press S to Decrease HZ" << endl;
	int Channel = 1;

	while (1)
	{
		/*for (int k = 0; k < 16; k++)
		{*/
			if (GetAsyncKeyState('1') & 0x8000)
			{
				Channel = 1;
			}
			else if (GetAsyncKeyState('2') & 0x8000)
			{
				Channel = 2;
			}
			else if (GetAsyncKeyState('3') & 0x8000)
			{
				Channel = 3;
			}
			else if (GetAsyncKeyState('4') & 0x8000)
			{
				Channel = 4;
			}
			if (GetAsyncKeyState('R') & 0x8000)
			{
				vecNotes.clear();
			}
			if (GetAsyncKeyState('Q') & 0x8000)
			{
				IncreaseAmplitude = 0.001;
				IncreaseHZ = 5.0f;
			}
			if (GetAsyncKeyState('W') & 0x8000)
			{
				IncreaseHZ += 0.01;
			}
			else if (GetAsyncKeyState('S') & 0x8000)
			{
				IncreaseHZ -= 0.01;
			}

			//short nKeyState = GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe\xbf"[k]));

			double dTimeNow = sound.GetTime();
			
			// Open the default MIDI In device. (DevID 0)
			result = midiInOpen(&inHandle, 0, (DWORD)midiCallback, 0, CALLBACK_FUNCTION);

		
			for (auto i = vecNotes.begin(); i != vecNotes.end(); ++i)
			{
				if (tempWord != 0)
				{
					if (tempWord == i->id)
					{
						if (i->off > i->on)
						{
							// Key has been pressed again during release phase
							i->on = dTimeNow;
							//matchFound = false;
							//noteFound->active = true;
							
							i->volume = tempVel;
							

						}
					}
				}
				else
				{
					// Key has been released, so switch off
					if (i->off < i->on)
					{
						i->off = dTimeNow;
						//i->active = false;
					}
				}
			}
			//result = midiInOpen(&inHandle, 0, 0, 0, 0);
			midiInStart(inHandle);
			
			

			//// Check if note already exists in currently playing notes
			//muxNotes.lock();
			//auto noteFound = find_if(vecNotes.begin(), vecNotes.end(), [&k](synthesizer::SNote const& item) { return item.id == k; });
			//if (noteFound == vecNotes.end())
			//{
			//	// Note not found in vector

			//	if (nKeyState & 0x8000)
			//	{
			//		// Key has been pressed so create a new note
			//		synthesizer::SNote n;
			//		n.id = k;
			//		n.on = dTimeNow;
			//		n.channel = Channel;
			//		n.active = true;
			//		
			//		// Add note to vector
			//		vecNotes.emplace_back(n);
			//	}
			//	else
			//	{
			//		// Note not in vector, but key has been released...
			//		// ...nothing to do
			//	}
			//}
			//else
			//{
			//	// Note exists in vector
			//	if (nKeyState & 0x8000)
			//	{
			//		// Key is still held, so do nothing
			//		if (noteFound->off > noteFound->on)
			//		{
			//			// Key has been pressed again during release phase
			//			noteFound->on = dTimeNow;
			//			noteFound->active = true;
			//		}
			//	}
			//	else
			//	{
			//		// Key has been released, so switch off
			//		if (noteFound->off < noteFound->on)
			//		{
			//			noteFound->off = dTimeNow;
			//		}
			//	}
			//}
			//muxNotes.unlock();
		/*}*/
		
		wcout << "\rNotes: " << vecNotes.size() << "    ";
		wcout << "Amplitude: " << IncreaseAmplitude << "    ";
		wcout << "HZ: " << IncreaseHZ << "    ";
	
		//this_thread::sleep_for(5ms);
	}

	return 0;
}