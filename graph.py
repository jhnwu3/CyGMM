import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from pathlib import Path
import sys

# def plot_confidence_interval(x, values, z=1.96, color='#2187bb', horizontal_line_width=0.25):
#     mean = np.mean(x)
#     stdev = np.std(x)
#     confidence_interval = z * stdev / np.sqrt(len(values))

#     left = x - horizontal_line_width / 2
#     top = mean - confidence_interval
#     right = x + horizontal_line_width / 2
#     bottom = mean + confidence_interval
#     plt.plot([x, x], [top, bottom], color=color)
#     plt.plot([left, right], [top, top], color=color)
#     plt.plot([left, right], [bottom, bottom], color=color)
#     plt.plot(x, mean, 'o', color='#f44336')

#     return mean, confidence_interval
def compute_moments(x):
    means = np.average(x,axis=0)
    variances = np.var(x, axis=0)
    # n means/variances, (n^2 -n)/2 covariances, 
    cov = []
    for i in range(x.shape[1]):
        for j in range(i+1, x.shape[1]):
           cov.append(np.cov(x[:,i],x[:,j])[0][1])
    return np.append(means, np.append(variances,cov))

def getFile(args, multi=False):
    if multi:
        return args[args.index('-f') + 1], args[args.index('-f') + 2]
    return args[args.index('-f') + 1]

def getName(args):
    if '-n' in args:
        return args[args.index('-n') + 1]
    return "Confidence Intervals"

def getTime(args):
    return args[args.index('-t') + 1]

def getGraphType(args):
    return args[args.index('-g') + 1]

def getSimulatedRates(args):
    return args[args.index('-r') + 1]

class Graph:
    
    def plot_confidence_interval(x, values, z=1.96, color='#2187bb', horizontal_line_width=0.25):
        mean = np.mean(values)
        stdev = np.std(values)
        confidence_interval = z * stdev / np.sqrt(len(values))

        left = x - horizontal_line_width / 2
        top = mean - confidence_interval
        right = x + horizontal_line_width / 2
        bottom = mean + confidence_interval
        plt.plot([x, x], [top, bottom], color=color)
        plt.plot([left, right], [top, top], color=color)
        plt.plot([left, right], [bottom, bottom], color=color)
        plt.plot(x, mean, 'o', color='#f44336')

        return mean, confidence_interval
    
    # def __init__(self, dir):
    #     self.graphingDirectory = "/frontend/graph/" + dir +"/"
    #     self.fileName = dir
    #     # self.name = Graph_Writer.getFileName(argv)
    #     # self.t = Graph_Writer.getTime(argv)
    #     # self.estimates = np.genfromtxt(Graph_Writer.graphingDirectory + self.name + '_estimates.csv', delimiter=',')
    #     # print(self.estimates.shape)
    #     # self.moments = np.genfromtxt()
        
        
    def plotConfidenceIntervals(z, file, simulated = False, trueRatesFile='', title=""):
        df = pd.read_csv(file)
        estimates = df.to_numpy()
        # estimates = np.genfromtxt(path, delimiter=',')
        nRates = estimates.shape[1] - 1
        categoriesNumeric = []
        for i in range(estimates.shape[1]): 
            categoriesNumeric.append(i+1)
            
        plt.xticks(categoriesNumeric, df.columns)
        plt.title(title)
        for i in range(nRates):
            Graph.plot_confidence_interval(i + 1, estimates[:,i], z)
        if simulated:
            tRates = np.genfromtxt(trueRatesFile, delimiter=',')
            print(tRates)
            for i in range(nRates):
                plt.plot(i+1,tRates[i], 'D', color='#013220')
        plt.show()
        plt.savefig(file[:-4] + '_estimates.png')
        
    def plotMomentsWithActualEvolvedMatrices(xName, yName, gTitle=""): # this might make more sense overall actually. From here, we can get means, variances, and covariances.
        dfX = pd.read_csv(xName)
        dfY = pd.read_csv(yName)
        x = dfX.to_numpy()
        y = dfY.to_numpy()
        plt.title(xName[:-4] + " and " + yName[:-4])
        plt.xlabel("Estimated Moment")
        plt.ylabel("Observed Moment")
        xMoments = compute_moments(x)
        yMoments = compute_moments(y)
        plt.plot(np.unique(xMoments), np.poly1d(np.polyfit(xMoments, yMoments, 1))(np.unique(xMoments)))
        x123 = np.arange(0, np.max(xMoments))
        y123 = x123
        plt.plot(np.unique(x123), np.poly1d(np.polyfit(x123, y123, 1))(np.unique(x123)), color='#f44336')
        plt.scatter(xMoments, yMoments)
        plt.savefig(xName[:-7] + '.png')
    
    # assume data format n moments X 2 columns (for X and Y) 
    def plotMoments(file): # get list of X, Y 
        df = pd.read_csv(file)
        moments = df.to_numpy()
        plt.title(file[:-4])
        plt.xlabel("Estimated Moment")
        plt.ylabel("Observed Moment")
        plt.scatter(moments[:,0],moments[:,1])
        x123 = np.arange(0, np.max(moments[:]))
        y123 = x123
        plt.plot(np.unique(x123), np.poly1d(np.polyfit(x123, y123, 1))(np.unique(x123)), color='red')
        plt.plot(np.unique(moments[:,0]), np.poly1d(np.polyfit(moments[:,0], moments[:,1], 1))(np.unique(moments[:,0])))
        plt.savefig(file[:-4] + '.png')
        
if '-h' in sys.argv:
    print("Specify graphing type with -g <graph type> ")
    print("Specify files with -f <filename> <optional 2nd final name only if graph type is 'dMoments'>")
    exit(0)
        
if "-f" not in sys.argv:
    print("Error Need to Specify Graph Files with -f")
    exit(0)
    
if "-g" not in sys.argv:
    print("Error Need to Specify Graph Types with -g")
    exit(0)

# graph.plotConfidenceIntervals(1.96)
graphType = getGraphType(sys.argv)
if graphType == 'CI':
    Graph.plotConfidenceIntervals(1.96, getFile(sys.argv), title=getName(sys.argv))
elif graphType == 'CI_truth':
    Graph.plotConfidenceIntervals(1.96, getFile(sys.argv),simulated=True, trueRatesFile=getSimulatedRates(sys.argv), title=getName(sys.argv))
elif graphType == 'Moments':
    Graph.plotMoments(getFile(sys.argv))
elif graphType == 'dMoments':
    dataX, dataY = getFile(sys.argv, multi=True)
    Graph.plotMomentsWithActualEvolvedMatrices(dataX,dataY)
else:
    print("Error Invalid Graph Type Inputted:", graphType)
    print("")