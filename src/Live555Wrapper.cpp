#include "Live555Wrapper.h"

#include "RTSPClient.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "BasicUsageEnvironment0.hh"
#include "MP4FileSink.h"

#include <iostream>

// Custom class to handle callback => Recorder
class CustomRTSPCLient : public RTSPClient {
public:
	CustomRTSPCLient(l555Receiver* parent,
		UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel = 0,
		char const* applicationName = NULL,
		portNumBits tunnelOverHTTPPortNum = 0,
		int socketNumToServer = -1) :
		RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, socketNumToServer),
		myParent(parent) {}

	l555Receiver* myParent;
};

struct l555Receiver {
	UsageEnvironment* myEnv = nullptr;
	TaskScheduler* myScheduler = nullptr;
	EventTriggerId eventidpause;
	EventTriggerId eventidresume;
	l555callbacks  cbs = { nullptr,nullptr,nullptr,nullptr,nullptr,nullptr };
	CustomRTSPCLient* myRTSPClient;
	MP4FileSink* qtOut;
	MediaSession* session;
	unsigned int fileSinkBufferSize = 1000000;
	MediaSubsession* subsession = nullptr;
	MediaSubsessionIterator* setupIter = nullptr;
};

//-------------------------------------------------------------------
void closeMediaSinks(l555Receiver* recv)
{
	Medium::close(recv->qtOut); recv->qtOut = NULL;

	if (recv->session == NULL) return;
	MediaSubsessionIterator iter(*recv->session);
	MediaSubsession* subsession;
	while ((subsession = iter.next()) != NULL) {
		Medium::close(subsession->sink);
		subsession->sink = NULL;
	}
}

//== CALLBACKS functors ===========================================================================
#pragma region CALLBACKS
static void continueAfterOPTIONS_CB(RTSPClient* client, int resultCode, char* resultString)
{
	CustomRTSPCLient* c = dynamic_cast<CustomRTSPCLient*>(client);
	if (c) c->myParent->cbs.continueAfterOPTIONS_CB(c->myParent->cbs.clientData, resultCode, resultString);
}

static void continueAfterDESCRIBE_CB(RTSPClient* client, int resultCode, char* resultString)
{
	CustomRTSPCLient* c = dynamic_cast<CustomRTSPCLient*>(client);
	if (c) c->myParent->cbs.continueAfterDESCRIBE_CB(c->myParent->cbs.clientData, resultCode, resultString);
}

static void continueAfterSETUP_CB(RTSPClient* client, int resultCode, char* resultString)
{
	CustomRTSPCLient* c = dynamic_cast<CustomRTSPCLient*>(client);
	if (c) c->myParent->cbs.continueAfterSETUP_CB(c->myParent->cbs.clientData, resultCode, resultString);
}

static void continueAfterPLAY_CB(RTSPClient* client, int resultCode, char* resultString)
{
	CustomRTSPCLient* c = dynamic_cast<CustomRTSPCLient*>(client);
	if (c) c->myParent->cbs.continueAfterPLAY_CB(c->myParent->cbs.clientData, resultCode, resultString);
}

static void continueAfterTEARDOWN_CB(RTSPClient* client, int resultCode, char* resultString)
{
	CustomRTSPCLient* c = dynamic_cast<CustomRTSPCLient*>(client);
	if (c) c->myParent->cbs.continueAfterTEARDOWN_CB(c->myParent->cbs.clientData, resultCode, resultString);
}

static void PAUSE_CB(void* clientData)
{
	l555Receiver* t = static_cast<l555Receiver*>(clientData);
	t->myRTSPClient->sendPauseCommand(*t->session, NULL);
}

static void RESUME_CB(void* clientData)
{
	l555Receiver* t = static_cast<l555Receiver*>(clientData);
	t->myRTSPClient->sendPlayCommand(*t->session, continueAfterPLAY_CB, -1.0f, -1.0f, 1.0f);
}

//-------------------------------------------------------------------
void sessionAfterPlaying(void* data)
{
	l555Receiver* t = static_cast<l555Receiver*>(data);
	t->cbs.sessionAfterPlaying(t->cbs.clientData, 0, NULL);
}

//-------------------------------------------------------------------
void sessionTimerHandler(void* data)
{
	sessionAfterPlaying(data);
}

#pragma endregion

//------------------------------------------------------------------------
L555W_API l555Receiver* l555_createReceiver()
{
	l555Receiver* recv = new l555Receiver;

	recv->myScheduler = BasicTaskScheduler::createNew();
	recv->myEnv = BasicUsageEnvironment::createNew(*recv->myScheduler);

	return recv;
}

