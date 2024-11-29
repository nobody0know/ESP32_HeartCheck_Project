import sys
import socket
import struct
import threading
import pandas as pd
import time
import os
from collections import deque
from queue import Queue, Empty
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QVBoxLayout, QWidget, QPushButton,
    QLabel, QLineEdit, QTextEdit, QSpinBox, QFormLayout, QHBoxLayout
)
from concurrent.futures import ThreadPoolExecutor
from PyQt5.QtCore import Qt, pyqtSignal, QObject, QTimer
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure

# 全局运行标志
running = False
connected_to_station = False  # 添加连接标志

# 信号类，用于在线程间传递消息
class SignalHandler(QObject):
    log_signal = pyqtSignal(str)

signal_handler = SignalHandler()

# 解析基站到电脑的报文
def parse_message(message):
    if len(message) < 20:
        return None
    try:
        header, device_id = struct.unpack('<BB', message[:2])
    except struct.error:
        return None
    if header != 170:
        return None

    data_points = []
    offset = 3
    for i in range(100):
        try:
            timestamp, adc_value = struct.unpack('<IH', message[offset:offset+6])
            data_points.append((time.time(), adc_value))  # 使用当前时间作为时间戳
            offset += 6
            # signal_handler.log_signal.emit(f"  Data {data_num}: Timestamp = {timestamp}, ADC Value = {adc_value}")
        except struct.error:
            break
    return device_id, data_points

def process_data(queue, data_storage, device_id, lock):
    buffer = []
    buffer_limit = 1000

    while running or not queue.empty():
        try:
            data_points = queue.get(timeout=1)
            if data_points is None:
                break
            buffer.extend(data_points)
            if len(buffer) >= buffer_limit:
                with lock:
                    # Debugging log
                    print(f"Buffer size: {len(buffer)}")
                    for timestamp, adc_value in buffer:
                        data_storage.append({"Timestamp": timestamp, "ADC Value": adc_value})
                buffer.clear()
        except Empty:
            continue

    if buffer:
        with lock:
            for timestamp, adc_value in buffer:
                data_storage.append({"Timestamp": timestamp, "ADC Value": adc_value})

    # Debugging log to check if data_storage is populated
    print(f"Device {device_id} data_storage length: {len(data_storage)}")


# 从 UDP 读取并解析数据
def read_from_udp(udp_port, device_id, queue):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('', udp_port))
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
    sock.settimeout(1)
    signal_handler.log_signal.emit(f"Listening on UDP port {udp_port} for device {device_id}")
    try:
        while running:
            try:
                data, _ = sock.recvfrom(2000)
                parsed_data = parse_message(data)
                if parsed_data:
                    _, data_points = parsed_data
                    queue.put(data_points)
            except socket.timeout:
                continue
    finally:
        sock.close()
        queue.put(None)

# 保存数据到文件
def save_data_to_csv(data_storage, device_id, output_dir):
    print(f"Saving data for device {device_id}...")  # 调试输出
    if not data_storage:
        print(f"No data to save for device {device_id}.")  # 检查是否有数据
    filename = os.path.join(output_dir, f"device_{device_id}_output.csv")
    df = pd.DataFrame(data_storage)
    df.to_csv(filename, index=False)
    print(f"Final data saved to {filename}.")

from collections import deque
import numpy as np

class RealtimePlot(FigureCanvas):
    def __init__(self, device_id, parent=None, window_size=10000):
        self.device_id = device_id
        self.fig = Figure()
        self.ax = self.fig.add_subplot(111)
        self.ax.clear()  # 清除当前子图的内容
        super().__init__(self.fig)
        self.setParent(parent)

        self.ax.set_title(f"Device {device_id} Real-Time Data")
        self.ax.set_xlabel("Data Points")
        self.ax.set_ylabel("ADC Value")

        self.line, = self.ax.plot([], [], lw=2)

        # 使用 deque 来存储数据，最大长度为 window_size，自动移除最旧的数据
        self.data = deque(maxlen=window_size)  
        self.time_stamp = 0  
        self.data_points_num = 0
        self.background = None

    def update_plot(self, data_points):
        """接收新的 data_points 数据，并更新图表"""
        processed_data = []
        if isinstance(data_points, list):
            for point in data_points:
                if isinstance(point, dict):
                    timestamp = point.get('Timestamp')
                    adc_value = point.get('ADC Value')
                    if isinstance(timestamp, (int, float)) and isinstance(adc_value, (int, float)):
                        processed_data.append(adc_value)  # 只存储 ADC 值

        if not processed_data:
            return

        # 将新数据添加到绘图数据中，超出窗口大小的最旧数据会被自动删除
        self.data.extend(processed_data)

        # 使用滑动窗口存储的数据绘制图形
        x_data = np.arange(len(self.data))  # 使用数据点的相对索引作为横轴
        y_data = list(self.data)  # 直接将 deque 中的数据转换为列表

        # 更新绘图数据
        self.line.set_data(x_data, y_data)

        # 检查数据点数量是否超过 1000，超过则更新 self.data_points_num
        if len(self.data) > 2000:
            self.data_points_num = len(self.data) - 2000  # 保持最多显示 1000 个数据点

        # X 轴范围：显示最新的 1000 个数据点
        self.ax.set_xlim(self.data_points_num, len(self.data))  # X 轴范围按数据点的数量动态调整
        self.ax.set_ylim(min(y_data) - 10, max(y_data) + 10)  # 动态调整 Y 轴范围

        # 使用 blit 来加速渲染
        if self.background is None:
            self.background = self.fig.canvas.copy_from_bbox(self.ax.bbox)

        self.fig.canvas.restore_region(self.background)
        self.ax.draw_artist(self.line)
        self.fig.canvas.blit(self.ax.bbox)






