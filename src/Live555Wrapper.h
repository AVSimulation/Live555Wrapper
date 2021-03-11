#pragma once

#if !defined(WIN32)
	#define L555W_API
#else
	#ifdef L555W_EXPORTS
		#define L555W_API __declspec(dllexport)
	#else
		#define L555W_API __declspec(dllimport)
	#endif
#endif // WIN32

#ifdef __cplusplus
extern "C" {
#endif

	struct l555Receiver;

	typedef void (l555responseHandler)(void* data, int resultCode, char* resultString);

	// Callbacks to be provided at prepare step
	typedef struct {
		l555responseHandler* continueAfterOPTIONS_CB;
		l555responseHandler* continueAfterDESCRIBE_CB;
		l555responseHandler* continueAfterSETUP_CB;
		l555responseHandler* continueAfterPLAY_CB;
		l555responseHandler* continueAfterTEARDOWN_CB;

		l555responseHandler* sessionAfterPlaying;

		void* clientData;
	} l555callbacks;

	L555W_API l555Receiver* l555_createReceiver();
	L555W_API void l555_destroyReceiver(l555Receiver*);

	L555W_API int l555_createFileReceiver(l555Receiver*, const char * outputfilename, int width, int height, int fps);

	//! Blocking fonction prepare and start event loop
	//! return 0: success 1: error
	L555W_API int l555_startReceiver(l555Receiver* rcv, l555callbacks* cbs, const char* address, volatile char* quitFLag);

	L555W_API void l555_pauseReceiver(l555Receiver*);
	L555W_API void l555_resumeReceiver(l555Receiver*);
	L555W_API void l555_playReceiver(l555Receiver*);

	L555W_API void l555_sendDescribeCmd(l555Receiver*);
	L555W_API int l555_setupStreamsReceiver(l555Receiver* recv, int useTCP);
	L555W_API int l555_createSession(l555Receiver*, char* sdp);
	L555W_API void l555_closeSession(l555Receiver*);


	//! subsession accessor
	L555W_API const char* l555_getMediumName(l555Receiver*);
	L555W_API const char* l555_getCodecName(l555Receiver*);
	L555W_API int   l555_rtcpIsMuxed(l555Receiver*);
	L555W_API int   l555_getClientPortNum(l555Receiver*);
	L555W_API const char* l555_getResultMsg(l555Receiver*);


#ifdef __cplusplus
}
#endif
