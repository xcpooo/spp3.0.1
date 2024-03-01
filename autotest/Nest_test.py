#!/usr/bin/python

import sys
import os
import re
import time
import datetime
from RobotHelper import *
from xml_result import *
from PackageMng import *

########## global config ###########
ECHOSVR_CMD = '/home/oicq/echo/echo_svr /home/oicq/echo/echo.conf'

NEST_CENTER = {'172.27.2.78':'nest2014'}
NEST_PROXY  = {'172.27.2.78':'nest2014', '172.27.2.90':'nest2014'}
NEST_WORKER = {'172.27.6.252':'first2012++', '10.158.17.91':'tcna2009', '10.129.131.150':'tcna2009'}

CENTER_MAIN_IP     = '172.27.2.78'
CENTER_MAIN_PASSWD = 'nest2014'

SET_ID = 902

REPORT_INSTANCE = CaseResultMng('spp_nest_test.xml')   # xml report

def DateTime2MS(timediff):
	time_ms  = timediff.microseconds/1000
	time_ms += timediff.seconds * 1000
	
def CheckError(msg):
	''' '''
	reg = re.search('err|fail', msg, re.I)
	if reg:
		return True
	else:
		return False
		
def CheckSuccess(msg):
	''' '''
	#reg = re.search(r'Success', msg, re.I|re.M)
	if msg.find('Success') < 0:
		return False
	else:
		return True
	
def StartCenter():
	''' '''
	err_msg = ''
	result = 0
	path = '/usr/local/services/nest_center'
	cmd = 'cd %s/bin; ./spp.sh start >/dev/null 2>&1;\n' % path

	print '----------------------------------------------------'
	print 'start nest center'
	time_begin = int(time.time())
	for ip, passwd in NEST_CENTER.items():
		ret, output = RobotHelper.ExecSshCmd(ip, cmd, 'root', passwd)
		if ret < 0:
			err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, ip, passwd)
			print err_msg
			result = -1
			break;
			
		if CheckError(output):
			err_msg = 'start spp failed(%d,%s,%s): %s' % (ret, ip, passwd, output)
			print err_msg
			result = -2
			break;

		ret = RobotHelper.ChkSppFrameError(ip, 'root', passwd, path)
		if ret == True:
			err_msg = 'center start failed'
			print err_msg
			result = -3
			break;
			
	time_end = int(time.time())
	REPORT_INSTANCE.AddCase('Nest', 'StartCenter', time_end-time_begin, err_msg)
	if result == 0:
		print 'start nest center success'
	print '----------------------------------------------------'
	
	return result, err_msg
	
def StopCenter():
	''' '''
	err_msg = ''
	result = 0
	cmd = 'cd /usr/local/services/nest_center/bin; ./spp.sh stop;'

	print '----------------------------------------------------'
	print 'stop nest center'
	time_begin = int(time.time())
	for ip, passwd in NEST_CENTER.items():
		ret, output = RobotHelper.ExecSshCmd(ip, cmd, 'root', passwd)
		if ret < 0:
			err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, ip, passwd)
			print err_msg
			result = -1
			break;
			
		if CheckError(output):
			err_msg = 'stop spp failed(%d,%s,%s): %s' % (ret, ip, passwd, output)
			print err_msg
			result = -2
			break;
			
	time_end = int(time.time())
	REPORT_INSTANCE.AddCase('Nest', 'StopCenter', time_end-time_begin, err_msg)
	print '----------------------------------------------------'
	
	return result, err_msg
	