# GUI 主窗口
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("UDP Data Logger with Realtime Plotting")
        self.setGeometry(300, 100, 1200, 800)
        self.threads = []
        self.queues = {}
        self.data_storages = {}
        self.locks = {}
        self.plots = {}
        self.init_ui()


    def init_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        layout = QVBoxLayout(central_widget)

        # 基站连接部分
        self.station_ip_input = QLineEdit()
        self.station_ip_input.setPlaceholderText("Enter Base Station IP")
        self.connect_button = QPushButton("Connect to Base Station")
        self.connect_button.clicked.connect(self.connect_to_station)
        
        layout.addWidget(self.station_ip_input)
        layout.addWidget(self.connect_button)

        # 表单区域
        form_layout = QFormLayout()
        self.device_count_spinbox = QSpinBox()
        self.device_count_spinbox.setRange(1, 50)
        self.device_count_spinbox.setValue(1)
        self.base_port_input = QLineEdit("10000")
        form_layout.addRow("Device Count:", self.device_count_spinbox)
        form_layout.addRow("Base Port:", self.base_port_input)
        layout.addLayout(form_layout)

        # 实时绘图区域
        self.plot_area = QVBoxLayout()
        layout.addLayout(self.plot_area)

        # 日志显示
        self.log_display = QTextEdit()
        self.log_display.setReadOnly(True)
        layout.addWidget(QLabel("Logs:"))
        layout.addWidget(self.log_display)

        # 按钮区域
        button_layout = QHBoxLayout()
        self.start_button = QPushButton("Start")
        self.start_button.clicked.connect(self.start_logging)
        self.stop_button = QPushButton("Stop")
        self.stop_button.clicked.connect(self.stop_logging)
        self.stop_button.setEnabled(False)
        button_layout.addWidget(self.start_button)
        button_layout.addWidget(self.stop_button)
        layout.addLayout(button_layout)

        # 信号连接
        signal_handler.log_signal.connect(self.append_log)

    def append_log(self, message):
        self.log_display.append(message)

    def start_logging(self):
        global running
        if not connected_to_station:
            self.append_log("Please connect to the base station first.")
            return
            
        global running
        if running:
            self.append_log("Logging is already running.")
            return

        running = True
        self.start_button.setEnabled(False)
        self.stop_button.setEnabled(True)

        device_count = self.device_count_spinbox.value()
        base_port = int(self.base_port_input.text())
        output_dir = "output_data"
        os.makedirs(output_dir, exist_ok=True)

        for device_id in range(1, device_count+1):
            udp_port = base_port + device_id
            queue = Queue()
            self.queues[device_id] = queue
            self.data_storages[device_id] = []
            self.locks[device_id] = threading.Lock()

            print(f"add device {device_count}")

            if device_id not in self.plots:
                plot = RealtimePlot(device_id)
                self.plot_area.addWidget(plot)
                self.plots[device_id] = plot

            udp_thread = threading.Thread(target=read_from_udp, args=(udp_port, device_id, queue))
            udp_thread.daemon = True
            udp_thread.start()
            self.threads.append(udp_thread)

            process_thread = threading.Thread(target=process_data, args=(queue, self.data_storages[device_id], device_id, self.locks[device_id]))
            process_thread.daemon = True
            process_thread.start()
            self.threads.append(process_thread)

        self.timer = QTimer()
        self.timer.timeout.connect(self.update_plots)
        self.timer.start(10)  # 每 100 毫秒更新一次
        self.append_log(f"Started logging for {device_count} devices starting at port {base_port}.")

    def connect_to_station(self):
        global connected_to_station
        station_ip = self.station_ip_input.text().strip()

        if not station_ip:
            self.append_log("Please enter a valid base station IP address.")
            return

        # 发送UDP连接请求
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.sendto(b"Connection Request", (station_ip, 10000))
            sock.settimeout(20)  # 设置超时
            sock.recvfrom(1024)  # 等待确认报文（假设基站会回应确认连接）
            connected_to_station = True
            self.append_log(f"Connected to base station at {station_ip}.")
            sock.close()
            self.connect_button.setEnabled(False)  # 禁用连接按钮
        except Exception as e:
            connected_to_station = False
            self.append_log(f"Failed to connect to base station: {e}")

    def update_plots(self):
        for device_id, plot in self.plots.items():
            with self.locks[device_id]:
                data_for_plot = self.data_storages[device_id][:]  # 创建绘图数据副本
            plot.update_plot(data_for_plot)  # 使用副本更新绘图

    def stop_logging(self):
        global running
        running = False
        self.append_log("Stopping logging...")

        for device_id, queue in self.queues.items():
            queue.put(None)

        for thread in self.threads:
            thread.join()

        self.timer.stop()
        self.append_log("All threads stopped. Saving data...")

        with ThreadPoolExecutor() as executor:
            futures = [
                executor.submit(save_data_to_csv, self.data_storages[device_id], device_id, "output_data")
                for device_id in self.data_storages
            ]
            for future in futures:
                try:
                    future.result()  # 等待任务完成并捕获异常
                except Exception as e:
                    print(f"Error saving data: {e}")
                future.result()

        self.append_log("All data saved. Logging stopped.")
        self.start_button.setEnabled(True)
        self.stop_button.setEnabled(False)



# 启动程序
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
