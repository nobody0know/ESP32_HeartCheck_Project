import socket

# 设置服务器的IP地址和端口号
UDP_IP = "192.168.4.54"  # 接收所有IP地址发来的数据
UDP_PORT = 12345     # 端口号，可与ESP32的发送端口号相对应

# 创建一个UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening on {UDP_IP}:{UDP_PORT}...")

while True:
    # 接收UDP信息
    data, addr = sock.recvfrom(1024)  # 1024字节的缓冲区
    print(f"Received message: {data.decode('utf-8')} from {addr}")
