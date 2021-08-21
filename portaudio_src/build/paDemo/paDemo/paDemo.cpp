#include "paDemo.h"
#include "loggerUtil.h"
#include "portaudio.h"
#include <process.h>
#include <QTimer>

#include "pa_ringbuffer.h"

#if PA_USE_ASIO
#include "pa_asio.h"
#endif

#ifndef printf
#define printf writeLogT
#endif

#define MIN(a,b) ((a)<(b)?(a):(b))
#define PaUtil_AllocateMemory(s) (GlobalAlloc(GPTR,(s)))
#define PaUtil_FreeMemory(s) (GlobalFree((s)))

extern int devListMain();
extern void PrintSupportedStandardSampleRates(
    PaStreamParameters* inputParameters,
    PaStreamParameters* outputParameters);

void showInfo(const   PaDeviceInfo* deviceInfo, PaDeviceIndex devIndex);

#define FILE_NAME       "audio_data.raw"
#define SAMPLE_RATE  (48000)
#define NUM_CHANNELS    (2)
// for frame callback setting
#define FRAMES_PER_BUFFER (512)
// per sample size = NUM_CHANNELS * typesize(SAMPLE)
#define NUM_WRITES_PER_BUFFER   (4)


/* Select sample format. */
#if 1
#define PA_SAMPLE_TYPE  paFloat32
typedef float SAMPLE;
#define SAMPLE_SILENCE  (0.0f)
#define PRINTF_S_FORMAT "%.8f"
#elif 0
#define PA_SAMPLE_TYPE  paInt16
typedef short SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#elif 0
#define PA_SAMPLE_TYPE  paInt8
typedef char SAMPLE;
#define SAMPLE_SILENCE  (0)
#define PRINTF_S_FORMAT "%d"
#else
#define PA_SAMPLE_TYPE  paUInt8
typedef unsigned char SAMPLE;
#define SAMPLE_SILENCE  (128)
#define PRINTF_S_FORMAT "%d"
#endif

typedef struct
{
    unsigned  long      frameIndex;
    int                 threadSyncFlag;
    SAMPLE* ringBufferData;
    PaUtilRingBuffer    ringBuffer;
    FILE* file;
    void* threadHandle;
}
paTestData;

typedef int (*ThreadFunctionType)(void*);

paTestData          globaldata = { 0 };


static unsigned NextPowerOf2(unsigned val)
{
    val--;
    val = (val >> 1) | val;
    val = (val >> 2) | val;
    val = (val >> 4) | val;
    val = (val >> 8) | val;
    val = (val >> 16) | val;
    return ++val;
}

/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int recordCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    static auto currentTime = std::chrono::system_clock::now();
    auto transformed = currentTime.time_since_epoch().count() / 10000;
    static auto lastTime = transformed;

    paTestData* data = (paTestData*)userData;
    ring_buffer_size_t elementsWriteable = PaUtil_GetRingBufferWriteAvailable(&data->ringBuffer);
    ring_buffer_size_t elementsToWrite = MIN(elementsWriteable, (ring_buffer_size_t)(framesPerBuffer * NUM_CHANNELS));
    const SAMPLE* rptr = (const SAMPLE*)inputBuffer;

    (void)outputBuffer; /* Prevent unused variable warnings. */
    (void)timeInfo;
    (void)statusFlags;
    (void)userData;

    data->frameIndex += PaUtil_WriteRingBuffer(&data->ringBuffer, rptr, elementsToWrite);

    currentTime = std::chrono::system_clock::now();
    transformed = currentTime.time_since_epoch().count() / 10000;  // in ns
    fprintf(stdout, "delta: %lld\n", transformed - lastTime);
    lastTime = transformed;

    return paContinue;
}

/* This routine is run in a separate thread to write data from the ring buffer into a file (during Recording) */
static int threadFunctionWriteToRawFile(void* ptr)
{
    paTestData* pData = (paTestData*)ptr;

    /* Mark thread started */
    pData->threadSyncFlag = 0;



    while (1)
    {
        ring_buffer_size_t elementsInBuffer = PaUtil_GetRingBufferReadAvailable(&pData->ringBuffer);
        if ((elementsInBuffer >= pData->ringBuffer.bufferSize / NUM_WRITES_PER_BUFFER) ||
            pData->threadSyncFlag)
        {
            void* ptr[2] = { 0 };
            ring_buffer_size_t sizes[2] = { 0 };

            /* By using PaUtil_GetRingBufferReadRegions, we can read directly from the ring buffer */
            ring_buffer_size_t elementsRead = PaUtil_GetRingBufferReadRegions(&pData->ringBuffer, elementsInBuffer, ptr + 0, sizes + 0, ptr + 1, sizes + 1);
            if (elementsRead > 0)
            {
                int i;
                for (i = 0; i < 2 && ptr[i] != NULL; ++i)
                {
                    fwrite(ptr[i], pData->ringBuffer.elementSizeBytes, sizes[i], pData->file);
                }
                PaUtil_AdvanceRingBufferReadIndex(&pData->ringBuffer, elementsRead);

            }
            
            if (pData->threadSyncFlag)
            {
                break;
            }
        }

        /* Sleep a little while... */
        Pa_Sleep(20);
    }

    pData->threadSyncFlag = 0;

    return 0;
}

