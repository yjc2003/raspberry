# -*- coding: utf8 -*- 
  
# socket 모듈을 임포트 
from socket import * 
from select import select 
import sys 
 
# 호스트, 포트와 버퍼 사이즈를 지정 
HOST = '127.0.0.1' 
PORT = 8800 
BUFSIZE = 1024 
ADDR = (HOST, PORT) 
 
# 소켓 객체를 만들고 
clientSocket = socket(AF_INET, SOCK_STREAM) 
  
# 서버와의 연결을 시도 
try: 
    clientSocket.connect(ADDR) 
except Exception as e: 
    print('서버(%s:%s)에 연결 할 수 없습니다.' % ADDR) 
    sys.exit() 
print('서버(%s:%s)에 연결 되었습니다.' % ADDR) 

''' 
def prompt(): 
    sys.stdout.write('{"id":1,"type":"temp","method":"get","index":0}') 
    sys.stdout.flush() 
'''
 
# 무한 루프를 시작 
while True: 
    try: 
        connection_list = [sys.stdin, clientSocket] 
        read_socket, write_socket, error_socket = select(connection_list, [], [], 10) 

        for sock in read_socket: 
            if sock == clientSocket: 
                data = sock.recv(BUFSIZE) 
                if not data: 
                    print('서버(%s:%s)와의 연결이 끊어졌습니다.' % ADDR) 
                    clientSocket.close() 
                    sys.exit() 
                else: 
                    print('received from server : %s' % data)  # 서버에서 읽은 값 출력
                    #prompt() 
            else: 
                message = sys.stdin.readline()
                clientSocket.send(message) # 콘솔에서 읽은 값 서버로 전송
                #prompt() 
    except KeyboardInterrupt: 
        clientSocket.close() 
        sys.exit() 

