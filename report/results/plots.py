import numpy as np
from matplotlib import pyplot as plt

runtime_data = np.loadtxt("runtimes.txt")


res1 = 192128
res2 = 256192
res3 = 512384
res4 = 768768

res1_pixels = 192 * 128
res2_pixels = 256 * 192
res3_pixels = 512 * 384
res4_pixels = 768 * 768

n_sims = 4
n = int(runtime_data.shape[0])
n_procs = int(runtime_data.shape[0]/n_sims)
n_cols = int(runtime_data.shape[1])

labels = ['768x768', '512x368', '256x192', '192x128']

image1_data = np.zeros((n_procs, n_cols))
image2_data = np.zeros((n_procs, n_cols))
image3_data = np.zeros((n_procs, n_cols))
image4_data = np.zeros((n_procs, n_cols))

i1 = 0
i2 = 0
i3 = 0
i4 = 0

for i in range(n):
    if runtime_data[i, 0] == res1:
        image1_data[i1, :] = runtime_data[i, :]
        i1 += 1
    if runtime_data[i, 0] == res2:
        image2_data[i2, :] = runtime_data[i, :]
        i2 += 1
    if runtime_data[i, 0] == res3:
        image3_data[i3, :] = runtime_data[i, :]
        i3 += 1
    if runtime_data[i, 0] == res4:
        image4_data[i4, :] = runtime_data[i, :]
        i4 += 1

# run time of parallel region against number of processes

fig = plt.figure(figsize=(12, 8))
ax1 = fig.add_subplot(111)
ax1.plot(image1_data[:, 1], image1_data[:, 2], 'x-', label=labels[3])
ax1.plot(image2_data[:, 1], image2_data[:, 2], 'o-', label=labels[2])
ax1.plot(image3_data[:, 1], image3_data[:, 2], '^-', label=labels[1])
ax1.plot(image4_data[:, 1], image4_data[:, 2], 'D-', label=labels[0])
ax1.set_xlabel(r"Number of processes $N_{p}$", fontsize="x-large")
ax1.set_ylabel(r"Run time (s)", fontsize="x-large")
ax1.set_xlim(0, 70)
ax1.set_ylim(0, 3)
ax1.legend(fontsize="x-large")
ax1.grid()

fig.tight_layout()
plt.savefig("np_runtime.pdf")
plt.close()

# speed up against number of processes

speedup_image1 = image1_data[0, 2]/image1_data[:, 2]
speedup_image2 = image2_data[0, 2]/image2_data[:, 2]
speedup_image3 = image3_data[0, 2]/image3_data[:, 2]
speedup_image4 = image4_data[0, 2]/image4_data[:, 2]

fig = plt.figure(figsize=(12, 8))
ax1 = fig.add_subplot(111)
ax1.plot(image1_data[:, 1], speedup_image1, 'x-', label=labels[3])
ax1.plot(image2_data[:, 1], speedup_image2, 'o-', label=labels[2])
ax1.plot(image3_data[:, 1], speedup_image3, '^-', label=labels[1])
ax1.plot(image4_data[:, 1], speedup_image4, 'D-', label=labels[0])
ax1.set_xlabel(r"Number of processes $N_{p}$", fontsize="x-large")
ax1.set_ylabel(r"Speed up $T_{1}/T_{N_{p}}$", fontsize="x-large")
ax1.set_xlim(0, 70)
ax1.set_ylim(0, 25)
ax1.legend(fontsize="x-large")
ax1.grid()

fig.tight_layout()
plt.savefig("np_speedup.pdf")
plt.close()

# average time for process

fig = plt.figure(figsize=(12, 8))
ax1 = fig.add_subplot(111)
ax1.plot(image1_data[:, 1], image1_data[:, 3], 'x-', label=labels[3])
ax1.plot(image2_data[:, 1], image2_data[:, 3], 'o-', label=labels[2])
ax1.plot(image3_data[:, 1], image3_data[:, 3], '^-', label=labels[1])
ax1.plot(image4_data[:, 1], image4_data[:, 3], 'D-', label=labels[0])
ax1.set_xlabel(r"Number of processes $N_{p}$", fontsize="x-large")
ax1.set_ylabel(r"Average run time per process (s)", fontsize="x-large")
ax1.set_xlim(0, 70)
ax1.set_ylim(-0.5, 3)
ax1.legend(fontsize="x-large")
ax1.grid()

fig.tight_layout()
plt.savefig("np_timeperprocess.pdf")
plt.close()

# average time per iteration

time_scale = 1000

image1_time_per_iter = image1_data[:, 2]/res1_pixels * time_scale
image2_time_per_iter = image2_data[:, 2]/res2_pixels * time_scale
image3_time_per_iter = image3_data[:, 2]/res3_pixels * time_scale
image4_time_per_iter = image4_data[:, 2]/res4_pixels * time_scale

fig = plt.figure(figsize=(12, 8))
ax1 = fig.add_subplot(111)

ax1.plot(image1_data[:, 1], image1_time_per_iter, 'x-', label=labels[3])
ax1.plot(image2_data[:, 1], image2_time_per_iter, 'o-', label=labels[2])
ax1.plot(image3_data[:, 1], image3_time_per_iter, '^-', label=labels[1])
ax1.plot(image4_data[:, 1], image4_time_per_iter, 'D-', label=labels[0])
ax1.set_xlabel(r"Number of processes $N_{p}$", fontsize="x-large")
ax1.set_ylabel(r"Average time per iter (ms)", fontsize="x-large")
# ax1.set_xlim(0, 70)
# ax1.set_ylim(-0.5, 3)
ax1.legend(fontsize="x-large")
ax1.grid()


fig.tight_layout()
plt.savefig("np_timeperiter.pdf")
plt.close()
