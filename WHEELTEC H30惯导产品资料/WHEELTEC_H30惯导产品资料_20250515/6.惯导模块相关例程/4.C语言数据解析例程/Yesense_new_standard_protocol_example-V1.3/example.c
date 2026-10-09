#define __USE_COM_PORT//__USE_STATIC_METHOD
#ifdef __USE_STATIC_METHOD
#include <stdlib.h>
#include "analysis_data.h"

/* --------------------------------------------------------------------- */
protocol_info_t g_output_info = {0};

unsigned char g_protocol_data[] =
{
    0x59, 0x53, 0x0E, 0xD3, 0x80, 0x01, 0x02, 0xD2, 0x11, 0x10, 0x0C, 0x15, 0x51, 0x01, 0x00, 0xB0,
    0x6C, 0x01, 0x00, 0xCD, 0x18, 0x96, 0x00, 0x20, 0x0C, 0x2E, 0x45, 0x02, 0x00, 0x34, 0x21, 0xFF,
    0xFF, 0x7F, 0xDD, 0x00, 0x00, 0x40, 0x0C, 0x58, 0xA5, 0xF1, 0xFF, 0x8E, 0x39, 0x07, 0x00, 0x18,
    0xC4, 0x8D, 0xFA, 0x41, 0x10, 0xD5, 0xA8, 0x0A, 0x00, 0x55, 0xF4, 0xFF, 0xFF, 0x0D, 0xDE, 0xFF,
    0xFF, 0x31, 0x15, 0xF5, 0xFF, 0x68, 0x14, 0xB7, 0x67, 0xC3, 0xE8, 0x5C, 0x00, 0x00, 0x00, 0x39,
    0x57, 0xB1, 0x44, 0x0F, 0x01, 0x00, 0x00, 0x57, 0x82, 0x00, 0x00, 0x70, 0x0C, 0xE8, 0xFF, 0xFF,
    0xFF, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x0B, 0xBB, 0x03, 0x00, 0x00, 0x16,
    0x00, 0x06, 0x10, 0x05, 0x36, 0x09, 0x51, 0x04, 0x00, 0x00, 0x00, 0x00, 0x52, 0x04, 0x61, 0xD8,
    0x97, 0x69, 0x80, 0x01, 0x54, 0x1E, 0xD3
};

/* --------------------------------------------------------------------- */
int main(void)
{
    while(1)
    {
        analysis_data(g_protocol_data, sizeof(g_protocol_data), &g_output_info);
        _sleep(10);
        printf("pitch: %f, roll: %f, yaw: %f lat: %.10f lon: %.10f alt:%f\n", g_output_info.attitude.pitch, g_output_info.attitude.roll, g_output_info.attitude.yaw, \
               g_output_info.location.latitude, g_output_info.location.longtidue, g_output_info.location.altidue);
        printf("UTC TIME: year-%d month-%d day-%d, hour-%d min-%d second-%d msecond-%d\n", g_output_info.utc.year, g_output_info.utc.month, g_output_info.utc.day, \
               g_output_info.utc.hour, g_output_info.utc.minute, g_output_info.utc.second, g_output_info.utc.msecond);
    }

	return 0;
}
#elif defined(__USE_COM_PORT)

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "analysis_data.h"

typedef struct {
    HANDLE FCommHandle; //INVALID_HANDLE_VALUE; comx handler.
    BOOL FTerminated; //FALSE; thread terminate flag.
    OVERLAPPED ovlap;
    // Event array.
    // One element is used for each event. There are two event handles for each port.
    // A Write event and a receive character event which is located in the overlapped structure (m_ov.hEvent).
    // There is a general shutdown when the port is closed.
    HANDLE hEventArray[3];

    CRITICAL_SECTION    mutex;

    char UserCmd[BUFSIZ];
    DWORD iCmdLen;

    char Recv[BUFSIZ];
    DWORD iRecved;

} ASYNC_COMM_CONTEXT;


unsigned char g_recv_buf[2048] = {0};
unsigned short g_recv_buf_idx = 0;
protocol_info_t g_output_info = {0};

static ASYNC_COMM_CONTEXT g_context;

