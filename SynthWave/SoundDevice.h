#pragma once

#pragma comment(lib, "winmm.lib")

#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <algorithm>
#include <condition_variable>
using namespace std;

#include <Windows.h>

#ifndef FTYPE
#define FTYPE double
#endif

//Global vareable rapresenting PI
const double PI = 2.0 * acos(0.0);

template<class T>
class CSoundDevice
{
public:
	CSoundDevice(wstring outputDevice, unsigned int sampleRate = 44100, unsigned int channels = 1, unsigned int blocks = 8, unsigned int blockSamples= 512)
	{
		//Initialise system 
		initialize(outputDevice, sampleRate, channels, blocks, blockSamples);
	}
	~CSoundDevice()
	{
		//Wipe Data
		Destroy();
	}

	bool initialize(wstring outputDevice, unsigned int sampleRate = 44100, unsigned int channels = 1, unsigned int blocks = 8, unsigned int blockSamples = 512)
	{
		//initialize basic data
		mReady = false;
		mSampleRate = sampleRate;
		mChannels = channels;
		mBlockCount = blocks;
		mBlockSamples = blockSamples;
		mBlockFree = mBlockCount;
		mBlockCurrent = 0;
		mpBlockMemory = nullptr;
		mpWaveHeaders = nullptr;

		mpExternFunction = nullptr;

		//Getting all aveliable devices in a vector to then simply inumerate trough them
		vector<wstring> devices = Enumerate();
		auto d = std::find(devices.begin(), devices.end(), outputDevice);
		if (d != devices.end())
		{
			//setting up aveilable device
			int deviceID = distance(devices.begin(), d);
			WAVEFORMATEX waveformat;
			waveformat.wFormatTag = WAVE_FORMAT_PCM;
			waveformat.nSamplesPerSec = mSampleRate;
			waveformat.wBitsPerSample = sizeof(T) * 8;
			waveformat.nChannels = mChannels;
			waveformat.nBlockAlign = (waveformat.wBitsPerSample / 8) * waveformat.nChannels;
			waveformat.nAvgBytesPerSec = waveformat.nSamplesPerSec * waveformat.nBlockAlign;
			waveformat.cbSize = 0;

			if (waveOutOpen(&mWaveDevice, deviceID, &waveformat, (DWORD_PTR)waveOutProcWrap, (DWORD_PTR)this, CALLBACK_FUNCTION) != S_OK)
			{
				//return the function to destroy
				return Destroy();
			}

			//allocate block memory
			mpBlockMemory = new T[mBlockCount * mBlockSamples];
			if (mpBlockMemory == nullptr)
			{
				return Destroy();
			}
			ZeroMemory(mpBlockMemory, sizeof(T) * mBlockCount * mBlockSamples);

			//allocate wave memory
			mpWaveHeaders = new WAVEHDR[mBlockCount];
			if (mpWaveHeaders == nullptr)
			{
				return Destroy();
			}
			ZeroMemory(mpWaveHeaders, sizeof(WAVEHDR) * mBlockCount);

			for (unsigned int i = 0; i < mBlockCount; ++i)
			{
				mpWaveHeaders[i].dwBufferLength = mBlockSamples * sizeof(T);
				mpWaveHeaders[i].lpData = (LPSTR)(mpBlockMemory + (i * mBlockSamples));
			}

			mReady = true;

			mThread = thread(&CSoundDevice::GeneralThread, this);

			//Start

			unique_lock<mutex> lm(mMuxBlockNotZero);
			mCvBlockNotZero.notify_one();

			return true;
		}
	}

	bool Destroy()
	{
		return false;
	}

	void Stop()
	{
		mReady = false;
		mThread.join();
	}

	//used to overide the proces of the current sample 
	virtual FTYPE UserProceses(int Channel, FTYPE Time)
	{
		return 0.0;
	}

	FTYPE GetTime()
	{
		return mGlobalTime;
	}

	static vector<wstring> Enumerate()
	{
		//getting the current number of aveliable sound devices trough a windows function
		//"waveOutGetNumDevs"
		int nDeviceCount = waveOutGetNumDevs();
		vector<wstring> sDevices;
		WAVEOUTCAPS woc;
		//inumerating trough each device found 
		for (int n = 0; n < nDeviceCount; n++)
		{
			if (waveOutGetDevCaps(n, &woc, sizeof(WAVEOUTCAPS)) == S_OK)
			{
				sDevices.push_back(woc.szPname);
			}
		}
		//return list of found devices
		return sDevices;
	}

