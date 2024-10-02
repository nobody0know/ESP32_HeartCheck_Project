import serial
import struct
from datetime import datetime
import pandas as pd
import time

# 初始化一个空的DataFrame，用于存储数据
data_frame = pd.DataFrame()

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

    for i in range(5):  # 循环解析5组（时间戳 + ADC 数据）
        if offset + 8 > len(message):
            break  # 如果剩余数据不足以解析，跳出循环
        try:
            timestamp, adc_value = struct.unpack('<If', message[offset:offset+8])
        except struct.error:
            continue  # 如果数据不完整或有错误，继续下一轮解析

        # timestamp = datetime.utcfromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')
        data_points.append((timestamp, adc_value))
        offset += 8

    return device_id, data_points

# 更新DataFrame
def update_dataframe(device_id, data_points):
    global data_frame
    for timestamp, adc_value in data_points:
        data_frame.at[timestamp, f'Device {device_id}'] = adc_value

# 从串口读取并解析数据
def read_from_serial(port, baudrate):
    ser = None
    while ser is None:
        try:
            ser = serial.Serial(port, baudrate, timeout=1)
        except serial.SerialException:
            print(f"Unable to connect to serial port {port}. Retrying in 5 seconds...")
            time.sleep(5)
    buffer = b''  # 用来存放接收的报文数据
    try:
        while True: # 无限循环，持续监听串口
            if ser.in_waiting:  # 检查是否有数据等待读取
                data = ser.read(ser.in_waiting)  # 读取所有等待的数据
                buffer += data  # 将读取的数据添加到缓冲区

                # 寻找换行符，表示报文结束
                while b'\n' in buffer:
                    frame_end = buffer.index(b'\n')
                    message_hex = buffer[:frame_end].strip()  # 读取一帧数据
                    buffer = buffer[frame_end+1:]  # 从缓冲区中移除这一帧数据
                    # print(f"msg len is {len(message_hex)}")
                    if len(message_hex) == 90:
                        message = bytes.fromhex(message_hex.decode())  # 将十六进制字符串转换为二进制数据
                        parsed_data = parse_message(message)
                        if parsed_data:
                            device_id, data_points = parsed_data
                            update_dataframe(device_id, data_points)  # 更新DataFrame
                            print(f"Device {device_id}:")
                            for i, (timestamp, adc_value) in enumerate(data_points, 1):
                                print(f"  Data {i}: Timestamp = {timestamp}, ADC Value = {adc_value}")
                        else:
                            print("Failed to parse message.") 
                            
    except KeyboardInterrupt:
        print("\nProgram terminated by user (Ctrl + C).")
    finally:
        ser.close()  # 确保退出时关闭串口
        # 保存DataFrame到Excel文件
        data_frame.to_excel("output.xlsx")

if __name__ == "__main__":
    port = 'COM21'  # 根据实际串口号调整
    baudrate = 115200  # 根据实际波特率调整

    # 读取串口数据并解析
    read_from_serial(port, baudrate)
