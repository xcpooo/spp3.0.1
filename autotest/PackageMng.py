#!/usr/bin/python

import sys
import os
import re
import time
import datetime
from RobotHelper import *

class PackageMng:
	'''
	nest package manage
	'''
	
	@staticmethod
	def GetRemotePkgName(path, service, ip, user, pwd):
		'''
		'''
		cmd = 'ls %s/%s -t' % (path, service)
		ret, output = RobotHelper.ExecSshCmd(ip, cmd, user, pwd)
		if ret < 0:
			err_msg = 'exeult ssh command failed, %d %s' % (ret, output)
			print err_msg
			return ""
		#print output
		return output.split()[0]	
	
	@staticmethod
	def ReplaceRemoteConf(file, old_str, new_str, ip, user, pwd):
		'''
		'''
		cmd = "sed -i 's/%s/%s/' %s" %(old_str, new_str, file)
		return RobotHelper.ExecSshCmd(ip, cmd, user, pwd)
	
	
	@staticmethod
	def DownloadPkgFile(remote_file, local_file, ip, user, pwd):
		'''
		'''
		remote = "%s@%s:%s" % (user, ip, remote_file)
		return RobotHelper.ExecScpCmd(remote, local_file, pwd)
	
	
	@staticmethod
	def DeploySppPkg(local_path, remote_path, ip, user, pwd):
		'''
		1.  default package name spp
		2.  default start script spp.sh start, spp.sh stop
		'''
		# copy agent package to dst
		pkg_name = local_path.split('/')[-1]
		remote   = '%s@%s:%s' % (user, ip, remote_path)
		ret, output = RobotHelper.ExecScpCmd(local_path, remote, pwd)
		if ret < 0:
			result = -1
			err_msg = 'exeult scp command failed, %d %s' % (ret, output)
			print err_msg
			return result, err_msg
		
		# update spp package
		cmd = 'cd %s; rm spp -rf; tar zxf %s' % (remote_path, pkg_name)
		#print cmd
		ret, output = RobotHelper.ExecSshCmd(ip, cmd, user, pwd)
		if ret < 0:
			result = -2
			err_msg = 'exeult ssh command failed, %d %s' % (ret, output)
			print err_msg
			return result, err_msg
		
		return 0, ''
		
		
	@staticmethod
	def StartSppFrame(remote_path, ip, user, pwd):
		'''
		'''
		cmd = "cd %s/bin; ./spp.sh start" % remote_path
		#print cmd
		ret, out_put = RobotHelper.ExecSshCmd(ip, cmd, user, pwd)
		if ret < 0:
			print 'exeult spp start command failed, %d %s' % (ret, out_put)
			return False
		
		return True
	
	@staticmethod
	def StopSppFrame(remote_path, ip, user, pwd):
		''' '''
		cmd = "cd %s/bin; ./spp.sh stop; rm ../log/*" % remote_path
		#print cmd
		ret, out_put = RobotHelper.ExecSshCmd(ip, cmd, user, pwd)
		if ret < 0:
			print 'exeult spp stop command failed, %d %s' % (ret, out_put)
			return False
			
		return True
	
	
	@staticmethod
	def GetSppPkgPath():
		'''
		'''
		str = ''
		handle = os.popen('ls package/spp/spp*tar.gz -t', 'r')
		str = handle.readline()
		return str.split()[0]

	@staticmethod
	def GetSrvPkgPath(pkg_name):
		'''
		'''

		str = ''
		cmd = 'ls package/service/*%s*tar.gz -t' % pkg_name
		handle = os.popen(cmd, 'r')
		str = handle.readline()
		return str.split()[0]

	@staticmethod
	def GetAgentPkgPath():
		'''
		'''

		str = ''
		cmd = 'ls package/nest_agent/nest_agent*.tar.gz -t'
		handle = os.popen(cmd, 'r')
		str = handle.readline()
		return str.split()[0]

	@staticmethod
	def UpdateAgentProc(ip, user, passwd):
		'''
		'''
		AGENT_LISTEN_PORT = '2014'

		result  = 0
		err_msg = ''

		# copy agent package to dst
		local_path  = PackageMng.GetAgentPkgPath()
		pkg_name = local_path.split('/')[-1]
		remote_path = '%s:/home/oicq/' % ip
		ret, output = RobotHelper.ExecScpCmd(local_path, remote_path, passwd)
		if ret < 0:
			result = -1
			err_msg = 'exeult scp command failed, %d %s' % (ret, output)
			print err_msg
			return result, err_msg

		# restart agent process
		cmd = 'cd /home/oicq/;tar zxf %s;cp nest_agent_release/* nest_agent -rf; cd nest_agent/bin;./reset_new.sh > /dev/null &' % pkg_name
		print cmd
		ret, output = RobotHelper.ExecSshCmd(ip, cmd, user, passwd)
		if ret < 0:
			result = -2
			err_msg = 'exeult ssh command failed, %d %s' % (ret, output)
			print err_msg
			return result, err_msg

		# wait for restart complete
		time.sleep(2)

		# check agent restart ok
		if RobotHelper.CheckTcpPort(ip, AGENT_LISTEN_PORT, user, passwd) == False:
			result = -3
			err_msg = 'restart nest agent failed'
			print err_msg
			return result, err_msg

		return result, err_msg

	@staticmethod
	def UpdateSppPkg(ip, user, passwd):

		'''
		'''

		local_path = PackageMng.GetSppPkgPath()
		remote_path = '%s:/usr/local/services' % ip
		pkg_name = local_path.split('/')[-1]

		# copy spp package
		ret, output = RobotHelper.ExecScpCmd(local_path, remote_path, passwd)
		if ret < 0:
			result = -1
			err_msg = 'execult scp command failed, %d %s' % (ret, output)
			print err_msg
			return result, err_msg

		# Update spp package
		cmd = 'cd /usr/local/services; rm spp -rf; tar zxf %s' % pkg_name
		print cmd
		ret, output = RobotHelper.ExecSshCmd(ip, cmd, user, passwd)
		if ret < 0:
			result = -2
			err_msg = 'exeult ssh command failed, %d %s' % (ret, output)
			print err_msg
			return result, err_msg
		
		return 0, ''

	@staticmethod
	def UpdateSrvPkg(ip, user, passwd, srvname, pkgname):
		'''
		'''

		local_path = PackageMng.GetSrvPkgPath(pkgname)
		remote_path = '%s:/usr/local/services' % ip
		pkg_name = local_path.split('/')[-1]


		# copy service package to dst
		ret, output = RobotHelper.ExecScpCmd(local_path, remote_path, passwd)
		if ret < 0:
			result = -1
			err_msg = 'execult scp command failed, %d %s' % (ret, output)
			print err_msg
			return result, err_msg

		# generate spp service directory	
		cmd = 'cd /usr/local/services; rm %s -rf; cp spp %s -rf; tar zxf %s; cd %s; mkdir moni stat log;cd bin; ln -s nest_proxy spp_%s_proxy; ln -s nest_worker spp_%s_worker; ln -s spp_dc_tool spp_%s_dc_tool; rm ../etc/*.xml -f;cd ../../;chown -R user_00 %s' % (pkgname, pkgname, pkg_name, pkgname, srvname, srvname, srvname, pkgname)
		print cmd
		ret, output = RobotHelper.ExecSshCmd(ip, cmd, user, passwd)
		if ret < 0:
			result = -2
			err_msg = 'exeult ssh command failed, %d %s' % (ret, output)
			print err_msg
			return result, err_msg
		
		return 0, ''
		
	@staticmethod
	def UpdateEchoConf(ip, user, passwd, confname):
		'''
		'''

		local_path  = 'package/service/%s' % confname
		remote_path = '%s:/home/oicq/echo/'

		# copy echo conf to dst
		ret, output = RobotHelper.ExecScpCmd(local_path, remote_path, passwd)
		if ret < 0:
			result = -1
			err_msg = 'execult scp command failed, %d %s' % (ret, output)
			print err_msg
			return result, err_msg

		return 0, ''


