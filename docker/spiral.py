#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt

N = 32
scale = 30

n = np.arange(N) / np.pi
r = np.sqrt(n)
a = 2 * np.pi * r
r *= scale
x = r * np.cos(a)
y = -r * np.sin(a)

plt.plot(x,y, marker='o')
plt.show()
