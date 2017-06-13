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

import logging, optparse, os, re, sys, time, fnmatch
import socket, traceback, types
# Never import everything!  E.g. enumerate() can be redefined somewhere.
# Check out: http://www.phidgets.com/phorum/viewtopic.php?f=26&t=3315.
import product_handler, titan_builder_cfg, titan_publisher, utils, threading

LOG_FILENAME = './titan_builder.log'
TRACE_FILENAME = './titan_builder.err-log'

vobtest_lock = threading.Lock()

class config_handler:
  """ Class to process the semi-configuration file and provide easy access to
      the configuration data read.  This very same configuration file will be
      reused by each slave.

      Or simply include the parts of the current shell script setting all
      environment variables?
  """
  def __init__(self, logger):
    self.logger = logger
    self.products = titan_builder_cfg.products
    self.recipients = titan_builder_cfg.recipients
    self.configs = titan_builder_cfg.configs
    self.slaves = titan_builder_cfg.slaves
    self.common = titan_builder_cfg.common

    self.validate_config_data()

  def __str__(self):
    """ For debugging purposes only.  """
    results = (str(self.configs), str(self.products), str(self.recipients), \
               str(self.slaves))
    return '\n'.join(results)

  def is_used_config(self, config):
    """ Check if the given build configuration is used by any of the slaves.
        It is assumed that the configuration file is already validated.

        Arguments:
          config: The name of the build configuration.
    """
    for slave in self.slaves:
      if config in self.slaves[slave]['configs']:
        return True
    # The build configuration will be skipped from the build process.
    return False

  def validate_config_data(self):
    """ We have only one configuration file.  The wrong addresses are filtered
        out automatically.  Add more checks.  Rewrite `installdir' in case of
        a FOA build.
    """
    self.recipients = dict([(key, self.recipients[key]) \
      for key in self.recipients \
        if re.match('^<[\w\-\.]+@(\w[\w\-]+\.)+\w+>$', self.recipients[key])])
#       elif current_section == 'slaves':
#         row_data = [data.strip() for data in line.split()]
#         if len(row_data) != 4:
#         elif not re.match('^\w+\s*(\d{1,3}.){3}\d{1,3}\s*\w+\s*[\w/]+$', line):
#         else:  # 100% correct data all over.
#           self.slaves[row_data[0]] = row_data[1:]
    for config_name, config_data in self.configs.iteritems():
      if 'foa' in config_data and config_data['foa'] and (not 'foadir' in config_data or len(config_data['foadir']) == 0):
        config_data['foadir'] = config_data['installdir']  # The final build directory, it'll be linked.
        config_data['installdir'] = "%s/temporary_foa_builds/TTCNv3-%s" % ('/'.join(config_data['foadir'].split('/')[0:-1]), utils.get_time(True))

class MasterThread(threading.Thread):
  def __init__(self, titan_builder, config, config_name, slave_list, log_dir, build_dir, tests):
    threading.Thread.__init__(self)
    self.titan_builder = titan_builder
    self.config = config
    self.config_name = config_name
    self.slave_list = slave_list
    self.log_dir = log_dir
    self.build_dir = build_dir
    self.tests = tests
    
  def run(self):
    self.slave_list.extend(self.titan_builder.master(self.config, self.config_name, self.log_dir, self.build_dir, self.tests))

class RegtestThread(threading.Thread):
  def __init__(self, titan_builder, config, slave_name):
    threading.Thread.__init__(self)
    self.titan_builder = titan_builder
    self.config = config
    self.slave_name = slave_name
    
  def run(self):
    self.titan_builder.pass_regtest(self.config, self.slave_name)

class FunctestThread(threading.Thread):
  def __init__(self, titan_builder, config, slave_name):
    threading.Thread.__init__(self)
    self.titan_builder = titan_builder
    self.config = config
    self.slave_name = slave_name
    
  def run(self):
    self.titan_builder.pass_functest(self.config, self.slave_name)

class PerftestThread(threading.Thread):
  def __init__(self, titan_builder, config, slave_name):
    threading.Thread.__init__(self)
    self.titan_builder = titan_builder
    self.config = config
    self.slave_name = slave_name
    
  def run(self):
    self.titan_builder.pass_perftest(self.config, self.slave_name)

class EclipseThread(threading.Thread):
  def __init__(self, titan_builder, config, slave_name):
    threading.Thread.__init__(self)
    self.titan_builder = titan_builder
    self.config = config
    self.slave_name = slave_name
    
  def run(self):
    self.titan_builder.pass_eclipse(self.config, self.slave_name)

class VobtestThread(threading.Thread):
  def __init__(self, titan_builder, config, slave_name):
    threading.Thread.__init__(self)
    self.titan_builder = titan_builder
    self.config = config
    self.slave_name = slave_name
    
  def run(self):
    self.titan_builder.pass_vobtest(self.config, self.slave_name)