def AddNodes():
	''' '''
	err_msg = ''
	result = 0
	
	print '----------------------------------------------------'
	print 'start add proxy node'
	# 1 add nest proxy node
	time_begin = int(time.time())
	for ip, passwd in NEST_PROXY.items():
		cmd = 'cd /usr/local/services/nest_center/bin/script; ./add_node.sh %s 1 %d %s' % (ip, SET_ID, CENTER_MAIN_IP)
		ret, output = RobotHelper.ExecSshCmd(CENTER_MAIN_IP, cmd, 'root', CENTER_MAIN_PASSWD)
		if ret < 0:
			err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, CENTER_MAIN_IP, CENTER_MAIN_PASSWD)
			print err_msg
			result = -1
			break;

		if CheckSuccess(output) == False:
			err_msg = 'add nest proxy node failed(%s): %s' % (ip, output)
			print err_msg
			result = -2
			break;
	
	time_end = int(time.time())
	REPORT_INSTANCE.AddCase('Nest', 'AddProxyNode', time_end-time_begin, err_msg)

	# if failed, we can't continue, shit
	if result < 0:
		print '----------------------------------------------------'
		return result, err_msg
	else:
		print 'add proxy node success'
		print '----------------------------------------------------'

	
	# 2 add nest worker node
	print '----------------------------------------------------'
	print 'start add worker node'
	time_begin = int(time.time())
	for ip, passwd in NEST_WORKER.items():
		cmd = 'cd /usr/local/services/nest_center/bin/script; ./add_node.sh %s 2 %d %s' % (ip, SET_ID, CENTER_MAIN_IP)
		ret, output = RobotHelper.ExecSshCmd(CENTER_MAIN_IP, cmd, 'root', CENTER_MAIN_PASSWD)
		if ret < 0:
			err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, CENTER_MAIN_IP, CENTER_MAIN_PASSWD)
			print err_msg
			result = -3
			break;

		if CheckSuccess(output) == False:
			err_msg = 'add nest worker node failed(%s): %s' % (ip, output)
			print err_msg
			result = -4
			break;

	time_end = int(time.time())
	REPORT_INSTANCE.AddCase('Nest', 'AddWorkerNode', time_end-time_begin, err_msg)
	if result == 0:
		print 'add worker node success'
	print '----------------------------------------------------'
	
	time.sleep(10)
	
	return result, err_msg
	
def DelNodes():
	''' '''
	err_msg = ''
	result = 0
	
	print '----------------------------------------------------'
	print 'start delete proxy node'
	# 1 delete nest proxy node
	time_begin = int(time.time())
	for ip, passwd in NEST_PROXY.items():
		cmd = 'cd /usr/local/services/nest_center/bin/script; ./del_node.sh %s %d %s' % (ip, SET_ID, CENTER_MAIN_IP)
		ret, output = RobotHelper.ExecSshCmd(CENTER_MAIN_IP, cmd, 'root', CENTER_MAIN_PASSWD)
		if ret < 0:
			err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, CENTER_MAIN_IP, CENTER_MAIN_PASSWD)
			print err_msg
			result = -1
			break;

		if CheckSuccess(output) == False:
			err_msg = 'delete nest proxy node failed(%s): %s' % (ip, output)
			print err_msg
			result = -2
			break;
	
	time_end = int(time.time())
	REPORT_INSTANCE.AddCase('Nest', 'DelProxyNode', time_end-time_begin, err_msg)

	# if failed, we can't continue, shit
	if result < 0:
		return result, err_msg
	if result < 0:
		print '----------------------------------------------------'
	else:
		print 'delete proxy node success'
		print '----------------------------------------------------'
	
	# 2 delete nest worker node
	print '----------------------------------------------------'
	print 'start delete worker node'
	time_begin = int(time.time())
	for ip, passwd in NEST_WORKER.items():
		cmd = 'cd /usr/local/services/nest_center/bin/script; ./del_node.sh %s %d %s' % (ip, SET_ID, CENTER_MAIN_IP)
		ret, output = RobotHelper.ExecSshCmd(CENTER_MAIN_IP, cmd, 'root', CENTER_MAIN_PASSWD)
		if ret < 0:
			err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, CENTER_MAIN_IP, CENTER_MAIN_PASSWD)
			print err_msg
			result = -3
			break;

		if CheckSuccess(output) == False:
			err_msg = 'delete nest worker node failed(%s): %s' % (ip, output)
			print err_msg
			result = -4
			break;

	time_end = int(time.time())
	REPORT_INSTANCE.AddCase('Nest', 'DelWorkerNode', time_end-time_begin, err_msg)
	
	if result == 0:
		print 'delete worker node success'
	print '----------------------------------------------------'
	
	return result, err_msg
	
