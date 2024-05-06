import re
import matplotlib.pyplot as plt
import numpy as np

###### Speedup
i = 1
filename = "./result/speedup_results_" + str(i) + ".txt"
with open(filename, "r") as file:
    outcome = file.read()

# Define regular expressions to extract relevant data
pattern = r'Threads: (\d+),.*?Insertion Speedup: (\d+\.\d+), Insertion Time \(Sequential\): (\d+\.\d+) milliseconds, Insertion Time \(Concurrent\): (\d+\.\d+) milliseconds,.*?Deletion Speedup: (\d+\.\d+), Deletion Time \(Sequential\): (\d+\.\d+) milliseconds, Deletion Time \(Concurrent\): (\d+\.\d+) milliseconds,.*?Search Speedup: (\d+\.\d+), Search Time \(Sequential\): (\d+\.\d+) milliseconds, Search Time \(Concurrent\): (\d+\.\d+) milliseconds'
matches = re.findall(pattern, outcome, re.DOTALL)

# Extracted data lists
threads = []
insertion_speedup = []
insertion_time_sequential = []
insertion_time_concurrent = []
deletion_speedup = []
deletion_time_sequential = []
deletion_time_concurrent = []
search_speedup = []
search_time_sequential = []
search_time_concurrent = []

# Parse the extracted data
for match in matches:
    threads.append(int(match[0]))
    insertion_speedup.append(float(match[1]))
    insertion_time_sequential.append(float(match[2]))
    insertion_time_concurrent.append(float(match[3]))
    deletion_speedup.append(float(match[4]))
    deletion_time_sequential.append(float(match[5]))
    deletion_time_concurrent.append(float(match[6]))
    search_speedup.append(float(match[7]))
    search_time_sequential.append(float(match[8]))
    search_time_concurrent.append(float(match[9]))

# Plotting speedup vs number of threads
plt.plot(threads, insertion_speedup, label='Insertion Speedup')
plt.plot(threads, deletion_speedup, label='Deletion Speedup')
plt.plot(threads, search_speedup, label='Search Speedup')
plt.xlabel('Number of Threads')
plt.ylabel('Speedup')
plt.title('Speedup vs Number of Threads')
plt.legend()
plt.show()
plt.savefig("./result/speedup_results_" + str(i) + ".png")
plt.clf()  # Clear the current figure


######## Throughput
filename = "./result/throughput_results_" + str(i) + ".txt"
with open(filename, "r") as file:
    data = file.read()

pattern = r'Implementation: (\d+), Capacity: (\d+), Threads: (\d+)\nInsert time: ([\d.]+) milliseconds\nDelete time: ([\d.]+) milliseconds\nSearch time: ([\d.]+) milliseconds\nTotal time: ([\d.]+) milliseconds\nTotal throughput: ([\d.e+]+) operations per second'
matches = re.findall(pattern, data)

implementation = []
capacity = []
threads = []
insert_time = []
delete_time = []
search_time = []
total_time = []
throughput = []

for match in matches:
    implementation.append(int(match[0]))
    capacity.append(int(match[1]))
    threads.append(int(match[2]))
    insert_time.append(float(match[3]))
    delete_time.append(float(match[4]))
    search_time.append(float(match[5]))
    total_time.append(float(match[6]))
    throughput.append(float(match[7]) / 1e5)  # Convert to million operations per second

total = np.array(capacity) * np.array(threads)
print(capacity)
print(threads)
print(throughput)
print(total)

# Plotting
capacity = [1000000, 100000, 10000]
len_threads = len(set(threads))
threads = threads[:len_threads]

k = 0
for cap in capacity:
    plt.plot(threads, throughput[k * len(threads) : k * len(threads) + len_threads], label='Throughput', color='r')
    plt.xlabel('Number of Threads')
    plt.ylabel('Throughput (Hundred thousands operations per second)')
    plt.title('Throughput vs Number of Threads with ' + str(cap) + ' operations')
    plt.legend()
    plt.grid(True)
    plt.show()
    plt.savefig("./result/throughput_results_" + str(i) + "_" + str(cap) + ".png")
    plt.clf()  # Clear the current figure
    k += 1
