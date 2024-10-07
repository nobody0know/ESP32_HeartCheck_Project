import socket
import struct
from datetime import datetime
import pandas as pd
import time
import signal
import sys

# 初始化一个空的DataFrame，用于存储数据
data_frame = pd.DataFrame()

# 解析基站到电脑的报文
def parse_message(message):
    if len(message) < 20:  # 确保数据长度至少为20字节
        print("len too short")
        return None

    # 解包报文内容
    try:
        header, device_id = struct.unpack('<BB', message[:2])
    except struct.error:
        print("len error")
        return None  # 如果数据长度不正确，解析会失败

    if header != 170:  # 检查头部是否为0xAA（十六进制的'AA'）
        print("head word fault")
        return None

    data_points = []
    offset = 3

    for i in range(25):  # 循环解析5组（时间戳 + ADC 数据）
        if offset + 6 > len(message):
            break  # 如果剩余数据不足以解析，跳出循环
        try:
            timestamp, adc_value = struct.unpack('<IH', message[offset:offset+6])
        except struct.error:
            continue  # 如果数据不完整或有错误，继续下一轮解析

        data_points.append((timestamp, adc_value))
        offset += 6

    return device_id, data_points

# 更新DataFrame
def update_dataframe(device_id, data_points):
    global data_frame
    temp_df = pd.DataFrame(data_points, columns=['Timestamp', f'Device {device_id}'])
    temp_df.set_index('Timestamp', inplace=True)

    if data_frame.empty:
        data_frame = temp_df
    else:
        # 使用combine_first来合并数据
        data_frame = data_frame.combine_first(temp_df)

    data_frame.sort_index(inplace=True)


# 从UDP读取并解析数据
def read_from_udp(udp_port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('', udp_port))  # 绑定端口
    sock.settimeout(1)  # 设置1秒超时以允许定期检查是否应该退出
    print(f"Listening on UDP port {udp_port}")

    running = True  # 控制循环的变量
    try:
        while running:  # 无限循环，持续监听UDP
            try:
                data, addr = sock.recvfrom(300)  
                if data:
                    parsed_data = parse_message(data)
                    if parsed_data:
                        device_id, data_points = parsed_data
                        update_dataframe(device_id, data_points)
                        print(f"Device {device_id}:")
                        for i, (timestamp, adc_value) in enumerate(data_points, 1):
                            print(f"  Data {i}: Timestamp = {timestamp}, ADC Value = {adc_value}")
            except socket.timeout:
                pass  # 捕获超时，不做任何事情，继续循环
    except KeyboardInterrupt:
        print("\nProgram terminated by user (Ctrl + C).")
        running = False  # 设置为False，以结束循环
    finally:
        sock.close()  # 确保退出时关闭套接字
        print("Socket closed.")
        # 保存DataFrame到Excel文件
        data_frame.to_excel("output.xlsx")
        print("Data saved to Excel.")


if __name__ == "__main__":
    udp_port = 10001  # 定义UDP端口号
    # 读取UDP数据并解析
    read_from_udp(udp_port)
    