def CheckSrvProc(name, proxy_num, worker_num):
	''' '''
	result = 0
	err_msg = ''
	
	# 1. check proxy total num 
	if proxy_num != 0 and proxy_num != len(NEST_PROXY):
		return -1, 'proxy total error'
		
	# 2. check every proxy node
	proxy_name = 'spp_' + name + '_proxy'
	cmd = 'ps -ef | grep "%s" | grep -v "grep" | wc -l' % proxy_name
	for ip, passwd in NEST_PROXY.items():
		ret, output = RobotHelper.ExecSshCmd(ip, cmd, 'root', passwd)
		if ret < 0:
			err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, ip, passwd)
			print err_msg
			result = -2
			break;
			
		proc_num = int(output)
		if proc_num != 1 and proc_num != 0:
			err_msg = 'service proxy process num error.(%s %d)' % (ip, proc_num)
			print err_msg
			result = -3
			break;
			
	if result < 0:
		return result, err_msg
	
	# 3. check worker proc total
	proc_num = 0
	worker_name = 'spp_' + name + '_worker'
	cmd = 'ps -ef | grep "%s" | grep -v "grep" | wc -l' % worker_name
	for ip, passwd in NEST_WORKER.items():
		ret, output = RobotHelper.ExecSshCmd(ip, cmd, 'root', passwd)
		if ret < 0:
			err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, ip, passwd)
			print err_msg
			return -4, err_msg
			
		proc_num += int(output)
		
	if proc_num != worker_num:
		err_msg = 'service worker process num error.(%d %d)' % (worker_num, proc_num)
		print err_msg
		result = -5
	
	return result, err_msg
	
	
def AddService(srvinfo):
	''' '''
	result = 0
	err_msg = ''
	nums = [0]
	print '----------------------------------------------------'
	print 'start add service %s' % srvinfo['name']
	cmd = 'cd /usr/local/services/nest_center/bin/script; ./add_service.sh %s %d %s' % (srvinfo['add_conf'], SET_ID, CENTER_MAIN_IP)
	time_begin = int(time.time())
	ret, output = RobotHelper.ExecSshCmd(CENTER_MAIN_IP, cmd, 'root', CENTER_MAIN_PASSWD)
	if ret < 0:
		err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, CENTER_MAIN_IP, CENTER_MAIN_PASSWD)
		print err_msg
		result = -1
	elif CheckSuccess(output) == False:
		err_msg = 'add service failed(%s): %s' % (srvinfo['name'], output)
		print err_msg
		result = -2
	else:
		reg = re.compile('[0-9]+')
		nums = reg.findall(output)
		if len(nums) != 2:
			err_msg = 'add service failed!(%s): %s' % (srvinfo['name'], output)
			print err_msg
			result = -3
		else:
			nums[0] = int(nums[0])
			nums[1] = int(nums[1])
			ret, err_msg = CheckSrvProc(srvinfo['name'], nums[0], nums[1])
			if ret < 0:
				err_msg = 'check srv proc failed(%s,%d): %s' % (srvinfo['name'], ret, output)
				print err_msg
				result = -4

	time_end = int(time.time())
	case_name = 'AddService.' + srvinfo['name']
	REPORT_INSTANCE.AddCase('Nest', case_name, time_end-time_begin, err_msg)
	if result == 0:
		print 'add service %s success' % srvinfo['name']
	print '----------------------------------------------------'
	
	#time.sleep(10)
	
	return result, err_msg, nums
	
