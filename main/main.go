// PC_Testing.go project main.go
package main

/*
   与FLEX通讯协议：

    <<< command:data
		0.search:any 查找可用串口
		1.com:$num  打开串口$num
		2.test:any  开始测试
		3.close:any 关闭串口

    >>> command:info
		0.search:port1/port2/... 返回可用串口
		1.com:$sate 串口打开成功 $sate=”ok“  否则 ”fail“
		2.test:$info $info返回串口通讯的信息
		3.close:$sate 串口关闭成功 $sate=”ok“  否则 ”fail“
*/
import (
	"Serial"
	"log"
	"net"
	"strconv"
	"strings"
)

func readloop(conn net.Conn, closeCh <-chan bool) {
	for {
		select {
		case <-closeCh:
			return
		case msg := <-Serial.ReciveChData():
			log.Println("<<< " + msg)
			conn.Write([]byte("test$@%" + msg))
		}
	}

	//for {
	//	s := Serial.ReciveData()
	//	log.Println("<<< " + s)
	//	conn.Write([]byte("test:" + s))
	//	if len(closeCh) > 0 {
	//		<-closeCh
	//		return
	//	}
	//	//select {
	//	//case <-closeCh:
	//	//	return
	//	//default:
	//	//}
	//}
}

func connloop(conn net.Conn) {
	defer conn.Close()
	var flag_Init bool
	var loopFlag bool
	ch := make(chan bool, 1)
	for {
		b := make([]byte, 1<<8)
		n, err := conn.Read(b)
		if err != nil {
			log.Println(err)
			break
		}
		cmd := string(b[:n])
		log.Println(cmd)
		cdarr := strings.SplitN(cmd, ":", 2)
		switch cdarr[0] {
		case "search":
			if loopFlag == true {
				ch <- true
				loopFlag = false
			}
			plist := Serial.SearchPort()
			var sl string
			if len(plist) > 0 {
				sl = strings.Join(plist, "/")
			} else {
				sl = "0"
			}
			_, err = conn.Write([]byte("search$@%" + sl))
			if err != nil {
				break
			}
		case "com":
			if loopFlag == true {
				ch <- true
				loopFlag = false
			}
			port, err := strconv.Atoi(cdarr[1])
			if err != nil {
				log.Println(err)
				break
			}
			if i := Serial.OpenComPort(port, 115200); i > 0 {
				flag_Init = true
				_, err = conn.Write([]byte("com$@%ok"))
				if err != nil {
					break
				}
			} else {
				flag_Init = false
				_, err = conn.Write([]byte("com$@%fail"))
				if err != nil {
					break
				}
			}
		case "test":
			if flag_Init == true {
				if loopFlag == false {
					go readloop(conn, ch)
					loopFlag = true
				}
				log.Println(cdarr[1])
				Serial.SendData(cdarr[1])
				//s := Serial.ReciveData()
				//log.Println(s)
				//conn.Write([]byte("test:" + s))
			}

		case "close":
			if loopFlag == true {
				ch <- true
				loopFlag = false
			}
			if flag_Init == true {
				Serial.CloseComPort()
				_, err = conn.Write([]byte("close$@%ok"))
				if err != nil {
					break
				}
				flag_Init = false
			}

		}
	}

}
func listenloop() {
	listen, err := net.Listen("tcp", ":8383")
	if err != nil {
		log.Println(err)
		return
	}
	for {
		conn, err := listen.Accept()
		if err != nil {
			log.Println(err)
			continue
		}
		go connloop(conn)
	}

}
func main() {
	//log.Println(Serial.SearchPort())
	listenloop()
}
