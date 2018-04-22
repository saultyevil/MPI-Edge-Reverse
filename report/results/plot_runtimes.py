import numpy as np
from matplotlib import pyplot as plt

image1 = np.loadtxt("runtimes1.txt")
image2 = np.loadtxt("runtimes2.txt")
image3 = np.loadtxt("runtimes3.txt")
image4 = np.loadtxt("runtimes4.txt")

label = ["192 x 128", "256 x 192", "512 x 384", "768 x 768"]

# n_procs v run time

fig = plt.figure(figsize=(12, 8))
ax1 = fig.add_subplot(111)
ax1.plot(image1[:, 1], image1[:, 2], 'x-', label=label[0])
ax1.plot(image2[:, 1], image2[:, 2], 'D-', label=label[1])
ax1.plot(image3[:, 1], image3[:, 2], '^-', label=label[2])
ax1.plot(image4[:, 1], image4[:, 2], 's-', label=label[3])
ax1.set_xlabel(r"Number of Processes, $N_{P}$")
ax1.set_ylabel(r"Run time (s)")
ax1.set_xlim(0, 125)
ax1.set_ylim(0, 3)
ax1.legend(fontsize="x-large")
ax1.grid()
plt.savefig("np_runtime.pdf")
plt.close()

# np v speedup

image1speedup = image1[0, 2]/image1[:, 2]
image2speedup = image2[0, 2]/image2[:, 2]
image3speedup = image3[0, 2]/image3[:, 2]
image4speedup = image4[0, 2]/image4[:, 2]

fig = plt.figure(figsize=(12, 8))
ax1 = fig.add_subplot(111)
ax1.plot(image1[:, 1], image1speedup, 'x-', label=label[0])
ax1.plot(image2[:, 1], image2speedup, 'D-', label=label[1])
ax1.plot(image3[:, 1], image3speedup, '^-', label=label[2])
ax1.plot(image4[:, 1], image4speedup, 's-', label=label[3])
ax1.set_xlabel(r"Number of Processes, $N_{P}$")
ax1.set_ylabel(r"Speedup")
ax1.set_xlim(0, 125)
ax1.set_ylim(0.5, 35)
ax1.legend(fontsize="x-large")
ax1.grid()
plt.savefig("np_speedup.pdf")
plt.close()

fig = plt.figure(figsize=(12, 8))
ax1 = fig.add_subplot(111)
ax1.plot(image1[:, 1], image1speedup, 'x-', label=label[0])
ax1.plot(image4[:, 1], image4speedup, 's-', label=label[3])
ax1.set_xlabel(r"Number of Processes, $N_{P}$")
ax1.set_ylabel(r"Speedup")
ax1.set_xlim(0, 125)
ax1.set_ylim(0.5, 35)
ax1.legend(fontsize="x-large")
ax1.grid()
plt.savefig("np_speedup2.pdf")
plt.close()

# np v time per iter

fig = plt.figure(figsize=(12, 8))
ax1 = fig.add_subplot(111)
ax1.plot(image1[:, 1], image1[:, 2]/(192*128)*1000, 'x-', label=label[0])
ax1.plot(image4[:, 1], image4[:, 2]/(768*768)*1000, 's-', label=label[3])
ax1.set_xlabel(r"Number of Processes, $N_{P}$")
ax1.set_ylabel(r"Time per iter (ms)")
ax1.set_xlim(0, 125)
ax1.set_ylim(0, 0.0055)
ax1.legend(fontsize="x-large")
ax1.grid()

plt.savefig("np_periter.pdf")
plt.close()
