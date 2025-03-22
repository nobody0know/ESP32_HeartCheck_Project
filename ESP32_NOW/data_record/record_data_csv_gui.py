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
import matplotlib.pyplot as plt
from PyQt5.QtWidgets import QComboBox
from PyQt5.QtWidgets import QSizePolicy


# 全局运行标志
running = False
connected_to_station = False  # 添加连接标志
max_data_count = 100000  # 设置最大数据量阈值

# 信号类，用于在线程间传递消息
class SignalHandler(QObject):
    log_signal = pyqtSignal(str)

signal_handler = SignalHandler()

def clear_data_by_count(data_storage, max_data_count=10000):
    if len(data_storage) > max_data_count:
        # 删除最旧的数据（即队列中的前面部分）
        excess_count = len(data_storage) - max_data_count
        data_storage[:] = data_storage[excess_count:]  # 保留最新的数据
        print(f"Data storage cleaned. Remaining data points: {len(data_storage)}")

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
            data_points.append((timestamp, adc_value))  # 使用当前时间作为时间戳
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
                    if len(data_storage) >= max_data_count:
                        # 提取将要删除的数据
                        data_to_save = data_storage[:len(data_storage) - max_data_count]
                        # 保存数据到文件
                        save_data_to_csv(data_to_save, device_id, "output_data", append=True)
                        # 每次添加新数据后，检查并清理旧数据
                        clear_data_by_count(data_storage, max_data_count)
                buffer.clear()
        except Empty:
            continue

    if buffer:
        with lock:
            for timestamp, adc_value in buffer:
                data_storage.append({"Timestamp": timestamp, "ADC Value": adc_value})
            if len(data_storage) >= max_data_count:
                # 提取将要删除的数据
                data_to_save = data_storage[:len(data_storage) - max_data_count]
                # 保存数据到文件
                save_data_to_csv(data_to_save, device_id, "output_data", append=True)
                # 每次添加新数据后，检查并清理旧数据
                clear_data_by_count(data_storage, max_data_count)

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
def save_data_to_csv(data_storage, device_id, output_dir,append=False):
    print(f"Saving data for device {device_id}...")  # 调试输出
    if not data_storage:
        print(f"No data to save for device {device_id}.")  # 检查是否有数据
    filename = os.path.join(output_dir, f"device_{device_id}_output.csv")
    df = pd.DataFrame(data_storage)
    # 增量保存数据到 CSV
    if append:
        # 如果文件存在，追加数据，否则创建新文件
        df.to_csv(filename, mode='a', header=False, index=False)
    else:
        # 如果是首次保存，覆盖现有文件
        df.to_csv(filename, mode='w', header=True, index=False)
    print(f"Final data saved to {filename}.")

from collections import deque
import numpy as np

