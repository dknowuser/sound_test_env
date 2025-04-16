#include <iostream>
#include <algorithm>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

WAVEFORMATEX GetFallbackFormat(const WAVEFORMATEX* pOriginal) {
    WAVEFORMATEX fallback = {0};
    fallback.wFormatTag = WAVE_FORMAT_PCM;
    fallback.nChannels = std::min<int>(pOriginal->nChannels, 2); // Max 2 channels
    fallback.nSamplesPerSec = pOriginal->nSamplesPerSec;
    fallback.wBitsPerSample = 16; // Standard 16-bit PCM
    fallback.nBlockAlign = fallback.nChannels * (fallback.wBitsPerSample / 8);
    fallback.nAvgBytesPerSec = fallback.nSamplesPerSec * fallback.nBlockAlign;
    fallback.cbSize = 0;
    return fallback;
}

void ModifyAudioBuffer(BYTE* pData, UINT32 numFrames, WAVEFORMATEX* pFormat) {
    if (pFormat->wFormatTag == WAVE_FORMAT_PCM) {
        // 16-bit PCM processing
        if (pFormat->wBitsPerSample == 16) {
            short* samples = (short*)pData;
            UINT32 totalSamples = numFrames * pFormat->nChannels;
            
            for (UINT32 i = 0; i < totalSamples; i++) {
                // Example: Reduce volume by half
                samples[i] = samples[i] * 0;
            }
        }
    }
    else if (pFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        // 32-bit float processing
        float* samples = (float*)pData;
        UINT32 totalSamples = numFrames * pFormat->nChannels;
        
        for (UINT32 i = 0; i < totalSamples; i++) {
            // Example: Apply simple gain
            samples[i] = samples[i] * 0.0f; // 20% reduction
            
            // Clip protection
            if (samples[i] > 1.0f) samples[i] = 1.0f;
            if (samples[i] < -1.0f) samples[i] = -1.0f;
        }
    }
    else if  (pFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
      WAVEFORMATEXTENSIBLE* waveFormatEx = (WAVEFORMATEXTENSIBLE*)&pFormat;
    
      // Get the actual format from SubFormat GUID
      if (IsEqualGUID(waveFormatEx->SubFormat, KSDATAFORMAT_SUBTYPE_PCM)) {
        // Process as PCM data
        std::cout << "PCM" << std::endl;
      }
      else if (IsEqualGUID(waveFormatEx->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
        // Process as float data
        std::cout << "float" << std::endl;
      }
      else if (IsEqualGUID(waveFormatEx->SubFormat, KSDATAFORMAT_SUBTYPE_DRM)) {
         std::cout << "DRM" << std::endl;
      }
      else if (IsEqualGUID(waveFormatEx->SubFormat, KSDATAFORMAT_SUBTYPE_ALAW)) {
         std::cout << "ALAW" << std::endl;
      }
      else if (IsEqualGUID(waveFormatEx->SubFormat, KSDATAFORMAT_SUBTYPE_MULAW)) {
         std::cout << "MULAW" << std::endl;
      }
      else if (IsEqualGUID(waveFormatEx->SubFormat, KSDATAFORMAT_SUBTYPE_ADPCM)) {
         std::cout << "ADPCM" << std::endl;
      }
      else {
        std::cout << "Unknown subformat" << std::endl;

        WAVEFORMATEX fallbackWaveFormatEx = GetFallbackFormat(pFormat);

        short* samples = (short*)pData;
        UINT32 totalSamples = numFrames * fallbackWaveFormatEx.nChannels;
      	std::cout << "total samples:" << totalSamples << std::endl;  

        for (UINT32 i = 0; i < totalSamples; i++) {
          // Example: Reduce volume by half
          samples[i] = samples[i] * 0;
        }
      };
    };
}

REFERENCE_TIME const hnsBufferDuration = 1e7; // in 100-nanosecond units it is 1 second duration

int main() {
  // Initialize COM
  CoInitializeEx(NULL, COINIT_MULTITHREADED);

  // Get the default audio capture device
  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDevice* pDevice = NULL;
  IAudioClient* pAudioClient = NULL;
  IAudioCaptureClient* pCaptureClient = NULL;
  HANDLE hEvent = NULL;

  CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, 
    __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
  pEnumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &pDevice);

  // Activate the audio client
  pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);

  // Set up the audio format (WAVEFORMATEX structure)
  WAVEFORMATEX* pwfx = NULL;
  pAudioClient->GetMixFormat(&pwfx);

  // Print format information
  std::cout << "Audio Format:\n";
  std::cout << "  Sample Rate: " << pwfx->nSamplesPerSec << "Hz\n";
  std::cout << "  Channels: " << pwfx->nChannels << std::endl;
  std::cout << "  Bits per Sample: " << pwfx->wBitsPerSample << std::endl;
  std::cout << "  Format Tag: " << pwfx->wFormatTag << std::endl;

  // Initialize the audio client
  pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 
   AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 
   hnsBufferDuration, 0, pwfx, NULL);

  // Create event for notifications
  hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
  pAudioClient->SetEventHandle(hEvent);

  // Get the capture client
  pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);

  // Start capturing
  pAudioClient->Start();

  // Capture loop
  BYTE* pData;
  UINT32 numFramesAvailable;
  DWORD flags;
  while(true) {
    pCaptureClient->GetNextPacketSize(&numFramesAvailable);
      if (numFramesAvailable > 0) {
        pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);
        
        // Process the audio data in pData
        if(!(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
          ModifyAudioBuffer(pData, numFramesAvailable, pwfx);
        };
        
        pCaptureClient->ReleaseBuffer(numFramesAvailable);

        // Sleep to prevent CPU overload
        Sleep(10);
      };
  };





  std::cout << "Hello, World!\n";
  return 0;
}
