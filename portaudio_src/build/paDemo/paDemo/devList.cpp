#include "portaudio.h"
#include "loggerUtil.h"

#ifdef WIN32
#include <windows.h>

#if PA_USE_ASIO
#include "pa_asio.h"
#endif

#ifndef printf
#define printf writeLogT
#endif

/*******************************************************************/
static void PrintSupportedStandardSampleRates(
    const PaStreamParameters* inputParameters,
    const PaStreamParameters* outputParameters)
{
    static double standardSampleRates[] = {
        8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0,
        44100.0, 48000.0, 88200.0, 96000.0, 192000.0, -1 /* negative terminated  list */
    };
    int     i, printCount;
    PaError err;

    printCount = 0;
    for (i = 0; standardSampleRates[i] > 0; i++)
    {
        err = Pa_IsFormatSupported(inputParameters, outputParameters, standardSampleRates[i]);
        if (err == paFormatIsSupported)
        {
            if (printCount == 0)
            {
                printf(L"\t%8.2f", standardSampleRates[i]);
                printCount = 1;
            }
            else if (printCount == 4)
            {
                printf(L",\n\t%8.2f", standardSampleRates[i]);
                printCount = 1;
            }
            else
            {
                printf(L", %8.2f", standardSampleRates[i]);
                ++printCount;
            }
        }
    }
    if (!printCount)
        printf(L"None\n");
    else
        printf(L"\n");
}

/*******************************************************************/
int devListMain(void);
int devListMain(void)
{
    int     i, numDevices, defaultDisplayed;
    const   PaDeviceInfo* deviceInfo;
    PaStreamParameters inputParameters, outputParameters;
    PaError err;


    err = Pa_Initialize();
    if (err != paNoError)
    {
        printf(L"ERROR: Pa_Initialize returned 0x%x\n", err);
        goto error;
    }

    printf(L"PortAudio version: 0x%08X\n", Pa_GetVersion());
    writeLogA("Version text: '%s'\n", Pa_GetVersionInfo()->versionText);

    numDevices = Pa_GetDeviceCount();
    if (numDevices < 0)
    {
        printf(L"ERROR: Pa_GetDeviceCount returned 0x%x\n", numDevices);
        err = numDevices;
        goto error;
    }

    printf(L"Number of devices = %d\n", numDevices);
    for (i = 0; i < numDevices; i++)
    {
        deviceInfo = Pa_GetDeviceInfo(i);
        printf(L"--------------------------------------- device #%d\n", i);

        /* Mark global and API specific default devices */
        defaultDisplayed = 0;
        if (i == Pa_GetDefaultInputDevice())
        {
            printf(L"[ Default Input");
            defaultDisplayed = 1;
        }
        else if (i == Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultInputDevice)
        {
            const PaHostApiInfo* hostInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
            writeLogA("[ Default %s Input", hostInfo->name);
            defaultDisplayed = 1;
        }

        if (i == Pa_GetDefaultOutputDevice())
        {
            printf((defaultDisplayed ? L"," : L"["));
            printf(L" Default Output");
            defaultDisplayed = 1;
        }
        else if (i == Pa_GetHostApiInfo(deviceInfo->hostApi)->defaultOutputDevice)
        {
            const PaHostApiInfo* hostInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
            printf((defaultDisplayed ? L"," : L"["));
            writeLogA(" Default %s Output", hostInfo->name);
            defaultDisplayed = 1;
        }

        if (defaultDisplayed)
            printf(L" ]\n");

        /* print device info fields */
#ifndef UNICODE
        {   /* Use wide char on windows, so we can show UTF-8 encoded device names */
            wchar_t wideName[MAX_PATH];
            MultiByteToWideChar(CP_UTF8, 0, deviceInfo->name, -1, wideName, MAX_PATH - 1);
            wprintf(L"Name                        = %s\n", wideName);
        }
#else
        writeLogA("Name                        = %s\n", deviceInfo->name);
#endif
        writeLogA("Host API                    = %s\n", Pa_GetHostApiInfo(deviceInfo->hostApi)->name);
        printf(L"Max inputs = %d", deviceInfo->maxInputChannels);
        printf(L", Max outputs = %d\n", deviceInfo->maxOutputChannels);

        printf(L"Default low input latency   = %8.4f\n", deviceInfo->defaultLowInputLatency);
        printf(L"Default low output latency  = %8.4f\n", deviceInfo->defaultLowOutputLatency);
        printf(L"Default high input latency  = %8.4f\n", deviceInfo->defaultHighInputLatency);
        printf(L"Default high output latency = %8.4f\n", deviceInfo->defaultHighOutputLatency);

#ifdef WIN32
#if PA_USE_ASIO
        /* ASIO specific latency information */
        if (Pa_GetHostApiInfo(deviceInfo->hostApi)->type == paASIO) {
            long minLatency, maxLatency, preferredLatency, granularity;

            err = PaAsio_GetAvailableLatencyValues(i,
                &minLatency, &maxLatency, &preferredLatency, &granularity);

            printf(L"ASIO minimum buffer size    = %ld\n", minLatency);
            printf(L"ASIO maximum buffer size    = %ld\n", maxLatency);
            printf(L"ASIO preferred buffer size  = %ld\n", preferredLatency);

            if (granularity == -1)
                printf(L"ASIO buffer granularity     = power of 2\n");
            else
                printf(L"ASIO buffer granularity     = %ld\n", granularity);
        }
#endif /* PA_USE_ASIO */
#endif /* WIN32 */

        printf(L"Default sample rate         = %8.2f\n", deviceInfo->defaultSampleRate);

        /* poll for standard sample rates */
        inputParameters.device = i;
        inputParameters.channelCount = deviceInfo->maxInputChannels;
        inputParameters.sampleFormat = paInt16;
        inputParameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
        inputParameters.hostApiSpecificStreamInfo = NULL;

        outputParameters.device = i;
        outputParameters.channelCount = deviceInfo->maxOutputChannels;
        outputParameters.sampleFormat = paInt16;
        outputParameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
        outputParameters.hostApiSpecificStreamInfo = NULL;

        if (inputParameters.channelCount > 0)
        {
            printf(L"Supported standard sample rates\n for half-duplex 16 bit %d channel input = \n",
                inputParameters.channelCount);
            PrintSupportedStandardSampleRates(&inputParameters, NULL);
        }

        if (outputParameters.channelCount > 0)
        {
            printf(L"Supported standard sample rates\n for half-duplex 16 bit %d channel output = \n",
                outputParameters.channelCount);
            PrintSupportedStandardSampleRates(NULL, &outputParameters);
        }

        if (inputParameters.channelCount > 0 && outputParameters.channelCount > 0)
        {
            printf(L"Supported standard sample rates\n for full-duplex 16 bit %d channel input, %d channel output = \n",
                inputParameters.channelCount, outputParameters.channelCount);
            PrintSupportedStandardSampleRates(&inputParameters, &outputParameters);
        }
    }

    Pa_Terminate();

    printf(L"----------------------------------------------\n");
    return 0;

error:
    Pa_Terminate();
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    return err;
}


#endif