class titan_builder:
  def __init__(self):
    self.logger = None
    self.logger = self.create_logger()
    self.config = None
    self.config = self.create_config()
    self.publisher = None
    self.publisher = self.create_publisher()

  def create_logger(self):
    if self.logger:
      return self.logger
    logger = logging.getLogger('titan_logger')
    logger.setLevel(logging.DEBUG)
    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
    handler = logging.FileHandler(LOG_FILENAME)
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    sth = logging.StreamHandler()
    sth.setLevel(logging.DEBUG)
    logger.addHandler(sth)
    return logger  # Just like in singleton.

  def create_config(self):
    """ Create the configuration file handler class.  If it's already created
        the existing one will be returned.  The configuration cannot be
        changed during operation.  It cannot be reloaded etc.
    """
    if self.config:
      return self.config
    return config_handler(self.logger)

  def create_publisher(self):
    if self.publisher:
      return self.publisher
    return titan_publisher.titan_publisher(self.logger, self.config)

  def remove_dups(self, list = []):
    """ Remove duplicates from a list.
    """
    tmp_list = []
    if len(list) > 0:
      [tmp_list.append(elem) for elem in list if not elem in tmp_list]
    return tmp_list

  def build(self, config, slave_name, reset, set_addressees, tests, path):
    """ Build the specified build configurations.  The configurations are
        built sequentially.  For the slaves a single build configuration should
        be specified in the command line.  The slave will abort build execution
        if there are more build configurations specified.  It's a limitation
        and should be relaxed later.

        Arguments:
          config: The list of build configurations specified in the command line.
          slave_name: The name of the slave if `--slave-mode' is on.
          reset: Reset statistics.
          set_addressees: List of recipients.
          tests: Tests to run for all configurations. 

        Returns:
          Nothing.  It's the main driver of the whole build.
    """
    config_list = []
    if not config:
      self.logger.warning('Running all available build configurations from ' \
                          'the configuration file...')
      config_list.extend(self.config.configs.keys())
    elif not re.match('^\w+(,\w+)*$', config):
      self.logger.error('Invalid build configuration list: `%s\'' % config)
    else:
      config = self.remove_dups(config.split(','))
      for config_elem in config:
        if not config_elem in self.config.configs.keys():
          self.logger.error('Build configuration `%s\' not found' \
                            % config_elem)
        else:
          config_list.append(config_elem)
    if not len(config_list) > 0:
      self.logger.error('No valid build configurations were found, ' \
                        'exiting...')
      return
    if set_addressees:
      self.config.recipients = {}
      addressees = set_addressees.split(',')
      for addressee in addressees:
        self.config.recipients[' '.join(addressee.split(' ')[:-1])] = addressee.split(' ')[-1];
    if not slave_name:
      everything_started_here = utils.get_time()
      utils.run_cmd('/bin/rm -rf %s %s && mkdir -p %s %s' \
                    % (self.config.common['builddir'], self.config.common['logdir'], self.config.common['builddir'], self.config.common['logdir']), None, 1800, self.logger)
      slave_list = []
      master_threads = []
      for config_name in config_list:
        if not self.config.is_used_config(config_name):
          self.logger.warning('Skipping unused build configuration: `%s\'' \
                              % config_name)
        else:
          # Create the slave and configuration specific log directory.  If the
          # logs haven't arrived yet from the given slave, that slave should
          # be considered lost.
          build_dir = os.path.join(self.config.common['builddir'], config_name)
          log_dir = os.path.join(self.config.common['logdir'], config_name)
          utils.run_cmd('/bin/rm -rf %s %s && mkdir -p %s %s' % (build_dir, log_dir, build_dir, log_dir), None, 1800, self.logger)
          master_thread = MasterThread(self, self.config.configs[config_name], config_name, slave_list, log_dir, build_dir, tests)
          master_thread.start()
          master_threads.append((config_name, master_thread))
      for config_name, master_thread in master_threads:
        master_thread.join()
        self.logger.debug('Master thread for `%s\' joined successfully' % config_name)
      everything_ended_here = utils.get_time()
      self.gather_all_stuff_together_and_present_to_the_public( \
        everything_started_here, everything_ended_here, slave_list, reset)
    else:
      # Run the tests on the given slave of each assigned build configuration.
      # It may cause problems if several configurations are run one after
      # another, but otherwise it's not possible assign multiple build
      # configurations at all.
      for config_name in config_list:
        self.logger.debug('Hello, from a slave `%s\' running build ' \
                          'configuration `%s\'' \
                          % (slave_name, config_name))
        if tests and len(tests) > 0:
          self.config.configs[config_name]['functest'] = tests.find('f') != -1
          self.config.configs[config_name]['perftest'] = tests.find('p') != -1
          self.config.configs[config_name]['regtest'] = tests.find('r') != -1
        self.slave(self.config.configs[config_name], config_name, slave_name)

  def get_titan(self, config, config_name, log_dir, build_dir):
    """ Get the TITAN sources from the CVS repository.  It can do checkouts by
        tag and date only.  If the version string is omitted HEAD will be
        used.  The checkout will be made into the build directory.  The output
        is not handled by the output handler yet.

        Arguments:
          The build configuration to get TITAN sources for.  Log/build
          directories.

        Returns:
          0 on success.  1 if the checkout failed for some reason.  It's most
          probably a timeout, since the parameters are validated and the
          existence of `cvs' is required.  So, it's safe to abort the build if
          1 is returned.
    """
    command_line = 'cd %s && cvs get TTCNv3' % build_dir
    if re.match('^v\d\-\d\-pl\d$', config['version']):
      command_line = 'cd %s && cvs co -r%s TTCNv3' \
                     % (build_dir, config['version'])
    elif re.match('2\d{7}', config['version']):
      command_line = 'cd %s && cvs co -D%s TTCNv3' \
                     % (build_dir, config['version'])
    command_line += ' 1>%s/cvs-%s.stdout 2>%s/cvs-%s.stderr' \
                    % (log_dir, config_name, log_dir, config_name)
    self.logger.debug('CVS checkout starting for config `%s\'', config_name)
    (retval, stdout, stderr) = utils.run_cmd(command_line, None, 10800)
    if retval:
      self.logger.error('The CVS checkout failed with command: `%s\', exit status: `%d\', stdout: `%s\', stderr: `%s\'' \
                        % (command_line, retval, stdout, stderr))
      return 1  # `retval' is not handled yet.
    self.logger.debug('CVS checkout finished for config `%s\'', config_name)
    return 0

  def master(self, config, config_name, log_dir, build_dir, tests):
    """ Prepare the packages for the slaves.  The regression tests and
        function tests are part of TITAN, hence the preparations regarding
        those tests are done together with TITAN.  It seems to make sense.
        Delete only the `TTCNv3' directory when switching between build
        configurations.  It's advised to use a different global build
        directory for the master and slaves.

        Arguments:
          The current build configuration and its name.
    """
    slave_list = []
    for slave_name in self.config.slaves:
      slave = self.config.slaves[slave_name]
      if not config_name in slave['configs']:
        continue
      slave_url = '%s@%s' % (slave['user'], slave['ip'])
      # Need more robust IP address checking.  It doesn't work on my Debian
      # laptop.  It can return simply `127.0.0.1' and fool this check
      # completely.
      is_localhost = socket.gethostbyname(socket.gethostname()) == slave['ip']
      if self.pass_prepare_titan(config, config_name, slave_name, log_dir, build_dir):
        continue  # Configuration for a given slave is failed.
      # The slave list is needed for the last pass.
      slave_list.append((slave_name, config_name, is_localhost))
      
      self.logger.debug('Removing old build `%s\' and log `%s\' ' \
                        'directories for slave `%s\' and build configuration `%s\'' \
                        % (config['builddir'], config['logdir'], slave_name, config_name))
      if is_localhost:  # Cleanup first.
        utils.run_cmd('/bin/rm -rf %s %s && mkdir -p %s %s' \
                      % (config['builddir'], config['logdir'],
                         config['builddir'], config['logdir']), None, 1800, self.logger)
      else:
        utils.run_cmd('ssh %s \'/bin/rm -rf %s %s && mkdir -p %s %s\'' \
                      % (slave_url, config['builddir'], config['logdir'],
                         config['builddir'], config['logdir']), None, 1800, self.logger)

      if config['perftest']:
        self.logger.debug('Copying performance tests for slave `%s\'' % slave_name)
        self.pass_prepare_perftest(config, config_name, slave, slave_name, slave_url, \
                                   is_localhost)
      if config['vobtest']:
        self.logger.debug('Copying VOB product tests for slave `%s\'' % slave_name)
        self.pass_prepare_vobtest(config, config_name, slave, slave_name, slave_url, \
                                  is_localhost)

      if is_localhost:  # Optimize local builds.
        self.logger.debug('It\'s a local build for slave `%s\' and build ' \
                          'configuration `%s\', working locally' \
                          % (slave_name, config_name))
        utils.run_cmd('cp %s/TTCNv3-%s.tar.bz2 %s' % \
                      (build_dir, config_name, config['builddir']), None, 1800)     
        utils.run_cmd('cp ./*.py ./*.sh %s' % config['builddir'])
        utils.run_cmd('cd %s && %s/titan_builder.sh -s %s -c %s %s' \
                      % (config['builddir'], config['builddir'], \
                         slave_name, config_name, ((tests and len(tests) > 0) and ('-t %s' % tests) or '')), None, 21600)
        utils.run_cmd('cp -r %s/%s/* %s' \
                      % (config['logdir'], slave_name, log_dir))
      else:
        self.logger.debug('It\'s a remote build for slave `%s\' and ' \
                          'build configuration `%s\', doing remote build' \
                          % (slave_name, config_name))
        (retval, stdout, stderr) = \
          utils.run_cmd('scp %s/TTCNv3-%s.tar.bz2 %s:%s' \
                        % (build_dir, config_name, slave_url,
                           config['builddir']), None, 1800)
        if not retval:
          self.logger.debug('The TITAN package is ready and distributed ' \
                            'for slave `%s\'' % slave_name)
        else:
          self.logger.error('Unable to distribute the TITAN package for ' \
                            'slave `%s\', it will be skipped from build ' \
                            'configuration `%s\'' % (slave_name, config_name))
          continue
        utils.run_cmd('scp ./*.py %s:%s' % (slave_url, config['builddir']))
        utils.run_cmd('scp ./*.sh %s:%s' % (slave_url, config['builddir']))
        utils.run_cmd('ssh %s \'cd %s && %s/titan_builder.sh -s %s -c ' \
                      '%s %s\'' % (slave_url, config['builddir'], \
                                config['builddir'], slave_name, config_name, ((tests and len(tests) > 0) and ('-t %s' % tests) or '')   ), None, 21600)
        utils.run_cmd('scp -r %s:%s/%s/* %s' \
                      % (slave_url, config['logdir'], slave_name, log_dir))
        
    return slave_list

  def gather_all_stuff_together_and_present_to_the_public(self, build_start, \
    build_end, slave_list, reset):
    """ Collect and process all logs.  Only the CVS logs are coming from the
        master.  If the CSV output is not arrived from a slave, then the slave
        will be considered lost.
    """
    build_root = utils.get_time(True)
    html_root = os.path.join(self.config.common['htmldir'], build_root)
    utils.run_cmd('mkdir -p %s' % html_root, None, 1800, self.logger)
    utils.run_cmd('cd %s && /bin/rm -f latest && ln -s %s latest' % (self.config.common['htmldir'], build_root))
    utils.run_cmd('cp -r %s/* %s' % (self.config.common['logdir'], html_root))
    email_file = '%s/report.txt' % html_root
    self.publisher.publish_csv2email(build_start, build_end, email_file, \
                                     slave_list, build_root, self.config.configs, reset)
    self.publisher.publish_html(build_root)
    utils.send_email(self.logger, self.config.recipients, email_file)

  def pass_prepare_titan(self, config, config_name, slave_name, log_dir, build_dir):
    """ Get TITAN from the CVS and configure it for the actual slave.  Then
        TITAN archive is created.  The archive is not copied to the actual
        slave, because this function can be a showstopper for the whole build
        process for the actual slave.

        Arguments:
          The build configuration and its name, the actual slave's name,
          log/build directories.

        Returns:
          0 if everything went fine.  1 is returned when e.g. the CVS was
          unreachable or the TITAN configuration failed for some reason.
          Returning 1 should stop the build process for the actual slave.
    """
    if self.get_titan(config, config_name, log_dir, build_dir):
      self.logger.error('The CVS checkout failed for slave `%s\' and ' \
                        'build configuration `%s\'' \
                        % (slave_name, config_name))
      return 1
    if self.config_titan(config, build_dir):
      self.logger.error('Configuring TITAN failed for slave `%s\' ' \
                        'and build configuration `%s\'' \
                        % (slave_name, config_name))
      return 1
    utils.run_cmd('cd %s && tar cf TTCNv3-%s.tar ./TTCNv3' \
                  % (build_dir, config_name), None, 1800)
    utils.run_cmd('cd %s && bzip2 TTCNv3-%s.tar' \
                  % (build_dir, config_name), None, 1800)
    utils.run_cmd('/bin/rm -rf %s/TTCNv3' % build_dir, None, 1800)
    return 0

  def pass_prepare_perftest(self, config, config_name, slave, slave_name, slave_url, \
                            is_localhost):
    """ Copy the performance test package to the actual slave.  It's a simple
        archive.  Its location is defined in the configuration file.

        Arguments:
          The build configuration and its name with the actual slave and its
          name.  Nothing is returned.
    """
    if os.path.isfile(config['perftestdir']):
      if is_localhost:
        utils.run_cmd('cp -f %s %s' % (config['perftestdir'], \
                      config['builddir']))
      else:
        (retval, stdout, stderr) = utils.run_cmd('scp %s %s:%s' \
          % (config['perftestdir'], slave_url, config['builddir']))
        if retval:
          self.logger.error('Unable to copy performance test package ' \
                            'to slave `%s\'' % slave_name)
    else:
      self.logger.error('The performance test package cannot be found at ' \
                        '`%s\'' % config['perftestdir'])

  def pass_prepare_vobtest(self, config, config_name, slave, slave_name, slave_url, \
                           is_localhost):
    """ Collect and configure the VOB products.  The products will be
        collected only if there's no file matching the `vobtest-*.tar.bz2'
        pattern in the build directory.  The resulting archive is copied to
        the given slave only if it's a remote slave.  The errors are reported
        to the local error log.  The URL of the slave is calculated locally.

        Arguments:
          The build configuration and its name with the actual slave and its
          name.  Nothing is returned.
    """
    vobtest_lock.acquire()
    really_collect_products = len([file for file in os.listdir(self.config.common['builddir']) \
                                   if fnmatch.fnmatch(file, 'vobtest\-.*\.tar\.bz2')]) == 0
    if really_collect_products:
      handler = product_handler.product_handler(self.logger, self.config)
      if handler.config_products('%s/vobtest' % self.config.common['builddir']):
        self.logger.error('Configuring VOB products failed for slave: ' \
                          '`%s\' and build configuration: `%s\'' \
                          % (slave_name, config_name))
        return
      utils.run_cmd('cd %s && tar cf vobtest-%s.tar ./vobtest' \
                    % (self.config.common['builddir'], \
                       time.strftime('%Y%m%d')), None, 1800)
      utils.run_cmd('cd %s && bzip2 vobtest-*.tar' \
                    % self.config.common['builddir'], None, 1800)
      utils.run_cmd('/bin/rm -rf %s/vobtest %s/vobtest-*.tar' \
                    % (self.config.common['builddir'], \
                       self.config.common['builddir']), None, 1800)
    else:
      self.logger.debug('VOB products don\'t need to be configured again')
    vobtest_lock.release()
    if not is_localhost:
      (retval, stdout, stderr) = \
        utils.run_cmd('scp %s/vobtest-*.tar.bz2 %s:%s' \
                      % (self.config.common['builddir'], slave_url, \
                         config['builddir']))
      if retval:
        self.logger.error('Copying the VOB package to slave: ' \
                          '`%s\' failed for build configuration: `%s\'' \
                          % (slave_name, config_name))
    else:
      utils.run_cmd('cp %s/vobtest-*.tar.bz2 %s' \
                    % (self.config.common['builddir'], config['builddir']))

  def slave(self, config, config_name, slave_name):
    """ Run the build passes sequentially.  If the TITAN build fails, the
        remaining passes are skipped.  Log everything.  All the results will
        be written in all supported formats.  It should be configurable.
    """
    self.logger.debug('Setting environment variables from `pass_setenv()\'')
    self.pass_setenv(config, slave_name)
    self.logger.debug('Building TITAN from `pass_titan()\'')
    stamp_old = utils.get_time()
    if not self.pass_titan(config, config_name, slave_name):
      test_threads = []
      if config['regtest']:
        regtest_thread = RegtestThread(self, config, slave_name)
        regtest_thread.start()
        test_threads.append(('regression tests', regtest_thread))
        self.logger.debug('Running regression tests from `pass_regtest()\'')
      if config['functest']:
        functest_thread = FunctestThread(self, config, slave_name)
        functest_thread.start()
        test_threads.append(('function tests', functest_thread))
        self.logger.debug('Running function tests from `pass_functest()\'')
      if config['perftest']:
        perftest_thread = PerftestThread(self, config, slave_name)
        perftest_thread.start()
        test_threads.append(('performance tests', perftest_thread))
        self.logger.debug('Running performance tests from `pass_perftest()\'')
      if 'eclipse' in config and config['eclipse']:
        eclipse_thread = EclipseThread(self, config, slave_name)
        eclipse_thread.start()
        test_threads.append(('eclipse tests', eclipse_thread))
        self.logger.debug('Running Eclipse build from `pass_eclipse()\'')
      if config['vobtest']:
        vobtest_thread = VobtestThread(self, config, slave_name)
        vobtest_thread.start()
        test_threads.append(('VOB product tests', vobtest_thread))
        self.logger.debug('Running VOB product tests from `pass_vobtest()\'')
      for test_thread_name, test_thread in test_threads:
        test_thread.join()
        self.logger.debug('Thread for `%s\' joined successfully' % test_thread_name)
    self.publisher.dump_csv(stamp_old, utils.get_time(), config, config_name, slave_name)
    self.publisher.dump_txt(stamp_old, utils.get_time(), config, config_name, slave_name)
    self.publisher.dump_html(stamp_old, utils.get_time(), config, config_name, slave_name)
    self.logger.debug('Finalizing build from using `pass_slave_postprocess()\'')
    self.pass_slave_postprocess(config, config_name, slave_name)

  def pass_slave_postprocess(self, config, config_name, slave_name):
    """ Archive stuff and make everything available for the master.  The
        master will copy all necessary stuff.  The build directory is
        available until the next build.  Do the cleanup here.  The installation
        directory is never removed.

        Arguments:
          The current build configuration.
    """
    utils.run_cmd('cd %s && tar cf TTCNv3-%s-bin.tar ./TTCNv3' \
                  % (config['builddir'], config_name), None, 1800)
    utils.run_cmd('bzip2 %s/TTCNv3-%s-bin.tar' \
                  % (config['builddir'], config_name), None, 1800)
    utils.run_cmd('/bin/rm -rf %s/TTCNv3' % config['builddir'], None, 1800)
    utils.run_cmd('/bin/rm -f %s/TTCNv3-%s.tar.bz2' \
                  % (config['builddir'], config_name))
    utils.run_cmd('cd %s && /bin/rm -f *.py *.pyc *.sh' % config['builddir'])
    utils.run_cmd('mv -f %s %s %s/%s' % (LOG_FILENAME, TRACE_FILENAME, \
                                         config['logdir'], slave_name))

  def pass_titan(self, config, config_name, slave_name):
    """ Build pass for TITAN itself.  It is assumed that the master have
        already copied the TITAN package to the build directory.  It's the
        only requirement here.  If the installation fails the TITAN build is
        considered as a failure.  Only the `make install' is taken into
        consideration.

        Arguments:
          The current build configuration of the slave and its name.

        Returns:
          1 on error, e.g. if the TITAN package is not present.  0 if the
          TITAN package was found and the full build completed successfully.
    """
    stamp_begin = utils.get_time()
    utils.run_cmd('mkdir -p %s/%s' % (config['logdir'], slave_name), None, 1800, self.logger)
    utils.run_cmd('bunzip2 %s/TTCNv3-%s.tar.bz2' \
                  % (config['builddir'], config_name), None, 1800, self.logger)
    utils.run_cmd('cd %s && tar xf TTCNv3-%s.tar && bzip2 %s/TTCNv3-*.tar' \
                  % (config['builddir'], config_name, config['builddir']), \
                  None, 1800, self.logger)
    if not os.path.isdir('%s/TTCNv3' % config['builddir']):
      self.logger.error('The `%s/TTCNv3\' directory is not found' \
                        % config['builddir'])
      self.publisher.titan_out(config, slave_name, \
                               (stamp_begin, utils.get_time(), None))
      return 1
    utils.run_cmd('cd %s && find . -exec touch {} \;' % config['builddir'], None, 1800)
    (ret_val_dep, stdout_dep, stderr_dep) = \
      utils.run_cmd('cd %s/TTCNv3 && make dep 2>&1' \
                    % config['builddir'], None, 1800)
    (ret_val_make, stdout_make, stderr_make) = \
      utils.run_cmd('cd %s/TTCNv3 && make -j4 2>&1' \
                    % config['builddir'], None, 1800)
    (ret_val_install, stdout_install, stderr_install) = \
      utils.run_cmd('cd %s/TTCNv3 && make install 2>&1' \
                    % config['builddir'], None, 1800)
    if ret_val_make or ret_val_install:
      self.logger.error('TITAN build failed for slave `%s\', please check ' \
                        'the logs for further investigation, stopping slave ' \
                        % slave_name)
    output = (stamp_begin, utils.get_time(), \
              ((ret_val_dep, stdout_dep, stderr_dep), \
               (ret_val_make, stdout_make, stderr_make), \
               (ret_val_install, stdout_install, stderr_install)))
    self.publisher.titan_out(config, slave_name, output)
    if ret_val_dep or ret_val_make or ret_val_install:
      return 1
    else:
      if 'foa' in config and config['foa'] and 'foadir' in config and config['foadir'] != config['installdir']:
        # The `installdir' must be removed by hand after a FOA period.  Cannot
        # be automated in a sane way.  For FOA `installdir' shall be a unique
        # directory.  E.g. date based.  Otherwise, the builds are always
        # overwritten.
        self.logger.debug('Linking directories for FOA build to `%s\'' % config['foadir'])
        (ret_val_rm, stdout_rm, stderr_rm) = utils.run_cmd('/bin/rm -rvf %s' % config['foadir'])
        if ret_val_rm:  # Sometimes it doesn't work.
          self.logger.error('Unable to remove `%s\': `%s\'' % (config['foadir'], ''.join(stderr_rm)))
        utils.run_cmd('ln -s %s %s' % (config['installdir'], config['foadir']))
      return 0

  def pass_setenv(self, config, slave_name):
    """ Set some environment variables needed to run the TITAN tests.  Don't
        use uppercase latters in directory names.  The GCC is added as well.
        Always check if an environment variable exists before reading it.

        Arguments:
          The current build configuration of the slave and its name.
    """
    path = os.environ.get('PATH')
    ld_library_path = os.environ.get('LD_LIBRARY_PATH')
    os.environ['PATH'] = '%s/bin:%s/bin:%s' % (config['installdir'], config['gccdir'], path and path or '')
    os.environ['LD_LIBRARY_PATH'] = '%s/lib:%s/lib:%s' % (config['installdir'], config['gccdir'], ld_library_path and ld_library_path or '')
    os.environ['TTCN3_DIR'] = config['installdir']
    os.environ['TTCN3_LICENSE_FILE'] = config['license']

  def pass_regtest_helper(self, config, slave_name, runtime):
    """ Run the regression tests with `make' and then `make run'.  The output
        is sent to the publisher as well.  At the end, `make clean' is done to
        save some bytes.  Don't use `tee', since its exit code will always be
        0.  Only `stdout' is used.

        Arguments:
          config: The current build configuration.
          slave_name: The name of the slave.
          runtime: 0 for the load-test run-time, 1 for the function-test
                   runtime.
    """
    utils.run_cmd('cd %s/TTCNv3/regression_test && make distclean' \
                  % config['builddir'], None, 1800)
    (ret_val_make, stdout_make, stderr_make) = \
      utils.run_cmd('cd %s/TTCNv3/regression_test && %s make 2>&1' \
                    % (config['builddir'], runtime and 'RT2=1' or ''), None, 3600)
    if ret_val_make:
      self.logger.error('The regression test failed to build for the ' \
                        '`%s\' runtime' % (runtime and 'function-test' or 'load-test'))
    (ret_val_run, stdout_run, stderr_run) = \
      utils.run_cmd('cd %s/TTCNv3/regression_test && %s make run 2>&1' \
                    % (config['builddir'], runtime and 'RT2=1' or ''), None, 1800)
    failed_lines = []
    all_fine = True
    for index, line in globals()['__builtins__'].enumerate(stdout_run):
      matched_line = re.search('Verdict stat.*pass \((\d+)\..*', line)
      if matched_line and int(matched_line.group(1)) != 100:
        if all_fine and not failed_lines:
          failed_lines.append('\nThe failed tests were the following:\n\n')
        if not re.search('TverdictOper', stdout_run[index - 1]):
          all_fine = False
        failed_lines.append(stdout_run[index - 1])
        failed_lines.append(line)
    stdout_run.extend(failed_lines)
    if ret_val_run or not all_fine:
      self.logger.error('The regression test failed to run for the ' \
                        '`%s\' runtime' % (runtime and 'function-test' or 'load-test'))
      ret_val_run = 1
    utils.run_cmd('cd %s/TTCNv3/regression_test && make clean' \
                  % config['builddir'], None, 1800)
    return ((ret_val_make, stdout_make, stderr_make), \
             (ret_val_run, stdout_run, stderr_run))

  def pass_regtest(self, config, slave_name):
    """ Build and run the regression tests and publish the results.  The
        `pass_regtest_helper()' does the dirty job.

        Arguments:
          config: The current build configuration.
          slave_name: The name of the slave.
    """
    output = {}
    stamp_begin = utils.get_time()
    rt1_results = self.pass_regtest_helper(config, slave_name, 0)
    output['rt1'] = (stamp_begin, utils.get_time(), rt1_results)
    if config['rt2']:
      stamp_begin = utils.get_time()
      rt2_results = self.pass_regtest_helper(config, slave_name, 1)
      output['rt2'] = (stamp_begin, utils.get_time(), rt2_results)
    self.publisher.regtest_out(config, slave_name, output)

  def pass_eclipse(self, config, slave_name):
    """ Build Eclipse plugins and publish them to an update site.

        Arguments:
          config: The current build configuration.
          slave_name: The name of the slave.
    """
    output = {}
    stamp_begin = utils.get_time()
    results = utils.run_cmd('cd %s/TTCNv3/eclipse/automatic_build && ant -d -l mylog.log -f build_main.xml updatesite.experimental 2>&1' \
                            % config['builddir'], None, 1800)
    log_dir = os.path.join(config['logdir'], slave_name)
    utils.run_cmd('cp %s/TTCNv3/eclipse/automatic_build/mylog.log %s/eclipse-mylog.log' \
                  % (config['builddir'], log_dir))                            
    output = (stamp_begin, utils.get_time(), os.path.join(log_dir, 'eclipse-mylog.log'), results)
    self.publisher.eclipse_out(config, slave_name, output)

  def pass_perftest_helper(self, config, slave_name, runtime):
    """ Build the performance test and run it for some predefined CPS values.
        These CPS values should come from the build configurations instead.
        Obviously, if the build fails all test runs are skipped.  It handles
        its own tarball as well.  It's unpacked at the beginning and removed
        at the end.  The results are also published.

        Arguments:
          The actual build configuration and the name of the slave.  The
          function returns nothing.
    """
    perftest_out = {}
    utils.run_cmd('cd %s/perftest && ttcn3_makefilegen -e titansim %s ' \
                  '*.ttcnpp *.ttcnin *.ttcn *.cc *.cfg' \
                  % (config['builddir'], (runtime and '-fpgR' or '-fpg')))
    # Avoid infinite recursion.
    utils.run_cmd('sed \'s/^-include $(DEPFILES)$//\' Makefile >Makefile-tmp && mv Makefile-tmp Makefile',
                  os.path.join(config['builddir'], 'perftest'))
    utils.run_cmd('cd %s/perftest && make clean' % config['builddir'])
    (ret_val_dep, stdout_dep, stderr_dep) = \
      utils.run_cmd('cd %s/perftest && find . -exec touch {} \; && make %s dep 2>&1' \
                    % (config['builddir'], (runtime and 'RT2=1' or '')), \
                    None, 900)
    (ret_val_make, stdout_make, stderr_make) = \
      utils.run_cmd('cd %s/perftest && make %s 2>&1' \
                    % (config['builddir'], (runtime and 'RT2=1' or '')), \
                    None, 1800)
    perftest_out['dep'] = (ret_val_dep, stdout_dep, stderr_dep)
    perftest_out['make'] = (ret_val_make, stdout_make, stderr_make)
    perftest_out['run'] = []
    if not ret_val_make:
      cps_min = config.get('cpsmin', 1000)
      cps_max = config.get('cpsmax', 2000)
      cps_diff = abs(cps_max - cps_min) / 5
      for cps in range(cps_min, cps_max + cps_diff, cps_diff):
        # These numbers should be platform dependent.  Lower on slow
        # machines and high on fast machines.
        (ret_val_run, stdout_run, stderr_run) = \
          utils.run_cmd('cd %s/perftest && cpp -DTSP_CPS_CPP=%d.0 config.cfg >config.cfg-tmp && ' \
                        'ttcn3_start ./titansim ./config.cfg-tmp 2>&1' \
                        % (config['builddir'], cps), None, 900)
        for line in stdout_run:
          matched_line = re.search('Verdict stat.*pass \((\d+)\..*', line)
          if matched_line and int(matched_line.group(1)) != 100:
            self.logger.error('Performance test failed to run for `%d\' CPSs' % cps)
            ret_val_run = 1
        perftest_out['run'].append((cps, (ret_val_run, stdout_run, stderr_run)))
    else:
      self.logger.error('Performance test compilation failed for the ' \
                        '`%s\' runtime' % (runtime and 'function-test' or 'load-test'))
    return perftest_out

  def pass_perftest(self, config, slave_name):
    """ Build and run the performance tests and publish the results.  The
        `pass_perftest_helper()' does the dirty job.

        Arguments:
          config: The current build configuration.
          slave_name: The name of the slave.
    """
    utils.run_cmd('bunzip2 perftest-*.tar.bz2', config['builddir'], 1800)
    utils.run_cmd('tar xf ./perftest-*.tar && bzip2 ./perftest-*.tar', \
                  config['builddir'], 1800)
    if not os.path.isdir('%s/perftest' % config['builddir']):
      self.logger.error('The performance test is not available at ' \
                        '`%s/perftest\'' % config['builddir'])
    else:
      output = {}
      stamp_begin = utils.get_time()
      rt1_results = self.pass_perftest_helper(config, slave_name, 0)
      output['rt1'] = (stamp_begin, utils.get_time(), rt1_results)
      if config['rt2']:
        stamp_begin = utils.get_time()
        rt2_results = self.pass_perftest_helper(config, slave_name, 1)
        output['rt2'] = (stamp_begin, utils.get_time(), rt2_results)
      self.publisher.perftest_out(config, slave_name, output)
    utils.run_cmd('/bin/rm -rf %s/perftest*' % config['builddir'], None, 1800)

  def pass_functest_helper(self, config, slave_name, runtime):
    function_test_prefix = '%s/TTCNv3/function_test' % config['builddir']
    function_test_prefixes = ('%s/BER_EncDec' % function_test_prefix, \
                              '%s/Config_Parser' % function_test_prefix, \
                              '%s/RAW_EncDec' % function_test_prefix, \
                              '%s/Semantic_Analyser' % function_test_prefix, \
                              '%s/Text_EncDec' % function_test_prefix, \
                              '%s/Semantic_Analyser/float' % function_test_prefix, \
                              '%s/Semantic_Analyser/import_of_iports' % function_test_prefix, \
                              '%s/Semantic_Analyser/options' % function_test_prefix, \
                              '%s/Semantic_Analyser/ver' % function_test_prefix, \
                              '%s/Semantic_Analyser/xer' % function_test_prefix)
    log_dir = os.path.join(config['logdir'], slave_name)
    functest_out = {}
    stamp_old = utils.get_time()
    for function_test in function_test_prefixes:
      utils.run_cmd('ln -s %s %s' % (config['perl'], function_test))
      function_test_name = function_test.split('/')[-1]
      ber_or_raw_or_text = not (function_test_name == 'Config_Parser' or function_test_name == 'Semantic_Analyser')
      utils.run_cmd('cd %s && %s ./%s %s 2>&1 | tee %s/functest-%s.%s' \
                    % (function_test, (runtime and 'RT2=1' or ''),
                       (os.path.isfile('%s/run_test_all' % function_test) \
                                      and 'run_test_all' or 'run_test'), \
                       ((runtime and not ber_or_raw_or_text) and '-rt2' or ''), \
                       log_dir, function_test_name, \
                       (runtime and 'rt2' or 'rt1')), None, 3600)
      error_target = os.path.join(log_dir, 'functest-%s-error.%s' % (function_test_name, (runtime and 'rt2' or 'rt1')))  
      if ber_or_raw_or_text:
        utils.run_cmd('cp %s/%s_TD.script_error %s' \
                      % (function_test, function_test_name, error_target))
        utils.run_cmd('cp %s/%s_TD.fast_script_error %s' \
                      % (function_test, function_test_name, error_target))
      functest_out[function_test_name] = \
        ('%s/functest-%s.%s' % (log_dir, function_test_name, (runtime and 'rt2' or 'rt1')), \
         (ber_or_raw_or_text and error_target or ''))
    return functest_out

  def pass_functest(self, config, slave_name):
    """ Build pass to build and run the function tests.  The
        `pass_functest_helper()' does the direty job.
    """
    output = {}
    stamp_begin = utils.get_time()
    rt1_results = self.pass_functest_helper(config, slave_name, 0)
    output['rt1'] = (stamp_begin, utils.get_time(), rt1_results)
    if config['rt2']:
      stamp_begin = utils.get_time()
      rt2_results = self.pass_functest_helper(config, slave_name, 1)
      output['rt2'] = (stamp_begin, utils.get_time(), rt2_results)
    self.publisher.functest_out(config, slave_name, output)

  def pass_vobtest(self, config, slave_name):
    """ Build pass for the VOB products.  Currently, the VOB products are
        compiled only due to the lack of usable tests written for them.  The
        output is stored here by the publisher.  The normal runtime should
        always be the first, it's a restriction of the publisher.

        Arguments:
          The actual build configuration and its name.
    """
    utils.run_cmd('bunzip2 %s/vobtest-*.tar.bz2' \
                  % config['builddir'], None, 1800)
    utils.run_cmd('cd %s && tar xf ./vobtest-*.tar && bzip2 ./vobtest-*.tar' \
                  % config['builddir'], None, 1800)
    if not os.path.isdir('%s/vobtest' % config['builddir']):
      self.logger.error('The products are not available at `%s/vobtest\'' \
                        % config['builddir'])
      self.publisher.vobtest_out(utils.get_time(), utils.get_time(), {})
    else:
      output = {}
      stamp_begin = utils.get_time()
      handler = product_handler.product_handler(self.logger, self.config)
      log_dir = '%s/%s/products' % (config['logdir'], slave_name)
      results = handler.build_products('%s/vobtest' % config['builddir'], \
                                       log_dir, config, False)
      output['rt1'] = (stamp_begin, utils.get_time(), results)
      if config['rt2']:
        stamp_begin = utils.get_time()
        results = handler.build_products('%s/vobtest' \
                                         % config['builddir'], log_dir, config, True)
        output['rt2'] = (stamp_begin, utils.get_time(), results)
      self.publisher.vobtest_out(config, slave_name, output)
    utils.run_cmd('/bin/rm -rf %s/vobtest*' % config['builddir'], None, 1800)

  def config_titan(self, config, build_dir):
    """ Modify TITAN configuration files to create a platform-specific source
        package.  The original files are always preserved in an `*.orig' file.
        `sed' would be shorter, but this way everything is under control.
        Improve file handling.  It is assumed, that there's a `TTCNv3' directory
        in the build directory.

        Arguments:
          config: The build configuration we're configuring for.

        Returns:
          If everything went fine 0 is returned.  Otherwise 1 is returned and
          the error messages will be logged.  The screen always stays intact.
    """
    if not os.path.isdir('%s/TTCNv3' % build_dir):
      self.logger.error('The `%s/TTCNv3\' directory is not found' % build_dir)
      return 1  # It's a fatal error, no way out.
    # Prepare all function tests.  Add links to the `perl' interpreter and
    # modify some some Makefiles containing the platform string.
    if config['functest']:
      function_test_prefix = '%s/TTCNv3/function_test' % build_dir
      for function_test in ('%s/BER_EncDec' % function_test_prefix, \
                            '%s/Config_Parser' % function_test_prefix, \
                            '%s/RAW_EncDec' % function_test_prefix, \
                            '%s/Semantic_Analyser' % function_test_prefix, \
                            '%s/Text_EncDec' % function_test_prefix):
        if os.path.isdir(function_test):
          if function_test.endswith('BER_EncDec') or \
             function_test.endswith('RAW_EncDec') or \
             function_test.endswith('Text_EncDec'):
            utils.run_cmd('mv %s/Makefile %s/Makefile.orig' \
                          % (function_test, function_test))
            berrawtext_makefile = open('%s/Makefile.orig' % function_test, 'rt')
            berrawtext_makefile_new = open('%s/Makefile' % function_test, 'wt')
            for line in berrawtext_makefile:
              if re.match('^PLATFORM\s*:?=\s*\w+$', line):  # Platform
                # autodetect later.  It is hard-coded into the build
                # configuration.
                berrawtext_makefile_new.write('PLATFORM = %s\n' % config['platform'])
              elif re.match('^CXX\s*:?=\s*.*$', line) and ('cxx' in config and len(config['cxx']) > 0):
                berrawtext_makefile_new.write('CXX = %s\n' % config['cxx'])
              else:
                berrawtext_makefile_new.write(line)
            if function_test.endswith('BER_EncDec'):
              utils.run_cmd('mv %s/run_test %s/run_test.orig' \
                            % (function_test, function_test))
              utils.run_cmd('cat %s/run_test.orig | ' \
                            'sed s/TD.script/TD.fast_script/ >%s/run_test ' \
                            '&& chmod 755 %s/run_test' \
                            % (function_test, function_test, function_test))  # Make it fast.
        else:
          self.logger.warning('Function test directory `%s\' is not found'
                              % function_test)
    # Add `-lncurses' for all `LINUX' targets.  It's not always needed, hence
    # platform autodetect won't help in this.
    if config['platform'] == 'LINUX':
      mctr_makefile_name = '%s/TTCNv3/mctr2/mctr/Makefile' % build_dir
      if os.path.isfile(mctr_makefile_name):
        utils.run_cmd('mv %s %s.orig' % (mctr_makefile_name, mctr_makefile_name))
        mctr_makefile = open('%s.orig' % mctr_makefile_name, 'rt')
        mctr_makefile_new = open(mctr_makefile_name, 'wt')
        for line in mctr_makefile:
          if re.match('^LINUX_CLI_LIBS\s*:?=\s*$', line):
            mctr_makefile_new.write('LINUX_CLI_LIBS := -lncurses\n')
          else:
            mctr_makefile_new.write(line)
        mctr_makefile.close()
        mctr_makefile_new.close()
      else:
        self.logger.warning('The `%s\' is not found' % mctr_makefile_name)
    # Prepare the main configuration file.
    makefile_cfg_name = '%s/TTCNv3/Makefile.cfg' % build_dir
    if os.path.isfile(makefile_cfg_name):
      utils.run_cmd('mv %s %s.orig' % (makefile_cfg_name, makefile_cfg_name))
      makefile_cfg = open('%s.orig' % makefile_cfg_name, 'rt')
      makefile_cfg_new = open(makefile_cfg_name, 'wt')
      for line in makefile_cfg:
        if re.match('^TTCN3_DIR\s*:?=\s*.*$', line):
          # Use the environment.
          continue
        elif re.match('^DEBUG\s*:?=\s*.*$', line):
          makefile_cfg_new.write('DEBUG := %s\n' % (config['debug'] and 'yes' or 'no'))
        elif re.match('^# PLATFORM\s*:?=\s*.*$', line) and len(config['platform']) > 0:
          # Automatic platform detection doesn't seem to work very well, so the
          # platform should always be set explicitly.
          makefile_cfg_new.write('PLATFORM := %s\n' % config['platform'])
        elif re.match('^JNI\s*:?=\s*.*$', line):
          # It's the so called `and-or' trick from http://diveintopython.org/
          # power_of_introspection/and_or.html.
          makefile_cfg_new.write('JNI := %s\n' % (config['jni'] and 'yes' or 'no'))
        elif re.match('^GUI\s*:?=\s*.*$', line):
          makefile_cfg_new.write('GUI := %s\n' % (config['gui'] and 'yes' or 'no'))
        elif re.match('^FLEX\s*:?=\s*.*$', line) and len(config['flex']) > 0:
          makefile_cfg_new.write('FLEX := %s\n' % config['flex'])
        elif re.match('^BISON\s*:?=\s*.*$', line) and len(config['bison']) > 0:
          makefile_cfg_new.write('BISON := %s\n' % config['bison'])
        elif re.match('^CC\s*:?=\s*.*$', line) and len(config['gccdir']) > 0:
          makefile_cfg_new.write('CC := %s/bin/%s\n' % (config['gccdir'], (('cc' in config and len(config['cc']) > 0) and config['cc'] or 'gcc')))
        elif re.match('^CXX\s*:?=\s*.*$', line) and len(config['gccdir']) > 0:
          makefile_cfg_new.write('CXX := %s/bin/%s\n' % (config['gccdir'], (('cxx' in config and len(config['cxx']) > 0) and config['cxx'] or 'g++')))
        elif re.match('^JDKDIR\s*:?=\s*.*$', line) and len(config['jdkdir']) > 0:
          makefile_cfg_new.write('JDKDIR := %s\n' % config['jdkdir'])
        elif re.match('^QTDIR\s*:?=\s*.*$', line) and len(config['qtdir']) > 0:
          makefile_cfg_new.write('QTDIR = %s\n' % config['qtdir'])
        elif re.match('^XMLDIR\s*:?=\s*.*$', line) and len(config['xmldir']) > 0:
          makefile_cfg_new.write('XMLDIR = %s\n' % config['xmldir'])
        elif re.match('^OPENSSL_DIR\s*:?=\s*.*$', line) and len(config['openssldir']) > 0:
          makefile_cfg_new.write('OPENSSL_DIR = %s\n' % config['openssldir'])
        elif re.match('^LDFLAGS\s*:?=\s*.*$', line) and len(config['ldflags']) > 0:
          makefile_cfg_new.write('LDFLAGS = %s\n' % config['ldflags'])
        elif re.match('^COMPILERFLAGS\s*:?=\s*.*$', line) and len(config['compilerflags']) > 0:
          makefile_cfg_new.write('COMPILERFLAGS = %s\n' % config['compilerflags'])
        else:
          makefile_cfg_new.write(line)
      makefile_cfg.close()
      makefile_cfg_new.close()
    else:
      self.logger.error('The `%s\' is not found, it seems to be a fake ' \
                        'installation' % makefile_cfg_name)
      return 1  # It's essential, exit immediately.
    if config['regtest']:
      regtest_makefile_name = '%s/TTCNv3/regression_test/Makefile.regression' % build_dir
      if os.path.isfile(regtest_makefile_name):
        utils.run_cmd('mv %s %s.orig' \
                      % (regtest_makefile_name, regtest_makefile_name))
        regtest_makefile = open('%s.orig' % regtest_makefile_name, 'rt')
        regtest_makefile_new = open(regtest_makefile_name, 'wt')
        for line in regtest_makefile:
          if re.match('^TTCN3_DIR\s*:?=\s*.*$', line):
            # Use the environment.
            continue
          elif re.match('^CC\s*:?=\s*.*$', line) and len(config['gccdir']) > 0:
            regtest_makefile_new.write('CC := %s/bin/%s\n' % (config['gccdir'], (('cc' in config and len(config['cc']) > 0) and config['cc'] or 'gcc')))
          elif re.match('^CXX\s*:?=\s*.*$', line) and len(config['gccdir']) > 0:
            regtest_makefile_new.write('CXX := %s/bin/%s\n' % (config['gccdir'], (('cxx' in config and len(config['cxx']) > 0) and config['cxx'] or 'g++')))
          elif re.match('^XMLDIR\s*:?=\s*.*$', line) and len(config['xmldir']) > 0:
            regtest_makefile_new.write('XMLDIR = %s\n' % config['xmldir'])            
          else:
            regtest_makefile_new.write(line)
        regtest_makefile.close()
        regtest_makefile_new.close()
      else:
        self.logger.warning('Regression test configuration file `%s\' is ' \
                            'not found' % regtest_makefile_name)
      if 'xsdtests' in config and not config['xsdtests']:
        self.logger.warning('Disabling `xsd2ttcn\' tests to save some time')
        utils.run_cmd('mv %s %s.orig' \
                      % (regtest_makefile_name.split('.')[0], \
                      regtest_makefile_name.split('.')[0]))
        utils.run_cmd('cat %s.orig | sed s/\'xsdConverter\'/\'\'/ >%s' \
                      % (regtest_makefile_name.split('.')[0], \
                      regtest_makefile_name.split('.')[0]))
    self.config_pdfs(config, build_dir)
    self.logger.debug('`%s/TTCNv3\' was configured and ready to build, all ' \
                      'Makefiles were modified successfully' % build_dir)
    return 0  # Allow the caller to catch errors.  Use exceptions later
    # instead.  Only `./TTCNv3' and `./TTCNv3/Makefile.cfg' is necessary for a
    # successful configuration.  Other Makefiles can be missing.

  def config_pdfs(self, config, build_dir):
    """ Optionally, copy .pdf files to the documentation directory or create
        fake .pdf files to make the installation successful.  If the build
        configuration doesn't have the appropriate key nothing is done with
        .pdf files.  If the directory of .pdf files doesn't exists the .pdf
        files will be faked.

        Arguments:
          The actual build configuration.
    """
    if 'pdfdir' in config:
      if not os.path.isdir(config['pdfdir']):
        self.logger.debug('Creating fake .pdf files in %s/TTCNv3/usrguide' % build_dir)
        for file in os.listdir('%s/TTCNv3/usrguide' % build_dir):
          if file.endswith('.doc'):
            utils.run_cmd('touch %s/TTCNv3/usrguide/%s.pdf' \
                          % (build_dir, file.split('.')[0]))
      else:
        self.logger.debug('Copying .pdf files from %s' % config['pdfdir'])
        utils.run_cmd('cp %s/*.pdf %s/TTCNv3/usrguide' % (config['pdfdir'], build_dir))
    else:
      self.logger.debug('The .pdf files are not in place, your ' \
                        'installation will fail if you haven\'t fixed the ' \
                        'Makefile...')

  def dump_addressees(self):
    for addressee in self.config.recipients:
      print('%s %s' % (addressee, self.config.recipients[addressee]))

  def dump_configs(self):
    configs = self.config.configs
    slaves = self.config.slaves
    for config_name, config_data in configs.iteritems():
      slave_list = []
      for slave_name, slave_data in slaves.iteritems():
        if config_name in slave_data['configs']:
          slave_list.append(slave_name)
      print('%s %s' % (config_name, ', '.join(slave_list)))