def DelService(srvinfo):
	''' '''
	result = 0
	err_msg = ''
	print '----------------------------------------------------'
	print 'start delete service %s' % srvinfo['name']
	cmd = 'cd /usr/local/services/nest_center/bin/script; ./del_service.sh %s %d %s' % (srvinfo['name'], SET_ID, CENTER_MAIN_IP)
	time_begin = int(time.time())
	ret, output = RobotHelper.ExecSshCmd(CENTER_MAIN_IP, cmd, 'root', CENTER_MAIN_PASSWD)
	if ret < 0:
		err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, CENTER_MAIN_IP, CENTER_MAIN_PASSWD)
		result = -1
		print result, err_msg
	elif CheckSuccess(output) == False:
		err_msg = 'del service failed(%s): %s' % (srvinfo['name'], output)
		result = -2
		print result, err_msg
	else: 
		ret, err_msg = CheckSrvProc(srvinfo['name'], 0, 0)
		if ret < 0:
			err_msg = 'check srv proc failed(%s): %s' % (srvinfo['name'], output)
			result = -3
			print result, ret, err_msg

	time_end = int(time.time())
	case_name = 'DelService.' + srvinfo['name']
	REPORT_INSTANCE.AddCase('Nest', case_name, time_end-time_begin, err_msg)
	if result == 0:
		print 'delete service %s success' % srvinfo['name']
	print '----------------------------------------------------'
	
	time.sleep(10)
	
	return result, err_msg
	

def ExpService(srvinfo, proc_nums):
	''' '''

	result = 0
	err_msg = ''

	if srvinfo.has_key('exp_conf') == False:
		return result, err_msg

	print '----------------------------------------------------'
	print 'Start expantion service %s' % srvinfo['name']
	cmd = 'cd /usr/local/services/nest_center/bin/script; ./exp_service.sh %s %d %s' % (srvinfo['exp_conf'], SET_ID, CENTER_MAIN_IP)
	time_begin = int(time.time())
	ret, output = RobotHelper.ExecSshCmd(CENTER_MAIN_IP, cmd, 'root', CENTER_MAIN_PASSWD)
	if ret < 0:
		err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, CENTER_MAIN_IP, CENTER_MAIN_PASSWD)
		print err_msg
		result = -1
	elif CheckSuccess(output) == False:
		err_msg = 'expantion service failed(%s): %s' % (srvinfo['name'], output)
		print err_msg
		result = -2
	else:
		reg = re.compile('[0-9]+')
		nums = reg.findall(output)
		if len(nums) != 2:
			err_msg = 'expantion service failed(%s): %s' % (srvinfo['name'], output)
			print err_msg
			result = -3
		else:
			nums[0] = int(nums[0])
			nums[1] = int(nums[1])
			ret, err_msg = CheckSrvProc(srvinfo['name'], proc_nums[0]+nums[0], proc_nums[1]+nums[1])
			if ret < 0:
				err_msg = 'check srv proc failed(%s): %d %s' % (srvinfo['name'], ret, output)
				print err_msg
				result = -4
					
	time_end = int(time.time())
	case_name = 'ExpService.' + srvinfo['name']
	REPORT_INSTANCE.AddCase('Nest', case_name, time_end-time_begin, err_msg)
	if result == 0:
		print 'expantion service %s success' % srvinfo['name']
	print '----------------------------------------------------'
	
	return result, err_msg
	
def RunIo(echo_conf):
	''' '''
	result = 0
	err_msg = ''
	
	# 1. Init echo server 
	if echo_conf.has_key('echo_server'):
		for name, server in echo_conf['echo_server'].items():
			cmd = 'cd %s; ./echo_svr %s &' % (server['path'], server['conf'])
			ret, output = RobotHelper.ExecSshCmd(server['ip'], cmd, 'root', server['passwd'])
			if ret < 0:
				err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, server['ip'], server['passwd'])
				print err_msg
				return -1, err_msg

	# 2. Init echo client 
	for name, client in echo_conf['echo_client'].items():
		cmd = 'cd %s; ./echo_print %s %s > /dev/null &' % (client['path'], client['conf'], client['log_path'])
		print client
		print cmd 
		ret, output = RobotHelper.ExecSshCmd(client['ip'], cmd, 'root', client['passwd'])
		if ret < 0:
			err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, client['ip'], client['passwd'])
			print err_msg
			return -2, err_msg
	
	return result, err_msg
	
