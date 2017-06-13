##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   
#   Balasko, Jeno
#   Beres, Szabolcs
#   Kovacs, Ferenc
#   Raduly, Csaba
#
##############################################################################
#!/usr/bin/env python

import os

USER = os.environ.get('USER', 'titanrt')
BASEDIR = os.environ.get('HOME', '/home/%s' % USER)

# Sending notifications and the generation of HTML is always done.  It's not a
# configurable option.
#
# Brief description of options:
#
#   buiddir=['...'|''] The build directory of the master.
#   logdir=['...'|'']  The logs of the master go here.
#   htmldir=['...'|''] All HTML files will be published here.
#   vob=['...'|'']     The VOB products will be copied from here.
#   archive=[#]        Archive the logs after a specified number of days.
#   cleanup=[#]        Move logs to a safe place.
#   measureperiod=[#]  Reset scores after a given number of days.
#
# It's important to use different directories for the master and the slaves.
# Especially for local builds.

common = {
  'builddir':os.path.join(BASEDIR, 'titan_nightly_builds/build_master'),
  'logdir':os.path.join(BASEDIR, 'titan_nightly_builds/build_master/logs'),
  'htmldir':os.path.join(BASEDIR, 'titan_nightly_builds/web/titan_builds'),
  'vob':os.path.join(BASEDIR, 'titan_nightly_builds/vobs/ttcn/TCC_Common'),
  'archive':4,
  'cleanup':8,
  'cleanupslave':{'slave':'tcclab1', 'dir':os.path.join(BASEDIR, 'titan_nightly_builds/archives')},
  'measureperiod':30
}

###############################
# Description of recipients.  #
###############################

recipients = {
  'Adam Delic':'<adam.delic@ericsson.com>',
  'Csaba Raduly':'<csaba.raduly@ericsson.com>',
  'Elemer Lelik':'<elemer.lelik@ericsson.com>',
  'Gabor Szalai':'<gabor.szalai@ericsson.com>',
  'Gyorgy Rethy':'<gyorgy.rethy@ericsson.com>',
  'Gyula Koos':'<gyula.koos@ericsson.com>',
  'Jeno Balasko':'<jeno.balasko@ericsson.com>',
  'Kristof Szabados':'<kristof.szabados@ericsson.com>',
  'Krisztian Pandi':'<krisztian.pandi@ericsson.com>',
  'Matyas Ormandi':'<matyas.ormandi@ericsson.com>',
  'Szabolcs Beres':'<szabolcs.beres@ericsson.com>',
  'Tibor Csondes':'<tibor.csondes@ericsson.com>',
  'Zsolt Szego':'<zsolt.szego@ericsson.com>',
  'a':'<bcdefghijklmnopqrstuvwxyz>'
}

###########################
# Description of slaves.  #
###########################
# The whole script will be copied to the target machine and the specified
# build configuration will be run.  To disable a slave simply comment it out.
# The password-less `ssh' is a requirement here.  The scheduling is done by
# `cron'.  If the slave is unreachable it will be skipped due to a timeout.
# Everything will be written to a logfile.
#
# Currently, one configuration is supported for each slave.
#
# A new directory will be created by the master with a name specified in the
# given build configuration.  The build will run from that very directory on
# the slave.

slaves = {}

slaves['tcclab1'] = {'ip':os.environ.get('TCCLAB1_IP', '172.31.21.7'), 'user':USER, 'configs':['x86_64_linux_tcclab1', 'vobtests_on_x86_64_linux_tcclab1']}
slaves['tcclab2'] = {'ip':os.environ.get('TCCLAB2_IP', '172.31.21.49'), 'user':USER, 'configs':['x86_64_linux_tcclab2']}
slaves['tcclab3'] = {'ip':os.environ.get('TCCLAB3_IP', '172.31.21.8'), 'user':USER, 'configs':['x86_64_linux_tcclab3', 'x86_64_linux_tcclab3_your_last_chance']}
slaves['tcclab4'] = {'ip':os.environ.get('TCCLAB4_IP', '172.31.21.10'), 'user':USER, 'configs':['x86_64_linux_tcclab4']}
slaves['tcclab5'] = {'ip':os.environ.get('TCCLAB5_IP', '172.31.21.9'), 'user':USER, 'configs':['x86_64_linux_tcclab5', 'x86_64_linux_tcclab5_clang']}
slaves['mwlx122'] = {'ip':os.environ.get('MWLX122_IP', '159.107.148.32'), 'user':USER, 'configs':[]}
slaves['esekits3013'] = {'ip':os.environ.get('ESEKITS3013_IP', '147.214.15.172'), 'user':USER, 'configs':['sparc_solaris_esekits3013', 'vobtests_on_sparc_solaris_esekits3013']}
slaves['esekilxxen1843'] = {'ip':os.environ.get('ESEKILXXEN1843_IP', '147.214.13.100'), 'user':USER, 'configs':['x86_64_linux_esekilxxen1843']}
slaves['mwux054'] = {'ip':os.environ.get('MWUX054_IP', '159.107.194.67'), 'user':USER, 'configs':[]}
slaves['bangjohansen'] = {'ip':os.environ.get('BANGJOHANSEN_IP', '172.31.21.76'), 'user':USER, 'configs':['i386_solaris_bangjohansen']}

