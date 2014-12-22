/******************************************************************************* 
02.* FileName    :    com.cpp 
03.* Author        :    
04.* Description   :    
05.* Create Data   :   2005-6-28 9:09 
06.* modify History:    
07.*******************************************************************************/  
  
//#include <windowsx.h>  
//#include <commctrl.h>  
//#include <tchar.h>  
#include <string.h>  
#include <stdlib.h>  
#include <stdio.h>  
  
#include "com.h"  
//#include "common.h"  
//#include "Debug.h"  
#include <windows.h>  
#include <process.h>  
  
#define DelayXms    Sleep  
//#define EB_Printf         Debug_Printf  
  
#define COM_OVERTIME        5   // 此延时控制Uart_ReadBuf中每次读串口的延时：步长为10ms  
#define IS_OVERLAPPED_IO    (TRUE)  
#define MAX_BLOCK_SIZE      (0x20000)   // 缓冲区最大128Kbytes  
#define MAX_Uart_SIZE       (4096)      // 缓冲区最大128Kbytes  
  
HANDLE idComDev;  
OVERLAPPED osWrite,osRead;  
volatile int isConnected=FALSE;  
  
volatile int txEmpty=TRUE;  

unsigned char rxBuf[MAX_Uart_SIZE+1];  
unsigned char RoundBuf[MAX_BLOCK_SIZE];  
  
struct {  
    int Enable; // 允许接收  
    int Ok;     // 接收成功  
    int Off1;   // 缓冲区当前有效数据 开始偏移  
    int Off2;   // 缓冲区当前有效数据 末尾偏移  
    unsigned int    Cnt;    // 缓冲区当前有效数据 计数器=Off2-Off1  
}rxFlag;  
  
