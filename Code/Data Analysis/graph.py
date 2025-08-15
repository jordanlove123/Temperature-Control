import numpy as np
import matplotlib.pyplot as plt
import numpy.typing as npt
import spectral3
from IPython.display import clear_output, display

def is_notebook():
    try:
        shell = get_ipython().__class__.__name__
        if shell == 'ZMQInteractiveShell':
            return True
        else:
            return False
    except NameError:
        return False

"""
Graph contents of 2D array containing different measurements in columns. First column must be timestamps.
Data must be 2D numpy array. Define a "flair function" which takes the ax list as an argument to add anthing
extra to your axes like horizontal lines.
"""
def graph(data, titles=None, fmtstrs=None, stime=None, etime=None, size=None, flair_func=None, scaling=None):
    # Check dataset format
    try:
        if (data.ndim != 2):
            raise ValueError("Dataset must be 2D numpy array")
    except:
        raise ValueError("Dataset must be 2D numpy array")
    if (len(data) < 2):
        raise ValueError("Dataset must contain data")
    
    N = len(data[0]) - 1
    if titles == None:
        titles = ["Measurement {0}".format(i+1) for i in range(N)]
    if fmtstrs == None:
        fmtstrs = ["r-" for _ in range(N)]
    if scaling == None:
        scaling = np.ones(N+1)

    # Check graph formatting arguments
    if (titles != None and len(titles) != N):
        raise ValueError("There must be the same number of titles and data columns in the dataset")
    if (fmtstrs != None and len(fmtstrs) != N):
        raise ValueError("There must be the same number of format strings and data columns in the dataset")
    if (scaling != None and len(scaling) != N+1):
        raise ValueError("There must be the same number of scale factors and columns in the dataset")
    
    # Check time bounds
    if stime == None:
        stime = data[0, 0]
    if etime == None:
        etime = data[-1, 0]
    if (stime > data[-1, 0] or stime < data[0, 0] or etime < data[0, 0] or etime > data[-1, 0] or stime > etime):
        raise ValueError("Invalid time bounds")
    
    if size == None:
        size = (15, int(2.5*N))

    data = np.multiply(data, scaling)
    data = data[(data[:, 0] >= stime) & (data[:,0] <= etime)]
    t = data[:,0]
    fig, ax = plt.subplots(N, figsize=size)
    for i in range(N):
        ax[i].plot(t, data[:, i+1], fmtstrs[i])
        ax[i].set_title(titles[i])
    
    if (flair_func != None):
        flair_func(ax)
    
    fig.tight_layout()

"""
Graph data from columns of data file. First column must be timestamps.
"""
def graph_file(path, titles=None, fmtstrs=None, stime=0, etime=None, size=None, flair_func=None, scaling=None):
    try:
        data = np.loadtxt(path)
    except:
        raise ValueError("Invalid data format in file")
    
    N = len(data[0, :]) - 1
    if size == None:
        size = (15, 2*N)

    graph(data, titles, fmtstrs, stime, etime, size, flair_func, scaling)

"""
Graph the psa of all columns in a dataset. Must be a 2D numpy array
"""
def graph_spectrum(data, dt, L=None, labels=None, fmtstrs=None, size=None, flair_func=None):
    # Check dataset format
    try:
        if (data.ndim != 2):
            raise ValueError("Dataset must be 2D numpy array")
    except:
        raise ValueError("Dataset must be 2D numpy array")
    if (len(data) < 2):
        raise ValueError("Dataset must contain data")

    N = len(data[0])

    # Check graph formatting arguments
    if (labels != None and len(labels) != N):
        raise ValueError("There must be the same number of labels and columns in the dataset")
    if (fmtstrs != None and len(fmtstrs) != N):
        raise ValueError("There must be the same number of format strings and columns in the dataset")

    if L == None:
        L = len(data[:,0])

    if labels == None:
        labels = ["Measurement {0}".format(i+1) for i in range(N)]
    if fmtstrs == None:
        fmtstrs = ["r-" for _ in range(N)]

    if size == None:
        size = (5, 5)

    fig, ax = plt.subplots(1, figsize=size)
    for i in range(N):
        psa, f = spectral3.mypsa(data[:, i], dt, L=L)
        ax.plot(f, psa, fmtstrs[i], label=labels[i])
    
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.legend()

    if flair_func != None:
        flair_func(ax)

def jupyter_plot_sliding_window(path, titles=None, fmtstrs=None, window=None, size=None, flair_func=None, scaling=None):
    # Check dataset format
    try:
        data = np.loadtxt(path)
    except:
        raise ValueError("Invalid data format in file")

    N = len(data[0]) - 1
    if titles == None:
        titles = ["Measurement {0}".format(i+1) for i in range(N)]
    if fmtstrs == None:
        fmtstrs = ["r-" for _ in range(N)]
    if scaling == None:
        scaling = np.ones(N+1)

    # Check graph formatting arguments
    if (titles != None and len(titles) != N):
        raise ValueError("There must be the same number of titles and columns in the dataset")
    if (fmtstrs != None and len(fmtstrs) != N):
        raise ValueError("There must be the same number of format strings and columns in the dataset")
    if (scaling != None and len(scaling) != N+1):
        raise ValueError("There must be the same number of scale factors and columns in the dataset")
    
    # Initialize default values
    if window == None:
        window = 150 * scaling[0]
    if size == None:
        size = (15, int(2.5*N))
    
    data = np.multiply(data, scaling)
    etime = data[-1, 0]
    stime = etime - window
    data = data[(data[:,0] >= stime) & (data[:,0] <= etime)]
    t = data[:,0]
    fig, ax = plt.subplots(N, figsize=size)
    for i in range(N):
        ax[i].plot(t, data[:, i+1], fmtstrs[i])
        ax[i].set_title(titles[i])

    if (flair_func != None):
        flair_func(ax)
    
    fig.tight_layout()

    while True:
        try:
            data = np.multiply(np.loadtxt(path), scaling)
            etime = data[-1, 0]
            stime = etime-window
            data = data[(data[:,0] >= stime) & (data[:,0] <= etime)]
            t = data[:,0]
            clear_output(wait=True)
            for i in range(N):
                ax[i].clear()
                ax[i].plot(t, data[:, i+1], fmtstrs[i])
                ax[i].set_title(titles[i])
                ax[i].relim()
                ax[i].autoscale_view()
            
            if flair_func != None:
                flair_func(ax)

            fig.canvas.draw_idle()
            display(fig)
            plt.pause(0.5)
        except ValueError:
            pass

"""
Todo
"""
def regular_plot_sliding_window(path, titles=None, fmtstrs=None, window=None, size=None, flair_func=None, scaling=None):
    raise NotImplementedError("To be implemented")

def plot_sliding_window(path, titles=None, fmtstrs=None, window=None, size=None, flair_func=None, scaling=None):
    if is_notebook():
        jupyter_plot_sliding_window(path, titles, fmtstrs, window, size, flair_func, scaling)
    else:
        regular_plot_sliding_window(path, titles, fmtstrs, window, size, flair_func, scaling)