/* Start up a new thread in the given function, at the moment only Windows, but should be very easy to extend
   to posix type OSs (Linux/Mac) */
static PaError startThread(paTestData* pData, ThreadFunctionType fn)
{
#ifdef _WIN32
    typedef unsigned(__stdcall* WinThreadFunctionType)(void*);
    pData->threadHandle = (void*)_beginthreadex(NULL, 0, (WinThreadFunctionType)fn, pData, CREATE_SUSPENDED, NULL);
    if (pData->threadHandle == NULL) return paUnanticipatedHostError;

    /* Set file thread to a little higher prio than normal */
    SetThreadPriority(pData->threadHandle, THREAD_PRIORITY_ABOVE_NORMAL);

    /* Start it up */
    pData->threadSyncFlag = 1;
    ResumeThread(pData->threadHandle);

#endif

    /* Wait for thread to startup */
    while (pData->threadSyncFlag) {
        Pa_Sleep(10);
    }

    return paNoError;
}

static int stopThread(paTestData* pData)
{
    pData->threadSyncFlag = 1;
    /* Wait for thread to stop */
    while (pData->threadSyncFlag) {
        Pa_Sleep(10);
    }
#ifdef _WIN32
    CloseHandle(pData->threadHandle);
    pData->threadHandle = 0;
#endif

    return paNoError;
}

void paDemo::startRecord() {

    PaError             err = paNoError;

    unsigned            delayCntr;
    unsigned            numSamples;
    unsigned            numBytes;

    err = Pa_Initialize();
    if (err != paNoError)
    {
        printf(L"ERROR: Pa_Initialize returned 0x%x\n", err);
        goto done;
    }

    writeLogA("patest_record.c\n");

    /* We set the ring buffer size to about 500 ms */
    numSamples = NextPowerOf2((unsigned)(SAMPLE_RATE * 0.05 * NUM_CHANNELS));
    numBytes = numSamples * sizeof(SAMPLE);
    globaldata.ringBufferData = (SAMPLE*)PaUtil_AllocateMemory(numBytes);
    if (globaldata.ringBufferData == NULL)
    {
        writeLogA("Could not allocate ring buffer data.\n");
        goto done;
    }
    globaldata.frameIndex = 0;

    if (PaUtil_InitializeRingBuffer(&globaldata.ringBuffer, sizeof(SAMPLE), numSamples, globaldata.ringBufferData) < 0)
    {
        writeLogA("Failed to initialize ring buffer. Size is not power of 2 ??\n");
        goto done;
    }

    // TODO: do not use fixed device index
    PaDeviceIndex touse = paNoDevice;
    if (ui.listFunctions->count() > 0) {
        if (ui.listFunctions->currentRow() != -1) {
            QListWidgetItem* curitem = ui.listFunctions->item(ui.listFunctions->currentRow());
            touse = curitem->data(Qt::UserRole).toInt();
        }
    }
    else {
        writeLogA("Error: No input device.\n");
        return;
    }
    inputParameters.device = touse; 
    if (inputParameters.device == paNoDevice) {
        writeLogA("Error: No default input device.\n");
        return;
    }
    inputParameters.channelCount = 2;                    /* stereo input */
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(inputParameters.device);
    showInfo(deviceInfo, inputParameters.device);
    /* Record some audio. -------------------------------------------- */
    err = Pa_OpenStream(
        &stream,
        &inputParameters,
        NULL,                  /* &outputParameters, */
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,      /* we won't output out of range samples so don't bother clipping them */
        recordCallback,
        &globaldata);
    if (err != paNoError) return;

    /* Open the raw audio 'cache' file... */
    globaldata.file = fopen(FILE_NAME, "wb");
    if (globaldata.file == 0) return;

    /* Start the file writing thread */
    err = startThread(&globaldata, threadFunctionWriteToRawFile);
    if (err != paNoError) return;

    err = Pa_StartStream(stream);
    if (err != paNoError) return;
    writeLogA("\n=== Now recording to '" FILE_NAME "' until Stop record pressed!! Please speak into the microphone. ===\n");

    timer->start(1000);
    ui.btnStartRecord->setEnabled(false);
    ui.btnStopRecord->setEnabled(true);
done:
    return;
}