#############################
# Description of products.  #
#############################
# The list of VOB-products to run.  The `product' is coming from the directory
# name of the product in CC.  The columns are describing the following in
# order (the `run' is very unrealistic at the moment):
#   `semantic': Run semantic checks only on all source files.
#   `translate': Code generation as well.
#   `compile': The generated code is compiled and the executable is created.
#   `run': Try to run the test in some way.
# False/True = disable/enable a specific phase.  All interesting products
# should be listed here.
# Possible scenarios:
# 1) We look for a `demo' directory under `product'.  If it doesn't exist most
#    of the phases will fail.  Otherwise go to 2).
# 2) Find the necessary files and try to reconstruct the product using a build
#    file or just simply using the list of files.  The .prj file always have
#    the top priority.  If it's not present the Makefile is examined.
#    Otherwise, the files in `demo' and `src' will be copied and a Makefile
#    will be generated for them.  (If there's a Makefile it will be regenerated
#    too.)  A new Makefile needs to be generated in all cases.
# 3) The files are in our ${HOME}, they're ready to be distributed.
# 4) Run the phases specified in this file.
#
# Using a list of products is better then having an exception list.  In this
# way everything is under control.  For products with .ttcnpp/.ttcnin files,
# the `semantic' and `translate' passes are disabled.  They don't really make
# sense without a Makefile, which calls `cpp' first.  Support for automatic
# detection of preprocessable files is missing.

products = {}

