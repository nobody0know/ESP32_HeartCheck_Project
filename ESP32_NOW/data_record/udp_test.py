import socket
import binascii

def print_udp_stream(udp_port):
    # 创建一个socket对象
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # 绑定到指定端口
    sock.bind(('', udp_port))
    print(f"Listening on UDP port {udp_port}")

    try:
        while True:
            # 接收数据
            data, addr = sock.recvfrom(1024)  # 1024字节的缓冲区大小
            if data:
                # 打印接收到的数据的来源和十六进制表示
                print(f"Received from {addr}: {binascii.hexlify(data).decode('utf-8')}")
    except KeyboardInterrupt:
        print("\nProgram terminated by user.")
    finally:
        sock.close()
        print("Socket closed.")

if __name__ == "__main__":
    udp_port = 10000  # 可以修改为你希望监听的端口号
    print_udp_stream(udp_port)
    # aa b5 09 01 00 2d 01 39 00 0e 00 000000000000000000