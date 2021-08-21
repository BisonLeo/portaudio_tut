#include "paDemo.h"
#include "loggerUtil.h"
#include "portaudio.h"

#if PA_USE_ASIO
#include "pa_asio.h"
#endif

#ifndef printf
#define printf writeLogT
#endif

extern int devListMain();
extern void PrintSupportedStandardSampleRates(
    const PaStreamParameters* inputParameters,
    const PaStreamParameters* outputParameters);
int ShowWSAPI();
void showInfo(const   PaDeviceInfo* deviceInfo, PaDeviceIndex devIndex);

paDemo::paDemo(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    initMwLog(this);
}

void paDemo::on_btnStart_clicked()
{
    // must add /utf-8 compiling option in msvc as this file is saved using encoding of 'utf-8'
    // such as writeLog(L"%s", L"hihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\n");
 //   devListMain();
 //   writeLogT(L"%s", L"==========================================\n");
    ShowWSAPI();
}

int ShowWSAPI()
{
    int     i, numDevices, defaultDisplayed;
    const   PaDeviceInfo* deviceInfo;
    
    PaError err;
    PaHostApiTypeId apitype;

    err = Pa_Initialize();
    if (err != paNoError)
    {
        printf(L"ERROR: Pa_Initialize returned 0x%x\n", err);
        goto error;
    }

    printf(L"PortAudio version: 0x%08X\n", Pa_GetVersion());
    writeLogA("Version text: '%s'\n", Pa_GetVersionInfo()->versionText);

    PaHostApiIndex wasAPIindex = Pa_HostApiTypeIdToHostApiIndex(paWASAPI);
    writeLogA("Listing all WASAPI devices: \n");

    numDevices = Pa_GetHostApiInfo(wasAPIindex)->deviceCount;
    if (numDevices < 0)
    {
        printf(L"ERROR: Pa_GetHostApiInfo returned 0x%x\n", numDevices);
        err = numDevices;
        goto error;
    }

    printf(L"Number of WASAPI devices = %d\n", numDevices);
    for (i = 0; i < numDevices; i++)
    {
        deviceInfo = Pa_GetDeviceInfo(Pa_HostApiDeviceIndexToDeviceIndex(wasAPIindex, i));
        showInfo(deviceInfo, i);
    }
error:
    Pa_Terminate();
    fprintf(stderr, "Error number: %d\n", err);
    fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
    return err;
}

void showInfo(const   PaDeviceInfo* deviceInfo, PaDeviceIndex devIndex)
{
    PaError err;
    PaStreamParameters inputParameters, outputParameters;
    int i = devIndex;

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
    inputParameters.channelCount = 1;//  deviceInfo->maxInputChannels;
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
