import matplotlib.pyplot as plt
import numpy as np

# !pip3 install matplotlib

data = [77399.38, 72992.70, 63091.48, 22517.45]
labels = ['redis', 'pmem-redis', 'go-pmem-redis', 'kvrocks']

plt.title('128 bytes kv set',fontsize='large')
plt.bar(range(len(data)), data, lw=2, tick_label=labels, color=['lightgray', 'lightgray', 'lightgray', 'lightgray'])
plt.show()

data = [76687.12, 77101.00, 76804.91, 27359.78]
labels = ['redis', 'pmem-redis', 'go-pmem-redis', 'kvrocks']

plt.title('128 bytes kv get',fontsize='large')
plt.bar(range(len(data)), data, lw=2, tick_label=labels, color=['lightgray', 'lightgray', 'lightgray', 'lightgray'])
plt.show()

data = [70077.09, 51948.05, 26178.01, 12642.23]
labels = ['redis', 'pmem-redis', 'go-pmem-redis', 'kvrocks']

plt.title('10240 bytes kv set',fontsize='large')
plt.bar(range(len(data)), data, lw=2, tick_label=labels, color=['lightgray', 'lightgray', 'lightgray', 'lightgray'])
plt.show()

data = [62189.05, 60569.35, 1260.72, 22114.11]
labels = ['redis', 'pmem-redis', 'go-pmem-redis', 'kvrocks']

plt.title('10240 bytes kv get',fontsize='large')
plt.bar(range(len(data)), data, lw=2, tick_label=labels, color=['lightgray', 'lightgray', 'lightgray', 'lightgray'])
plt.show()