static void RecvFromCom(void)
{
    BOOL bResult = TRUE;
    DWORD dwError = 0;
    DWORD BytesRead = 0;
    COMSTAT comstat;

    for (;;) {
        EnterCriticalSection(&g_context.mutex);
        bResult = ClearCommError(g_context.FCommHandle, &dwError, &comstat);
        LeaveCriticalSection(&g_context.mutex);
        if (comstat.cbInQue == 0) { // break out when all bytes have been read
            //break;
            goto __YIS_PROC_;
        }

        EnterCriticalSection(&g_context.mutex);

        bResult = ReadFile(g_context.FCommHandle,        // Handle to COMM port
                             g_context.Recv,                // RX Buffer Pointer
                             1,                    // Read one byte
                             &BytesRead,            // Stores number of bytes read
                             &g_context.ovlap);        // pointer to the m_ov structure
        // deal with the error code
        if (!bResult){
            switch (dwError = GetLastError()) {
                    case ERROR_IO_PENDING: {
                            GetOverlappedResult(g_context.FCommHandle,    // Handle to COMM port
                                         &g_context.ovlap,        // Overlapped structure
                                         &BytesRead,        // Stores number of bytes read
                                         TRUE);             // Wait flag
                        }
                        break;
                    default:
                        break;
            }
        }

        LeaveCriticalSection(&g_context.mutex);

        #if 0
        if (BytesRead > 0) {
            g_context.Recv[BytesRead] = 0;
            printf("%s", g_context.Recv);
            BytesRead = 0;
        }
        #else
__YIS_PROC_:
        if(BytesRead > 0)
        {
            g_recv_buf[g_recv_buf_idx++] = g_context.Recv[0];
        }

        unsigned short cnt = g_recv_buf_idx;
        int pos = 0;
        if(cnt < YIS_OUTPUT_MIN_BYTES)
        {
            continue;
        }

        while(cnt > (unsigned int)0)
        {
            int ret = analysis_data(g_recv_buf + pos, cnt, &g_output_info);
            /*未查找到帧头*/
            if(analysis_done == ret)
            {
                pos++;
                cnt--;
            }
            else if(data_len_err == ret)
            {
                break;
            }
            else if(crc_err == ret || analysis_ok == ret)
            {
                /*删除已解析完的完整一帧*/
                output_data_header_t *header = (output_data_header_t *)(g_recv_buf + pos);
                unsigned int frame_len = header->len + YIS_OUTPUT_MIN_BYTES;
                cnt -= frame_len;
                pos += frame_len;
                //memcpy(g_recv_buf, g_recv_buf + pos, cnt);

                if(analysis_ok == ret)
                {
                    printf("pitch: %f, roll: %f, yaw: %f lat: %.10f lon: %.10f alt:%f\n", g_output_info.attitude.pitch, g_output_info.attitude.roll, g_output_info.attitude.yaw, \
                       g_output_info.location.latitude, g_output_info.location.longtidue, g_output_info.location.altidue);
                    printf("UTC TIME: year-%d month-%d day-%d, hour-%d min-%d second-%d msecond-%d\n", g_output_info.utc.year, g_output_info.utc.month, g_output_info.utc.day, \
                       g_output_info.utc.hour, g_output_info.utc.minute, g_output_info.utc.second, g_output_info.utc.msecond);
                }

            }
        }

        memcpy(g_recv_buf, g_recv_buf + pos, cnt);
        g_recv_buf_idx = cnt;
        #endif
    }
}