	void SetExternalFunction(FTYPE(*newFunction)(int, FTYPE))
	{
		//set the function to the externaly chosen one 
		mpExternFunction = newFunction;
	}

	FTYPE Clip(FTYPE sample, FTYPE maxLevel)
	{
		if (sample >= 0.0)
		{
			return fmin(sample, maxLevel);
		}
		else
		{
			return fmax(sample, -maxLevel);
		}
	}

private:

	//pointer to function given by the user 
	FTYPE(*mpExternFunction)(int, FTYPE);

	unsigned int mSampleRate;
	unsigned int mChannels;
	unsigned int mBlockCount;
	unsigned int mBlockSamples;
	unsigned int mBlockCurrent;

	T* mpBlockMemory;
	WAVEHDR* mpWaveHeaders;
	HWAVEOUT mWaveDevice;

	thread mThread;
	atomic<bool> mReady;
	atomic<unsigned int> mBlockFree;
	condition_variable mCvBlockNotZero;
	mutex mMuxBlockNotZero;

	atomic<FTYPE> mGlobalTime;

	//Handles the request for more data to the soundcard
	void waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwParam1, DWORD dwParam2)
	{
		//data request
		//checks if current block of data is done 
		if (uMsg != WOM_DONE)
		{
			return;
		}
		//add extra block
		mBlockFree++;
		unique_lock<mutex> lm(mMuxBlockNotZero);
		//notify thread to fill with data
		mCvBlockNotZero.notify_one();
	}

	//The waveOutProc function is the callback function used with the waveform-audio output device. 
	//The waveOutProc function is a placeholder for the application-defined function name. The address of this function 
	//can be specified in the callback-address parameter of the waveOutOpen function.
	static void CALLBACK waveOutProcWrap(HWAVEOUT hWaveOut, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
	{
		((CSoundDevice*)dwInstance)->waveOutProc(hWaveOut, uMsg, dwParam1, dwParam2);
	}

	//handles requests from the sound card to fill "Blocks" with data
	//if there are no requests it will just wait until further requests are sent
	//then sent back to the soundcard
	void GeneralThread()
	{
		mGlobalTime = 0.0;
		FTYPE dTimeStep = 1.0 / (FTYPE)mSampleRate;

		// Goofy hack to get maximum integer for a type at run-time
		T nMaxSample = (T)pow(2, (sizeof(T) * 8) - 1) - 1;
		FTYPE dMaxSample = (FTYPE)nMaxSample;
		T nPreviousSample = 0;

		while (mReady)
		{
			//start when block is aveilable 
			if (mBlockFree == 0)
			{
				unique_lock<mutex> lm(mMuxBlockNotZero);
				//just in case windows sends an incorect signal
				while (mBlockFree == 0)
				{
					mCvBlockNotZero.wait(lm);
				}
			}

			//clear blocks and tell it is free
			mBlockFree--;

			//prepare block for processisng 
			if (mpWaveHeaders[mBlockCurrent].dwFlags & WHDR_PREPARED)
			{
				waveOutPrepareHeader(mWaveDevice, &mpWaveHeaders[mBlockCurrent], sizeof(WAVEHDR));
			}

			T newSample = 0;
			int CurrentBlock = mBlockCurrent * mBlockSamples;

			//looping trough the block samples
			for (unsigned int i = 0; i < mBlockSamples; i += mChannels)
			{
				//looping trough all the channels 
				for (unsigned int n = 0; n < mChannels; ++n)
				{
					//checks if external function has been given 
					if(mpExternFunction == nullptr)
					{
						//uses "UserProceses" instead of external function
						newSample = (T)(Clip(UserProceses(n, mGlobalTime), 1.0) * dMaxSample);
					}
					else
					{
						//uses the external function
						newSample = (T)(Clip(mpExternFunction(n, mGlobalTime), 1.0) * dMaxSample);
					}

					//move on to the next sample and block
					mpBlockMemory[CurrentBlock + i + n] = newSample;
					nPreviousSample = newSample;
				}

				mGlobalTime = mGlobalTime + dTimeStep;

			}

			//send current data to the chosen sound device
			waveOutPrepareHeader(mWaveDevice, &mpWaveHeaders[mBlockCurrent], sizeof(WAVEHDR));
			waveOutWrite(mWaveDevice, &mpWaveHeaders[mBlockCurrent], sizeof(WAVEHDR));
			mBlockCurrent++;
			mBlockCurrent %= mBlockCount;
		}
	}
};