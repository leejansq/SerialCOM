#ifndef __COM_H__  
#define __COM_H__  
#ifdef __cplusplus  
extern "C" {  
#endif   
//打开串口  
int OpenComPort(int port, int userBaudRate);//打开串口  
//关闭串口  
void CloseComPort(void);//关闭串口  
int ReadComBlock(unsigned char *buf,int maxLen);  
void _DoRxTx(void *args);  
  
int ReadComBuf(unsigned char *buf, int maxLen);//读串口  
int WriteComBuf(unsigned char *buf, int maxLen);//写串口  
int WriteComByte(unsigned char byte);//往串口写一个字节  
int ReadComByte(unsigned char byte);//从串口读取一个字节  
void resetComPort(void);//重置串口  
void resetRTS(void);//resetRTS_sub(150,20);  
int ReceiveMessage(unsigned char *pBuf, int maxlen, int time);//带时间的接收操作  
void resetRTSLowHigh(int Low,int High);//CTS高低电平操作  
//void SetRTSControlEnable(bool enable);//CTS操作:enable=true:上拉,false:下拉  
  
#ifdef __cplusplus  
}  
#endif   
  
#endif  