//------------------------------------------------------------------------
L555W_API void l555_destroyReceiver(l555Receiver* recv)
{
	recv->myEnv->reclaim();
	//delete recv->myScheduler;

	delete recv;
}


//------------------------------------------------------------------------
L555W_API int l555_createFileReceiver(l555Receiver* recv, const char* outputfilename, int width, int height, int fps)
{
	Boolean generateHintTracks = False;
	Boolean syncStreams = False;
	Boolean packetLossCompensate = False;

	recv->qtOut = MP4FileSink::createNew(*recv->myEnv, *recv->session, outputfilename, recv->fileSinkBufferSize, width, height, fps, packetLossCompensate, syncStreams, generateHintTracks);
	if (recv->qtOut == NULL)
		return 1;

	// Add wait commande for blank
	recv->qtOut->startPlaying(sessionAfterPlaying, recv);

	return 0;
}


//------------------------------------------------------------------------
int l555_startReceiver(l555Receiver* rcv, l555callbacks* cbs, const char * address, volatile char * quitFLag)
{
	rcv->cbs = *cbs;

	int verbosityLevel = 0; // by default, print no verbose output
	rcv->myRTSPClient = new CustomRTSPCLient(rcv, *rcv->myEnv, address, verbosityLevel);
	if (rcv->myRTSPClient == NULL) {
		char const* clientProtocolName = "RTSP";
		std::cerr << "Failed to create " << clientProtocolName << " client: " << rcv->myEnv->getResultMsg() << std::endl;
		return 1;
	}

	rcv->myRTSPClient->setUserAgentString(NULL);
	// Begin by sending an "OPTIONS" command:
	rcv->myRTSPClient->sendOptionsCommand(continueAfterOPTIONS_CB);


	rcv->eventidpause = rcv->myEnv->taskScheduler().createEventTrigger(PAUSE_CB);
	rcv->eventidresume = rcv->myEnv->taskScheduler().createEventTrigger(RESUME_CB);

	rcv->myEnv->taskScheduler().doEventLoop(quitFLag);

	//	Close the sessions
	closeMediaSinks(rcv);
	if (rcv->session)
	{
		Medium::close(rcv->session);
		rcv->session = NULL;
	}

	// Finally, shut down our client:
	if (rcv->myRTSPClient)
	{
		Medium::close(rcv->myRTSPClient);
		rcv->myRTSPClient = NULL;
	}

	return 0;
}

//------------------------------------------------------------------------
void l555_pauseReceiver(l555Receiver* recv)
{
	recv->myEnv->taskScheduler().triggerEvent(recv->eventidpause, recv);
}

//------------------------------------------------------------------------
void l555_resumeReceiver(l555Receiver* recv)
{
	recv->myEnv->taskScheduler().triggerEvent(recv->eventidresume, recv);
}

//------------------------------------------------------------------------
void l555_playReceiver(l555Receiver* recv)
{
	recv->myRTSPClient->sendPlayCommand(*recv->session, continueAfterPLAY_CB, 0.0, -1.0f, 1.0f);
}

//------------------------------------------------------------------------
void l555_sendDescribeCmd(l555Receiver* recv)
{
	recv->myRTSPClient->sendDescribeCommand(continueAfterDESCRIBE_CB);
}

//------------------------------------------------------------------------
int l555_setupStreamsReceiver(l555Receiver* recv, int useTCP)
{
	// Perform additional 'setup' on each subsession, before playing them:
	if (recv->setupIter == NULL)
		recv->setupIter = new MediaSubsessionIterator(*recv->session);

	while ((recv->subsession = recv->setupIter->next()) != NULL) {
		// We have another subsession left to set up:
		if (recv->subsession->clientPortNum() == 0) continue; // port # was not set

		Boolean forceMulticastOnUnspecified = False;

		recv->myRTSPClient->sendSetupCommand(*recv->subsession, continueAfterSETUP_CB, False, useTCP != 0, forceMulticastOnUnspecified);
		return 1;
	}

	// We're done setting up subsessions.
	delete recv->setupIter;
	recv->setupIter = NULL;
	return 0;
}