def main(argv = None):
  if argv is None:
    argv = sys.argv
  usage = 'Usage: %prog [options]'
  version = '%prog 0.0.5'
  parser = optparse.OptionParser(usage = usage, version = version)
  parser.add_option('-a', '--addressees', action = 'store_true', dest = 'addressees', help = 'dump all addressees')
  parser.add_option('-A', '--set-addressees', action = 'store', type = 'string', dest = 'set_addressees', help = 'set addressees from command line')
  parser.add_option('-c', '--config-list', action = 'store', type = 'string', dest = 'config_list', help = 'list of build configurations')
  parser.add_option('-d', '--dump', action = 'store_true', dest = 'dump', help = 'dump build configurations and the attached slaves', default = False)
  parser.add_option('-p', '--source-path', action = 'store', type = 'string', dest = 'source_path', help = 'instead of CVS use the given path')
  parser.add_option('-r', '--reset', action = 'store_true', dest = 'reset', help = 'reset statistics', default = False)
  parser.add_option('-s', '--slave-mode', action = 'store', type = 'string', dest = 'slave_mode', help = 'enable slave mode', default = None)
  parser.add_option('-t', '--tests', action = 'store', type = 'string', dest = 'tests', help = 'tests to run') 
  (options, args) = parser.parse_args()
  # The slaves are always executing a specific build configuration.
  if not options.config_list and options.slave_mode:
    parser.print_help()
  elif options.addressees:
    titan_builder().dump_addressees()
  elif options.dump:
    titan_builder().dump_configs()
  else:
    builder = titan_builder()
    builder.build(options.config_list, options.slave_mode, options.reset, options.set_addressees, options.tests, options.source_path)
  return 0

if __name__ == '__main__':
  ret_val = 1
  try:
    ret_val = main()
  except SystemExit, e:
    ret_val = e
  except:
    print('Exception caught, writing traceback info to log file `%s\'' \
          % TRACE_FILENAME)
    traceback.print_exc(file = open(TRACE_FILENAME, 'at'))
    sys.exit(1)  # Don't fall through.
  sys.exit(ret_val)
