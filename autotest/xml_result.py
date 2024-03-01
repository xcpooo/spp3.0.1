#!/usr/bin/python

import sys
import os
import time
from xml.dom.minidom import Document
from xml.dom import minidom

class CaseResultMng:
    ''' Testcase suites data MNG '''

    m_suites_data = {}

    def __init__(self, name='AllTests.xml'):
        ''' Initializes the XmlMng object '''
        time_str = time.strftime('%Y-%m-%dT%H:%M:%S', time.localtime(time.time()))
        self.m_suites_data['tests'] = 0
        self.m_suites_data['failures'] = 0
        self.m_suites_data['time'] = 0
        self.m_suites_data['name'] = name
        self.m_suites_data['timestamp'] = time_str

    def AddCase(self, suite, case, time_cost, err_msg=''):
        ''' Add test case '''
        suite_key = 'name_' + suite
        case_key  = 'name_' + case

        # 1. init suite data
        if self.m_suites_data.has_key(suite_key):
            suite_data = self.m_suites_data[suite_key]
            suite_data['tests'] = suite_data['tests'] + 1
        else:
            suite_data = {'name':suite, 'tests':1, 'failures':0, 'time':0}

        if 0 != err_msg.__len__():
            suite_data['failures'] += + 1
        suite_data['time'] += time_cost

        self.m_suites_data[suite_key]= suite_data

        # 2. init case data
        case_data = {'name':case, 'time':time_cost, 'classname':suite, 'failed':0}
        if 0 != err_msg.__len__():
            case_data['failed'] = 1

        case_data['err_msg'] = err_msg

        self.m_suites_data[suite_key][case_key] = case_data

        # 3. init global suites data
        self.m_suites_data['tests'] += 1
        self.m_suites_data['time'] += time_cost
        if 0 != err_msg.__len__():
            self.m_suites_data['failures'] += 1

    def DomWriteXml(self):
        ''' Write xml to file '''

        # 1. set testsuites attribute
        if os.path.exists(self.m_suites_data['name']):
            doc = minidom.parse(self.m_suites_data['name'])
            testsuites = doc.documentElement
            tests      = testsuites.getAttribute('tests')
            failures   = testsuites.getAttribute('failures')
            time       = testsuites.getAttribute('time')
            testsuites.setAttribute('tests', str(int(tests) + self.m_suites_data['tests']))
            testsuites.setAttribute('failures', str(int(failures) + self.m_suites_data['failures']))
            testsuites.setAttribute('time', str(int(time) + self.m_suites_data['time']))
        else:
            doc = Document()
            testsuites = doc.createElement('testsuites')
            testsuites.setAttribute('tests', str(self.m_suites_data['tests']))
            testsuites.setAttribute('failures', str(self.m_suites_data['failures']))
            testsuites.setAttribute('timestamp', self.m_suites_data['timestamp'])
            testsuites.setAttribute('time', str(self.m_suites_data['time']))
            testsuites.setAttribute('name', self.m_suites_data['name'])
            doc.appendChild(testsuites)

        # 2. set all testsuite units
        for suite_key, suite_data in self.m_suites_data.items():
            if type(suite_data) != dict:
                continue

            testsuite = doc.createElement('testsuite')
            testsuite.setAttribute('name', suite_data['name'])
            testsuite.setAttribute('tests', str(suite_data['tests']))
            testsuite.setAttribute('failures', str(suite_data['failures']))
            testsuite.setAttribute('time', str(suite_data['time']))

            testsuites.appendChild(testsuite)

            # 3. set all testcase
            for case_key, case_data in suite_data.items():
                if type(case_data) != dict:
                    continue
                testcase = doc.createElement('testcase')
                testcase.setAttribute('name', case_data['name'])
                testcase.setAttribute('time', str(case_data['time']))
                testcase.setAttribute('classname', case_data['classname'])
                testsuite.appendChild(testcase)

                # 4. if failed, need add failure
                if case_data['failed'] == 1:
                    failure = doc.createElement('failure')
                    failure.setAttribute('message', case_data['err_msg'])
                    testcase.appendChild(failure)

        f = file(self.m_suites_data['name'], 'w+')
        #print doc.toprettyxml()
        doc.writexml(f, ' ', '  ', '\n', 'utf-8')
        #f.write(doc.toprettyxml())
        f.close()

        # delete null lines
        cmdline = "sed -i '/^ *$/d' " + self.m_suites_data['name']
        os.system(cmdline)

    def __del__(self):
        ''' Delete test case result MNG object '''
        #self.DomWriteXml()

if __name__ == '__main__':
    suites = CaseResultMng('spp_test_result.xml')
    suites.AddCase('suite1', 'case1', 0, 'failed')
    suites.AddCase('suite1', 'case3', 0)
    suites.AddCase('suite2', 'case21', 0)
    suites.AddCase('suite2', 'case21', 0)
    suites.AddCase('suite3', 'case3', 1)
    suites.DomWriteXml()

