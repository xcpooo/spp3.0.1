#!/usr/bin/env python
# Copyright (C) 2014, Tencent
# All rights reserved.
#
# Filename   : RobotHelper.py
# Description: Robot common function
# Author     : xxx@tencent.com
# Version    : 1.0
# Date       : 2014-10-17
# Last Update: 2014-10-17


import sys
import os
from RobotHelper import *
from xml_result import *


########## global config ###########
ECHOSVR_CMD	=   '/home/oicq/echo/echo_svr /home/oicq/echo/echo.conf'
L5_RESTART  =   '/usr/local/services/l5_protocol_32os-1.0/admin/restart.sh all'

ECHO_SVR_A	=	'172.27.2.78'
ECHO_SVR_B	=	'172.27.2.90'
ECHO_SVR_C	=	'172.27.6.252'
SPP_FRAME	=   '10.198.0.231'

MAGIC_NUM_1 =	'nest2014'
MAGIC_NUM_2 =	'first2012++'
MAGIC_NUM_3 =   'c86a7ee3d8ef'

XML_SUITES   =   CaseResultMng("spp_l5_test.xml")

##########  function def ###########

def EnvInit():
	''' '''
	# 1. init echo server up
	b_chk = CheckSvrUp()
	if b_chk == False:
		return False
	
	# 2. check l5 agent up
	b_chk = CheckL5agent()
	if b_chk == False:
		return False
	
	# 3. stop server
	EnvTerm()
	
	# 3. ok
	return True
	
	
def EnvTerm():
	'''
	'''
	# 1. term 32bit spp
	StopSppFrame('/tmp/spp_usr00/3.0_L5/spp32')
	
	# 2. term 64bit spp
	StopSppFrame('/tmp/spp_usr00/3.0_L5/spp64')
	
	return
	

def Run():
	''' '''
	os.system('rm -f spp_l5_test.xml')
	RunL5Test(SPP_FRAME, '/tmp/spp_usr00/3.0_L5/spp32', 'user_00', MAGIC_NUM_3)
	RunL5Test(SPP_FRAME, '/tmp/spp_usr00/3.0_L5/spp64', 'user_00', MAGIC_NUM_3)
	XML_SUITES.DomWriteXml()
	
	
def RunL5Test(ip, path, user, passwd) :
	''' '''
	# 1.1 test non state
	cmd = 'cd %s/bin; install ./relay_logic.so_no_state ./relay_logic.so' % path
	ret, output = RobotHelper.ExecSshCmd(ip, cmd, user, passwd)
	if ret < 0 :
		print "set non stat env failed"
		return
	
	# 1.2 exec test
	print '------------------------------------------------'
	print 'Start test L5 non state route! (%s)' % path
	cost, errmsg = TestOneTime(ip, path, user, passwd)
	XML_SUITES.AddCase('SPP_L5_%s' % path[-2:] , 'non-state', cost, errmsg)
	
	# 2.1 test state
	cmd = 'cd %s/bin; install ./relay_logic.so_state ./relay_logic.so' % path
	ret, output = RobotHelper.ExecSshCmd(ip, cmd, user, passwd)
	if ret < 0 :
		print "set state route env failed"
		return
	
	# 2.2 exec test
	print '------------------------------------------------'
	print 'Start test L5 state route! (%s)' % path
	cost, errmsg = TestOneTime(ip, path, user, passwd)
	XML_SUITES.AddCase('SPP_L5_%s' % path[-2:] , 'state', cost, errmsg)
	

	# 3.1 test anti
	cmd = 'cd %s/bin; install ./relay_logic.so_anti ./relay_logic.so' % path
	ret, output = RobotHelper.ExecSshCmd(ip, cmd, user, passwd)
	if ret < 0 :
		print "set state route env failed"
		return
	
	# 2.2 exec test  
	print '------------------------------------------------'
	print 'Start test L5 anti route! (%s)' % path
	cost, errmsg = TestOneTime(ip, path, user, passwd)
	XML_SUITES.AddCase('SPP_L5_%s' % path[-2:] , 'anti', cost, errmsg)
	

	
