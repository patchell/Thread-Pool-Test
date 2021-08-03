// ThreadPoolTimerTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <stdio.h>

constexpr auto ITERATIONS = 10;
constexpr auto WAIT_MY_HANDLE_ERROR = -2;

class CMyEvent
{
	HANDLE m_hHandle;
public:
	CMyEvent()
	{
		m_hHandle = 0;
	}

	virtual ~CMyEvent()
	{
		::CloseHandle(m_hHandle);
	}

	virtual BOOL Create(WCHAR *name)
	{
		BOOL rV = FALSE;
		m_hHandle = ::CreateEventW(NULL, FALSE, FALSE, name);
		if (m_hHandle) rV = TRUE;
		return rV;
	}

	bool Post()
	{
		//----------------------------------
		//	Post
		//		This function is used to
		//	the object to the signaled state.
		//
		// return value
		//	TRUE if succesfull
		//	FALSE on fail
		//--------------------------------------

		if (m_hHandle) {
			if (::SetEvent(m_hHandle))
				return TRUE;
		}
		return FALSE;
	}
	int Pend(DWORD timeout = INFINITE)
	{
		//-----------------------------------
		// Pend
		//	This function will wait for an
		//	Event to be signaled.
		// Parameter:
		//	timeout---Time, in milliseconds
		//	that the method will wait for
		//	the object to be signalled.
		//	The default is Infinate
		//
		//	return value:
		//		WAIT_OBJECT_0....Success
		//		WAIT_TIMEOUT.....Timeout
		//		WAIT_FALED.......ERROR
		//		WAIT_MY_HANDLE...ERROR, band handle
		//------------------------------------

		int rV = WAIT_MY_HANDLE_ERROR;	//object not created error
		if (m_hHandle)
			rV = ::WaitForSingleObject(m_hHandle, timeout);
		return rV;
	}
};


union FILETIME64
{
	INT64 quad;
	FILETIME ft;
};

FILETIME TimerRate;
volatile unsigned Count = 0;
CMyEvent evTimerDone;
INT AveragesError[ITERATIONS];
INT AverageTime[ITERATIONS];

void CALLBACK PoolTimer(PTP_CALLBACK_INSTANCE, void* context, PTP_TIMER t)
{
	SetThreadpoolTimer(t, &TimerRate, 0, 0);
	++Count;
	if (Count == 100)
		evTimerDone.Post();
}

FILETIME ConvertMS(DWORD ms)
{
	FILETIME64 t;
	t.quad = -((INT64)10000) * ((INT64)ms);
	return t.ft;
}

int main(int argc, char *argv[])
{
	UINT starttime,endtime,elapsedTime;
	UINT expectedTime;
	INT timeError;
	PTP_TIMER TheTimer;
	BOOL bF;
	UINT DueTimeInMS = 100;
	INT i;

	for (i = 0; i < ITERATIONS; ++i)
	{
		AveragesError[i] = 0;
		AverageTime[i] = 0;
	}
	evTimerDone.Create(NULL);
	TheTimer = CreateThreadpoolTimer(PoolTimer, NULL, NULL);
	for ( i = 0; i < 20; ++i)
	{
		if (i == 1)
			printf("One\n");
		DueTimeInMS = 1;
		for (int j = 0; j < ITERATIONS; ++j)
		{

			TimerRate = ConvertMS(DueTimeInMS);
			SetThreadpoolTimer(TheTimer, &TimerRate, 0, 0);
			starttime = GetTickCount();

			evTimerDone.Pend();

			endtime = GetTickCount();
			SetThreadpoolTimer(TheTimer, 0, 0, 0);
			elapsedTime = endtime - starttime;
			AverageTime[j] += elapsedTime;
			printf("Due Time:%dnS  Eleacpsed Time: %d  Count= %d\n", DueTimeInMS, elapsedTime, Count);
			expectedTime = DueTimeInMS * 100;
			timeError =  elapsedTime - expectedTime;
			printf("Expected Time :%d  Time Error %d\n", expectedTime, timeError);
			AveragesError[j] += timeError;
			printf("*** %d:%d Average Error = %d\n\n", i, j, AveragesError[j] / (i + 1));
			Count = 0;
			DueTimeInMS *= 2;
		}
	}
	int et = 1;
	for (int j = 0; j < ITERATIONS; ++j,et *= 2)
	{
		printf("Expected Due Time:%6dmS  Avg Actual Due Time: %8.3fmS Avg Error%8.3fmS\n", et, double(AverageTime[j]) / double(100 * i), double(AveragesError[j]) / double(100 * i));
	}
	return 0;
}