static void SendDataToCom(void)
{
    BOOL bResult = TRUE;
    DWORD BytesSent = 0;

    ResetEvent(g_context.hEventArray[2]);

    EnterCriticalSection(&g_context.mutex);

    //start write Data.
    {
        bResult = WriteFile(g_context.FCommHandle,                            // Handle to COMM Port
                            g_context.UserCmd,                    // Pointer to message buffer in calling finction
                            g_context.iCmdLen,     // Length of message to send
                            &BytesSent,                                // Where to store the number of bytes sent
                            &g_context.ovlap);                            // Overlapped structure
        // deal with any error codes
        if (!bResult) {
            DWORD dwError = GetLastError();
            switch (dwError) {
                case ERROR_IO_PENDING:    { // continue to GetOverlappedResults()
                        GetOverlappedResult(g_context.FCommHandle,    // Handle to COMM port
                                     &g_context.ovlap,        // Overlapped structure
                                     &BytesSent,        // Stores number of bytes sent
                                     TRUE);             // Wait flag
                        break;
                    }
                default:
                 break;
            }
        }
    }
    //write complete.
    LeaveCriticalSection(&g_context.mutex);

    //Verify that the data size send equals what we tried to send
    if (BytesSent != g_context.iCmdLen)    {// Length of message to send
        //do what???
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//// WIN32 异步串口
///////////////////////////////////////////////////////////////////////////////////////////////////
static VOID FinalComPort(void)
{
    CloseHandle( g_context.ovlap.hEvent );

    if (g_context.FCommHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(g_context.FCommHandle);
        g_context.FCommHandle = INVALID_HANDLE_VALUE;
    }
}

static BOOL InitEvent(void)
{
    //initial Overlapped struct.
    g_context.ovlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ResetEvent(g_context.ovlap.hEvent);

    g_context.hEventArray[0] = CreateEvent(NULL, TRUE, FALSE, NULL); //rs232 shutdown event.
    g_context.hEventArray[1] = g_context.ovlap.hEvent; //rs232 read
    g_context.hEventArray[2] = CreateEvent(NULL, TRUE, FALSE, NULL); //rs232 write event.

    return TRUE;
}

static BOOL InitComPort(void)
{
    COMMTIMEOUTS CommTimeOut;
    DCB dcb;
    char com[64] = {0};

    sprintf(com, "%s%s", "\\\\.\\", "COM25");
    g_context.FCommHandle = CreateFile(TEXT(com),
                         GENERIC_READ | GENERIC_WRITE,    // read/write types
                     0,                                // comm devices must be opened with exclusive access
                     NULL,                            // no security attributes
                     OPEN_EXISTING,                    // comm devices must use OPEN_EXISTING
                     FILE_FLAG_OVERLAPPED,            // Async I/O
                     0);                            // template must be 0 for comm devices

    if (g_context.FCommHandle == INVALID_HANDLE_VALUE) {
        printf("Open Comm failed. [%ld] \r\n", GetLastError());
        return FALSE;
    }
    else {
        printf("Open Comm success. \r\n");
    }

    //Set Commport File InQueue/OutQueue Buffer Size
    SetupComm(g_context.FCommHandle, 8*1024, 8*1024);

    //Set TimeOut Info
    GetCommTimeouts( g_context.FCommHandle, &CommTimeOut);
    CommTimeOut.ReadIntervalTimeout = 1000;
    CommTimeOut.ReadTotalTimeoutMultiplier = 1000;
    CommTimeOut.ReadTotalTimeoutConstant = 1000;
    CommTimeOut.WriteTotalTimeoutMultiplier = 1000;
    CommTimeOut.WriteTotalTimeoutConstant = 1000;
    SetCommTimeouts( g_context.FCommHandle, &CommTimeOut);

    SetCommMask(g_context.FCommHandle, EV_RXFLAG | EV_RXCHAR);

    //Set DCB Info
    GetCommState(g_context.FCommHandle, &dcb);
#if 1
    dcb.DCBlength = sizeof(DCB);
    dcb.BaudRate = 460800;
    dcb.ByteSize = DATABITS_8 ;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
 #else
    BuildCommDCB("baud=460800 parity=N data=8 stop=1" ,&dcb);
 #endif
    dcb.EvtChar = 'q';
    dcb.fRtsControl = RTS_CONTROL_DISABLE;        // set RTS bit
    SetCommState(g_context.FCommHandle, &dcb);
    PurgeComm(g_context.FCommHandle, (PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR));

    return TRUE;
}


void WriteUserCmd(char *Buf, DWORD iLen)
{
    memcpy(g_context.UserCmd, Buf, iLen);
    g_context.iCmdLen = iLen;
    // set event for write
    SetEvent(g_context.hEventArray[2]);
}


DWORD WINAPI ThreadFunc_UartASync(LPVOID args)
{
    BOOL bRet = FALSE;
    DWORD Event = 0;
    DWORD CommEvent = 0;
    DWORD dwError = 0;
    COMSTAT comstat;

    while (!g_context.FTerminated)
    {
        // Make a call to WaitCommEvent(). This call will return immediatly
        // because our port was created as an async port (FILE_FLAG_OVERLAPPED)
        bRet = WaitCommEvent(g_context.FCommHandle, &Event, &g_context.ovlap);
        if (!bRet) { // If WaitCommEvent() returns FALSE, process the last error to determin the reason..
            switch (dwError = GetLastError())
            {
                case ERROR_IO_PENDING: // This is a normal return value if there are no bytes to read at the port.
                    break;
                default: // All other error codes indicate a serious error has occured. Process this error.
                    break;
            }
        }
        else {
            // If WaitCommEvent() returns TRUE, check to be sure there are actually bytes in the buffer to read.
            bRet = ClearCommError(g_context.FCommHandle, &dwError, &comstat);
            if (comstat.cbInQue == 0)
                continue;
        }

        // Main wait function. This function will normally block the thread
        // until one of nine events occur that require action.
        Event = WaitForMultipleObjects(3, g_context.hEventArray, FALSE, INFINITE);

        switch (Event)
        {
        case 0: //shutdown event.
            FinalComPort();
            return 0;
         break;
        case 1: //read event. Event will be set by Overlapped via System.
            GetCommMask(g_context.FCommHandle, &CommEvent);
            if (CommEvent & EV_RXCHAR) {// Receive character event from port.
                RecvFromCom();
            }
            break;
        case 2: //write event. this Event should be set by User Manual.
            SendDataToCom();
            break;
        } //end switch
    } //for ;;

    return 0;
}

DWORD WINAPI ThreadFunc_Console(LPVOID args)
{
    DWORD iLen=0;
    char Buf[BUFSIZ];
    while (!g_context.FTerminated)
    {
        memset(Buf, 0, sizeof(Buf));
        gets(Buf);
        iLen = strlen(Buf);
        Buf[iLen] = '\r';
        Buf[iLen+1] = '\n';
        iLen += 2;
        WriteUserCmd(Buf, iLen);
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (!InitComPort()) exit(-1);
    if (!InitEvent()) exit(-2);

    // initialize critical section
    InitializeCriticalSection(&g_context.mutex);
    g_context.FTerminated = FALSE;

    //create thread to recv and send via Uart Device.
    HANDLE hdr_async = CreateThread(NULL, 0, ThreadFunc_UartASync, NULL, 0, NULL);
    HANDLE hdr_console = CreateThread(NULL, 0, ThreadFunc_Console, NULL, 0, NULL);

    //waiting for some time
    Sleep(60*1000);
    g_context.FTerminated = TRUE;

    WaitForSingleObject(hdr_async, INFINITE);
    WaitForSingleObject(hdr_console, INFINITE);

    DeleteCriticalSection(&g_context.mutex);
    FinalComPort();
    return 0;
}
#endif
