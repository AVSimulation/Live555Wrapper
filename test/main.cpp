#include "../src/Live555Wrapper.h"

#include <iostream>
#include <windows.h> 
#include <stdio.h> 

// Globals
l555Receiver* receiver;
std::string url;
std::string outfile;
char quitflag = 0;

void printUsage(const char* prog)
{
	std::cout << prog << " <RTSP url> <mp4file>" << std::endl;
}

void shutdown()
{
	l555_closeSession(receiver);
}

void setupStreams()
{
	int res = l555_setupStreamsReceiver(receiver, (true) ? 1 : 0); // TCP

	if (res == 0)
	{
		// Create output files:
		int res = l555_createFileReceiver(receiver, outfile.c_str(), 0, 0, 25);

		if (res != 0)
		{
			std::cerr << "Failed to create a \"QuickTimeFileSink\" for outputting to \"" << outfile << "\": " << l555_getResultMsg(receiver) << std::endl;
			shutdown();
		}
		else
		{
			std::cout << "Outputting to the file: \"" << outfile << std::endl;
		}
	}
}


#pragma region CALLBACKS
void continueAfterOPTIONS_CB(void* client, int resultCode, char* resultString)
{
	if (resultCode != 0)
	{
		std::cerr << "[RTSPRECORDER] ERROR on  stream: " << url << std::endl;
		std::cerr << "[RTSPRECORDER] ERROR: " << resultString << std::endl;
		delete[] resultString;
		shutdown();
	}
	else
	{
		delete[] resultString;

		// Next, get a SDP description for the stream:
		l555_sendDescribeCmd(receiver);
	}
}
void continueAfterDESCRIBE_CB(void* client, int resultCode, char* resultString)
{
	if (resultCode != 0)
	{
		std::cerr << "[RTSPRECORDER] ERROR: Failed to get a SDP description for the URL \"" << url << "\": " << resultString << std::endl;
		delete[] resultString;
		resultString = NULL;
		shutdown();
		return;
	}

	char* sdpDescription = resultString;
	std::cout << "Opened URL \"" << url << "\", returning a SDP description:" << std::endl << sdpDescription << std::endl;


	int res = l555_createSession(receiver, sdpDescription);
	if (res != 0)
	{
		std::cerr << "Failed to create a session from the SDP description." << std::endl;
		shutdown();
	}

	setupStreams();

	delete[] sdpDescription;
	sdpDescription = NULL;
}

void continueAfterSETUP_CB(void* client, int resultCode, char* resultString)
{
	bool madeProgress = false;

	if (resultCode == 0) {
		std::cout << "Setup \"" << l555_getMediumName(receiver)	<< "/" << l555_getCodecName(receiver)	<< "\" subsession (";
		if (l555_rtcpIsMuxed(receiver) != 0) 
			std::cout << "client port " << l555_getClientPortNum(receiver);
		else 
			std::cout << "client ports " << l555_getClientPortNum(receiver) << "-" << l555_getClientPortNum(receiver) + 1;
		std::cout << ")" << std::endl;
		madeProgress = true;
	}
	else {
		std::cerr << "Failed to setup \"" << l555_getMediumName(receiver) << "/" << l555_getCodecName(receiver) << "\" subsession: " << resultString << std::endl;
	}
	delete[] resultString;

	// Set up the next subsession, if any:
	setupStreams();

	l555_playReceiver(receiver);
}

void continueAfterPLAY_CB(void* client, int resultCode, char* resultString)
{
	if (resultCode != 0)
	{
		std::cerr << "Failed to start playing session: " << resultString << std::endl;
		delete[] resultString;
		resultString = NULL;
		shutdown();
		return;
	}
	else {
		std::cout << "Started playing session" << std::endl;
	}
	delete[] resultString;
}

void continueAfterTEARDOWN_CB(void* client, int resultCode, char* resultString)
{
	delete[] resultString;
}

//-------------------------------------------------------------------
void sessionAfterPlaying(void* data, int resultCode, char* resultString)
{
	shutdown();
}
#pragma endregion


bool consoleHandler(int signal) {

	if (signal == CTRL_C_EVENT) {
		quitflag = 1;
//		shutdown();
	}
	return true;
}

int main(int argc, char** argv)
{
	if (argc < 2)
		printUsage(argv[0]);

	url = argv[1];
	outfile = argv[2];

	std::cout << "===================================" << std::endl;
	std::cout << "URL: " << url << std::endl;
	std::cout << "Out file: " << outfile << std::endl;

	std::cout << "===================================" << std::endl;
	std::cout << "Creating receiver" << std::endl;
	receiver = l555_createReceiver();

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)consoleHandler, TRUE);

	l555callbacks CBs;
	CBs.continueAfterOPTIONS_CB = continueAfterOPTIONS_CB;
	CBs.continueAfterDESCRIBE_CB = continueAfterDESCRIBE_CB;
	CBs.continueAfterSETUP_CB = continueAfterSETUP_CB;
	CBs.continueAfterPLAY_CB = continueAfterPLAY_CB;
	CBs.continueAfterTEARDOWN_CB = continueAfterTEARDOWN_CB;
	CBs.sessionAfterPlaying = sessionAfterPlaying;


	int res = l555_startReceiver(receiver, &CBs, url.c_str(), &quitflag);
	if (res != 0)
	{
		char const* clientProtocolName = "RTSP";
		std::cerr << "Failed to create " << clientProtocolName << " client: " << l555_getResultMsg(receiver) << std::endl;
		shutdown();
		return 1;
	}

	shutdown();


	std::cout << "===================================" << std::endl;
	std::cout << "Deleting receiver" << std::endl;
	l555_destroyReceiver(receiver);

	return 0;
}