def StopIo(echo_conf):
	''' '''
	result = 0
	err_msg = ''
	
	# 1. stop echo client 
	for name, client in echo_conf['echo_client'].items():
		cmd = 'killall -9 echo_print'
		ret, output = RobotHelper.ExecSshCmd(client['ip'], cmd, 'root', client['passwd'])
		if ret < 0:
			err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, client['ip'], client['passwd'])
			print err_msg
			result = -1
	
	# 2. Stop echo server 
	if echo_conf.has_key('echo_server') == False:
		return result, err_msg
		
	for name.server in echo_conf['echo_server'].items():
		cmd = 'killall -9 echo_svr'
		ret, output = RobotHelper.ExecSshCmd(server['ip'], cmd, 'root', server['passwd'])
		if ret < 0:
			err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, server['ip'], server['passwd'])
			print err_msg
			result = -2
	
	return result, err_msg
	
def CheckClientLog(echo_log, client_conf) :
	''' '''
	avg_time = []
	
	data = echo_log.split('\n')
	
	if len(data) == 0:
		print 'no echo client log'
		return False
	
	for line in data:
		if line.find('AVG') <= 0:
			continue
		avg = line.split()[1].split('/')[1]
		avg_time.append(int(avg))
		
	if len(avg_time) == 0:
		print 'no echo client log(avg_time)'
		return False
		
	if max(avg_time) > client_conf['delay_max']:
		print 'echo client timeout(avg bigger than delay_max:%d)' % client_conf['delay_max']
		return False
		
	return True
		
	
def CheckIoResult(srvinfo, echo_conf):
	''' '''
	time.sleep(10)

	result = 0
	err_msg = ''

	time_begin = int(time.time())

	print '----------------------------------------------------'
	print 'Start IO test, service name:%s' % srvinfo['name']
	for name, client in echo_conf['echo_client'].items():
		cmd = 'cd %s; cat %s' % (client['path'], client['log_path'])
		ret, output = RobotHelper.ExecSshCmd(client['ip'], cmd, 'root', client['passwd'])
		if ret < 0:
			result = -1
			err_msg = 'execult ssh cmd failed(%d,%s,%s)' % (ret, client['ip'], client['passwd'])
			print err_msg
			break;

		print output

		# check Io Result
		ret = CheckClientLog(output, client)
		if ret == False:
			result = -2
			errmsg = 'echo client log error'
			print errmsg
			break;
	if result == 0:
		print 'IO test OK'	
	print '----------------------------------------------------'

	time_end = int(time.time())
	case_name = 'IOtest.' + srvinfo['name']
	REPORT_INSTANCE.AddCase('Nest', case_name, time_end-time_begin, err_msg)

	# Stop Io
	StopIo(echo_conf)

	return 0, ''

def UpdateServicePkg(srvinfo, echo_conf):
	''' '''
	result = 0
	err_msg = ''
	
	# 1 update all service pkg
	for ip, passwd in NEST_PROXY.items():
		ret, output = PackageMng.UpdateSrvPkg(ip, 'root', passwd, srvinfo['name'], srvinfo['pkg_name'])
		if ret < 0:
			result = -1
			err_msg = 'update srv pkg failed, ip:%s ret:%d output:%s' % (ip, ret, output)
			print err_msg
			return result, err_msg


	for ip, passwd in NEST_WORKER.items():
		ret, output = PackageMng.UpdateSrvPkg(ip, 'root', passwd, srvinfo['name'], srvinfo['pkg_name'])
		if ret < 0:
			result = -2
			err_msg = 'update srv pkg failed, ip:%s ret:%d output:%s' % (ip, ret, output)
			print err_msg
			return result, err_msg

	# 2 update echo conf file TODO:shit
	

	return result, err_msg
	

