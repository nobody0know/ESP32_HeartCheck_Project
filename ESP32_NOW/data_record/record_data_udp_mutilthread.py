import socket
import struct
import pandas as pd
import threading
import sys
import signal

# 全局运行标志
running = True

# 解析基站到电脑的报文
def parse_message(message):
    if len(message) < 20:  # 确保数据长度至少为20字节
        return None

    # 解包报文内容
    try:
        header, device_id = struct.unpack('<BB', message[:2])
    except struct.error:
        return None  # 如果数据长度不正确，解析会失败

    if header != 170:  # 检查头部是否为0xAA（十六进制的'AA'）
        return None

    data_points = []
    offset = 3

    for i in range(100):  # 循环解析5组（时间戳 + ADC 数据）
        if offset + 6 > len(message):
            break  # 如果剩余数据不足以解析，跳出循环
        try:
            timestamp, adc_value = struct.unpack('<IH', message[offset:offset+6])
        except struct.error:
            continue  # 如果数据不完整或有错误，继续下一轮解析

        # timestamp = datetime.utcfromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')
        data_points.append((timestamp, adc_value))
        offset += 6

    return device_id, data_points
# 更新DataFrame
def update_dataframe(device_id, data_points,data_frame):
    for timestamp, adc_value in data_points:
        data_frame.at[timestamp, f'Device {device_id}'] = adc_value

# 从UDP读取并解析数据
def read_from_udp(udp_port, device_id):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('', udp_port))
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)  # 增加缓冲区
    sock.settimeout(1)
    print(f"Listening on UDP port {udp_port} for device {device_id}")

    data_frame = pd.DataFrame()

    try:
        while running:
            try:
                data, addr = sock.recvfrom(2000)
                if data:
                    parsed_data = parse_message(data)
                    if parsed_data:
                        device_id, data_points = parsed_data
                        update_dataframe(device_id, data_points,data_frame)  # 更新DataFrame
                        print(f"Device {device_id}:")
                        for i, (timestamp, adc_value) in enumerate(data_points, 1):
                            print(f"  Data {i}: Timestamp = {timestamp}, ADC Value = {adc_value}")
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

    device_ids = range(1, 2)  # 假设有15个设备
    base_port = 10001
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