products['TestPorts'] = [
  {'name':'AGTproc_CNL113391',         'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'ASISmsg_CNL113338',         'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'CPDEBUG_OIPmsg_CNL113381',  'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'CPDEBUG_PLEXmsg_CNL113324', 'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'CPDEBUG_RESmsg_CNL113339',  'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'DIAMETERmsg_CNL113310',     'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'EINMGRasp_CNL113468',       'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'EPmsg_CNL113406',           'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'GMLOGmsg_CNL113351',        'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'GiGnREPLAYasp_CNL113604',   'semantic':False, 'translate':False, 'compile':False, 'run':False},
  {'name':'H225v5msg_CNL113486',       'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'HTTPmsg_CNL113312',         'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'IPL4asp_CNL113531',         'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'LANL2asp_CNL113519',        'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'LAPDasp_Q.921_CNL113436',   'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'LDAPasp_RFC4511_CNL113513', 'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'LDAPmsg_CNL113385',         'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'LLCasp_CNL113343',          'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'LOADMEASasp_CNL113585',     'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'M2PAasp_CNL113557',         'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'MMLasp_CNL113490',          'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'MTP3asp_CNL113337',         'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'MTP3asp_EIN_CNL113421',     'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'NSasp_CNL113386',           'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'PCAPasp_CNL113443',         'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'PIPEasp_CNL113334',         'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'RLC_RNC_host',              'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'RPMOmsg_CNL113350',         'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'Rexec_Rshmsg_CNL113476',    'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'SCCPasp_CNL113348',         'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'SCTPasp_CNL113469',         'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'SCTPproc_CNL113409',        'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'SEA_IPasp_CNL113544',       'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'SEA_NWMasp_CNL113586',      'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'SEA_OCPasp_CNL113612',      'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'SIPmsg_CNL113319',          'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'SMPPmsg_CNL113321',         'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'SNMPmsg_CNL113344',         'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'SSHCLIENTasp_CNL113484',    'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'SSHSERVERasp_CNL113489',    'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'STDINOUTmsg_CNL113642',     'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'SUAasp_CNL113516',          'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'SUNRPCasp_CNL113493',       'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'TCAPasp_CNL113349',         'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'TCPasp_CNL113347',          'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'TELNETasp_CNL113320',       'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'UDPasp_CNL113346',          'semantic':True,  'translate':True,  'compile':True,  'run':False},
  {'name':'XTDPasp_CNL113494',         'semantic':True,  'translate':True,  'compile':False, 'run':False}
]

products['ProtocolModules'] = [
  {'name':'ABM_RealTime_3.0_CNL113524 ',     'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'ACR_v1.1_CNL113680',              'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'ADMS_UPG_CNL113555',              'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'AIN_v2.0_CNL113556',              'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BICC_ANSI_CNL113397',             'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BICC_Brazil_CNL113403',           'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BICC_China_CDMA_CNL113441',       'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BICC_China_CNL113402',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BICC_Q.1902.1_CNL113359',         'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BICC_TTC_CNL113416',              'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BICC_UK_CNL113401',               'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BNSI_CNL113475',                  'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BSSAPP_v5.4.0_CNL113373',         'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BSSAPP_v5.5.0_CNL113470',         'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BSSGP_v5.9.0_CNL113388',          'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BSSGP_v6.12.0_CNL113497',         'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BSSGP_v6.7.0_CNL113445',          'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BSSMAP_v4.8.0_CNL113413',         'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'BSSMAP_v6.3.0_CNL113361',         'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CAI3G1.1_UPG1.0_CNL113549',       'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'CAI3G1.2_SP_UP_EMA5.0_CNL113551', 'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'CAI3G_CNL113423',                 'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'CAI3G_v1.1_CNL113511',            'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'CAI_CNL113422',                   'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CAI_CNL113502',                   'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CAI_CNL113504',                   'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CAI_MINSAT_CNL113548',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CAP_v5.4.0_CNL113374',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CAP_v5.6.1_CNL113510',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CAP_v610_CNL113358',              'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CAP_v6.4.0_CNL113433',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CAP_v7.3.0_CNL113581',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CCAPI_MINSAT_531_CNL113546',      'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CDR_v6.1.0_CNL113505',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CDR_v8.5.0_CNL113665',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CIP_CS3.0Y_CNL113506',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'CIP_CS4.0_CNL113535',             'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'DASS2_CNL113464',                 'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'DHCP_CNL113461',                  'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'DNS_CNL113429',                   'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'DPNSS_CNL113455',                 'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'DSS1_ANSI_CNL113481',             'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'DUA_CNL113449',                   'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'EricssonRTC_CNL113414',           'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'Ericsson_INAP_CS1plus_CNL113356', 'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'GCP_31r1_CNL113364',              'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'GMLOG_CNL113408',                 'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'GTP97_v6.11.0_CNL113379',         'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'GTPP_v6.0.0_CNL113448',           'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'GTPP_v7.7.0_CNL113376',           'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'GTP_v5.6.0_CNL113375',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'GTP_v6.11.0_CNL113499',           'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'GTP_v6.7.0_CNL113446',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'GTP_v7.6.0_CNL113559',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'H225.0_v0298_CNL113354',          'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'H225.0_v10_CNL113479',            'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'H245_v5_CNL113480',               'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'H248_v2_CNL113424',               'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'ICMP_CNL113529',                  'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'IP_CNL113418',                    'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'ISUP_ANSI_CNL113411',             'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'ISUP_Brazil_CNL113400',           'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'ISUP_China_CDMA_CNL113442',       'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'ISUP_China_CNL113399',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'ISUP_Q.762_CNL113365',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'ISUP_TTC_CNL113417',              'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'ISUP_UK_CNL113398',               'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'IUA_CNL113439',                   'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'IUP_CNL113554',                   'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'M3UA_CNL113536',                  'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'MAP_v5.6.1_CNL113372',            'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'MAP_v6.11.0_CNL113500',           'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'MAP_v7.12.0_CNL113635',           'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'MobileL3_v5.10.0_CNL113471',      'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'MobileL3_v7.8.0_CNL113561',       'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'RANAP_v6.4.1_CNL113434',          'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'RANAP_v6.8.0_CNL113498',          'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'RANAP_v6.9.0_CNL113527',          'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'ROHC_CNL113426',                  'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'SGsAP_v8.3.0_CNL113668',          'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'SGsAP_v9.0.0_CNL113684',          'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'SOAP_MMS_CNL113518',              'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'STUN_CNL113644',                  'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'TBCP_CNL113463',                  'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'TRH_AXD7.5_CNL113574',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'TRH_CNL113485',                   'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'TRH_TSS4.0_CNL113547',            'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'UDP_CNL113420',                   'semantic':True, 'translate':True, 'compile':True,  'run':False},
  {'name':'ULP_CNL113457',                   'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'XCAP_CNL113460',                  'semantic':True, 'translate':True, 'compile':False, 'run':False},
  {'name':'XML_RPC_CNL113488',               'semantic':True, 'translate':True, 'compile':False, 'run':False}
]

products['ProtocolEmulations'] = [
  {'name':'M3UA_CNL113537', 'semantic':True,  'translate':True,  'compile':False, 'run':False},
  {'name':'SCCP_CNL113341', 'semantic':False, 'translate':False, 'compile':True,  'run':False},
  {'name':'TCAP_CNL113342', 'semantic':True,  'translate':True,  'compile':True,  'run':False}
]

products['Servers'] = []

products['Libraries'] = []

products['Applications'] = []

#########################################
# Description of build configurations.  #
#########################################
# The build configurations are slave specific and these can be overridden
# through command line arguments.  Add configuration inheritance to avoid
# redundancy.  The build directory will stay intact until the next build to
# help debugging.  The next build will wipe out everything.
#
# Brief description of options:
#
#   version=['...'|'']         Version of TITAN to use.  It can be a CVS tag or
#                              date.  If it's not set the HEAD will be taken.
#   license=['...'|'']         Location of the license file.
#   gui=[True|False]           The `GUI' part in Makefile.cfg.
#   jni=[True|False]           The `JNI' part in Makefile.cfg.
#   debug=[True|False]         The `DEBUG' part in Makefile.cfg.
#   compilerflags=['...'|'']   The `COMPILERFLAGS' in Makefile.cfg.
#   ldflags=['...'|'']         The `LDFLAGS' in Makefile.cfg.
#   gccdir=['...'|'']          This will affect `CC' and `CXX'.
#   *cc=['...'|'']             Value of `CC' in synch with the previous option.
#   *cxx=['...'|'']            Value of `CXX' in synch with the previous option.
#   qtdir=['...'|'']           For `QTDIR'.
#   xmldir=['...'|'']          For `XMLDIR'.
#   openssldir=['...'|'']      For `OPENSSL_DIR'.
#   flex=['...'|'']            Replace `FLEX'.
#   perl=['...'|'']            Location of the `perl' interpreter.
#   bison=['...'|'']           Replace `BISON'.
#   regtest=[True|False]       Run regression tests.
#   perftest=[True|False]      Run performance tests.  The location of the
#                              testsuite must be known, since it's not part of
#                              CVS.  It should be available for the master
#                              locally.
#   perftestdir=['...'|'']     Location of the performance tests.
#   *cpsmin=[#]                Minimum CPS value for performance tests.
#   *cpsmax=[#]                Maximum CPS value for performance tests.
#   functest=[True|False]      Run function tests.
#   vobtest=[True|False]       Run product tests.
#   *vobtest_logs=[True|False] Save logs for product tests.
#   rt2=[True|False]           Run tests with both run-times.
#   builddir=['...'|'']        Everything will be done here.  It should be
#                              different from the master's.
#   installdir=['...'|'']      The `TTCN3_DIR' variable.  It's never removed.
#   logdir=['...'|'']          Place of the logs.
#   *pdfdir=['...'|'']         Local directory to copy .pdf files from.  If not
#                              present no .pdf files will be there.  If it's an
#                              empty string the .pdf files will be faked with
#                              empty files.
#   *xsdtests=[True|False]     Disable regression tests for `xsd2ttcn'.  It's
#                              very time consuming.
#   *foa=[True|False]          The builds are left in a directory.
#   *foadir=['...'|'']         Link location of the latest build for FOA.  If
#                              not set, its value will be equal to `installdir'.
#   *measure=[True|False]      Enable `quality' measurements.
#   *eclipse=[True|False]      Enable Eclipse build.
#
# The results will be sent back to the originating machine, the master.  It
# will assemble the notifications and generate HTML pages.  Make sure that the
# build and log directories are always unique!

configs = {}

configs['x86_64_linux_esekilxxen1843'] = {
  'foa':False,
  'foadir':'',
  'version':'',  # Or date in format `YYYYMMDD'.
  'license':'/home/titanrt/license_8706.dat',
  'platform':'LINUX',
  'gui':True,
  'jni':True,
  'debug':False,
  'compilerflags':'$(MINGW) -fPIC',
  'ldflags':'$(MINGW) -fPIC',
  'gccdir':'/usr',
  'flex':'/usr/bin/flex',
  'bison':'/usr/bin/bison',
  'jdkdir':'/usr/lib64/jvm/java-1.6.0-sun-1.6.0',
  'qtdir':'/usr/lib64/qt3',
  'xmldir':'default',
  'openssldir':'default',
  'regtest':True,
  'perftest':True,
  'functest':True,
  'vobtest':False,
  'vobtest_logs':False,
  'rt2':True,
  'xsdtests':True,
  'pdfdir':os.path.join(BASEDIR, 'docs/TTCNv3-1.8.pl5'),
  'perl':'/usr/bin/perl',
  'perftestdir':os.path.join(BASEDIR, 'titan_nightly_builds/balls/perftest-20090927.tar.bz2'),
  'cpsmin':2000,
  'cpsmax':7000,
  'builddir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/x86_64_linux_esekilxxen1843'),
  'installdir':'/home/titanrt/TTCNv3-bleedingedge-x86_64_linux_esekilxxen1843',
  'logdir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/logs/x86_64_linux_esekilxxen1843'),
  'measure':False,
  'eclipse':True
}

configs['sparc_solaris_esekits3013'] = {
  'foa':False,
  'foadir':'',
  'version':'',  # Or date in format `YYYYMMDD'.
  'license':'/home/titanrt/license_8706.dat',
  'platform':'SOLARIS8',
  'gui':True,
  'jni':True,
  'debug':False,
  'compilerflags':'-Wall',
  'ldflags':'$(MINGW)',
  'gccdir':'/proj/TTCN/Tools/gcc-3.4.6-sol8',
  'flex':'/proj/TTCN/Tools/flex-2.5.35/bin/flex',
  'bison':'/proj/TTCN/Tools/bison-2.4.3/bin/bison',
  'jdkdir':'/proj/TTCN/Tools/jdk1.6.0_23',
  'qtdir':'/proj/TTCN/Tools/qt-x11-free-3.3.8-gcc3.4.6-sol8',
  'xmldir':'/proj/TTCN/Tools/libxml2-2.7.8',
  'openssldir':'/proj/TTCN/Tools/openssl-0.9.8r',
  'regtest':True,
  'perftest':True,
  'functest':True,
  'vobtest':False,
  'vobtest_logs':False,
  'rt2':True,
  'xsdtests':True,
  'pdfdir':os.path.join(BASEDIR, 'docs/TTCNv3-1.8.pl5'),
  'perl':'/proj/TTCN/Tools/perl-5.10.1/bin/perl',
  'perftestdir':os.path.join(BASEDIR, 'titan_nightly_builds/balls/perftest-20090927.tar.bz2'),
  'cpsmin':1000,
  'cpsmax':2000,
  'builddir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/sparc_solaris_esekits3013'),
  'installdir':'/home/titanrt/TTCNv3-bleedingedge-sparc_solaris_esekits3013',
  'logdir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/logs/sparc_solaris_esekits3013'),
  'measure':False
}

