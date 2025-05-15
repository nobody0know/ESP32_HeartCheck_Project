import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
import seaborn as sns
from scipy.fft import fft, fftfreq
import neurokit2 as nk

# 设置绘图风格
sns.set(style="whitegrid")
plt.rcParams["figure.figsize"] = (12, 6)
plt.rcParams['font.sans-serif'] = ['SimHei'] # 用来正常显示中文标签SimHei
plt.rcParams['axes.unicode_minus'] = False # 用来正常显示负号

# ----------------------------
# 1. 读取CSV数据（无标题）
# ----------------------------
def read_ecg_data(file_path):
    # 读取CSV，跳过可能的无效首行（例如你的示例中的"1"）
    df = pd.read_csv(
        file_path, 
        header=None, 
        names=['timestamp', 'ecg_level'], 
        skiprows=1  # 跳过首行（如果你的第一行是无效数据）
    )
    return df

# ----------------------------
# 2. 数据质量检查（基于整数时间戳）
# ----------------------------
def check_data_quality(df, expected_fs=1000):
    # 计算时间戳间隔（应为1）
    time_diff = np.diff(df['timestamp'])
    mean_interval = np.mean(time_diff)
    std_interval = np.std(time_diff)
    
    print(f"平均时间间隔: {mean_interval:.2f} (理论值=1)")
    print(f"时间间隔标准差: {std_interval:.2f}")
    
    # 计算缺失率（基于连续性）
    min_time = df['timestamp'].min()
    max_time = df['timestamp'].max()
    expected_points = max_time - min_time + 1  # 时间戳连续时应有的点数
    actual_points = len(df)
    missing_ratio = 1 - (actual_points / expected_points)
    print(f"缺失率: {missing_ratio*100:.2f}%")

# ----------------------------
# 3. 预处理（滤波）
# ----------------------------
def preprocess_ecg(ecg_signal, fs=1000):
    # 去基线漂移（高通滤波，0.5 Hz）
    b, a = signal.butter(2, 0.5/(fs/2), 'highpass')
    ecg_highpass = signal.filtfilt(b, a, ecg_signal)
    
    # 去工频干扰（50 Hz陷波滤波）
    f0 = 50.0  # 干扰频率
    Q = 30     # 滤波器品质因数
    b, a = signal.iirnotch(f0, Q, fs)
    ecg_notch = signal.filtfilt(b, a, ecg_highpass)
    
    return ecg_notch

# ----------------------------
# 4. 信噪比（SNR）计算（无需修改）
# ----------------------------
def calculate_snr(original_signal, filtered_signal):
    noise = original_signal - filtered_signal
    signal_power = np.mean(filtered_signal**2)
    noise_power = np.mean(noise**2)
    snr_db = 10 * np.log10(signal_power / noise_power)
    return snr_db

# ----------------------------
# 5. 频域分析（FFT）
# ----------------------------
def plot_spectrum(signal, fs=1000):
    n = len(signal)
    yf = fft(signal)
    xf = fftfreq(n, 1/fs)[:n//2]
    plt.plot(xf, 2.0/n * np.abs(yf[0:n//2]))
    plt.xlabel('频率 (Hz)',fontsize=20)
    plt.ylabel('振幅',fontsize=20)
    plt.title('频谱图',fontsize=20)
    plt.xlim(0, 100)  # 只看0-100 Hz范围

# ----------------------------
# 6. QRS波检测与心率计算
# ----------------------------
def detect_qrs(ecg_signal, fs=1000):
    cleaned = nk.ecg_clean(ecg_signal, sampling_rate=fs)
    signals, info = nk.ecg_peaks(cleaned, sampling_rate=fs)
    r_peaks = info['ECG_R_Peaks']
    heart_rate = np.mean(60 / np.diff(r_peaks/fs)) if len(r_peaks) > 1 else 0
    return r_peaks, heart_rate

# ----------------------------
# 主程序
# ----------------------------
if __name__ == "__main__":
    # 读取CSV文件（替换为你的文件路径）
    file_path = "output_data\device_1_output.csv"
    df = read_ecg_data(file_path)
    ecg_raw = df['ecg_level'].values
    
    # 检查数据质量
    check_data_quality(df)
    
    # 预处理（滤波）
    ecg_filtered = preprocess_ecg(ecg_raw)
    
    # 计算信噪比
    snr = calculate_snr(ecg_raw, ecg_filtered)
    print(f"信噪比 (SNR): {snr:.2f} dB")
    
    # 频域分析
    plt.figure()
    plot_spectrum(ecg_filtered)
    plt.show()
    
    # QRS波检测与绘图
    r_peaks, hr = detect_qrs(ecg_filtered)
    print(f"平均心率: {hr:.2f} BPM")
    
    # 绘制时域信号
    plt.figure()
    plt.plot(ecg_raw[:5000], label='原始心电图', alpha=0.5)
    plt.plot(ecg_filtered[:5000], label='滤波后心电图')
    plt.scatter(r_peaks[r_peaks < 5000], ecg_filtered[r_peaks[r_peaks < 5000]], c='r', label='R Peaks')
    plt.xlabel('采样点',fontsize = 20)
    plt.ylabel('电压幅值 (mV)',fontsize = 20)
    plt.legend()
    plt.show()