class RealtimePlot(FigureCanvas):
    def __init__(self, device_id, parent=None, window_size=2000):
        self.device_id = device_id
        self.fig = Figure()
        self.ax = self.fig.add_subplot(111)
        self.ax.clear()  # 清除当前子图的内容
        super().__init__(self.fig)
        self.setParent(parent)

        # 设置自适应大小
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setMinimumSize(200, 150)

        self.ax.set_title(f"Device {device_id} Real-Time Data")
        self.ax.set_xlabel("Time (s)")  # 修改为显示实际时间
        self.ax.set_ylabel("ADC Value")

        self.line, = self.ax.plot([], [], lw=2,drawstyle='steps-post')

        # 数据存储
        self.data = deque(maxlen=window_size)          # 存储 ADC 值
        self.timestamps = deque(maxlen=window_size)    # 存储时间戳
        self.background = None

    def update_plot(self, data_points):
        """接收新的 data_points 数据，并更新图表"""
        if not isinstance(data_points, list):
            return

        for point in data_points:
            if isinstance(point, dict):
                timestamp = point.get('Timestamp')
                adc_value = point.get('ADC Value')
                # 跳过异常或无效数据
                if not isinstance(timestamp, (int, float)) or not isinstance(adc_value, (int, float)):
                    continue     
                        # 检查时间戳连续性
                if self.timestamps and timestamp <= self.timestamps[-1]:
                    continue  # 丢弃不连续或重复的时间戳
                self.timestamps.append(timestamp / 1000)  # 存储时间戳
                self.data.append(adc_value)        # 存储 ADC 值

        if not self.data or not self.timestamps:
            return

        # 转换为列表
        x_data = list(self.timestamps)  # 使用时间戳作为 X 轴
        y_data = list(self.data)        # ADC 值作为 Y 轴

        # 更新线条数据
        self.line.set_data(x_data, y_data)

        # 更新坐标范围，确保动态显示
        self.ax.set_xlim(min(x_data), max(x_data))  # 时间范围
        self.ax.set_ylim(min(y_data) - 10, max(y_data) + 10)  # 动态调整 Y 轴范围

        # 设置 X 轴刻度：根据时间范围动态设置
        time_step = (max(x_data) - min(x_data)) / 10  # 生成10个等间隔刻度
        self.ax.set_xticks(np.arange(min(x_data), max(x_data), step=max(1, time_step)))

        # 强制重绘
        self.ax.figure.canvas.draw_idle()

        # 使用 blit 加速渲染
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

        # 设备选择下拉框
        self.device_selector = QComboBox()
        self.device_selector.addItem("Select Device")
        layout.addWidget(QLabel("Select Device:"))
        layout.addWidget(self.device_selector)

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
        # 设置自适应大小
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)  # X 和 Y 方向均可扩展
        self.setMinimumSize(50, 50)  # 设置最小尺寸，避免缩得太小
        self.log_display.append(message)

    def start_logging(self):
        global running
        connected_to_station = True#debug
        if not connected_to_station:
            self.append_log("Please connect to the base station first.")
            return

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

        # 清空现有的设备选择下拉框
        self.device_selector.clear()
        self.device_selector.addItem("Select Device")

        for device_id in range(1, device_count + 1):
            udp_port = base_port + device_id
            queue = Queue()
            self.queues[device_id] = queue
            self.data_storages[device_id] = []
            self.locks[device_id] = threading.Lock()

            if device_id not in self.plots:
                plot = RealtimePlot(device_id)
                self.plot_area.addWidget(plot)
                self.plots[device_id] = plot

            # 添加设备到下拉框
            self.device_selector.addItem(f"Device {device_id}")

            udp_thread = threading.Thread(target=read_from_udp, args=(udp_port, device_id, queue))
            udp_thread.daemon = True
            udp_thread.start()
            self.threads.append(udp_thread)

            process_thread = threading.Thread(target=process_data, args=(queue, self.data_storages[device_id], device_id, self.locks[device_id]))
            process_thread.daemon = True
            process_thread.start()
            self.threads.append(process_thread)
            
            save_data_to_csv(self.data_storages, device_id, output_dir, append=False)

        self.timer = QTimer()
        self.timer.timeout.connect(self.update_plots)
        self.timer.start(10)  # 每 10 毫秒更新一次
        self.append_log(f"Started logging for {device_count} devices starting...")
        

    def connect_to_station(self):
        global connected_to_station
        station_ip = self.station_ip_input.text().strip()

        if not station_ip:
            self.append_log("Please enter a valid base station IP address.")
            return

        connected_to_station = True#debug used
        # 发送UDP连接请求
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.sendto(b"Connection Request", (station_ip, 3333))
            sock.settimeout(2)  # 设置超时
            # 等待确认报文（假设基站会回应确认连接）
            sock_request = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock_request.settimeout(2)  # 设置超时
            sock_request.bind(('', 10000))
            sock_request.recvfrom(1024)
            connected_to_station = True
            self.append_log(f"Connected to base station at {station_ip}.")
            sock.close()
            # self.connect_button.setEnabled(False)  # 禁用连接按钮
        except socket.timeout:
            connected_to_station = False
            self.append_log("Connection timed out. Please check the base station IP and port.")
        except Exception as e:
            connected_to_station = False
            self.append_log(f"Failed to connect to base station: {e}")


    def update_plots(self):
        selected_device_id = self.device_selector.currentIndex()
        if selected_device_id == 0:
           selected_device_id = 1
        
        if selected_device_id == 1:
            # 如果没有选择设备
            for device_id, plot in self.plots.items():
                            # 只显示设备 1 的图表
                plot.setVisible(device_id == 1)
                data_for_plot = self.data_storages[selected_device_id][:]  # 创建绘图数据副本
                plot.update_plot(data_for_plot)  # 使用副本更新绘图
        else:
            # 只显示选择的设备图表
            for device_id, plot in self.plots.items():
                if device_id == selected_device_id:
                    plot.setVisible(True)
                    data_for_plot = self.data_storages[selected_device_id][:]  # 创建绘图数据副本
                    plot.update_plot(data_for_plot)  # 使用副本更新绘图
                else:
                    plot.setVisible(False)


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
                executor.submit(save_data_to_csv, self.data_storages[device_id], device_id, "output_data", append=True)
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
