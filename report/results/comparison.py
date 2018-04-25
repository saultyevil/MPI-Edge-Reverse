import numpy as np
from matplotlib import pyplot as plt

image4_nonblocking = np.loadtxt("nonblocking4.txt")
image4 = np.loadtxt("runtimes4blocking.txt")

x_img = 15
y_img = 12

image4[:, 2] = image4[:, 2] - image4[:, 5] - image4[:, 6]

image4speedup = image4[0, 2]/image4[:, 2]
image4_nonblocking_speedup = image4_nonblocking[0, 2]/image4_nonblocking[:, 2]


fig = plt.figure(figsize=(x_img, 2*y_img))
ax1 = fig.add_subplot(211)
ax2 = fig.add_subplot(212)
# speed up

ax1.plot(image4[:, 1], image4speedup, '^-', label="Blocking Receive")
ax1.plot(image4_nonblocking[:, 1], image4_nonblocking_speedup, 'x-',
         label="Non-blocking Receive")
ax1.set_xlabel(r"Number of Processes, $N_{P}$", fontsize=20)
ax1.set_ylabel(r"Speed Up of Iterations", fontsize=20)
ax1.set_xlim(1, 120)
ax1.set_ylim(0)
ax1.set_title("Speed up against the number of processes", fontsize=20)
ax1.legend(fontsize=20)
ax1.grid()
# parallel efficiency
ax2.semilogy(image4[:, 1], image4[:, 2]/1300*1000, '^-',
             label="Blocking Receive")
ax2.semilogy(image4_nonblocking[:, 1], image4_nonblocking[:, 2]/1300*1000,
             '^-', label="Non-blocking Receive")
ax2.set_xlabel(r"Number of Processes, $N_{P}$", fontsize=20)
ax2.set_ylabel(r"Average Time per Iteration (ms)", fontsize=20)
ax2.set_xlim(1, 120)
ax2.set_ylim(0.01)
ax2.set_title(
        "The average run time per iteration against the number of processes",
        fontsize=20)
ax2.legend(fontsize=20, loc="upper right")
ax2.grid()
fig.tight_layout()
plt.savefig("np_comparison.pdf")
plt.close()