configs['x86_64_linux_tcclab1'] = {
  'foa':False,
  'foadir':'',
  'version':'',
  'license':'/home/titanrt/license_8706.dat',
  'platform':'LINUX',
  'gui':True,
  'jni':True,
  'debug':False,
  # For Linux platforms the `-lncurses' is added automatically to the
  # appropriate Makefile.  (This should be part of the CVS instead.)
  'compilerflags':'-Wall -fPIC',
  'ldflags':'$(MINGW) -fPIC',
  'gccdir':'/usr',
  'flex':'/usr/bin/flex',
  'bison':'/usr/bin/bison',
  'jdkdir':'/mnt/TTCN/Tools/jdk1.6.0_14',
  'qtdir':'/usr/lib/qt3',
  'xmldir':'default',
  'openssldir':'default',
  'regtest':True,
  'perftest':True,
  'functest':True,
  'vobtest':False,
  'vobtest_logs':False,
  'rt2':True,
  'xsdtests':True,
  'pdfdir':os.path.join(BASEDIR, 'docs/TTCNv3-1.8.pl5'),
  'perl':'/usr/bin/perl',
  'perftestdir':os.path.join(BASEDIR, 'titan_nightly_builds/balls/perftest-20090927.tar.bz2'),
  'cpsmin':4000,
  'cpsmax':9000,
  'builddir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/x86_64_linux_tcclab1'),
  'installdir':'/mnt/TTCN/Releases/TTCNv3-bleedingedge',
  'logdir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/logs/x86_64_linux_tcclab1'),
  'measure':False
}

