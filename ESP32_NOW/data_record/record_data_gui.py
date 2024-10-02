import tkinter as tk
from tkinter import ttk
import serial
import threading
import struct
from datetime import datetime
import serial.tools.list_ports
from datetime import datetime
import pandas as pd
import time

#pyinstaller --onefile record_data_gui.py 

# 初始化一个空的DataFrame，用于存储数据
data_frame = pd.DataFrame()

class SerialApp:
    def __init__(self, master):
        self.master = master
        master.title("串口通信")

        # 当窗口关闭时执行清理操作
        master.protocol("WM_DELETE_WINDOW", self.on_close)

        # Serial connection
        self.ser = None

        # Frame for controls
        self.frame_controls = ttk.Frame(master)
        self.frame_controls.pack(padx=10, pady=10)

        # Dropdown Menu for Serial Ports
        self.serial_var = tk.StringVar()
        self.port_label = ttk.Label(self.frame_controls, text="选择串口:")
        self.port_label.grid(column=0, row=0, sticky='w')
        self.port_combobox = ttk.Combobox(self.frame_controls, textvariable=self.serial_var, state='readonly')
        self.port_combobox.grid(column=1, row=0)
        self.refresh_ports()

        # Refresh Button
        self.refresh_button = ttk.Button(self.frame_controls, text="刷新串口列表", command=self.refresh_ports)
        self.refresh_button.grid(column=2, row=0)

        # Start Button
        self.start_button = ttk.Button(self.frame_controls, text="开始读取", command=self.start_reading)
        self.start_button.grid(column=3, row=0)

        # Text Field for logs
        self.log = tk.Text(master, height=20, width=80)
        self.log.pack(padx=10, pady=10)

    def refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        self.port_combobox['values'] = [port.device for port in ports]
        if ports:
            self.port_combobox.current(0)

    def start_reading(self):
        selected_port = self.serial_var.get()
        if selected_port:
            self.log.insert(tk.END, f"Selected port: {selected_port}\n")
            threading.Thread(target=self.read_from_serial, args=(selected_port,), daemon=True).start()

    def on_close(self):
        if self.ser and self.ser.is_open:
            self.ser.close()
        # 保存DataFrame到Excel文件
        data_frame.to_excel("output.xlsx")
        self.master.destroy()

    # 更新DataFrame
    def update_dataframe(device_id, data_points):
        global data_frame
        for timestamp, adc_value in data_points:
            data_frame.at[timestamp, f'Device {device_id}'] = adc_value

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

    def read_from_serial(self, port):
        ser = serial.Serial(port, 115200, timeout=1)
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
                            parsed_data = SerialApp.parse_message(message)
                            if parsed_data:
                                device_id, data_points = parsed_data
                                SerialApp.update_dataframe(device_id, data_points)  # 更新DataFrame
                                print(f"Device {device_id}:")
                                self.log.insert(tk.END, f"Device {device_id}:\n")
                                for i, (timestamp, adc_value) in enumerate(data_points, 1):
                                    print(f"  Data {i}: Timestamp = {timestamp}, ADC Value = {adc_value}")
                                    self.log.insert(tk.END, f"  Data {i}: Timestamp = {timestamp}, ADC Value = {adc_value}\n")
                            else:
                                print("Failed to parse message.") 
                                self.log.insert(tk.END, "Failed to parse message.\n")
                        else : 
                            print(f"msg length is error.")
                            self.log.insert(tk.END, "msg length is error.\n")
        except KeyboardInterrupt:
            print("\nProgram terminated by user (Ctrl + C).")
        finally:
            ser.close()  # 确保退出时关闭串口
            # 保存DataFrame到Excel文件
            data_frame.to_excel("output.xlsx")




if __name__ == "__main__":
    root = tk.Tk()
    app = SerialApp(root)
    root.mainloop()