def  TestOneTime(ip, path, user, passwd) :
	''' '''
	os.system("sleep 2")	

	# 1. init env
	ret = EnvInit()
	if ret == False:
		errmsg = 'EnvInit error'
		print errmsg
		return 0, errmsg
	
	# 2. start test frame
	ret = StartSppFrame(path)
	if ret == False:
		errmsg = 'start %s error' % path
		print errmsg
		return 0, errmsg
	
	# 3. start client
	log_path = '/home/oicq/echo/tmp.log'
	start_time = int(time.time())  
	os.system("/home/oicq/echo/echo_print /home/oicq/echo/echo.conf %s > /dev/null &" % log_path)
	
	loop = 12; errline = 0; logline = 0
	while loop > 0:
		loop = loop - 1
		os.system("sleep 10")
		
		# 3.1 check spp frame error
		ret = RobotHelper.ChkSppFrameError(ip, user, passwd, path)
		if ret == True:
			errmsg = 'spp frame error!!'
			print errmsg
			return 0,errmsg
		
		# 3.2 check spp service error
		num = RobotHelper.GetSppErrorLogNum(ip, user, passwd, path)
		if num - errline > 100 :
			errmsg = 'spp more error: %d' % (num - errline)
			print errmsg
			return 0,errmsg
		errline = num
		
		# 3.3 check client output
		logline, avg = CheckClientLog(log_path, logline)
		if logline < 0 or len(avg) == 0:
			errmsg = 'client log error: %d' % logline
			print errmsg
			return 0,errmsg
		if max(avg) > 8000:
			errmsg = 'client avgtime %d error' % max(avg)
			print errmsg
			return 0,errmsg
		#print (logline, sum(avg)/len(avg))
				
		# 3.4 check l5 report error rate
	
	# 4. stop client, check cpu
	os.system("killall -9 echo_print; rm %s" % log_path)
	end_time = int(time.time())  
	cpu = RobotHelper.GetAvgCpuLoad(ip, start_time, end_time)
	if cpu > 50:
		errmsg = 'spp more cpu load: %d' % cpu
		print errmsg
		return 0,errmsg
	print 'Test %s this time ok' % path
	
	EnvTerm()
	return (end_time - start_time, '')
	
	
def CheckClientLog(log_path, log_line) :
	''' '''
	line_num = 0; avg_time = []
	try:
		file = open(log_path)
		data = file.readlines()
		line_num = len(data)
		if line_num < log_line:
			return -1,[]
		data = data[log_line:]
		for line in data:
			if line.find('AVG') <= 0:
				continue	
			avg = line.split()[1].split('/')[1] 
			avg_time.append(int(avg))
	except IOError:
		return -2,[]
	else:
		file.close()
		return line_num, avg_time


def CheckSvrUp():
	''' '''
	# 1. check start echo server
	CheckStartEchoSvr(ECHO_SVR_A, '15000', 'root', MAGIC_NUM_1)
	CheckStartEchoSvr(ECHO_SVR_B, '15000', 'root', MAGIC_NUM_1)
	CheckStartEchoSvr(ECHO_SVR_C, '15000', 'root', MAGIC_NUM_2)
	
	
def CheckStartEchoSvr(ip, port, user, passwd):
	''' '''
	ret = RobotHelper.CheckUdpPort(ip, port, user, passwd)
	if ret == False:
		print "restart " + ip + " echo server" 
		RobotHelper.ExecSshCmd(ip, ECHOSVR_CMD, user, passwd)

		
def CheckL5agent():
	''' '''
	ret = RobotHelper.CheckUdpPort(SPP_FRAME, '8888', 'user_00', MAGIC_NUM_3)
	if ret == False:
		RobotHelper.ExecSshCmd(SPP_FRAME, L5_RESTART, 'user_00', MAGIC_NUM_3)
	

def StopSppFrame(path):
	''' '''
	cmd = "cd %s/bin; ./spp.sh stop; rm ../log/*" % path
	ret, out_put = RobotHelper.ExecSshCmd(SPP_FRAME, cmd, 'user_00', MAGIC_NUM_3)
	if ret < 0:
		return False
	

def StartSppFrame(path):
	''' '''
	cmd = "cd %s/bin; ./spp.sh start" % path
	ret, out_put = RobotHelper.ExecSshCmd(SPP_FRAME, cmd, 'user_00', MAGIC_NUM_3)
	if ret < 0:
		return False
	ret = RobotHelper.CheckUdpPort(SPP_FRAME, '6000', 'user_00', MAGIC_NUM_3)
	if ret == False:
		return False
	return True



if __name__ == '__main__':  
	Run()
	