//打开串口  
int OpenComPort(int port, int userBaudRate)  
//port=0,1,2,3  
{  
	  if(port>0) port--;
    TCHAR *textCom[]={TEXT("COM1"),TEXT("COM2"),TEXT("COM3"),TEXT("COM4"),TEXT("COM5"),TEXT("COM6"),TEXT("COM7"),TEXT("COM8"),TEXT("COM9"),TEXT("\\\\.\\COM10"),TEXT("\\\\.\\COM11"),TEXT("\\\\.\\COM12"),TEXT("\\\\.\\COM13"),TEXT("\\\\.\\COM14"),TEXT("\\\\.\\COM15"),TEXT("\\\\.\\COM16"),TEXT("\\\\.\\COM17"),TEXT("\\\\.\\COM18"),TEXT("\\\\.\\COM19"),TEXT("\\\\.\\COM20"),TEXT("\\\\.\\COM21"),TEXT("\\\\.\\COM22"),TEXT("\\\\.\\COM23"),TEXT("\\\\.\\COM24"),TEXT("\\\\.\\COM25"),TEXT("\\\\.\\COM26"),TEXT("\\\\.\\COM27"),TEXT("\\\\.\\COM28"),TEXT("\\\\.\\COM29"),TEXT("\\\\.\\COM30"),TEXT("\\\\.\\COM31"),TEXT("\\\\.\\COM32"),TEXT("\\\\.\\COM33"),TEXT("\\\\.\\COM34"),TEXT("\\\\.\\COM35"),TEXT("\\\\.\\COM36"),TEXT("\\\\.\\COM37"),TEXT("\\\\.\\COM38"),TEXT("\\\\.\\COM39"),TEXT("\\\\.\\COM40"),TEXT("\\\\.\\COM41"),TEXT("\\\\.\\COM42"),TEXT("\\\\.\\COM43"),TEXT("\\\\.\\COM44"),TEXT("\\\\.\\COM45"),TEXT("\\\\.\\COM46"),TEXT("\\\\.\\COM47"),TEXT("\\\\.\\COM48"),TEXT("\\\\.\\COM49"),TEXT("\\\\.\\COM50"),TEXT("\\\\.\\COM51"),TEXT("\\\\.\\COM52"),TEXT("\\\\.\\COM53")};  
    //TCHAR textCom[255][14];  
    int i;   
      
    DCB dcb;  
    COMMTIMEOUTS commTimeOuts;  
      
    //for(i=0;i<255;i++)  
   // {  
    //    swprintf(textCom[i],14,L"\\\\.\\COM%d",i);  
   // }  
    //for(i=0;i<255;i++)  
   // {  
    //    snprintf(textCom[i],14,"\\\\.\\COM%d",i);  
    //}  
      
    if(isConnected) // 若重复打开时先关闭  
    {  
        CloseComPort();  
        /*Debug_Printf("\n[OK:Already Opened. Close first!]");*/  
    }  
    //====================================================  
    osRead.Offset=0;  
    osRead.OffsetHigh=0;  
    osWrite.Offset=0;  
    osWrite.OffsetHigh=0;  
      
    osRead.hEvent = CreateEvent(NULL,TRUE/*bManualReset*/,FALSE,NULL);  
    //manual reset event object should be used.   
    //So, system can make the event objecte nonsignalled.  
    //osRead.hEvent & osWrite.hEvent may be used to check the completion of   
    // WriteFile() & ReadFile(). But, the DNW doesn't use this feature.  
    if(osRead.hEvent==NULL)  
    {  
        /*Debug_Printf("\n[ERROR:CreateEvent for osRead.]");*/  
        return FALSE;  
    }  
      
    osWrite.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);  
    if(osWrite.hEvent==NULL)  
    {  
        /*Debug_Printf("\n[ERROR:CreateEvent for osWrite.]");*/  
        return FALSE;  
    }  
      
      
    //====================================================  
    idComDev=CreateFile(textCom[port],  
        GENERIC_READ|GENERIC_WRITE,  
        0, //exclusive access  
        /*FILE_SHARE_READ|FILE_SHARE_WRITE,*/   
        NULL,     
        OPEN_EXISTING,  
#if IS_OVERLAPPED_IO  
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,  
#else  
        /*FILE_ATTRIBUTE_NORMAL,*/ 0,  
#endif    
        NULL);  
      
    if(idComDev==INVALID_HANDLE_VALUE)  
    {  
        /*Debug_Printf("\n[ERROR:CreateFile for opening COM port.]" );*/  
        return FALSE;  
    }  
      
    SetCommMask(idComDev,EV_RXCHAR);  
    SetupComm(idComDev,MAX_Uart_SIZE,MAX_Uart_SIZE);  
    PurgeComm(idComDev,PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);  
      
    commTimeOuts.ReadIntervalTimeout=0xffffffff;  
    commTimeOuts.ReadTotalTimeoutMultiplier=0;  
    commTimeOuts.ReadTotalTimeoutConstant=1000;  
    commTimeOuts.WriteTotalTimeoutMultiplier=0;  
    commTimeOuts.WriteTotalTimeoutConstant=1000;  
    SetCommTimeouts(idComDev,&commTimeOuts);  
      
    //====================================================  
    dcb.DCBlength=sizeof(DCB);  
    GetCommState(idComDev,&dcb);  
      
    dcb.fBinary=TRUE;  
    dcb.fParity=FALSE;  
    dcb.BaudRate=userBaudRate; //CBR_115200;  
    dcb.ByteSize=8;  
    dcb.Parity=0;  
    dcb.StopBits=0;  
    dcb.fDtrControl=DTR_CONTROL_ENABLE;//DTR_CONTROL_DISABLE;    //2009-9-4 9:36:26   zzt  
    dcb.fRtsControl=RTS_CONTROL_DISABLE;  
    dcb.fOutxCtsFlow=0;  
    dcb.fOutxDsrFlow=0;  
      
    if(SetCommState(idComDev,&dcb)==TRUE)  
    {     
        isConnected=TRUE;  
        _beginthread( (void (*)(void *))_DoRxTx,0x2000,(void *)0);  
          
        rxFlag.Off1=0;  
        rxFlag.Off2=0;  
        rxFlag.Cnt=0;  
        rxFlag.Enable=1;    // 允许串口接收  
        /*Debug_Printf("\n[OK:OpenComPort port=%d baud=%d.]", port, userBaudRate);*/  
          
        return TRUE;  
    }  
    else  
    {  
        isConnected=FALSE;  
        CloseHandle(idComDev);  
        return FALSE;  
    }  
}  
  
 
void resetComPort(void)  
{  
    rxFlag.Off1=0;  
    rxFlag.Off2=0;  
    rxFlag.Cnt=0;  
    rxFlag.Enable=1;    // 允许串口接收  
    rxFlag.Ok=0;        // 允许串口接收      
}     
  