configs['x86_64_linux_tcclab2'] = {
  'foa':False,
  'foadir':'',
  'version':'',
  'license':'/home/titanrt/license_8706.dat',
  'platform':'LINUX',
  'gui':True,
  'jni':True,
  'debug':True,
  # For Linux platforms the `-lncurses' is added automatically to the
  # appropriate Makefile.  (This should be part of the CVS instead.)
  'compilerflags':'-Wall -fPIC',
  'ldflags':'$(MINGW) -fPIC',
  'gccdir':'/usr',
  'flex':'/usr/bin/flex',
  'bison':'/usr/bin/bison',
  'jdkdir':'/mnt/TTCN/Tools/jdk1.6.0_14',
  'qtdir':'/mnt/TTCN/Tools/qt3',
  'xmldir':'default',
  'openssldir':'default',
  'regtest':True,
  'perftest':True,
  'functest':True,
  'vobtest':False,
  'vobtest_logs':False,
  'rt2':True,
  'xsdtests':True,
  'pdfdir':os.path.join(BASEDIR, 'docs/TTCNv3-1.8.pl5'),
  'perl':'/usr/bin/perl',
  'perftestdir':os.path.join(BASEDIR, 'titan_nightly_builds/balls/perftest-20090927.tar.bz2'),
  'cpsmin':1000,
  'cpsmax':6000,
  'builddir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/x86_64_linux_tcclab2'),
  'installdir':'/mnt/TTCN/Releases/TTCNv3-bleedingedge',
  'logdir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/logs/x86_64_linux_tcclab2'),
  'measure':False
}

