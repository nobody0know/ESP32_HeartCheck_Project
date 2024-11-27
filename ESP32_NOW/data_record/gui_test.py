import sys
import random
from collections import deque
import numpy as np
from PyQt5.QtWidgets import QApplication, QWidget, QVBoxLayout, QPushButton
from PyQt5.QtCore import QTimer
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure

class RealtimePlot(FigureCanvas):
    def __init__(self, device_id, window_duration=10, parent=None):
        self.device_id = device_id
        self.window_duration = window_duration  # 设置时间窗口为10秒
        self.fig = Figure()
        self.ax = self.fig.add_subplot(111)
        super().__init__(self.fig)
        self.setParent(parent)
        
        self.ax.set_title(f"Device {device_id} Real-Time Data")
        self.ax.set_xlabel("Timestamp (s)")
        self.ax.set_ylabel("ADC Value")
        
        self.line, = self.ax.plot([], [], lw=2)
        
        # 使用deque来存储数据，自动移除超出时间窗口的数据
        self.data = deque()
        self.time_stamp = 0  # 时间戳初始化
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_data)

    def start_plotting(self):
        """开始数据更新"""
        self.timer.start(100)  # 每100ms更新一次数据

    def stop_plotting(self):
        """停止数据更新"""
        self.timer.stop()

    def update_data(self):
        """生成新的数据点并更新图表"""
        # 模拟从设备接收新的 ADC 数据
        adc_value = random.randint(0, 1023)  # 模拟一个 10 位 ADC 数据
        self.time_stamp += 0.1  # 每次更新时间戳，假设每次更新时间为100ms
        
        # 新数据点
        new_data = (self.time_stamp, adc_value)
        
        # 将新数据添加到deque中
        self.data.append(new_data)

        # 如果时间窗口外的数据存在，移除
        if self.data and self.data[0][0] < self.time_stamp - self.window_duration:
            self.data.popleft()

        # 更新图表
        self.update_plot()

    def update_plot(self):
        """接收新的 data_points 数据，并更新图表"""
        if self.data:
            x_data, y_data = zip(*self.data)  # 拆分时间戳和值
            
            # 更新数据
            self.line.set_data(x_data, y_data)
            self.ax.set_xlim(min(x_data), max(x_data))  # X 轴范围按时间戳动态调整
            self.ax.set_ylim(min(y_data) - 10, max(y_data) + 10)  # 动态调整 Y 轴范围
            
            # 重绘图形
            self.draw()

class MainWindow(QWidget):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Real-Time ADC Data Plotting")
        self.setGeometry(100, 100, 800, 600)

        # 创建布局
        self.layout = QVBoxLayout(self)

        # 创建 RealTimePlot 实例
        self.plot_widget = RealtimePlot(device_id=1, window_duration=10)
        self.layout.addWidget(self.plot_widget)

        # 添加开始和停止按钮
        self.start_button = QPushButton("Start Plotting")
        self.start_button.clicked.connect(self.plot_widget.start_plotting)
        self.layout.addWidget(self.start_button)

        self.stop_button = QPushButton("Stop Plotting")
        self.stop_button.clicked.connect(self.plot_widget.stop_plotting)
        self.layout.addWidget(self.stop_button)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())
