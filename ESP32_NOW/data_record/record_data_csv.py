import socket
import struct
import threading
import signal
from queue import Queue, Empty
import pandas as pd
import time
import os
from concurrent.futures import ThreadPoolExecutor

# 全局运行标志
running = True

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
    while offset + 6 <= len(message):
        try:
            timestamp, adc_value = struct.unpack('<IH', message[offset:offset+6])
            data_points.append((timestamp, adc_value))
            offset += 6
        except struct.error:
            break
    return device_id, data_points

# 数据处理线程
def process_data(queue, data_storage, device_id, lock):
    buffer = []
    buffer_limit = 1000  # 缓存数据量

    while running or not queue.empty():
        try:
            data_points = queue.get(timeout=1)
            if data_points is None:  # 收到退出信号
                break
            buffer.extend(data_points)
            if len(buffer) >= buffer_limit:
                with lock:
                    for timestamp, adc_value in buffer:
                        if timestamp not in data_storage:
                            data_storage[timestamp] = {}
                        data_storage[timestamp][f'Device {device_id}'] = adc_value
                buffer.clear()
        except Empty:
            continue

    # 退出时保存剩余缓存
    if buffer:
        with lock:
            for timestamp, adc_value in buffer:
                if timestamp not in data_storage:
                    data_storage[timestamp] = {}
                data_storage[timestamp][f'Device {device_id}'] = adc_value

# 从 UDP 读取并解析数据
def read_from_udp(udp_port, device_id, queue):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('', udp_port))
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
    sock.settimeout(1)
    print(f"Listening on UDP port {udp_port} for device {device_id}")
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
        queue.put(None)  # 通知数据处理线程退出

# 保存数据到文件
def save_data_to_csv(data_storage, device_id, output_dir):
    filename = os.path.join(output_dir, f"device_{device_id}_output.csv")
    df = pd.DataFrame.from_dict(data_storage, orient='index').sort_index()
    df.to_csv(filename, index_label="Timestamp")
    print(f"Final data saved to {filename}.")

# 信号处理函数
def signal_handler(signum, frame):
    global running
    running = False
    print("Interrupt received, stopping threads...")

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)

    device_ids = range(1, 2)  # 假设有5个设备
    base_port = 10001
    threads = []
    queues = {}
    data_storages = {}
    locks = {}

    # 创建文件夹保存数据
    output_dir = "output_data"
    os.makedirs(output_dir, exist_ok=True)

    for device_id in device_ids:
        udp_port = base_port + device_id
        queue = Queue()
        queues[device_id] = queue
        data_storages[device_id] = {}
        locks[device_id] = threading.Lock()

        # 启动 UDP 接收线程
        udp_thread = threading.Thread(target=read_from_udp, args=(udp_port, device_id, queue))
        udp_thread.daemon = True
        udp_thread.start()
        threads.append(udp_thread)

        # 启动数据处理线程
        process_thread = threading.Thread(target=process_data, args=(queue, data_storages[device_id], device_id, locks[device_id]))
        process_thread.daemon = True
        process_thread.start()
        threads.append(process_thread)

    try:
        while any(t.is_alive() for t in threads):
            [t.join(1) for t in threads if t.is_alive()]
    except KeyboardInterrupt:
        print("Keyboard interrupt in main, stopping...")
        running = False

    # 等待所有线程退出
    for device_id, queue in queues.items():
        queue.put(None)  # 通知数据处理线程退出

    for t in threads:
        t.join()

    # 多线程保存最终数据
    with ThreadPoolExecutor() as executor:
        futures = [
            executor.submit(save_data_to_csv, data_storages[device_id], device_id, output_dir)
            for device_id in device_ids
        ]
        for future in futures:
            future.result()  # 等待所有保存任务完成
