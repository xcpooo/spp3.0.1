spp自动化测试包含L5和蜂巢的测试，分别是L5_test.py和Nest_test.py脚本

L5测试步骤:
1. 直接到SPP_FRAME(10.198.0.231)机器更新spp框架包，/tmp/spp_usr00/3.0_L5/spp*

Nest测试步骤:
1. agent发布包放入package/nest_agent/目录
2. nest center需要自己部署，NEST_CENTER指定IP地址
3. 指定proxy/worker节点
4. 更新spp框架，spp框架需要放入package/spp/目录
5. 配置ECHO_CONF,SRVINFO_test
6. 开始测试