//------------------------------------------------------------------------
int l555_createSession(l555Receiver* recv, char* sdp)
{
	// Create a media session object from this SDP description:
	recv->session = MediaSession::createNew(*recv->myEnv, sdp);
	if (recv->session == NULL) {
		std::cerr << "Failed to create a MediaSession object from the SDP description: " << recv->myEnv->getResultMsg() << std::endl;
		return 1;
	}
	else if (!recv->session->hasSubsessions()) {
		std::cerr << "This session has no media subsessions (i.e., no \"m=\" lines)" << std::endl;
		return 1;
	}

	// Then, setup the "RTPSource"s for the session:
	MediaSubsessionIterator iter(*recv->session);
	MediaSubsession* subsession;
	Boolean madeProgress = False;
	char const* singleMedium = NULL;
	char const* singleMediumToTest = singleMedium;
	while ((subsession = iter.next()) != NULL) {
		// If we've asked to receive only a single medium, then check this now:
		if (singleMediumToTest != NULL) {
			if (strcmp(subsession->mediumName(), singleMediumToTest) != 0) {
				std::cerr << "Ignoring \"" << subsession->mediumName()
					<< "/" << subsession->codecName()
					<< "\" subsession, because we've asked to receive a single " << singleMedium
					<< " session only" << std::endl;
				continue;
			}
			else {
				// Receive this subsession only
				singleMediumToTest = "xxxxx";
				// this hack ensures that we get only 1 subsession of this type
			}
		}


		if (!subsession->initiate()) {
			std::cerr << "Unable to create receiver for \"" << subsession->mediumName()
				<< "/" << subsession->codecName()
				<< "\" subsession: " << recv->myEnv->getResultMsg() << std::endl;
		}
		else {
			std::cout << "Created receiver for \"" << subsession->mediumName()
				<< "/" << subsession->codecName() << "\" subsession (";
			if (subsession->rtcpIsMuxed()) {
				std::cout << "client port " << subsession->clientPortNum();
			}
			else {
				std::cout << "client ports " << subsession->clientPortNum()
					<< "-" << subsession->clientPortNum() + 1;
			}
			std::cout << ")" << std::endl;
			madeProgress = True;

			if (subsession->rtpSource() != NULL) {
				// Because we're saving the incoming data, rather than playing
				// it in real time, allow an especially large time threshold
				// (1 second) for reordering misordered incoming packets:
				unsigned const thresh = 1000000; // 1 second
				subsession->rtpSource()->setPacketReorderingThresholdTime(thresh);

				// Set the RTP source's OS socket buffer size as appropriate - either if we were explicitly asked (using -B),
				// or if the desired FileSink buffer size happens to be larger than the current OS socket buffer size.
				// (The latter case is a heuristic, on the assumption that if the user asked for a large FileSink buffer size,
				// then the input data rate may be large enough to justify increasing the OS socket buffer size also.)
				int socketNum = subsession->rtpSource()->RTPgs()->socketNum();
				unsigned curBufferSize = getReceiveBufferSize(*recv->myEnv, socketNum);

				unsigned socketInputBufferSize = 0;
				if (socketInputBufferSize > 0 || recv->fileSinkBufferSize > curBufferSize) {
					unsigned newBufferSize = socketInputBufferSize > 0 ? socketInputBufferSize : recv->fileSinkBufferSize;
					newBufferSize = setReceiveBufferTo(*recv->myEnv, socketNum, newBufferSize);
					if (socketInputBufferSize > 0) { // The user explicitly asked for the new socket buffer size; announce it:
						std::cout << "Changed socket receive buffer size for the \""
							<< subsession->mediumName()
							<< "/" << subsession->codecName()
							<< "\" subsession from "
							<< curBufferSize << " to "
							<< newBufferSize << " bytes" << std::endl;
					}
				}
			}
		}
	}
	if (!madeProgress) return 1;

	return 0;
}

//------------------------------------------------------------------------
L555W_API const char* l555_getMediumName(l555Receiver* recv)
{
	return recv->subsession->mediumName();
}

//------------------------------------------------------------------------
L555W_API const char* l555_getCodecName(l555Receiver* recv)
{
	return recv->subsession->codecName();
}

//------------------------------------------------------------------------
L555W_API int   l555_rtcpIsMuxed(l555Receiver* recv)
{
	return recv->subsession->rtcpIsMuxed();
}

//------------------------------------------------------------------------
L555W_API int   l555_getClientPortNum(l555Receiver* recv)
{
	return recv->subsession->clientPortNum();
}

//------------------------------------------------------------------------
L555W_API const char* l555_getResultMsg(l555Receiver* recv)
{
	return recv->myEnv->getResultMsg();
}


//------------------------------------------------------------------------
L555W_API void l555_closeSession(l555Receiver* recv)
{
	// Teardown, then shutdown, any outstanding RTP/RTCP subsessions
	if (recv->session != NULL)
		recv->myRTSPClient->sendTeardownCommand(*recv->session, continueAfterTEARDOWN_CB);
}