static void resetRTS_sub(int Low,int High)  
{  
    DCB dcb;  
      
    //====================================================  
    dcb.DCBlength=sizeof(DCB);  
      
    GetCommState(idComDev,&dcb);  
    dcb.fRtsControl=RTS_CONTROL_ENABLE;  
    SetCommState(idComDev,&dcb);  
      
    DelayXms(Low);  
      
    GetCommState(idComDev,&dcb);  
    dcb.fRtsControl=RTS_CONTROL_DISABLE;  
    SetCommState(idComDev,&dcb);  
      
    DelayXms(High);   
}  
 /* 
void SetRTSControlEnable(bool enable)  
{  
    DCB dcb;  
    dcb.DCBlength=sizeof(DCB);  
    GetCommState(idComDev,&dcb);  
    if(true ==enable)  
    {  
         dcb.fRtsControl=RTS_CONTROL_ENABLE;  
    }  
    else  
    {  
       dcb.fRtsControl=RTS_CONTROL_DISABLE;  
    }  
    SetCommState(idComDev,&dcb);  
}  */
void resetRTS(void)  
{  
    resetRTS_sub(150,20);  
}  
void resetRTSLowHigh(int Low,int High)  
{  
    return resetRTS_sub(Low,High);  
}  
//关闭串口  
void CloseComPort(void)  
{  
    if(isConnected)  
    {  
        isConnected=FALSE;  
        SetCommMask(idComDev,0);  
        //disable event notification and wait for thread to halt  
        EscapeCommFunction(idComDev,CLRDTR);  
        PurgeComm(idComDev,PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);  
        CloseHandle(idComDev);  
          
        CloseHandle(osRead.hEvent);  
        CloseHandle(osWrite.hEvent);  
    }  
      
    Sleep(100);   
    //wait until CloseComPort() effective.  
    //If not, OpenComPort()::CreateFile(...) will fail.  
      
}  
  
