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
import time
import subprocess
import pexpect
import json
import urllib
import urllib2


class RobotHelper:
	''' Robot helper class '''
	
	@staticmethod
	def ExecSshCmd(remote_ip, cmd, user, passwd):
		''' 
		Exec ssh command
		'''
		result = -1
		output = ""
		ssh = pexpect.spawn('ssh %s@%s "%s"' % (user, remote_ip, cmd))
		ssh.setecho(False)
		try:
			index = ssh.expect(['password:', 'continue connecting (yes/no)?'], timeout=60)
			if index == 0:
				ssh.sendline(passwd)
			elif index == 1:
				ssh.sendline('yes\n')
				ssh.expect('password: ')
				ssh.sendline(passwd)
			
			output = ssh.read()
			result = 0
			
		except pexpect.EOF:
			ssh.close()
			result = -2
		
		except pexpect.TIMEOUT:
			ssh.close()
			result = -3
		
		return result, output

	@staticmethod
	def ExecScpCmd(local_file, remote_file, passwd):
		'''
		Exec scp command
		'''
		result = -1
		output = ""
		scp = pexpect.spawn('scp %s %s' % (local_file, remote_file))
		scp.setecho(False)
		try:
			index = scp.expect(['password:', 'continue connecting (yes/no)?'], timeout=60)
			if index == 0:
				scp.sendline(passwd)
			elif index == 1:
				scp.sendline('yes\n')
				scp.expect('password: ')
				scp.sendline(passwd)

			output = scp.read()
			result = 0

		except pexpect.EOF:
			scp.close()
			result = -2
				
		except pexpect.TIMEOUT:
			scp.close()
			result = -3

		return result, output
		
		
	@staticmethod
	def GetMonitorValue(ip, start_time, end_time, attr):
		''' Get Monitor info'''
		query =[]
		ip_attrs = {"ip":ip,"attrid":attr}
		query.append(ip_attrs)
		query_str = "&query=" + json.dumps(query)
		monitor_url = "http://monitor-api.server.com/get_min_data?" 
		monitor_url += urllib.urlencode({"idtype":1, "begtime": start_time, "endtime": end_time, "datatype":0})
		monitor_url += query_str
		monitor_url = monitor_url.replace(" ", "")
		
		resp = urllib2.urlopen(monitor_url);
		resp_html = resp.read();
		monitor_data = json.loads(bytes.decode(resp_html),encoding="utf-8");
		if "data" not in monitor_data:
			return []
		for ip_data in monitor_data["data"]:
			if ip_data["ip"] == ip:
				return ip_data["value"]
		return []
		
	
	@staticmethod
	def GetAvgCpuLoad(ip, start_time, end_time):
		''' Get avg cpu load '''
		cpu_array = RobotHelper.GetMonitorValue(ip, start_time, end_time, 69123)
		if len(cpu_array) > 0 :
			return sum(cpu_array)/len(cpu_array)
		else:
			return 0
	
	@staticmethod
	def ChkSppFrameError(ip, user, passwd, path):
		''' check spp frame error '''
		chk_cmd = "cd %s/log; grep ERROR spp_frame_*" % path
		ret, output = RobotHelper.ExecSshCmd(ip, chk_cmd, user, passwd)
		if ret < 0:
			return False
		if output.find("ERROR") >= 0:
			return True
		else:
			return False
			
	@staticmethod
	def GetSppErrorLogNum(ip, user, passwd, path):
		''' check spp frame error '''
		chk_cmd = "cd %s/log; grep ERROR spp_* |wc -l" % path
		ret, output = RobotHelper.ExecSshCmd(ip, chk_cmd, user, passwd)
		if ret < 0:
			return -1
		if output.find("ERROR") >= 0:
			return True
		else:
			return False

	@staticmethod
	def CheckUdpPort(ip, port, user, passwd):
		''' check udp port '''
		chk_cmd = "netstat -lunp |grep %s" % port
		ret, output = RobotHelper.ExecSshCmd(ip, chk_cmd, user, passwd)
		if ret < 0:
			return -1
		if output.find("udp") >= 0:
			return True
		else:
			return False

	@staticmethod
	def CheckTcpPort(ip, port, user, passwd):
		''' check tcp port '''
		chk_cmd = "netstat -ltnp |grep %s" % port
		ret, output = RobotHelper.ExecSshCmd(ip, chk_cmd, user, passwd)
		if ret < 0:
			return -1
		if output.find("tcp") >= 0:
			return True
		else:
			return False

if __name__ == '__main__':  
	ret, output = RobotHelper.ExecSshCmd('172.27.2.90', 'cd /home/oicq; ls ./', 'root', 'nest2014')
	if ret == -3:
		print 'TimeOut'
	else :
		print output
	value = RobotHelper.GetMonitorValue('172.27.2.90', 1413807000, 1413807400, 69123)
	print value
	cpu = RobotHelper.GetAvgCpuLoad('172.27.2.90', 1413807000, 1413807400)
	print cpu
	berror = RobotHelper.ChkSppFrameError('172.27.2.90', 'root', 'nest2014', '/home/zhiguocai/spp2.10.9/spp')
	print berror
	errnum = RobotHelper.GetSppErrorLogNum('172.27.2.90', 'root', 'nest2014', '/home/zhiguocai/spp2.10.9/spp')
	print errnum
	errnum = RobotHelper.CheckUdpPort('172.27.2.90', '15000', 'root', 'nest2014')
	print errnum
	
	
