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
#   Kovacs, Ferenc
#
##############################################################################
import os, re, types, time
import utils

class titan_publisher:
  def __init__(self, logger, config):
    self._logger = logger
    self._config = config
    
    self._plotter = plotter(self._logger, self._config)

    self._platform = None
    self._titan = None
    self._regtest = None
    self._perftest = None
    self._eclipse = None
    self._functest = None
    self._vobtest = None

  def __str__(self):
    return self.as_text()

  def titan_out(self, config, slave_name, titan_out):
    """ Write TITAN results to file.  """
    if not self._titan:
      self._titan = titan_out
      if not self._titan:
        return
      log_dir = os.path.join(config.get('logdir', ''), slave_name)
      (stamp_begin, stamp_end, \
        ((ret_val_dep, stdout_dep, stderr_dep), \
         (ret_val_make, stdout_make, stderr_make), \
         (ret_val_install, stdout_install, stderr_install))) = self._titan
      file_dep = open('%s/titan.dep' % log_dir, 'wt')
      file_make = open('%s/titan.make' % log_dir, 'wt')
      file_install = open('%s/titan.install' % log_dir, 'wt')
      file_dep.write(''.join(stdout_dep))
      file_make.write(''.join(stdout_make))
      file_install.write(''.join(stdout_install))
      file_dep.close()
      file_make.close()
      file_install.close()
    else:
      self._logger.error('More than one TITAN builds are not allowed in the ' \
                         'build cycle, ignoring the results')

  def regtest_out(self, config, slave_name, regtest_out):
    """ Write regression test results to file.  """
    if not self._regtest:
      self._regtest = regtest_out
      if not self._regtest:
        return
      log_dir = os.path.join(config.get('logdir', ''), slave_name)
      for rt, rt_data in self._regtest.iteritems():
        (stamp_begin, stamp_end, ((ret_val_make, stdout_make, stderr_make), \
          (ret_val_run, stdout_run, stderr_run))) = rt_data
        file_make = open('%s/regtest-make.%s' % (log_dir, rt), 'wt')
        file_run = open('%s/regtest-run.%s' % (log_dir, rt), 'wt')
        file_make.write(''.join(stdout_make))
        file_run.write(''.join(stdout_run))
        file_make.close()
        file_run.close()
    else:
      self._logger.error('The regression test results are already set')

  def perftest_out(self, config, slave_name, perftest_out):
    """ Write performance test results to file.  """
    if not self._perftest:
      self._perftest = perftest_out
      if not self._perftest:
        return
      log_dir = os.path.join(config.get('logdir', ''), slave_name)
      for rt, rt_data in self._perftest.iteritems():
        (stamp_begin, stamp_end, results) = rt_data
        (ret_val_make, stdout_make, stderr_make) = results.get('make', ([], [], []))
        file_make = open('%s/perftest.%s' % (log_dir, rt), 'wt')
        file_make.write(''.join(stdout_make))
        file_make.close()
        for run in results.get('run', []):
          (cps, (ret_val_run, stdout_run, stderr_run)) = run
          file_run = open('%s/perftest.%s-%d' % (log_dir, rt, cps), 'wt')
          file_run.write(''.join(stdout_run))
          file_run.close()
    else:
      self._logger.error('The performance test results are already set')

  def eclipse_out(self, config, slave_name, eclipse_out):
    if not self._eclipse:
      self._eclipse = eclipse_out
    else:
      self._logger.error('The Eclipse build results are already set')

  def functest_out(self, config, slave_name, functest_out):
    """ Store function test results for publishing.  """
    if not self._functest:
      self._functest = functest_out
    else:
      self._logger.error('The function test results are already set')

  def vobtest_out(self, config, slave_name, vobtest_out):
    """ Store VOB test results for publishing.  """
    if not self._vobtest:
      self._vobtest = vobtest_out
    else:
      self._logger.error('The VOB product results are already set')

  def dump_csv(self, stamp_old, stamp_new, config, config_name, slave_name):
    out_file = os.path.join(self._config.configs[config_name]['logdir'], \
                            os.path.join(slave_name, 'report.csv'))
    try:  
      out_csv = open(out_file, 'wt')
      out_csv.write(self.as_csv(stamp_old, stamp_new, config, config_name, slave_name))
      out_csv.close()
    except IOError, (errno, strerror):
      self._logger.error('Cannot open file `%s\': %d: %s' \
                         % (out_file, errno, strerror))

  def dump_txt(self, stamp_old, stamp_new, config, config_name, slave_name):
    out_file = os.path.join(self._config.configs[config_name]['logdir'], \
                            os.path.join(slave_name, 'report.txt'))
    try:
      out_txt = open(out_file, 'wt')
      out_txt.write(self.as_txt(stamp_old, stamp_new, config, config_name, slave_name))
      out_txt.close()
    except IOError, (errno, strerror):
      self._logger.error('Cannot open file `%s\': %d: %s' \
                         % (out_file, errno, strerror))

  def dump_html(self, stamp_old, stamp_new, config, config_name, slave_name):
    out_file = os.path.join(self._config.configs[config_name]['logdir'], \
                            os.path.join(slave_name, 'report.html'))
    try:
      out_html = open(out_file, 'wt')
      out_html.write(self.as_html(stamp_old, stamp_new, config, config_name, slave_name))
      out_html.close()
    except IOError, (errno, strerror):
      self._logger.error('Cannot open file `%s\': %d: %s' \
                         % (out_file, errno, strerror))

  def as_csv(self, stamp_begin, stamp_end, config, config_name, slave_name):
    """ Return a very brief summary of the build.  The used runtimes are not
        distinguished.  Neither the compile time errors and runtime errors.
        Take care of the (header-)order when adding new columns.

        Arguments:
          stamp_begin: Start of the whole build.
          stamp_end: End of the whole build.
          config: The actual build configuration.
          config_name: The name of the actual build configuration.
          slave_name: The name of the actual slave.  It's defined in the
                      configuration file.

        Returns:
          The slave specific results in a brief CSV format suitable for
          notification e-mails.  The master can easily generate a fancy table
          from this CSV data.
    """
    # `gcc' writes to the standard error.
    results = []
    uname_out = utils.run_cmd('uname -srmp')[1]
    gcc_out = filter(lambda v: v.find(' ver') > 0, utils.run_cmd('%s -v' % (('cc' in config and len(config['cc']) > 0) and config['cc'] or 'gcc'))[2])
    results.append('%s,%s,%s,%s,%s,%s' \
                   % (stamp_begin, stamp_end, \
                   uname_out[0].strip(), gcc_out[0].strip(), \
                   config_name, slave_name))
    if self._titan:
      (stamp_begin, stamp_end, \
       ((ret_val_dep, stdout_dep, stderr_dep), \
        (ret_val_make, stdout_make, stderr_make), \
        (ret_val_install, stdout_install, stderr_install))) = self._titan
      if ret_val_dep or ret_val_make or ret_val_install:
        results.append(',1,1,1,1,1,1')
        return ''.join(results)
      results.append(',0')
    else:
      self._logger.error('The output of TITAN build was not set')
      results.append(',-1,-1,-1,-1,-1,-1')
      return ''.join(results)
    if self._regtest:
      all_fine = True
      for rt, rt_data in self._regtest.iteritems():
        (stamp_begin, stamp_end, ((ret_val_make, stdout_make, stderr_make), \
          (ret_val_run, stdout_run, stderr_run))) = rt_data
        if ret_val_make or ret_val_run:
          all_fine = False
          break
      results.append(all_fine and ',0' or ',1')
    else:
      results.append(',-1')
    if self._perftest:
      all_fine = True
      for rt, rt_data in self._perftest.iteritems():
        (stamp_begin, stamp_end, compile_run_data) = rt_data
        (ret_val_make, stdout_make, stderr_make) = compile_run_data['make']
        if ret_val_make:
          all_fine = False
          break
        for run_data in compile_run_data['run']:
          (cps, (ret_val_run, stdout_run, stderr_run)) = run_data
          if ret_val_run:
            all_fine = False
            break
      results.append(all_fine and ',0' or ',1')
    else:
      results.append(',-1')
    if self._functest:
      all_fine = True
      for rt, rt_data in self._functest.iteritems():
        (stamp_begin, stamp_end, functest_data) = rt_data
        for test, test_results in functest_data.iteritems():
          (log_file_name, error_file_name) = test_results
          satester_report = test == 'Config_Parser' or test == 'Semantic_Analyser'
          if satester_report:
            log_file = open(log_file_name, 'rt')
            log_file_data = log_file.readlines()
            log_file.close()
            log_file_data.reverse()
            total_matched = passed = None
            for line in log_file_data:
              if not total_matched:
                total_matched = re.match('^Total number of.*: (\d+)$', line)
              if not passed:
                passed = re.match('\s*PASSED.*cases: (\d+)', line)
              if total_matched and passed:
                if int(total_matched.group(1)) != int(passed.group(1)):
                  all_fine = False
                  break
            if not total_matched or not passed:
              self._logger.error('There\'s something wrong with the ' \
                                 'function test logs, it\'s treated as an ' \
                                 'error')
              all_fine = False
              break
          else:
            if error_file_name and os.path.isfile(error_file_name):
              error_file = open(error_file_name, 'rt')
              error_file_data = error_file.readlines()
              error_file.close()
              if len(error_file_data) != 0:
                all_fine = False
                break
      results.append(all_fine and ',0' or ',1')
    else:
      results.append(',-1')
    if self._vobtest:
      # Unfortunately there's no `goto' in Python.  However, returning from
      # multiple loops can be done using exceptions...
      all_fine = True
      for rt, rt_data in self._vobtest.iteritems():
        (stamp_begin, stamp_end, vobtest_data) = rt_data
        for kind, products in vobtest_data.iteritems():
          if not len(products) > 0:
            continue
          for product in products:
            for name, name_data in product.iteritems():
              if not isinstance(name_data, types.DictType):
                all_fine = False
                break
              else:
                for action, action_data in name_data.iteritems():
                  if isinstance(action_data, types.TupleType):
                    (ret_val, output_files, stdout, stderr) = action_data
                    if ret_val:
                      all_fine = False
                      break
      results.append(all_fine and ',0' or ',1')
    else:
      results.append(',-1')
    if self._eclipse:
      (stamp_begin, stamp_end, log_file, (ret_val_ant, stdout_ant, stderr_ant)) = self._eclipse
      results.append(ret_val_ant and ',1' or ',0')
    else:
      results.append(',-1')
    return ''.join(results)

  def as_txt_regtest(self):
    result = []
    for rt, rt_data in self._regtest.iteritems():
      (stamp_begin, stamp_end, ((ret_val_make, stdout_make, stderr_make), \
        (ret_val_run, stdout_run, stderr_run))) = rt_data
      result.append('%s [%s - %s] Regression test results for the `%s\' ' \
                    'runtime\n\n' % (utils.get_time_diff(False, stamp_begin, stamp_end), \
                    stamp_begin, stamp_end, rt == 'rt2' and 'function-test' or 'load-test'))
      if ret_val_make:
        result.append('Regression test failed to build:\n\n%s\n' \
                      % ''.join(stdout_make[-20:]))
      elif ret_val_run:
        result.append('Regression test failed to run:\n\n%s\n' \
                      % ''.join(stdout_run[-20:]))
      else:
        result.append('Regression test built successfully.\n\n%s\n' \
                      % ''.join(stdout_run[-20:]))
    return ''.join(result)

  def as_txt_perftest(self):
    result = []
    for rt, rt_data in self._perftest.iteritems():
      (stamp_begin, stamp_end, perftest_results) = rt_data
      result.append('%s [%s - %s] Performance test results for the `%s\' ' \
                    'runtime\n\n' % (utils.get_time_diff(False, stamp_begin, stamp_end), \
                       stamp_begin, stamp_end, rt == 'rt2' and 'function-test' or 'load-test'))
      (ret_val_dep, stdout_dep, stderr_dep) = perftest_results['dep']
      (ret_val_make, stdout_make, stderr_make) = perftest_results['make']
      run_data = perftest_results['run']
      if ret_val_dep or ret_val_make:
        result.append('Performance test failed to build:\n\n%s\n' \
                      % ''.join(ret_val_dep and stdout_dep[-20:] or stdout_make[-20:]))
      else:
        result.append('Performance test compiled successfully.\n\n')
      for run in run_data:
        (cps, (ret_val_run, stdout_run, stderr_run)) = run
        result.append('For `%d\' CPS: ' % cps)
        if ret_val_run:
          result.append('Failed\n%s\n\n' % ''.join(stdout_run[-20:]))
        else:
          result.append('Succeeded\nExpected Calls/Measured Calls/' \
                        'Expected CPS/Measured CPS: %s\n' \
                        % ' '.join(''.join(filter(lambda run_info: \
                                   'Entities/Time' in run_info, stdout_run)).split()[-5:-1]))
    return ''.join(result)

  def as_txt_eclipse(self):
    result = []
    (stamp_begin, stamp_end, log_file, (ret_val_ant, stdout_ant, stderr_ant)) = self._eclipse
    result.append('%s [%s - %s] Eclipse build results\n\n'
                  % (utils.get_time_diff(False, stamp_begin, stamp_end), stamp_begin, stamp_end))
    f = open(log_file, 'rt')
    log_file_data = f.readlines()
    f.close()
    if ret_val_ant:
      result.append('Eclipse plug-ins failed to build:\n%s\n\n' \
                    % ''.join(log_file_data[-20:]))
    else:
      result.append('Eclipse plug-ins built successfully.\n\n%s\n' \
                    % ''.join(log_file_data[-20:]))
    return ''.join(result)

  def as_txt_functest(self):
    result = []
    for rt, rt_data in self._functest.iteritems():
      (stamp_begin, stamp_end, functest_results) = rt_data
      result.append('%s [%s - %s] Function test results for the `%s\' runtime\n\n' \
                    % (utils.get_time_diff(False, stamp_begin, stamp_end), \
                       stamp_begin, stamp_end, (rt == 'rt2' and 'function-test' or 'load-test')))
      for function_test, test_results in functest_results.iteritems():
        (log_file_name, error_file_name) = test_results
        satester_report = function_test == 'Config_Parser' or function_test == 'Semantic_Analyser'
        if satester_report:
          log_file = open(log_file_name, 'rt')
          log_file_data = log_file.readlines()
          log_file.close()
          total_matched = passed = None
          for line in log_file_data:
            if not total_matched:
              total_matched = re.match('^Total number of.*: (\d+)$', line)
            if not passed:
              passed = re.match('\s*PASSED.*cases: (\d+)', line)
            if passed and total_matched:
              if int(passed.group(1)) == int(total_matched.group(1)):
                result.append('All `%s\' function tests succeeded.\n' \
                              % function_test)
              else:
                result.append('\n`%s\' function tests failed:\n\n%s\n' \
                              % (function_test, \
                                 ''.join(log_file_data[-20:])))
              break
        else:
          if error_file_name and os.path.isfile(error_file_name):
            error_file = open(error_file_name, 'rt')
            error_file_data = error_file.readlines()
            error_file.close()
            if len(error_file_data) == 0:
              result.append('All `%s\' function tests succeeded.\n' \
                            % function_test)
            else:
              result.append('\n`%s\' function tests failed:\n\n%s\n' \
                            % (function_test, \
                               ''.join(error_file_data[-20:])))
          else:
            result.append('All `%s\' function tests succeeded.\n' \
                          % function_test)
      result.append('\n')
    return ''.join(result)

  def as_txt_vobtest(self):
    result = []
    header = ('Product/Action', '`compiler -s\'', '`compiler\'', '`make\'', '`make run\'\n')
    for rt, rt_data in self._vobtest.iteritems():
      (stamp_begin, stamp_end, vobtest_results) = rt_data
      result.append('%s [%s - %s] VOB product results for the %s runtime\n\n' \
                    % (utils.get_time_diff(False, stamp_begin, stamp_end), \
                       stamp_begin, stamp_end, (rt == 'rt2' and 'function-test' or 'load-test')))
      for kind, products in vobtest_results.iteritems():
        if not len(products) > 0:
          continue
        title = 'Results for %d `%s\' products using the %s runtime:' \
                % (len(products), kind, (rt == 'rt2' and 'function-test' \
                                         or 'load-test'))
        result.append('%s\n%s\n' % (title, '-' * len(title)))
        body = []
        for product in products:
          for name, name_data in product.iteritems():
            row = [name]
            if not isinstance(name_data, types.DictType):
              row.extend(['Unavailable'] * (len(header) - 1))
              body.append(row)
            else:
              action_order = {'semantic':1, 'translate':2, 'compile':3, 'run':4}
              row.extend([''] * len(action_order.keys()))
              for action, action_data in name_data.iteritems():
                if not action in action_order.keys():
                  self._logger.error('Unknown action `%s\'while preparing ' \
                                     'the text output' % action)
                  continue
                action_index = action_order[action]
                if not isinstance(action_data, types.TupleType):
                  row[action_index] = 'Disabled'
                else:
                  (ret_val, output_files, stdout, stderr) = action_data
                  row[action_index] = '%s' % (ret_val != 0 and '*Failure*' or 'Success')
          body.append(row)
        result.append(self.as_txt_table(header, body) + '\n')
    return ''.join(result)

  def as_txt(self, stamp_begin, stamp_end, config, config_name, slave_name):
    """ Return the string representation of the test results.
    """
    results = []
    uname_out = utils.run_cmd('uname -srmp')[1]
    gcc_out = filter(lambda v: v.find(' ver') > 0, utils.run_cmd('%s -v' % (('cc' in config and len(config['cc']) > 0) and config['cc'] or 'gcc'))[2])
    results.append('Platform: %s\nGCC/LLVM version: %s\n\n' \
                   % (uname_out[0].strip(), gcc_out[0].strip()))
    if self._titan:
      (stamp_begin, stamp_end, \
       ((ret_val_dep, stdout_dep, stderr_dep), \
        (ret_val_make, stdout_make, stderr_make), \
        (ret_val_install, stdout_install, stderr_install))) = self._titan
      results.append('%s [%s - %s] TITAN build\n\n' \
                     % (utils.get_time_diff(False, stamp_begin, stamp_end), \
                        stamp_begin, stamp_end))
      if ret_val_dep or ret_val_make or ret_val_install:
        # The `stderr' is always redirected to `stdout'.
        results.append('TITAN build failed, check the logs for further ' \
                       'investigation...\n\n%s\n' \
                       % ''.join(stdout_install[-20:]))
      else:
        results.append('TITAN build succeeded.\n\n%s\n' \
                       % utils.get_license_info('%s/bin/compiler' \
                                                % self._config.configs[config_name]['installdir']))
    if self._regtest:
      results.append(self.as_txt_regtest())
    if self._perftest:
      results.append(self.as_txt_perftest())
    if self._eclipse:
      results.append(self.as_txt_eclipse())
    if self._functest:
      results.append(self.as_txt_functest())
    if self._vobtest:
      results.append(self.as_txt_vobtest())
    return ''.join(results)

  def as_txt_table(self, header = None, body = []):
    """ Create a table like ASCII composition using the given header and the
        rows of the table.  The header is an optional string list.  If the
        header is present and there are more columns in the body the smaller
        wins.

        Arguments:
          header: The columns of the table.
          body: Cell contents.

        Returns:
          The table as a string.
    """
    if len(body) == 0 or len(body) != len([row for row in body \
                                           if isinstance(row, types.ListType)]):
      self._logger.error('The second argument of `as_text_table()\' must be ' \
                         'a list of lists')
      return ''
    num_cols = len(body[0])
    max_widths = []
    if header and len(header) < num_cols:
      num_cols = len(header)
    for col in range(num_cols):
      max_width = -1
      for row in range(len(body)):
        if max_width < len(body[row][col]):
          max_width = len(body[row][col])
      if header and max_width < len(header[col]):
        max_width = len(header[col])
      max_widths.append(max_width + 2)  # Ad-hoc add.
    ret_val = ''  # Start filling the table.
    if header:
      ret_val += ''.join([cell.ljust(max_widths[i]) \
                          for i, cell in enumerate(header[:num_cols])]) + '\n'
    for row in range(len(body)):
      ret_val += ''.join([cell.ljust(max_widths[i]) \
                          for i, cell in enumerate(body[row][:num_cols])]) + '\n'
    return ret_val

  def as_html_titan(self, config_name, slave_name):
    """ Return the HTML representation of the TITAN build results as a string.
    """
    result = []
    (stamp_begin, stamp_end, \
     ((ret_val_dep, stdout_dep, stderr_dep), \
      (ret_val_make, stdout_make, stderr_make), \
      (ret_val_install, stdout_install, stderr_install))) = self._titan
    result.append('<span class="%s">TITAN build</span><br/><br/>\n' \
                        % ((ret_val_dep or ret_val_make or ret_val_install) \
                           and 'error_header' or 'header'))
    result.append('( `<a href="titan.dep">make dep</a>\' )<br/><br/>\n')
    result.append('( `<a href="titan.make">make</a>\' )<br/><br/>\n')
    result.append('( `<a href="titan.install">make install</a>\' )' \
                  '<br/><br/>\n')
    result.append('<span class="stamp">%s - %s [%s]</span>\n' \
                  % (stamp_begin, stamp_end, \
                     utils.get_time_diff(False, stamp_begin, stamp_end)))
    result.append('<pre>\n')
    if ret_val_dep or ret_val_make or ret_val_install:
      result.append('The TITAN build failed, check the logs for further ' \
                    'investigation...\n\n%s\n' % self.strip_tags(''.join(stdout_install[-20:])))
    else:
      result.append('TITAN build succeeded.\n\n%s\n' \
                    % self.strip_tags(utils.get_license_info('%s/bin/compiler' \
                                      % self._config.configs[config_name]['installdir'])))
    result.append('</pre>\n')
    return ''.join(result)

  def as_html_regtest(self, config_name, slave_name):
    """ Return the HTML representation of the regression test results as a
        string.  The last part of the output is always included.
    """
    result = []
    for rt, rt_data in self._regtest.iteritems():
      (stamp_begin, stamp_end, ((ret_val_make, stdout_make, stderr_make), \
        (ret_val_run, stdout_run, stderr_run))) = rt_data
      result.append('<span class="%s">Regression test results for the `%s\' ' \
                    'runtime</span><br/><br/>\n' \
                    % (((ret_val_make or ret_val_run) and 'error_header' or 'header'), \
                       (rt == 'rt2' and 'function-test' or 'load-test')))
      result.append('( `<a href="regtest-make.%s">make</a>\' )<br/><br/>\n' % rt)
      result.append('( `<a href="regtest-run.%s">make run</a>\' )<br/><br/>\n' % rt)
      result.append('<span class="stamp">%s - %s [%s]</span>\n<pre>\n' \
                    % (stamp_begin, stamp_end, \
                       utils.get_time_diff(False, stamp_begin, stamp_end)))
      if ret_val_make:
        result.append('Regression test failed to build:\n\n%s\n</pre>\n' \
                      % self.strip_tags(''.join(stdout_make[-20:])))
      elif ret_val_run:
        result.append('Regression test failed to run:\n\n%s\n</pre>\n' \
                      % self.strip_tags(''.join(stdout_run[-20:])))
      else:
        result.append('Regression test built successfully.\n\n%s\n</pre>\n' \
                      % self.strip_tags(''.join(stdout_run[-20:])))
    return ''.join(result)

  def as_html_perftest(self, config_name, slave_name):
    """ Return the HTML representation of the performance test results as a
        string.  Some logic is included.
    """
    result = []
    for rt, rt_data in self._perftest.iteritems():
      (stamp_begin, stamp_end, perftest_results) = rt_data
      (ret_val_dep, stdout_dep, stderr_dep) = perftest_results['dep']
      (ret_val_make, stdout_make, stderr_make) = perftest_results['make']
      run_data = perftest_results['run']
      run_failed = False
      for run in run_data:
        (cps, (ret_val_run, stdout_run, stderr_run)) = run
        if ret_val_run:
          run_failed = True
          break
      result.append(
        '<span class="%s">Performance test results for the `%s\' ' \
        'runtime</span><br/><br/>\n' \
        % (((ret_val_dep or ret_val_make or run_failed) \
            and 'error_header' or 'header'), \
           (rt == 'rt2' and 'function-test' or 'load-test')))
      result.append('( `<a href="perftest.%s">make</a>\' )<br/><br/>\n' % rt)
      result.append('( `<a href=".">make run</a>\' )<br/><br/>')
      result.append('<span class="stamp">%s - %s [%s]</span>\n' \
                    % (stamp_begin, stamp_end, \
                       utils.get_time_diff(False, stamp_begin, stamp_end)))
      result.append('<pre>\n')
      if ret_val_dep or ret_val_make:
        result.append('Performance test failed to build:\n\n%s\n' \
                      % self.strip_tags(''.join(ret_val_dep and stdout_dep[-20:] or stdout_make[-20:])))
      else:
        result.append('Performance test compiled successfully.\n\n')
        result.append('<embed src="perftest-stats-%s.svg" width="640" height="480" type="image/svg+xml"/>\n\n' % rt)
        for run in run_data:
          (cps, (ret_val_run, stdout_run, stderr_run)) = run
          if ret_val_run:
            result.append('Failed for `%d\' CPS.\n\n%s\n\n' \
                          % (cps, self.strip_tags(''.join(stdout_run[-20:]))))
          else:
            result.append('Expected Calls/Measured Calls/' \
                          'Expected CPS/Measured CPS: %s\n' \
                          % ' '.join(''.join(filter(lambda run_info: \
                                     'Entities/Time' in run_info, stdout_run)).split()[-5:-1]))
      result.append('\n</pre>\n')
    return ''.join(result)

  def as_html_eclipse(self, config_name, slave_name):
    result = []
    (stamp_begin, stamp_end, log_file, (ret_val_ant, stdout_ant, stderr_ant)) = self._eclipse
    result.append('<span class="%s">Eclipse plug-in build results</span><br/><br/>\n' \
                  % ((ret_val_ant and 'error_header' or 'header')))
    result.append('( `<a href="eclipse-mylog.log">ant</a>\' )<br/><br/>\n')
    result.append('<span class="stamp">%s - %s [%s]</span>\n<pre>\n' \
                  % (stamp_begin, stamp_end, \
                     utils.get_time_diff(False, stamp_begin, stamp_end)))
    f = open(log_file, 'rt')
    log_file_data = f.readlines()
    f.close()
    if ret_val_ant:
      result.append('Eclipse plug-ins failed to build:\n\n%s\n</pre>\n' \
                    % self.strip_tags(''.join(log_file_data[-20:])))
    else:
      result.append('Eclipse plug-ins built successfully.\n\n%s\n</pre>\n' \
                    % self.strip_tags(''.join(log_file_data[-20:])))
    return ''.join(result)

  def as_html_functest(self, config_name, slave_name):
    """ Return the HTML representation of the function test results as a
        string.  Some logic is included.
    """
    result = []
    for rt, rt_data in self._functest.iteritems():
      (stamp_begin, stamp_end, functest_results) = rt_data
      any_failure = False
      result_tmp = []
      for function_test, test_results in functest_results.iteritems():
        (log_file_name, error_file_name) = test_results
        satester_report = function_test == 'Config_Parser' or function_test == 'Semantic_Analyser'
        if satester_report:
          log_file = open(log_file_name, 'rt')
          log_file_data = log_file.readlines()
          log_file.close()
          total_matched = passed = None
          for line in log_file_data:
            if not total_matched:
              total_matched = re.match('^Total number of.*: (\d+)$', line)
            if not passed:
              passed = re.match('\s*PASSED.*cases: (\d+)', line)
            if passed and total_matched:
              if int(passed.group(1)) == int(total_matched.group(1)):
                result_tmp.append('All `%s\' function tests succeeded.\n' \
                                  % function_test)
              else:
                result_tmp.append('\n`%s\' function tests failed:\n\n%s\n' \
                                  % (function_test, \
                                     self.strip_tags(''.join(log_file_data[-20:]))))
                any_failure = True
              break
        else:
          if error_file_name and os.path.isfile(error_file_name):
            error_file = open(error_file_name, 'rt')
            error_file_data = error_file.readlines()
            error_file.close()
            if len(error_file_data) == 0:
              result_tmp.append('All `%s\' function tests succeeded.\n' \
                                % function_test)
            else:
              result_tmp.append('\n`%s\' function tests failed:\n\n%s\n' \
                                % (function_test, \
                                   self.strip_tags(''.join(error_file_data[-20:]))))
              any_failure = True
          else:
            result_tmp.append('All `%s\' function tests succeeded.\n' \
                              % function_test)
      result.append('<span class="%s">Function test results for the ' \
                    '`%s\' runtime</span><br/><br/>\n' \
                    % ((any_failure and 'error_header' or 'header'), \
                       (rt == 'rt2' and 'function-test' or 'load-test')))
      result.append('( `<a href=".">make all</a>\')<br/><br/>\n')
      result.append('<span class="stamp">%s - %s [%s]</span>\n' \
                    % (stamp_begin, stamp_end, \
                       utils.get_time_diff(False, stamp_begin, stamp_end)))
      result.append('<pre>\n')
      result.extend(result_tmp)
      result.append('\n</pre>\n')
    return ''.join(result)

  def as_html_vobtest(self, config_name, slave_name):
    """ Return the HTML representation of the VOB product tests as a string.
        Some logic is included.
    """
    result = []
    header = ('Product/Action', '`compiler -s\'', '`compiler\'', '`make\'', '`make run\'\n')
    for rt, rt_data in self._vobtest.iteritems():
      (stamp_begin, stamp_end, vobtest_results) = rt_data
      any_failure = False
      result_tmp = []
      for kind, products in vobtest_results.iteritems():
        if not len(products) > 0:
          continue
        body = []
        for product in products:
          for name, name_data in product.iteritems():
            row = [name]
            if not isinstance(name_data, types.DictType):
              row.extend(['Unavailable'] * (len(header) - 1))
              body.append(row)
              any_failure = True
            else:
              action_order = {'semantic':1, 'translate':2, 'compile':3, 'run':4}
              row.extend([''] * len(action_order.keys()))
              for action, action_data in name_data.iteritems():
                if not action in action_order.keys():
                  self._logger.error('Unknown action `%s\'while preparing ' \
                                     'the HTML output' % action)
                  continue
                action_index = action_order[action]
                if not isinstance(action_data, types.TupleType):
                  row[action_index] = 'Disabled'
                else:
                  (ret_val, output_files, stdout, stderr) = action_data
                  row[action_index] = (ret_val and '*Failure*' or 'Success')
                  if ret_val:
                    any_failure = True
          body.append(row)
        title = 'Results for %d `%s\' products using the %s runtime:' \
                % (len(products), kind, (rt == 'rt2' and 'function-test' \
                                         or 'load-test'))
        result_tmp.append('%s\n%s\n' % (title, '-' * len(title)))
        result_tmp.append(self.as_txt_table(header, body) + '\n')
      result.append('<span class="%s">VOB product results for the %s ' \
                    'runtime</span><br/><br/>\n' \
                    % ((any_failure and 'error_header' or 'header'), \
                       (rt == 'rt2' and 'function-test' or 'load-test')))
      result.append('( `<a href="products/">make all</a>\' )<br/><br/>\n')
      result.append('<span class="stamp">%s - %s [%s]</span>\n' \
                    % (stamp_begin, stamp_end, \
                       utils.get_time_diff(False, stamp_begin, stamp_end)))
      result.append('<pre>\n')
      result.extend(result_tmp)
      result.append('</pre>\n')
    return ''.join(result)

  def as_html(self, stamp_old, stamp_new, config, config_name, slave_name):
    """ Return the HTML representation of all test results of the given slave
        as a string.
    """
    result = [
      '<?xml version="1.0" encoding="ISO8859-1"?>\n' \
      '<html>\n' \
      '<head>\n' \
      '<meta http-equiv="content-type" content="text/html; charset=ISO8859-1"/>\n' \
      '<link rel="stylesheet" type="text/css" href="../../index.css"/>\n' \
      '<title>Shouldn\'t matter...</title>\n' \
      '</head>\n' \
      '<body>\n'
    ]
    uname_out = utils.run_cmd('uname -srmp')[1]
    gcc_out = filter(lambda v: v.find(' ver') > 0, utils.run_cmd('%s -v' % (('cc' in config and len(config['cc']) > 0) and config['cc'] or 'gcc'))[2])
    result.append('<pre>\nPlatform: %s\nGCC/LLVM version: %s</pre>\n\n' \
                  % (uname_out[0].strip(), gcc_out[0].strip()))
    if self._titan:
      result.append(self.as_html_titan(config_name, slave_name))
    if self._regtest:
      result.append(self.as_html_regtest(config_name, slave_name))
    if self._perftest:
      result.append(self.as_html_perftest(config_name, slave_name))
    if self._eclipse:
      result.append(self.as_html_eclipse(config_name, slave_name))
    if self._functest:
      result.append(self.as_html_functest(config_name, slave_name))
    if self._vobtest:
      result.append(self.as_html_vobtest(config_name, slave_name))
    result += [
      '</body>\n' \
      '</html>\n'
    ]
    return ''.join(result)

  def publish_csv2email(self, build_start, build_end, email_file, \
                        slave_list, build_root, configs, reset):
    """ Assemble a compact e-mail message from the CSV data provided by each
        slave in the current build.  The assembled e-mail message is written
        to a file.  It's ready to send.  It's called by the master.

        Arguments:
          build_start: Start of the whole build for all slaves.
          build_end: End of the whole build for all slaves.
          email_file: Store the e-mail message here.
          slave_list: Slaves processed.
          build_root: The actual build directory.
          configs: All configurations.
          reset: Reset statistics.
    """
    email_header = 'Full build time:\n----------------\n\n%s <-> %s\n\n' \
                   % (build_start, build_end)
    email_footer = 'For more detailed results, please visit:\n' \
                   'http://ttcn.ericsson.se/titan-testresults/titan_builds or\n' \
                   'http://ttcn.ericsson.se/titan-testresults/titan_builds/%s.\n\n' \
                   'You\'re receiving this e-mail, because you\'re ' \
                   'subscribed to daily TITAN build\nresults.  If you want ' \
                   'to unsubscribe, please reply to this e-mail.  If you\n' \
                   'received this e-mail by accident please report that ' \
                   'too.  Thank you.\n' % build_root
    email_matrix = 'The result matrix:\n------------------\n\n'
    header = ('Slave/Action', 'TITAN build', 'Reg. tests', 'Perf. tests', \
              'Func. tests', 'VOB tests', 'Eclipse build')  # It's long without abbrevs.
    rows = []
    slave_names = []
    stat_handler = None
    for slave in slave_list:
      (slave_name, config_name, is_localhost) = slave
      slave_names.append(config_name)
      csv_file_name = '%s/%s/report.csv' \
                      % (self._config.common['logdir'], config_name)
      if 'measure' in configs[config_name] and configs[config_name]['measure']:
        stat_handler = StatHandler(self._logger, self._config.common, configs, slave_list, reset) 
      if not os.path.isfile(csv_file_name):
        self._logger.error('It seems that we\'ve lost `%s\' for configuration `%s\'' % (slave_name, config_name))
        local_row = [slave_name]
        local_row.extend(['Lost'] * (len(header) - 1))
        rows.append(local_row)
        if stat_handler:
          stat_handler.lost(config_name)
        continue
      csv_file = open(csv_file_name, 'rt')
      csv_data = csv_file.readlines()
      csv_file.close()
      if len(csv_data) != 1:
        self._logger.error('Error while processing `%s/%s/report.csv\' at ' \
                           'the end, skipping slave' \
                           % (self._config.common['logdir'], config_name))
      else:
        csv_data = csv_data[0].split(',')
        local_row = [csv_data[4]]  # Should be `config_name'.
        if stat_handler:
          stat_handler.disabled_success_failure(config_name, csv_data[6:])
        for result in csv_data[6:]:
          if int(result) == -1:
            local_row.append('Disabled')
          elif int(result) == 0:
            local_row.append('Success')
          elif int(result) == 1:
            local_row.append('*Failure*')
        rows.append(local_row)
    email_matrix += '%s\n' % self.as_txt_table(header, rows)
    file = open(email_file, 'wt')
    file.write(email_header)
    if stat_handler:
      file.write(str(stat_handler))
    file.write(email_matrix)
    file.write(email_footer)
    file.close()

  def backup_logs(self):
    """ Handle archiving and backup activities.

        Returns:
          A dictionary with None values.
    """
    archived_builds = {}
    for file in os.listdir(self._config.common['htmldir']):
      if os.path.isdir('%s/%s' % (self._config.common['htmldir'], file)):
        matched_dir = re.search('(\d{8}_\d{6})', file)
        if not matched_dir:
          continue
        diff_in_days = utils.diff_in_days(matched_dir.group(1), utils.get_time(True))
        if diff_in_days > self._config.common['archive']:
          self._logger.debug('Archiving logs for build `%s\'' % matched_dir.group(1))
          utils.run_cmd('cd %s && tar cf %s.tar %s' \
                        % (self._config.common['htmldir'], \
                           matched_dir.group(1), matched_dir.group(1)), None, 1800)
          utils.run_cmd('bzip2 %s/%s.tar && rm -rf %s/%s' \
                        % (self._config.common['htmldir'], matched_dir.group(1), \
                           self._config.common['htmldir'], matched_dir.group(1)), None, 1800)
          archived_builds[matched_dir.group(1)] = None
      else:
        matched_archive = re.search('(\d{8}_\d{6}).tar.bz2', file)
        if not matched_archive:
          continue
        diff_in_days = utils.diff_in_days(matched_archive.group(1), utils.get_time(True))
        if 'cleanup' in self._config.common and 'cleanupslave' in self._config.common and \
           diff_in_days > self._config.common['cleanup']:
          slave_name = self._config.common['cleanupslave']['slave']
          if slave_name in self._config.slaves:
            slave = self._config.slaves[slave_name]
            slave_url = '%s@%s' % (slave['user'], slave['ip'])
            utils.run_cmd('ssh %s \'mkdir -p %s\'' \
                          % (slave_url, self._config.common['cleanupslave']['dir']))
            (ret_val_scp, stdout_scp, stderr_scp) = \
              utils.run_cmd('scp %s/%s %s:%s' \
                            % (self._config.common['htmldir'], file, slave_url, \
                               self._config.common['cleanupslave']['dir']))
            if not ret_val_scp:
              utils.run_cmd('rm -f %s/%s' % (self._config.common['htmldir'], file))
              continue
          else:
            self._logger.error('Slave with name `%s\' cannot be found in ' \
                               'the slaves\' list' % slave_name)
        archived_builds[matched_archive.group(1)] = None
    return archived_builds

  def strip_tags(self, text):
    """ Replace all '<', '>' etc. characters with their HTML equivalents.  """
    return text.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')

  def publish_html(self, build_root):
    """ Create basic HTML output from the published directory structure.  It
        should be regenerated after every build.  The .css file is generated
        from here as well.  No external files used.  It is responsible for
        publishing in general.

        Arguments:
          build_root: The actual build directory.
    """
    self.generate_css()
    html_index = os.path.join(self._config.common['htmldir'], 'index.html')
    html_menu = os.path.join(self._config.common['htmldir'], 'menu.html')
    index_file = open(html_index, 'wt')
    index_file.write(
      '<?xml version="1.0" encoding="ISO8859-1"?>\n' \
      '<html>\n' \
      '<head>\n' \
      '<meta http-equiv="content-type" content="text/html; charset=ISO8859-1"/>\n' \
      '<link rel="stylesheet" type="text/css" href="index.css"/>\n' \
      '<title>Build results (Updated: %s)</title>\n' \
      '</head>\n' \
      '<frameset cols="285,*">\n' \
      '<frame src="menu.html" name="menu"/>\n' \
      '<frame src="%s/report.txt" name="contents"/>\n' \
      '</frameset>\n' \
      '</html>\n' % (build_root, build_root))
    index_file.close()
    menu_file = open(html_menu, 'wt')
    menu_contents_dict = self.backup_logs()
    for root, dirs, files in os.walk(self._config.common['htmldir']):
      build_match = re.match('(\d{8}_\d{6})', root.split('/')[-1])
      if build_match:
        dirs.sort()
        dirs_list = ['<li><a href="%s/%s/report.html" target="contents">%s' \
                     '</a></li>\n' % (build_match.group(1), elem, elem) for elem in dirs]
        menu_contents_dict[build_match.group(1)] = dirs_list
    sorted_keys = menu_contents_dict.keys()
    sorted_keys.sort(reverse = True)
    menu_contents = ''
    bg_toggler = False
    for build in sorted_keys:
      build_data = menu_contents_dict[build]
      if build_data:
        menu_contents += \
          '<tr>\n' \
          '<td bgcolor="%s">\nBuild #: <b>' \
          '<a href="%s/report.txt" target="contents">%s</a></b>\n' \
          '<ul>\n%s</ul>\n' \
          '</td>\n' \
          '</tr>\n' % ((bg_toggler and '#a9c9e1' or '#ffffff'), build, \
                       build, ''.join(build_data))
        bg_toggler = not bg_toggler
      else:
        menu_contents += \
          '<tr>\n' \
          '<td bgcolor="#c1c1ba">\nBuild #: <b>' \
          '<a href="%s.tar.bz2" target="contents">%s</a> (A)</b>\n' \
          '</td>\n' \
          '</tr>\n' % (build, build)
    menu_file.write(
      '<?xml version="1.0" encoding="ISO8859-1"?>\n' \
      '<html>\n' \
      '<head>\n' \
      '<meta http-equiv="content-type" content="text/html; charset=ISO8859-1"/>\n' \
      '<link rel="stylesheet" type="text/css" href="index.css"/>' \
      '<title>Shouldn\'t matter...</title>\n' \
      '</head>\n' \
      '<body>\n<pre>\n' \
      '      _\n'
      ' ____( )___________\n'
      '/_  _/ /_  _/  \   \\\n'
      ' /_//_/ /_//_/\_\_\_\\\n'
      '</pre>\n'
      '<table class="Menu">\n' \
      '%s\n' \
      '</table>\n' \
      '</body>\n' \
      '</html>\n' % menu_contents)
    menu_file.close()
    self._plotter.collect_data()
    self._plotter.plot(build_root)

  def generate_css(self):
    css_file = file('%s/index.css' % self._config.common['htmldir'], 'wt')
    css_file.write(
      'body, td {\n' \
      '  font-family: Verdana, Cursor;\n' \
      '  font-size: 10px;\n' \
      '  font-weight: bold;\n' \
      '}\n\n' \
      'table {\n' \
      '  border-spacing: 1px 1px;\n' \
      '}\n\n' \
      'table td {\n' \
      '  padding: 8px 4px 8px 4px;\n' \
      '}\n\n' \
      'table.Menu td {\n' \
      '  border: 1px gray solid;\n' \
      '  text-align: left;\n' \
      '  width: 160px;\n' \
      '}\n\n' \
      'pre {\n' \
      '  font-size: 11px;\n' \
      '  font-weight: normal;\n' \
      '}\n\n'
      'a:link,a:visited,a:active {\n' \
      '  color: #00f;\n' \
      '}\n\n'
      'a:hover {\n' \
      '  color: #444;\n' \
      '}\n\n' \
      '.error_header {\n' \
      '  font-weight: bold;\n' \
      '  font-size: 18px;\n' \
      '  color: #f00;\n' \
      '}\n\n' \
      '.header {\n' \
      '  font-weight: bold;\n' \
      '  font-size: 18px;\n' \
      '  color: #000;\n' \
      '}\n\n' \
      '.stamp {\n' \
      '  font-size: 11px;\n' \
      '}\n'
    )
    css_file.close()

