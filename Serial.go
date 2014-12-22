// Serial project Serial.go
package Serial

/*
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "com.h"
int Search(char* com){
	FILE *fp;
	if((fp=fopen(com,"r"))==NULL){
		//fclose(fp);
		return 0;
	}else{
		fclose(fp);
		return 1;
	}
}
char* ReadS(){
	char RBUF[1<<12];

	while(1){
		memset(RBUF,0,1<<12);
		int ret=ReadComBuf(RBUF, 1<<12);
		//printf("AFt GetC %s >>>%d\n",RBUF,ret);

		if(ret>0) break;
		Sleep(100);
	}

	return RBUF;
}
int WriteS(char *WBUF,int maxLen){
	return WriteComBuf(WBUF, maxLen);
}
*/
import "C"
import "strconv"

//var readable bool=true
func SearchPort() []string {
	bye := []string{}
	for i := 0; i < 50; i++ {
		var cstr *C.char
		if i < 9 {
			cstr = C.CString("COM" + strconv.Itoa(i+1))
		} else {
			cstr = C.CString("\\\\.\\COM" + strconv.Itoa(i+1))
		}

		if p := C.Search(cstr); p > 0 {
			bye = append(bye, strconv.Itoa(i+1))
		}
	}
	return bye
}
func OpenComPort(port, band int) int {
	cport, cband := C.int(port), C.int(band)
	return int(C.OpenComPort(cport, cband))
}

func SendData(data string) int {
	//defer readable=true
	//readable=false
	return int(C.WriteS(C.CString(data), C.int(len(data)+1)))
}

func ReciveData() string {
	return C.GoString(C.ReadS())
}

func ReciveChData() <-chan string {
	ch := make(chan string, 1)
	go func() {
		ch <- C.GoString(C.ReadS())
	}()
	return ch
}
func CloseComPort() {
	C.CloseComPort()
}