configs['x86_64_linux_tcclab3'] = {
  'foa':False,
  'foadir':'',
  'version':'',
  'license':'/home/titanrt/license_8706.dat',
  'platform':'LINUX',
  'gui':True,
  'jni':True,
  'debug':False,
  # For Linux platforms the `-lncurses' is added automatically to the
  # appropriate Makefile.  (This should be part of the CVS instead.)
  'compilerflags':'-Wall -fPIC',
  'ldflags':'$(MINGW) -fPIC',
  'gccdir':'/usr',
  'flex':'/usr/bin/flex',
  'bison':'/usr/bin/bison',
  'jdkdir':'/mnt/TTCN/Tools/jdk1.6.0_14',
  'qtdir':'/usr/lib/qt3',
  'xmldir':'default',
  'openssldir':'default',
  'regtest':True,
  'perftest':True,
  'functest':True,
  'vobtest':False,
  'vobtest_logs':False,
  'rt2':True,
  'xsdtests':True,
  'pdfdir':os.path.join(BASEDIR, 'docs/TTCNv3-1.8.pl5'),
  'perl':'/usr/bin/perl',
  'perftestdir':os.path.join(BASEDIR, 'titan_nightly_builds/balls/perftest-20090927.tar.bz2'),
  'cpsmin':1000,
  'cpsmax':6000,
  'builddir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/x86_64_linux_tcclab3'),
  'installdir':'/mnt/TTCN/Releases/TTCNv3-bleedingedge',
  'logdir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/logs/x86_64_linux_tcclab3'),
  'measure':False
}

configs['x86_64_linux_tcclab3_your_last_chance'] = {
  'foa':False,
  'foadir':'',
  'version':'',
  'license':'/home/titanrt/license_8706.dat',
  'platform':'LINUX',
  'gui':True,
  'jni':True,
  'debug':False,
  # For Linux platforms the `-lncurses' is added automatically to the
  # appropriate Makefile.  (This should be part of the CVS instead.)
  'compilerflags':'-Wall -fPIC',
  'ldflags':'$(MINGW) -fPIC',
  'gccdir':'/usr',
  'flex':'/usr/bin/flex',
  'bison':'/usr/bin/bison',
  'jdkdir':'/mnt/TTCN/Tools/jdk1.6.0_14',
  'qtdir':'/usr/lib/qt3',
  'xmldir':'default',
  'openssldir':'default',
  'regtest':True,
  'perftest':True,
  'functest':True,
  'vobtest':False,
  'vobtest_logs':False,
  'rt2':True,
  'xsdtests':True,
  'pdfdir':os.path.join(BASEDIR, 'docs/TTCNv3-1.8.pl5'),
  'perl':'/usr/bin/perl',
  'perftestdir':os.path.join(BASEDIR, 'titan_nightly_builds/balls/perftest-20090927.tar.bz2'),
  'cpsmin':1000,
  'cpsmax':6000,
  'builddir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/x86_64_linux_tcclab3_your_last_chance'),
  'installdir':'/mnt/TTCN/Releases/TTCNv3-bleedingedge',
  'logdir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/logs/x86_64_linux_tcclab3_your_last_chance'),
  'measure':False
}