def ServiceTest(srvinfo, echo_conf):
	''' '''
	result = 0
	err_msg = ''

	ret, err_msg = UpdateServicePkg(srvinfo, echo_conf)
	if ret < 0:
		return ret, err_msg
	
	# 1 add service
	ret, err_msg, proc_nums = AddService(srvinfo)
	if ret < 0:
		return ret, err_msg
	
	# 2 run io
	ret, err_msg = RunIo(echo_conf)
	
	# 3 test expantion
	ret, err_msg = ExpService(srvinfo, proc_nums)
	
	# 4 check io result and stop io 
	ret, err_msg = CheckIoResult(srvinfo, echo_conf)
	
	# 5 Delete Service
	ret, err_msg = DelService(srvinfo)
	
	return ret, err_msg

def RestartAgent():
	''''''
	result = 0
	err_msg = ''

	print '----------------------------------------------------'
	print 'restart nest agent'
	for ip, passwd in NEST_PROXY.items():
		ret, output = PackageMng.UpdateAgentProc(ip, 'root', passwd)
		if ret < 0:
			result = -1
			err_msg = 'update agent process failed, %d, %s' % (ret, output)
			print err_msg
			print 'restart nest agent failed'
			print '----------------------------------------------------'
			return result, err_msg

	for ip, passwd in NEST_WORKER.items():
		ret, output = PackageMng.UpdateAgentProc(ip, 'root', passwd)
		if ret < 0:
			result = -2
			err_msg = 'update agent process failed, %d, %s' % (ret, output)
			print err_msg
			print 'restart nest agent failed'
			print '----------------------------------------------------'

	print 'restart nest agent Success'
	print '----------------------------------------------------'
	return result, err_msg

def UpdateSpp():
	''' '''
	
	result = 0
	err_msg = ''
	for ip, passwd in NEST_PROXY.items():
		ret, output = PackageMng.UpdateSppPkg(ip, 'root', passwd)
		if ret < 0:
			result = -1
			err_msg = 'update spp package failed:%d %s' % (ret, output)
			print err_msg
			return result, err_msg


	for ip, passwd in NEST_WORKER.items():
		ret, output = PackageMng.UpdateSppPkg(ip, 'root', passwd)
		if ret < 0:
			result = -2
			err_msg = 'update spp package failed:%d %s' % (ret, output)
			print err_msg
			return result, err_msg

	return result, err_msg

	
def RunNestTest():
	''' '''

	# 1 restart nest agent
	ret, err_msg = RestartAgent()
	if ret < 0:
		return -1

	# 2 start nest center
	ret, err_msg = StartCenter()
	if ret < 0:
		return -2

	# 3 add nest node test
	ret, err_msg = AddNodes()
	if ret < 0:
		return -3

	# 4 update spp package
	ret, err_msg = UpdateSpp()
	
	# 5 service test
	ECHO_CONF = {'echo_client':
			{ 'client1': {'ip':'172.27.2.90', 'passwd':'nest2014', 'path':'/home/oicq/echo', 'conf':'echo_1.conf', 'log_path':'nest_echo.log', 'delay_max':80000} }
	#	     'echo_server':
	#		{ 'server1': {'ip':'172.27.2.78', 'passwd':'nest2014', 'path':'/home/oicq/echo', 'conf':'echo.conf'}}
		    }

	SRVINFO_test = {'name':'test', 'pkg_name':'test', 'add_conf':'service_add.conf', 'exp_conf':'service_exp_worker.conf'}
	ServiceTest(SRVINFO_test, ECHO_CONF)
	
	# 6 delete nest node 
	ret, err_msg = DelNodes()
	
	# 7 stop nest center
	ret , err_msg = StopCenter()

def Run():
	os.system('rm -f spp_nest_test.xml')

	# 1 run nest test
	RunNestTest()

	# 2 write test result to xml file
	REPORT_INSTANCE.DomWriteXml()

if __name__ == '__main__':
	Run()
