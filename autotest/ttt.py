

def CheckClientLog(log_path) :
	''' '''
	line_num = 0; avg_time = []
	try:
		file = open(log_path)
		data = file.readlines()
		line_num = len(data)
		if line_num <= 7:
			return -1,[]
		data = data[7:]
		for line in data:
			if line.find('AVG') <= 0:
				break 			
			avg = line.split()[1].split('/')[1] 
			avg_time.append(int(avg))
	except IOError:
		return -2,[]
	else:
		file.close()
		return line_num, avg_time


if __name__ == '__main__':  
	ret, avg = CheckClientLog("./tt")
	print (ret, avg)
	print max(avg)
