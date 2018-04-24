import numpy as np
from matplotlib import pyplot as plt

image1 = np.loadtxt("runtimes1.txt")
image2 = np.loadtxt("runtimes2.txt")
image3 = np.loadtxt("runtimes3.txt")
image4 = np.loadtxt("runtimes4.txt")

label = ["192 x 128", "256 x 192", "512 x 384", "768 x 768"]

x_img = 15
y_img = 12

# apply correction fudge because I output the wrong stuff :^)
image1[:, 1] -= image1[:, 5] - image1[:, 6]
image2[:, 1] -= image2[:, 5] - image2[:, 6]
image3[:, 1] -= image3[:, 5] - image3[:, 6]
image4[:, 1] -= image4[:, 5] - image4[:, 6]


# n_procs v run time

fig = plt.figure(figsize=(x_img, y_img))
ax1 = fig.add_subplot(111)
ax1.semilogy(image1[:, 1], image1[:, 2]*1000, 'x-', label=label[0])
ax1.semilogy(image2[:, 1], image2[:, 2]*1000, 'D-', label=label[1])
ax1.semilogy(image3[:, 1], image3[:, 2]*1000, '^-', label=label[2])
ax1.semilogy(image4[:, 1], image4[:, 2]*1000, 's-', label=label[3])
ax1.set_xlabel(r"Number of Processes, $N_{P}$", fontsize="x-large")
ax1.set_ylabel(r"Iterations Run Time (ms)", fontsize="x-large")
ax1.set_xlim(1, 120)
# ax1.set_ylim(0, 3)
ax1.legend(fontsize="x-large")
ax1.grid()
plt.savefig("np_runtime.pdf")
plt.close()

# np v speedup

image1speedup = image1[0, 2]/image1[:, 2]
image2speedup = image2[0, 2]/image2[:, 2]
image3speedup = image3[0, 2]/image3[:, 2]
image4speedup = image4[0, 2]/image4[:, 2]

fig = plt.figure(figsize=(x_img, y_img))
ax1 = fig.add_subplot(111)
ax1.plot(image1[:, 1], image1speedup, 'x-', label=label[0])
ax1.plot(image2[:, 1], image2speedup, 'D-', label=label[1])
ax1.plot(image3[:, 1], image3speedup, '^-', label=label[2])
ax1.plot(image4[:, 1], image4speedup, 's-', label=label[3])
ax1.set_xlabel(r"Number of Processes, $N_{P}$", fontsize="x-large")
ax1.set_ylabel(r"Speed Up of Iterations", fontsize="x-large")
ax1.set_xlim(1, 120)
ax1.set_ylim(0)
ax1.legend(fontsize="x-large")
ax1.grid()
plt.savefig("np_speedup.pdf")
plt.close()

# np v time per iter

label_iters = ["192 x 128: 1500 iterations", "256 x 192: 1100 iterations",
               "512 x 384: 1500 iterations", "768 x 768: 1300 iterations"]

fig = plt.figure(figsize=(x_img, y_img))
ax1 = fig.add_subplot(111)
ax1.semilogy(image1[:, 1], image1[:, 2]/1500*1000, 'x-', label=label_iters[0])
ax1.semilogy(image2[:, 1], image2[:, 2]/1100*1000, 'D-', label=label_iters[1])
ax1.semilogy(image3[:, 1], image3[:, 2]/1500*1000, '^-', label=label_iters[2])
ax1.semilogy(image4[:, 1], image4[:, 2]/1300*1000, 's-', label=label_iters[3])
ax1.set_xlabel(r"Number of Processes, $N_{P}$", fontsize="x-large")
ax1.set_ylabel(r"Average Time per Iteration (ms)", fontsize="x-large")
ax1.set_xlim(1, 120)
ax1.set_ylim(0.01)
ax1.legend(fontsize="x-large", loc="upper right")
ax1.grid()

plt.savefig("np_periter.pdf")
plt.close()

# parallel efficiency

fig = plt.figure(figsize=(x_img, y_img))
ax1 = fig.add_subplot(111)
ax1.plot(image1[:, 1], image1[0, 2]/(image1[:, 1]*image1[:, 2]),
         'x-', label=label[0])
ax1.plot(image2[:, 1], image2[0, 2]/(image2[:, 1]*image2[:, 2]), 'D-',
         label=label[1])
ax1.plot(image3[:, 1], image3[0, 2]/(image3[:, 1]*image3[:, 2]), '^-',
         label=label[2])
ax1.plot(image4[:, 1], image4[0, 2]/(image4[:, 1]*image4[:, 2]), 's-',
         label=label[3])
ax1.set_xlabel(r"Number of Processes, $N_{P}$", fontsize="x-large")
ax1.set_ylabel(r"Parallel Efficiency", fontsize="x-large")
ax1.set_xlim(0, 120)
ax1.set_ylim(0, 1)
ax1.legend(fontsize="x-large")
ax1.grid()
plt.savefig("np_parallelefficiency.pdf")
plt.close()

# total runtime

fig = plt.figure(figsize=(x_img, y_img))
ax1 = fig.add_subplot(111)
ax1.semilogy(image1[:, 1], image1[:, -1]*1000, 'x-', label=label[0])
ax1.semilogy(image2[:, 1], image2[:, -1]*1000, 'D-', label=label[1])
ax1.semilogy(image3[:, 1], image3[:, -1]*1000, '^-', label=label[2])
ax1.semilogy(image4[:, 1], image4[:, -1]*1000, 's-', label=label[3])
ax1.set_xlabel(r"Number of Processes, $N_{P}$", fontsize="x-large")
ax1.set_ylabel(r"Total Run Time (ms)", fontsize="x-large")
ax1.set_xlim(0, 120)
# ax1.set_ylim(0, 1)
ax1.legend(fontsize="x-large")
ax1.grid()
plt.savefig("np_totalruntime.pdf")
plt.close()