configs['x86_64_linux_tcclab5_clang'] = {
  'foa':False,
  'foadir':'',
  'version':'',
  'license':'/home/titanrt/license_8706.dat',
  'platform':'LINUX',
  'gui':True,
  'jni':True,
  'debug':False,
  'compilerflags':'-Wall -fPIC',
  'ldflags':'$(MINGW) -fPIC',
  'gccdir':'/local/ecsardu',
  'cc':'clang',
  'cxx':'clang++',  # It's just a link.
  'flex':'/usr/bin/flex',
  'bison':'/usr/bin/bison',
  'jdkdir':'/usr/lib64/jvm/java-1.6.0',
  'qtdir':'/usr/lib64/qt3',
  'xmldir':'default',
  'openssldir':'default',
  'regtest':True,
  'perftest':True,
  'functest':True,
  'vobtest':False,
  'vobtest_logs':False,
  'rt2':True,
  'xsdtests':True,
  'pdfdir':os.path.join(BASEDIR, 'docs/TTCNv3-1.8.pl5'),
  'perl':'/usr/bin/perl',
  'perftestdir':os.path.join(BASEDIR, 'titan_nightly_builds/balls/perftest-20090927.tar.bz2'),
  'cpsmin':4000,
  'cpsmax':9000,
  'builddir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/x86_64_linux_tcclab5_clang'),
  'installdir':'/mnt/TTCN/Releases/TTCNv3-bleedingedge-clang',
  'logdir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/logs/x86_64_linux_tcclab5_clang'),
  'measure':False
}

configs['x86_64_linux_tcclab4'] = {
  'foa':False,
  'foadir':'',
  'version':'',
  'license':'/home/titanrt/license_8706.dat',
  'platform':'LINUX',
  'gui':True,
  'jni':True,
  'debug':False,
  'compilerflags':'-Wall -fPIC',
  'ldflags':'$(MINGW) -fPIC',
  'gccdir':'/usr',
  'flex':'/usr/bin/flex',
  'bison':'/usr/bin/bison',
  'jdkdir':'/usr/lib/jvm/java-1.6.0-openjdk',
  'qtdir':'/mnt/TTCN/Tools/qt',
  'xmldir':'default',
  'openssldir':'default',
  'regtest':True,
  'perftest':True,
  'functest':True,
  'vobtest':False,
  'vobtest_logs':False,
  'rt2':True,
  'xsdtests':True,
  'pdfdir':os.path.join(BASEDIR, 'docs/TTCNv3-1.8.pl5'),
  'perl':'/usr/bin/perl',
  'perftestdir':os.path.join(BASEDIR, 'titan_nightly_builds/balls/perftest-20090927.tar.bz2'),
  'cpsmin':1000,
  'cpsmax':6000,
  'builddir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/x86_64_linux_tcclab4'),
  'installdir':'/mnt/TTCN/Releases/TTCNv3-bleedingedge',
  'logdir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/logs/x86_64_linux_tcclab4'),
  'measure':False
}

configs['x86_64_linux_tcclab5'] = {
  'foa':False,
  'foadir':'',
  'version':'',
  'license':'/home/titanrt/license_8706.dat',
  'platform':'LINUX',
  'gui':True,
  'jni':True,
  'debug':False,
  'compilerflags':'-Wall -fPIC',
  'ldflags':'$(MINGW) -fPIC',
  'gccdir':'/usr',
  'flex':'/usr/bin/flex',
  'bison':'/usr/bin/bison',
  'jdkdir':'/usr/lib64/jvm/java-1.6.0',
  'qtdir':'/usr/lib64/qt3',
  'xmldir':'default',
  'openssldir':'default',
  'regtest':True,
  'perftest':True,
  'functest':True,
  'vobtest':False,
  'vobtest_logs':False,
  'rt2':True,
  'xsdtests':True,
  'pdfdir':os.path.join(BASEDIR, 'docs/TTCNv3-1.8.pl5'),
  'perl':'/usr/bin/perl',
  'perftestdir':os.path.join(BASEDIR, 'titan_nightly_builds/balls/perftest-20090927.tar.bz2'),
  'cpsmin':4000,
  'cpsmax':9000,
  'builddir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/x86_64_linux_tcclab5'),
  'installdir':'/mnt/TTCN/Releases/TTCNv3-bleedingedge',
  'logdir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/logs/x86_64_linux_tcclab5'),
  'measure':False
}

