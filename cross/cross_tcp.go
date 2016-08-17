package main

//flash 跨域专用

import (
	"flag"
	"fmt"
	"net"
	"os"
)

var host = flag.String("host", "", "host")
var port = flag.String("port", "1881", "port")

func main() {
	flag.Parse()
	var l net.Listener
	var err error
	l, err = net.Listen("tcp", *host+":"+*port)
	if err != nil {
		fmt.Println("Error listening:", err)
		os.Exit(1)
	}
	defer l.Close()
	fmt.Println("Listening on " + *host + ":" + *port)
	for {
		conn, err := l.Accept()
		if err != nil {
			fmt.Println("Error accepting: ", err)
			os.Exit(1)
		}
		//logs an incoming message
		fmt.Printf("Received message %s -> %s \n", conn.RemoteAddr(), conn.LocalAddr())
		// Handle connections in a new goroutine.
		go handleRequest(conn)
	}
}
func handleRequest(conn net.Conn) {
	defer conn.Close()
	buf := make([]byte, 1024)
	reqLen, err := conn.Read(buf)
	if err != nil {
		fmt.Println("Error to read message because of ", err)
		return
	}
	fmt.Println(string(buf[:reqLen-1]))

	data := `<?xml version="1.0"?>
<cross-domain-policy>
  <allow-access-from domain="*"  to-ports="1883"/>
</cross-domain-policy>`

	_, err = conn.Write([]byte(data))
	if err != nil {
		fmt.Println("Error to send message because of ", err)
		return
	}

}