//读取串口块  
int ReadComBlock(unsigned char *buf,int maxLen)  
{  
    BOOL fReadStat;  
    COMSTAT comStat;  
    DWORD dwErrorFlags;  
    DWORD dwLength;  
      
    ClearCommError(idComDev,&dwErrorFlags,&comStat);  
    dwLength=min((DWORD)maxLen,comStat.cbInQue);  
    if(dwLength>0)  
    {  
#if IS_OVERLAPPED_IO  
        fReadStat=ReadFile(idComDev,buf,dwLength,&dwLength,&osRead);  
        if(!fReadStat)   //Apr.28.2003:GetOverlappedResult() may be needed.  
        {  
            //By experiment, fReadStat was always TRUE,of course, and the following was never executed.  
            /*Debug_Printf("\n[RX_RD_WAIT]\n" ); */  
            if(GetLastError()==ERROR_IO_PENDING)  
            {  
                GetOverlappedResult(idComDev,&osRead,&dwLength,TRUE);   
            }  
            else  
            {  
                /*Debug_Printf("[RXERR]" );*/  
            }  
        }  
#else  
        fReadStat=ReadFile(idComDev,buf,dwLength,&dwLength,NULL);  
        if(!fReadStat)  
        {  
            Debug_Printf("[RXERR]" );  
        }  
#endif  
    }  
    return dwLength;  
}  
  
  
void _DoRxTx(void *args)  
{  
    OVERLAPPED os;  
    DWORD dwEvtMask;  
    int nLength;  
    BOOL fStat;  
    DWORD temp;  
      
    int i;  
      
    memset(&os,0,sizeof(OVERLAPPED));  
    os.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);  
    if(os.hEvent==NULL)  
    {  
       /*EB_Printf("[ERROR:DoRxTx os.hEvent]\n");*/  
        _endthread();  
    }  
      
    if(!SetCommMask(idComDev,EV_RXCHAR|EV_TXEMPTY|EV_ERR))  
    {  
        /*EB_Printf("[ERROR:SetCommMask()]\n");*/  
        CloseHandle(os.hEvent);  
        _endthread();  
    }  
      
   while(isConnected)  
    {  
        dwEvtMask=0;  
#if IS_OVERLAPPED_IO  
        fStat=WaitCommEvent(idComDev,&dwEvtMask,&os);     
        //Apr.28.2003: fStat should be checked for the cpu time saving.   
        if(!fStat)   //Apr.28.2003:GetOverlappedResult() is needed.    
        {  
            //By experiment. Only when there was no signalled event, the following was executed.  
            //EB_Printf("\n[WaitCommEvent=false]\n");   
            if(GetLastError()==ERROR_IO_PENDING)  
           {  
                GetOverlappedResult(idComDev,&os,&temp,TRUE);   
            }  
           else  
            {  
                /*EB_Printf("[RXTX_THREAD_ERR]" );*/  
            }  
        }  
#else  
        WaitCommEvent(idComDev,&dwEvtMask,NULL);  //wait until any event is occurred.  
#endif  
          
       if( (dwEvtMask & EV_TXEMPTY) == EV_TXEMPTY )  
            txEmpty=TRUE;  
          
        if((dwEvtMask & EV_RXCHAR) == EV_RXCHAR)  
        {  
            if(rxFlag.Enable) {  
                do{   //Apr.28.2003:The caveat on MSDN,"Serial Communications in Win32" recommends while();  
                    if( nLength=ReadComBlock(rxBuf,MAX_Uart_SIZE) )  
                    {  
                        //                      if(nLength>=MAX_BLOCK_SIZE /*for debug*/)  
                       //                      {  
                        //                          EB_Printf("\n[ERROR1:nLength>MAX]\n");  
                        //                          rxFlag.Off1=0;  
                       //                          rxFlag.Off2=0;  
                        //                          rxFlag.Cnt=0;  
                        //                      }  
                        //                      else  
                        {  
                            rxFlag.Ok=0;  
                            for(i=0; i<nLength; i++){  
                                if(rxFlag.Cnt >= MAX_BLOCK_SIZE) {  
                                    /*EB_Printf("\n[ERROR2:nLength>MAX]\n");*/  
                                    rxFlag.Off1=0;  
                                   rxFlag.Off2=0;  
                                    rxFlag.Cnt=0;  
                                }  
                                rxFlag.Cnt++;  
                                RoundBuf[rxFlag.Off2++] = rxBuf[i];  
                                rxFlag.Off2 %= MAX_BLOCK_SIZE;  
                            }  
                            rxFlag.Ok=1;  
                        }  
                    }  
                }while(nLength);    
            }  
        }  
          
        // Clear OVERRUN condition.  
        // If OVERRUN error is occurred,the tx/rx will be locked.  
        if(dwEvtMask & EV_ERR)  
        {  
            COMSTAT comStat;  
            DWORD dwErrorFlags;  
            ClearCommError(idComDev,&dwErrorFlags,&comStat);  
            //EB_Printf("[DBG:EV_ERR=%x]\n",dwErrorFlags);  
        }  
    }     
    CloseHandle(os.hEvent);  
    _endthread();  
}  
  