# On this platform we need to force `SOLARIS8', otherwise we get:
# g++ -c -DNDEBUG -I /mnt/TTCN/Tools/libxml2-2.7.1/include/libxml2 -DSOLARIS -DLICENSE -I/mnt/TTCN/Tools/openssl-0.9.8k/include -I../common -DUSES_XML -Wall -Wno-long-long -O2 Communication.cc
# Communication.cc: In static member function `static void TTCN_Communication::connect_mc()':
# Communication.cc:242: error: invalid conversion from `int*' to `socklen_t*'
# Communication.cc:242: error: initializing argument 3 of `int getsockname(int, sockaddr*, socklen_t*)'
# Communication.cc: In static member function `static boolean TTCN_Communication::increase_send_buffer(int, int&, int&)':
# Communication.cc:409: error: invalid conversion from `int*' to `socklen_t*'
configs['i386_solaris_bangjohansen'] = {
  'foa':False,
  'foadir':'',
  'version':'',
  'license':'/home/titanrt/license_8706.dat',
  'platform':'SOLARIS8',
  'gui':True,
  'jni':True,
  'debug':False,
  'compilerflags':'-Wall',
  'ldflags':'',
  'gccdir':'/mnt/TTCN/Tools/gcc-3.4.6-sol10',
  'flex':'/mnt/TTCN/Tools/flex-2.5.33/bin/flex',
  'bison':'/mnt/TTCN/Tools/bison-2.3/bin/bison',
  'jdkdir':'/mnt/TTCN/Tools/jdk1.6.0',
  'qtdir':'/mnt/TTCN/Tools/qt-x11-free-3.3.8-gcc4.1.1-sol10',
  'xmldir':'',
  'openssldir':'',
  'regtest':True,
  'perftest':False,
  'functest':False,
  'vobtest':False,
  'vobtest_logs':False,
  'rt2':True,
  'xsdtests':True,
  'pdfdir':os.path.join(BASEDIR, 'docs/TTCNv3-1.8.pl5'),
  'perl':'/usr/bin/perl',
  'perftestdir':os.path.join(BASEDIR, 'titan_nightly_builds/balls/perftest-20090927.tar.bz2'),
  'cpsmin':1000,
  'cpsmax':6000,
  'builddir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/i386_solaris_bangjohansen'),
  'installdir':'/mnt/TTCN/Releases/TTCNv3-bleedingedge',
  'logdir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/logs/i386_solaris_bangjohansen'),
  'measure':False
}

configs['vobtests_on_sparc_solaris_esekits3013'] = {
  'foa':False,
  'foadir':'',
  'version':'',
  'license':'/home/titanrt/license_8706.dat',
  'platform':'SOLARIS8',
  'gui':False,
  'jni':False,
  'debug':False,
  'compilerflags':'-Wall',
  'ldflags':'$(MINGW)',
  'gccdir':'/proj/TTCN/Tools/gcc-3.4.6-sol8',
  'flex':'/proj/TTCN/Tools/flex-2.5.35/bin/flex',
  'bison':'/proj/TTCN/Tools/bison-2.4.3/bin/bison',
  'jdkdir':'/proj/TTCN/Tools/jdk1.6.0_23',
  'qtdir':'/proj/TTCN/Tools/qt-x11-free-3.3.8-gcc3.4.6-sol8',
  'xmldir':'/proj/TTCN/Tools/libxml2-2.7.8',
  'openssldir':'/proj/TTCN/Tools/openssl-0.9.8r',
  'regtest':False,
  'perftest':False,
  'functest':False,
  'vobtest':True,
  'vobtest_logs':True,
  'rt2':True,
  'xsdtests':False,
  'pdfdir':os.path.join(BASEDIR, 'docs/TTCNv3-1.8.pl5'),
  'perl':'/proj/TTCN/Tools/perl-5.10.1/bin/perl',
  'perftestdir':os.path.join(BASEDIR, 'titan_nightly_builds/balls/perftest-20090927.tar.bz2'),
  'builddir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/vobtests_on_sparc_solaris_esekits3013'),
  'installdir':'/home/titanrt/TTCNv3-bleedingedge-vobtests_on_sparc_solaris_esekits3013',
  'logdir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/logs/vobtests_on_sparc_solaris_esekits3013'),
  'measure':False
}

configs['vobtests_on_x86_64_linux_tcclab1'] = {
  'foa':False,
  'foadir':'',
  'version':'',
  'license':'/home/titanrt/license_8706.dat',
  'platform':'LINUX',
  'gui':False,
  'jni':False,
  'debug':False,
  'compilerflags':'-Wall -fPIC',
  'ldflags':'$(MINGW) -fPIC',
  'gccdir':'/usr',
  'flex':'/usr/bin/flex',
  'bison':'/usr/bin/bison',
  'jdkdir':'',
  'qtdir':'',
  'xmldir':'default',
  'openssldir':'default',
  'regtest':False,
  'perftest':False,
  'functest':False,
  'vobtest':True,
  'vobtest_logs':True,
  'rt2':True,
  'xsdtests':False,
  'pdfdir':os.path.join(BASEDIR, 'docs/TTCNv3-1.8.pl5'),
  'perl':'/usr/bin/perl',
  'perftestdir':os.path.join(BASEDIR, 'titan_nightly_builds/balls/perftest-20090927.tar.bz2'),
  'builddir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/vobtests_on_x86_64_linux_tcclab1'),
  'installdir':'/tmp/TTCNv3-bleedingedge-vobs',
  'logdir':os.path.join(BASEDIR, 'titan_nightly_builds/build_slave/logs/vobtests_on_x86_64_linux_tcclab1'),
  'measure':False
}