class plotter:
  def __init__(self, logger, config):
    self._logger = logger
    self._config = config
    self._htmldir = self._config.common.get('htmldir', '')
    
    self._stats = {}
    
  def collect_data(self):
    self._logger.debug('Collecting statistical data for plotting to `%s\'' % self._htmldir)
    dirs_to_check = [dir for dir in os.listdir(self._htmldir) \
      if os.path.isdir(os.path.join(self._htmldir, dir)) \
        and re.match('(\d{8}_\d{6})', dir)]
    dirs_to_check.sort()
    for dir in dirs_to_check:
      date = '%s-%s-%s' % (dir[0:4], dir[4:6], dir[6:8])
      date_dir = os.path.join(self._htmldir, dir)
      platforms = [platform for platform in os.listdir(date_dir) \
        if os.path.isdir(os.path.join(date_dir, platform))]
      for platform in platforms:
        platform_dir = os.path.join(date_dir, platform)
        files = os.listdir(platform_dir)
        files.sort()
        stat_files = [file for file in files if 'perftest-stats' in file and file.endswith('csv')]
        if len(stat_files) > 0 and len(stat_files) <= 2:
          for file in stat_files:
            rt = 'rt2' in file and 'rt2' or 'rt1'
            if not rt in self._stats:
              self._stats[rt] = {}
            if not platform in self._stats[rt]:
              self._stats[rt][platform] = []
            file = open(os.path.join(platform_dir, file), 'rt')
            for line in file:
              dates_in = [d[0] for d in self._stats[rt][platform]] 
              if not line.split(',')[0] in dates_in:
                self._stats[rt][platform].append(line.split(','))
            file.close()
        else:
          data_rt1 = [date]
          data_rt2 = [date]
          for file in files:
            rt = 'rt2' in file and 'rt2' or 'rt1'
            if not rt in self._stats:
              self._stats[rt] = {}
            if not platform in self._stats[rt]:
              self._stats[rt][platform] = []
            if re.match('perftest\.rt\d{1}\-\d+', file):
              file = open(os.path.join(platform_dir, file), 'rt')
              for line in file:
                if re.search('=>>>Entities/Time', line):
                  if rt == 'rt1':
                    data_rt1.extend(line.split()[-5:-1])
                  else:
                    data_rt2.extend(line.split()[-5:-1])
                  break
              file.close()
          if len(data_rt1) > 1:
            dates_in = [d[0] for d in self._stats['rt1'][platform]]
            if not data_rt1[0] in dates_in:
              self._stats['rt1'][platform].append(data_rt1)
          if len(data_rt2) > 1:
            dates_in = [d[0] for d in self._stats['rt2'][platform]]
            if not data_rt2[0] in dates_in:
              self._stats['rt2'][platform].append(data_rt2)
   
  def plot(self, build_dir):
    self._logger.debug('Plotting collected statistical data')
    for runtime, runtime_data in self._stats.iteritems():
      for config_name, config_data in runtime_data.iteritems():
        target_dir = os.path.join(os.path.join(self._htmldir, build_dir), config_name)
        if len(config_data) < 1 or not os.path.isdir(target_dir):
          continue     
        csv_file_name = os.path.join(target_dir, 'perftest-stats-%s.csv-tmp' % runtime)
        cfg_file_name = os.path.join(target_dir, 'perftest-stats-%s.cfg' % runtime)
        csv_file = open(csv_file_name, 'wt')
        cfg_file = open(cfg_file_name, 'wt')
        youngest = config_data[0][0]
        oldest = config_data[0][0]
        for line in config_data:
          if line[0] < oldest:
            oldest = line[0]
          if line[0] > youngest:
            youngest = line[0]
          csv_file.write('%s\n' % ','.join(line).strip())
        csv_file.close()
        # `gnuplot' requires it to be sorted...
        utils.run_cmd('cat %s | sort >%s' % (csv_file_name, csv_file_name[0:-4]))
        utils.run_cmd('rm -f %s' % csv_file_name)
        csv_file_name = csv_file_name[0:-4]
        config = self._config.configs.get(config_name, {})
        cps_min = config.get('cpsmin', 1000)
        cps_max = config.get('cpsmax', 2000)
        cps_diff = abs(cps_max - cps_min) / 5
        cfg_file.write( \
          'set title "TITANSim CPS Statistics with LGenBase\\n(%d-%d CPS on \\`%s\\\')"\n' \
          'set datafile separator ","\n' \
          'set xlabel "Date"\n' \
          'set xdata time\n' \
          'set timefmt "%%Y-%%m-%%d"\n' \
          'set xrange ["%s":"%s"]\n' \
          'set format x "%%b %%d\\n%%Y"\n' \
          'set ylabel "CPS"\n' \
          'set terminal svg size 640, 480\n' \
          'set grid\n' \
          'set key right bottom\n' \
          'set key spacing 1\n' \
          'set key box\n' \
          'set output "%s/perftest-stats-%s.svg"\n' \
          'plot "%s" using 1:5 title "%d CPS" with linespoints, \\\n' \
          '"%s" using 1:9 title "%d CPS" with linespoints, \\\n' \
          '"%s" using 1:13 title "%d CPS" with linespoints, \\\n' \
          '"%s" using 1:17 title "%d CPS" with linespoints, \\\n' \
          '"%s" using 1:21 title "%d CPS" with linespoints, \\\n' \
          '"%s" using 1:25 title "%d CPS" with linespoints\n' \
          % (cps_min, cps_max, config_name, oldest, youngest, target_dir,
             runtime, csv_file_name, cps_min, csv_file_name,
             cps_min + cps_diff, csv_file_name, cps_min + 2 * cps_diff,
             csv_file_name, cps_min + 3 * cps_diff, csv_file_name,
             cps_min + 4 * cps_diff, csv_file_name, cps_max))
        cfg_file.close()
        utils.run_cmd('gnuplot %s' % cfg_file_name)