//读取串口缓冲区  
int ReadComBuf(unsigned char *buf, int maxLen)  
{  
    unsigned int time=0;  
    unsigned int i, len;  
      
    if(!rxFlag.Enable)return 0;  
    while(rxFlag.Cnt < (unsigned int)maxLen){    //delay 50ms  
        Sleep(10);  
        if( time++ >= COM_OVERTIME )break;  
    }  
    if(0==rxFlag.Ok)return 0;  
      
    len =(rxFlag.Cnt > (unsigned int)maxLen)? maxLen:rxFlag.Cnt;  
    for(i=0; i<len; i++){  
        *(buf+i)= RoundBuf[rxFlag.Off1++];  
        rxFlag.Cnt--;  
        rxFlag.Off1 %= MAX_BLOCK_SIZE;  
    }  
    return len;  
}  
//写串口缓冲区  
int WriteComBuf(unsigned char *buf, int maxLen)  
{  
    void *txBlk;  
    DWORD txBlkSize;  
    DWORD temp;  
      
    while(txEmpty==FALSE);  
    txBlk=buf;  
    txBlkSize=maxLen;  
      
   //  Debug_Printf("[OK:WriteComBuf len=%d.]\n", maxLen);  
    //  for(temp=0; temp<maxLen; temp++) {  
   //      Debug_Printf(" %02x", *(buf+temp));  
    //  }  
      
    //txEmpty=FALSE; why needed??? this line should be removed.  
#if IS_OVERLAPPED_IO  
    WriteFile(idComDev,txBlk,txBlkSize,&temp,&osWrite);  
#else  
    WriteFile(idComDev,txBlk,txBlkSize,&temp,NULL);  
#endif  
    while(txEmpty==FALSE);  
    return TRUE;  
}  
  
  
//往串口写一个字节  
int WriteComByte(unsigned char byte)  
{  
    return WriteComBuf(&byte, 1);  
}  
//从串口读取一个字节  
int ReadComByte(unsigned char byte)  
{  
    return ReadComBuf(&byte, 1);  
}  
  
  
/******************************************************************************* 
434.* Fuc Name      :   PC_DelayXms 
435.* Input         :   num--延时的毫秒数 
436.* Return        :   void 
437.* Description   :   PC延时程序,毫秒级 
438.*******************************************************************************/  
#include <sys/timeb.h>  
#include <time.h>  
void PC_DelayXms(int num)  
{  
    LARGE_INTEGER  litmp;   
    LONGLONG       QPart1,QPart2;  
    double         dfMinus, dfFreq, dfTim;   
     
    QueryPerformanceFrequency(&litmp);   
    dfFreq = (double)litmp.QuadPart;        // 获得计数器的时钟频率  
    QueryPerformanceCounter(&litmp);   
    QPart1 = litmp.QuadPart;                // 获得初始值  
    do{  
       QueryPerformanceCounter(&litmp);   
        QPart2 = litmp.QuadPart;            // 获得中止值   
        dfMinus = (double)(QPart2-QPart1);   
        dfTim = dfMinus / dfFreq;           // 获得对应的时间值，单位为秒  
    }while(dfTim<(0.001*num));  
}  
  
int ReceiveMessage(unsigned char *pBuf, int maxlen, int time)  
{  
    int ret =0;  
    DWORD  startTime;  
    int nLen;  
  
    startTime=GetTickCount();  
    int len =0;  
    while(1)  
    {  
        //timeout  
        if(GetTickCount() -startTime >time)  
            return 0;  
  
        len =ReadComBuf(pBuf,maxlen-1);  
       if(len)  
       {  
            *(pBuf +len) ='\0';  
            return len;  
        }  
        else  
        {  
            Sleep(1);  
        }  
    }  
    return 0;  
}  
