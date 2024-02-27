import sys
import numpy as np
import matplotlib.pyplot as plt


def read_binary_file(filename):
    SIZEOF_FLOAT32 = np.dtype(np.float32).itemsize

    frames = []
    tags   = []

    with open(filename, 'rb') as f:
        width, height = [int(x) for x in f.readline().split()]

        line = f.readline()
        while line:
            tags.append(int(line))

            buffer = f.read(SIZEOF_FLOAT32 * width * height)
            frames.append(np.frombuffer(buffer, dtype = np.float32))

            line = f.readline()

        U = np.array(frames).reshape(-1, height, width)

        return (U, tags)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python compare.py <reference_binary_file> <output binary file>")
        sys.exit(1)


    ref, tags = read_binary_file(sys.argv[1])
    out, _    = read_binary_file(sys.argv[2])


    if ref.shape != out.shape:
        print("The two files are incompatible")
        sys.exit(1)


    plt.plot(tags, np.max(np.abs(ref - out), axis = (1, 2)))
    plt.xlabel('steps')
    plt.ylabel('maximum difference')
    plt.show()