if __name__ == '__main__':
	#ret, output = RobotHelper.ExecScpCmd('tmp', '172.27.2.78:/home/davisxie', 'nest2014')
	#print ret
	#print output
	#ret, output = RobotHelper.ExecSshCmd('172.27.2.78', 'ls', 'root', 'nest2014')
	#print ret
	#print output
	#handle = os.popen('ls package/spp/spp*tgz', r)
	#str = handle.readline()
	#print str
	#ret, output = RobotHelper.ExecScpCmd('package/')
	
	pkg_name = PackageMng.GetRemotePkgName("/auto_test_package/spp", "spp*32bit*", "172.27.2.90", 'root', 'nest2014')
	print pkg_name
	
	ret, err_msg = PackageMng.DownloadPkgFile(pkg_name, "spp.tgz", "172.27.2.90", 'root', 'nest2014')
	print ret, err_msg
	
	ret, err_msg = PackageMng.DeploySppPkg("spp.tgz", "/usr/local/services/tmp", '172.27.2.78', 'root', 'nest2014')
	print ret, err_msg
	
	print PackageMng.StartSppFrame("/usr/local/services/tmp/spp", '172.27.2.78', 'root', 'nest2014')
	
	print PackageMng.StopSppFrame("/usr/local/services/tmp/spp", '172.27.2.78', 'root', 'nest2014')
	
	print  '-------------------'
	
	str = PackageMng.GetSppPkgPath()
	print str
	str = PackageMng.GetSrvPkgPath('test')
	print str
	str = PackageMng.GetAgentPkgPath()
	print str
	ret, err_msg = PackageMng.UpdateAgentProc('172.27.2.78', 'root', 'nest2014')
	print ret, err_msg

	ret, err_msg = PackageMng.UpdateSppPkg('172.27.2.78', 'root', 'nest2014')
	print ret, err_msg

	ret, err_msg = PackageMng.UpdateSrvPkg('172.27.2.78', 'root', 'nest2014', 'test', 'test')
	print ret, err_msg