class StatHandler:
  """ The implementation of this class is based on the format of `result.txt'.
  """
  def __init__(self, logger, common_configs, configs, slave_list, reset):
    self._logger = logger
    self._configs = configs
    self._common_configs = common_configs
    self._html_root = self._common_configs.get('htmldir') 
    self._configs_to_support = []
    self._first_period_started = None
    self._period_started = None
    self._overall_score = 0
    self._overall_score_all = 0
    self._period_score = 0
    self._period_score_all = 0
    for slave in slave_list:  # Prepare list of active configurations.
      (slave_name, config_name, is_localhost) = slave
      if not self.is_weekend_or_holiday() and config_name in self._configs and 'measure' in self._configs[config_name] and self._configs[config_name]['measure']: 
        self._configs_to_support.append(config_name)
    # Scan and parse the latest `report.txt' file.
    dirs_to_check = [dir for dir in os.listdir(self._html_root) if os.path.isdir(os.path.join(self._html_root, dir)) and re.match('(\d{8}_\d{6})', dir)]
    dirs_to_check.sort()
    dirs_to_check.reverse()
    for dir in dirs_to_check:
      report_txt_path = os.path.join(self._html_root, os.path.join(dir, 'report.txt'))
      if os.path.isfile(report_txt_path):
        report_txt = open(report_txt_path, 'rt')
        for line in report_txt:
          first_period_line_matched = re.search('^First period.*(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}).*', line)
          overall_score_line_matched = re.search('^Overall score.*(\d+)/(\d+).*', line)
          period_started_line_matched = re.search('^This period.*(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}).*', line)
          period_score_line_matched = re.search('^Period score.*(\d+)/(\d+).*', line)
          if first_period_line_matched:
            self._first_period_started = first_period_line_matched.group(1)
          elif overall_score_line_matched:
            self._overall_score = int(overall_score_line_matched.group(1))
            self._overall_score_all = int(overall_score_line_matched.group(2))
          elif period_started_line_matched:
            self._period_started = period_started_line_matched.group(1)
          elif period_score_line_matched:
            self._period_score = int(period_score_line_matched.group(1))
            self._period_score_all = int(period_score_line_matched.group(2))
        report_txt.close()
        if self._first_period_started is None or self._period_started is None \
          or self._overall_score is None or self._overall_score_all is None \
          or self._period_score is None or self._period_score_all is None:
          self._logger.debug('Something is wrong with the report file `%s\'' \
                             % report_txt_path)
          continue
        self._logger.debug('Using report file `%s\'' % report_txt_path)
        break
    if not self.is_weekend_or_holiday():
      self._overall_score_all += (2 * len(self._configs_to_support))
      self._period_score_all += (2 * len(self._configs_to_support))
    if not self._first_period_started:
      self._first_period_started = utils.get_time()
    if not self._period_started:
      self._period_started = utils.get_time()
    if reset or int(utils.get_time_diff(False, self._period_started, utils.get_time(), True)[0]) / 24 >= self._common_configs.get('measureperiod', 30):
      self._period_started = utils.get_time()
      self._period_score = self._period_score_all = 0      
      
  def is_weekend_or_holiday(self):
    """ Weekends or any special holidays to ignore.  """
    ignore = int(time.strftime('%w')) == 0 or int(time.strftime('%w')) == 6
    if not ignore:
      holidays = ((1, 1), (3, 15), (5, 1), (8, 20), (10, 23), (11, 1), (12, 25), (12, 26))
      month = int(time.strftime('%m'))
      day = int(time.strftime('%d'))
      for holiday in holidays:
        if (month, day) == holiday:
          ignore = True
          break
    return ignore

  def lost(self, config_name):
    if not config_name in self._configs_to_support:
      return
    self._overall_score += 1
    self._period_score += 1
  
  def disabled_success_failure(self, config_name, results):
    """ `results' is coming from the CSV file.  """
    if not config_name in self._configs_to_support:
      return
    titan = int(results[0])
    regtest = int(results[1])
    perftest = int(results[2])  # Not counted.
    functest = int(results[3])
    # Nothing to do, unless a warning.
    if titan == -1 or regtest == -1 or functest == -1:
      self._logger.warning('Mandatory tests were disabled for build '
                           'configuration `%s\', the generated statistics ' \
                           'may be false, check it out' % config_name)
    if titan == 0 and regtest == 0 and functest == 0:
      self._overall_score += 2
      self._period_score += 2 

  def percent(self, score, score_all):
    try:
      ret_val = (float(score) / float(score_all)) * 100.0
    except:
      return 0.0; 
    return ret_val;
  
  def buzzword(self, percent):
    if percent > 80.0: return 'Stretched'
    elif percent > 70.0: return 'Commitment'
    elif percent > 60.0: return 'Robust'
    else: return 'Unimaginable'
    
  def __str__(self):
    if len(self._configs_to_support) == 0:
      return ''
    overall_percent = self.percent(self._overall_score, self._overall_score_all)
    period_percent = self.percent(self._period_score, self._period_score_all)
    ret_val = 'Statistics:\n-----------\n\n' \
      'Configurations: %s\n' \
      'First period: %s\n' \
      'Overall score: %d/%d (%.2f%%) %s\n' \
      'This period: %s\n' \
      'Period score: %d/%d (%.2f%%) %s\n\n' \
      % (', '.join(self._configs_to_support), self._first_period_started, self._overall_score, self._overall_score_all,
         overall_percent, self.buzzword(overall_percent), self._period_started,
         self._period_score, self._period_score_all, period_percent, self.buzzword(period_percent))
    return ret_val
