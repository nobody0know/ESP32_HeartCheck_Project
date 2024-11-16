import socket
import struct
import pandas as pd
import threading
import sys
import signal
import serial  # 串口通信模块
import time

# 全局运行标志
running = True

# 获取本机IP地址
def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # 连接到外部IP地址获取本机IP（不会真的发送数据）
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
    finally:
        s.close()
    return ip

# 通过串口发送本机IP地址
def send_local_ip_to_serial(serial_port, baudrate):
    try:
        ser = serial.Serial(serial_port, baudrate, timeout=1)
        local_ip = get_local_ip()
        ser.write(f"IP:{local_ip}\n".encode())
        print(f"Sent IP to serial: {local_ip}")
        ser.close()
    except Exception as e:
        print(f"Error sending IP to serial: {e}")

# 从串口接收设备数信息
def receive_device_count_from_serial(serial_port, baudrate):
    try:
        ser = serial.Serial(serial_port, baudrate, timeout=1)
        print("Waiting for device count from serial...")
        while running:
            line = ser.readline().decode().strip()
            if line.startswith("Devices:"):
                device_count = int(line.split(":")[1])
                print(f"Received device count: {device_count}")
                ser.close()
                return device_count
        ser.close()
    except Exception as e:
        print(f"Error receiving device count from serial: {e}")
        return 0

# 解析基站到电脑的报文
def parse_message(message):
    if len(message) < 20:  # 确保数据长度至少为20字节
        return None

    try:
        header, device_id = struct.unpack('<BB', message[:2])
    except struct.error:
        return None

    if header != 170:  # 检查头部是否为0xAA（十六进制的'AA'）
        return None

    data_points = []
    offset = 3

    for i in range(25):  # 循环解析5组（时间戳 + ADC 数据）
        if offset + 8 > len(message):
            break
        try:
            timestamp, adc_value = struct.unpack('<IH', message[offset:offset+6])
        except struct.error:
            continue

        data_points.append((timestamp, adc_value))
        offset += 6

    return device_id, data_points

# 更新DataFrame
def update_dataframe(device_id, data_points, data_frame):
    for timestamp, adc_value in data_points:
        data_frame.at[timestamp, f'Device {device_id}'] = adc_value

# 从UDP读取并解析数据
def read_from_udp(udp_port, device_id):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('', udp_port))
    sock.settimeout(1)
    print(f"Listening on UDP port {udp_port} for device {device_id}")

    data_frame = pd.DataFrame()

    try:
        while running:
            try:
                data, addr = sock.recvfrom(300)
                if data:
                    parsed_data = parse_message(data)
                    if parsed_data:
                        device_id, data_points = parsed_data
                        update_dataframe(device_id, data_points, data_frame)
                        print(f"Device {device_id}:")
                    else:
                        print("Failed to parse message.")
            except socket.timeout:
                continue
    finally:
        sock.close()
        filename = f"device_{device_id}_output.xlsx"
        data_frame.to_excel(filename)
        print(f"Data saved to {filename}.")

def signal_handler(signum, frame):
    global running
    running = False
    print("Interrupt received, stopping threads...")

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)

    # 串口配置
    serial_port = "COM3"  # 根据实际情况修改
    baudrate = 9600

    # 发送本机IP地址
    send_local_ip_to_serial(serial_port, baudrate)

    # 接收设备数
    device_count = receive_device_count_from_serial(serial_port, baudrate)

    # 启动数据采集线程
    device_ids = range(1, device_count + 1)  # 动态获取设备ID数量
    base_port = 10000
    threads = []

    for device_id in device_ids:
        udp_port = base_port + device_id
        thread = threading.Thread(target=read_from_udp, args=(udp_port, device_id))
        thread.daemon = True
        thread.start()
        threads.append(thread)

    while any(t.is_alive() for t in threads):
        try:
            [t.join(1) for t in threads if t.is_alive()]
        except KeyboardInterrupt:
            print("Keyboard interrupt in main, stopping...")
            running = False