paDemo::paDemo(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    initMwLog(this);
    PaError err = paNoError;

    err = Pa_Initialize();
    if (err != paNoError)
    {
        printf(L"ERROR: Pa_Initialize returned 0x%x\n", err);   
    }
    else {
        ui.btnStartRecord->setEnabled(true);
        ui.btnStopRecord->setEnabled(false);
    }
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, QOverload<>::of(&paDemo::updateTimer));


}
paDemo::~paDemo()
{
    Pa_Terminate();
}

void paDemo::updateTimer()
{
    /* Note that the RECORDING part is limited with TIME, not size of the file and/or buffer, so you can
   increase NUM_SECONDS until you run out of disk */
    writeLogA("recorded %ld samples (%.3f secs)\n", globaldata.frameIndex / NUM_CHANNELS, globaldata.frameIndex*1.0f/NUM_CHANNELS/SAMPLE_RATE);
}

void paDemo::on_btnStart_clicked()
{
    // must add /utf-8 compiling option in msvc as this file is saved using encoding of 'utf-8'
    // such as writeLog(L"%s", L"hihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\n");
    if (ui.chkPrint->isChecked())
    {
        devListMain();
        writeLogT(L"%s", L"==========================================\n");
    }
    ShowWSAPI();
}
void paDemo::on_btnStartRecord_clicked()
{
    // must add /utf-8 compiling option in msvc as this file is saved using encoding of 'utf-8'
    // such as writeLog(L"%s", L"hihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\n");
 //   devListMain();
    writeLogT(L"%s", L"============start===============\n");

    startRecord();
}
void paDemo::on_btnStopRecord_clicked()
{
    PaError err;
    // must add /utf-8 compiling option in msvc as this file is saved using encoding of 'utf-8'
    // such as writeLog(L"%s", L"hihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\nhihi我\n");
 //   devListMain();
    
    timer->stop();
    //    if (err < 0) return;

    err = Pa_CloseStream(stream);
    if (err != paNoError) return;

    /* Stop the thread */
    err = stopThread(&globaldata);
    if (err != paNoError) return;

    /* Close file */
    fclose(globaldata.file);
    globaldata.file = 0;

    writeLogA("==stopped====%s=====%ld written====\n", FILE_NAME, globaldata.frameIndex* sizeof(SAMPLE));
done:
    if (globaldata.ringBufferData)       /* Sure it is NULL or valid. */
        PaUtil_FreeMemory(globaldata.ringBufferData);
    if (err != paNoError)
    {
        fprintf(stderr, "An error occurred while using the portaudio stream\n");
        fprintf(stderr, "Error number: %d\n", err);
        fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(err));
        err = 1;          /* Always return 0 or 1, but no other return codes. */
    }
    ui.btnStartRecord->setEnabled(true);
    ui.btnStopRecord->setEnabled(false);
}


int paDemo::ShowWSAPI()
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
    if (numDevices > 0) {
        ui.listFunctions->clear();
    }
    for (i = 0; i < numDevices; i++)
    {
        PaDeviceIndex devId = Pa_HostApiDeviceIndexToDeviceIndex(wasAPIindex, i);
        deviceInfo = Pa_GetDeviceInfo(devId);
        if (ui.chkPrint->isChecked()) {
            showInfo(deviceInfo, devId);
        }
        if (deviceInfo->maxInputChannels > 0) {
            QListWidgetItem *newItem = new QListWidgetItem();
            newItem->setText(QString("%1.%2").arg(i).arg(deviceInfo->name));
            newItem->setData(Qt::UserRole, devId);
            ui.listFunctions->insertItem(ui.listFunctions->count(), newItem);
        }
    }
    return err;
error:
    Pa_Terminate();
    printf(L"Error number: %d\n", err);
    writeLogA("Error message: %s\n", Pa_GetErrorText(err));
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
    writeLogA("Name  (id:%d)               = %s\n", devIndex, deviceInfo->name);
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
    inputParameters.channelCount = MIN(deviceInfo->maxInputChannels,1);
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = 0; /* ignored by Pa_IsFormatSupported() */
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = i;
    outputParameters.channelCount = MIN(deviceInfo->maxOutputChannels,1);
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
