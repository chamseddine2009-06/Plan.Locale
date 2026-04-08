#include <array>
#include <functional>
#define SOUND_IO_CPP
#include "soundIO.hpp"
#include "utils.hpp"
#include "server.h"
#include <portaudio.h>


#define SAMPEL_RATE 44100
#define FRAME_PER_BUFFUER 1024



std::vector<float> dataToPlay;

int devRec ;
int devOut ;

PaStream* inputStream;
PaStream* OutputStream;

PaStreamParameters inputParm;
PaStreamParameters outputParm;

PaStreamParameters fakeIPar;



std::function<void(float* r , unsigned int lng)> readSoundFromMicrophoneHandler = [](auto,auto){return ;};





static int paCallBackReadFromMocrophone(const void* inBuf , void* outBuf , unsigned long framesPerBuf  ,
		const PaStreamCallbackTimeInfo* timeInfo , PaStreamCallbackFlags statusFlag , void* userData)
{
	//for(int i = 0 ; i < framesPerBuf*2 ; i++)((float*)outBuf)[i] = ((float*)inBuf)[i];;
	readSoundFromMicrophoneHandler((float*)inBuf,framesPerBuf*2);	
	/*for(int i = 0 ; i < framesPerBuf*2 ; i++ ){
		dataToPlay.push_back(((float*)inBuf)[i]);

	}*/
	return 0;
}

static int paCallBackOutSound(const void* inBuf , void* outBuf , unsigned long framesPerBuf  ,
		const PaStreamCallbackTimeInfo* timeInfo , PaStreamCallbackFlags statusFlag , void* userData)
{
	float* out = (float*)outBuf;
	for(int i = 0 ; i < framesPerBuf*2; i++){
		if(i < dataToPlay.size()){
			out[i]=dataToPlay[i];
		}else {
			out[i]=0.0;
		}
	}
	if(framesPerBuf*2 >= dataToPlay.size()){
		dataToPlay.clear();

	}else {
		dataToPlay.erase(dataToPlay.begin() , framesPerBuf*2+dataToPlay.begin());
	}
	return 0;
}


void PAErr(PaError err){
	if(err!=paNoError){
		logMsgs("PORT AUDIO ERORR",Pa_GetErrorText(err));
	}
	return;
}


void soundIOInit(){
	dataToPlay.reserve(FRAME_PER_BUFFUER);
	PAErr(Pa_Initialize());
	devRec = Pa_GetDefaultInputDevice();
	devOut = Pa_GetDefaultOutputDevice();
	memset(&inputParm , 0 , sizeof(inputParm));
	memset(&outputParm , 0 , sizeof(outputParm));
	MicrophoneInit();
	SpeakersInit();
}



void soundIOStop(){
	if(devOut!=-1){
		PAErr(Pa_StopStream(OutputStream));
		PAErr(Pa_CloseStream(OutputStream));
	}
	if(devRec!=-1){
		PAErr(Pa_StopStream(inputStream));
		PAErr(Pa_CloseStream(inputStream));
	}
	
	PAErr(Pa_Terminate());
}


void MicrophoneInit(){	
	if(devRec == -1){
		logMsgsErr("No defult microphone detected");
		return;
	}
	inputParm.channelCount = 2;
	inputParm.device = devRec;
	inputParm.hostApiSpecificStreamInfo=NULL;
	inputParm.sampleFormat = paFloat32;
	inputParm.suggestedLatency=Pa_GetDeviceInfo(devRec)->defaultLowInputLatency;
	
	outputParm.channelCount = 2;
	outputParm.device = devOut;
	outputParm.hostApiSpecificStreamInfo=NULL;
	outputParm.sampleFormat = paFloat32;
	outputParm.suggestedLatency=Pa_GetDeviceInfo(devRec)->defaultLowInputLatency;
		
	PAErr(Pa_OpenStream(&inputStream, &inputParm, &outputParm, SAMPEL_RATE, FRAME_PER_BUFFUER, paNoFlag,paCallBackReadFromMocrophone, NULL));

	PAErr(Pa_StartStream(inputStream));

}
void SpeakersInit(){
	if(devOut == -1){
		logMsgsErr("No defult sound output (aka. spekers) detected");
		return;
	}
	outputParm.channelCount = 2;
	outputParm.device = devOut;
	outputParm.hostApiSpecificStreamInfo=NULL;
	outputParm.sampleFormat = paFloat32;
	outputParm.suggestedLatency=Pa_GetDeviceInfo(devRec)->defaultLowInputLatency;
	PAErr(Pa_OpenStream(&OutputStream, nullptr, &outputParm, SAMPEL_RATE, FRAME_PER_BUFFUER, paNoFlag,paCallBackOutSound, NULL));

	PAErr(Pa_StartStream(OutputStream));
}
