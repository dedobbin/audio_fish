import numpy as np
import matplotlib.pyplot as plt
import socket
import sys
import re

#16 bit stereo
BYTES_PER_SAMPLE = 2
NUM_CHANNELS = 2
SAMPLE_MIN = -32768
SAMPLE_MAX = 32767


def readConfigRule(path, key):
    f = open(path, "r")
    content = f.read()
    f.close() 
    result = re.search(key+'=(.*)\n*', content, re.M)
    return result.group(1)

def mapFunc(input):
    input = np.int16(input)
    return (input - SAMPLE_MIN) * (1.0 - - 1.0) / (SAMPLE_MAX - SAMPLE_MIN) + -1.0;

def getFromSocket(sock, port, amount):
	socketData = []
	#Keep reading until got expected size
	while (len(socketData) < amount):
		u = 0;
		bytesToRec = int(readConfigRule('config.txt', 'message_size'))
		try:
			newData = sock.recv(amount)
		except Exception as e:
			print(e)
		socketData += newData
		if (len(newData) == 0):
			print("Client: Peer closed socket")
			break
	if (len(socketData) != amount and len(socketData) != 0):
		print("message size not expected size, expected: " + str(amount) + ', got: ' + str(len(socketData)))

	return socketData

def rawDataToSamples(socketData, nSamples):
	u = 0 
	x = 0
	j = 0
	channels = [[], []] #stereo

	while u < nSamples / 4:# 2bytes per samples, 2 samples because stereo
		try:
			channels[0].append(mapFunc(socketData[u] | socketData[u+1] << 8)) 
			channels[1].append(mapFunc(socketData[u+2] | socketData[u+3] << 8)) 
			u += 4
		except Exception as e: 
			print("Error while converting raw data to samples: " +str(e))
			return False
	return channels

def connectToServer(port):
	try:
		server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		server_sock.connect(("127.0.0.1", port))
	except Exception as e:
		return False
	return server_sock
	print("Connected to " + str(port));

def plotCloseHandler(evt):
	global plzClose 
	plzClose = True

def plotDraw(channels, nSamplesToDraw):
	yValues = list(range(len(channels[0]))) 
	plt.cla()
	plt.ylim(-1.1, 1.1)
	plt.plot(yValues, channels[0])

def plotInit():
	fig = plt.figure()
	fig.canvas.mpl_connect('close_event', plotCloseHandler)

def main():
	global plzClose 
	nSamplesToDraw = 44100 * 8
	port = int(readConfigRule('config.txt', 'server_port'))
	serverMessageSize = int(readConfigRule('config.txt', 'message_size'))
	
	serverSock = connectToServer(port)
	if not serverSock:
		print("Cannot connect to server")
		exit()

	plotInit()
	
	socketData = []
	while (not plzClose):
		if serverSock:
			socketData += getFromSocket(serverSock, port, serverMessageSize)
			channels = rawDataToSamples(socketData, nSamplesToDraw)

		if not channels:
			#print("No connection")
			plt.cla()
			#Keep checking for a connection
			if serverSock:
				serverSock.close()
			serverSock = connectToServer(port)
		else:
			plotDraw(channels, nSamplesToDraw)
		
		#remove 'used' data
		socketData = socketData[nSamplesToDraw:]
		plt.pause(0.00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001)

	if (serverSock):
		serverSock.close()
	print("Exit")

if __name__ == '__main__':
	#globals
	plzClose = False

	main()