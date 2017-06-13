##############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Kovacs, Ferenc
#
##############################################################################
import os, re
import titan_publisher, utils

MAKEFILE_PATCH = 'makefile_patch.sh'

class product_handler:
  """ It is assumed that the VOB is reachable (e.g. through an SSHFS mounted
      directory) on the master machine.  The source package is copied to all
      slaves with SCP.  It is assumed that the logger is always set.

      Saving stdout and stderr of product actions is optional.  During
      publishing these files should be linked or copied to somewhere under
      `public_html'.
  """
  def __init__(self, logger = None, config = None):
    self._logger = logger
    self._config = config

  def config_products(self, target_path):
    """ Localize first in the following order: `test', `demo'.  If neither of
        these directories are found the `src' will be copied, it's a must
        have.  If there's a Makefile or .prj in `test', `demo', the list of
        files will be gathered from there.  Otherwise the contents of these
        directories excluding the Makefile will be copied to the target
        directory.  A new Makefile will be generated for the copied files.
        Configspec 129 is used.  It's assumed that the ClearCase access is
        working and all files are accessible through SSHFS etc.  The `test'
        directory always has higher priority than `demo'.  However, `demo' is
        the one released to the customer and not `test', but we're using the
        TCC_Common VOB anyway.

        We generate single mode Makefiles here for simplicity.  We can grep
        through the sources for a `create' call, but it's so ugly.  It is
        assumed that the files enumerated in Makefiles or .prj files are all
        relative to the current directory.  The source and target paths are
        class parameters.  It is assumed that the Makefile and the project
        file are all in the `test' directory.  The Makefile will be ignored if
        there's a project file in the same directory.
        
        The distribution of the VOB package is the job of the master.
        Configuring the products with `ttcn3_makefilegen' is the job of the
        slaves.

        Returns:
          0 if everything went fine and the VOB package is ready for
          distribution.  1 otherwise.
    """
    utils.run_cmd('rm -rf ' + target_path)
    for kind, products in self._config.products.iteritems():
      for product in products:
        product_name = product['name'].strip()
        local_src_path = os.path.join(os.path.join(self._config.common['vob'], kind), product_name)
        src_dir = os.path.join(local_src_path, 'src')
        test_dir = os.path.join(local_src_path, 'test')
        demo_dir = os.path.join(local_src_path, 'demo')
        if not os.path.isdir(src_dir):
          self._logger.error('Missing `src\' directory for product `%s\' ' \
                             'in %s, skipping product' % (product_name, local_src_path))
          continue
        else:
          dirs_to_copy = []
          files_to_copy = []
          if os.path.isdir(test_dir):
            dirs_to_copy.append(test_dir)
          elif os.path.isdir(demo_dir):
            dirs_to_copy.append(demo_dir)
          else:
            # No `demo' or `test'.  The `src' is copied only if the other
            # directories are missing.  There can be junk files as well.  The
            # Makefile patch script must have a fixed name
            # `makefile_patch.sh'.
            dirs_to_copy.append(src_dir)
            self._logger.debug('Product `%s\' in %s doesn\'t have the `demo\' or `test\' directories'
                               % (product_name, local_src_path))
          product_target_path = os.path.join(os.path.join(target_path, kind), product_name)
          utils.run_cmd('mkdir -p ' + product_target_path)
          has_prj_file = False
          for dir in dirs_to_copy:
            for dir_path, dir_names, file_names in os.walk(dir):
              if not has_prj_file and \
                len([file_name for file_name in file_names \
                     if file_name.endswith('.prj')]) > 0: has_prj_file = True
              for file_name in file_names:
                if not has_prj_file:  # Trust the project file if we have one.
                  files_to_copy.append(os.path.join(dir_path, file_name))
                if (file_name == 'Makefile' and not has_prj_file) or file_name.endswith('.prj'):
                  (makefile_patch, extracted_files) = \
                    self.extract_files(dir_path, os.path.join(dir_path, file_name))
                  files_to_copy.extend(extracted_files)
                  if makefile_patch:
                    utils.run_cmd('cp -Lf %s %s' \
                      % (makefile_patch, os.path.join(product_target_path, MAKEFILE_PATCH))) 
          utils.run_cmd('cp -Lf %s %s' % (' '.join(files_to_copy), product_target_path))
          utils.run_cmd('chmod 644 * ; chmod 755 *.sh', product_target_path)
          self._logger.debug('Product `%s\' was configured successfully ' \
                             'with %d files' % (product_name, len(files_to_copy)))
    return 0 

  def build_products(self, proddir, logdir, config, rt2 = False):
    """ Build the products provided in the list.  Simple `compiler -s' etc.
        commands are executed from the directories of the products.  The
        actions must be synchronized with the product configuration files.
        The stderr and stdout of actions is captured here, but it's optional.

        Arguments:
          The directory of the products, the actual build configuration and
          runtime.

        Returns:
          The build results in the following format:

          results['kind1'] = [
            {'name1':{'action1':(1, o1, e1), 'action2':-1}},
            {'name2':{'action1':(1, o1, e1), 'action2':-1}},
            {'name3':-1}
          ]

          The standard output and error channels are returned for each action
          with the return value.  The return value is usually the exit status
          of `make' or the `compiler'.  If the element is a simple integer
          value the action was disabled for the current product.  The output
          of this function is intended to be used by the presentation layer.
    """
    results = {}
    if not os.path.isdir(logdir):
      utils.run_cmd('mkdir -p %s' % logdir)
    for kind, products in self._config.products.iteritems():
      results[kind] = []
      for product in products:
        product_name = product['name'].strip()
        product_dir = os.path.join(proddir, os.path.join(kind, product_name))
        if not os.path.isdir(product_dir):
          # No `src' was found for the product.  Maybe a list would be better
          # instead.
          results[kind].append({product_name:-1})
          continue
        info = {product_name:{}}
        for product_key in product.iterkeys():
          files = ' '.join(filter(lambda file: file.endswith('.ttcn') \
                                  or file.endswith('.asn'), \
                                  os.listdir(product_dir)))
          cmd = None
          if product_key == 'semantic':
            if product[product_key]:
              cmd = '%s/bin/compiler -s %s %s' % (config['installdir'], rt2 and '-R' or '', files)
            else:
              info[product_name][product_key] = -1
              continue
          elif product_key == 'translate':
            if product[product_key]:
              cmd = '%s/bin/compiler %s %s' % (config['installdir'], rt2 and '-R' or '', files)
            else:
              info[product_name][product_key] = -1
              continue
          elif product_key == 'compile' or product_key == 'run':
            if product[product_key]:
              utils.run_cmd('cd %s && %s/bin/ttcn3_makefilegen ' \
                            '-fp %s *' % (product_dir, config['installdir'], rt2 and '-R' or ''))
              if os.path.isfile(os.path.join(product_dir, MAKEFILE_PATCH)):
                self._logger.debug('Patching Makefile of product `%s\' for the %s runtime'
                                   % (product_name, rt2 and 'function-test' or 'load-test'))
                utils.run_cmd('cd %s && mv Makefile Makefile.bak' % product_dir)
                utils.run_cmd('cd %s && ./%s Makefile.bak Makefile' % (product_dir, MAKEFILE_PATCH))
              cmd = 'make clean ; make dep ; make -j4 ; %s' % (product_key == 'run' and 'make run' or '')
            else:
              info[product_name][product_key] = -1
              continue
          else:
            # Skip `name' or other things.
            continue
          (retval, stdout, stderr) = utils.run_cmd(cmd, product_dir, 900)
          prod_stdout = os.path.join(logdir, '%s_%s_%s_stdout.%s' \
            % (kind, product_name, product_key, rt2 and 'rt2' or 'rt1'))
          prod_stderr = os.path.join(logdir, '%s_%s_%s_stderr.%s' \
            % (kind, product_name, product_key, rt2 and 'rt2' or 'rt1'))
          output_files = (prod_stdout, prod_stderr)
          try:
            out_file = open(prod_stdout, 'wt')
            err_file = open(prod_stderr, 'wt')
            if 'vobtest_logs' not in config or config['vobtest_logs']:
              out_file.write(' '.join(stdout))
              err_file.write(' '.join(stderr))
            out_file.close()
            err_file.close()
          except IOError, (errno, strerror):
            self._logger.error('Error while dumping product results: %s (%s)' \
                               % (strerror, errno))
          info[product_name][product_key] = (retval, output_files, stdout, stderr)
        results[kind].append(info)
    return results

  def extract_files(self, path, filename):
    """ Extract the files need to be copied all around from a given Makefile
        or .prj file.  It handles wrapped lines (i.e. '\') in Makefiles.  """

    # Interesting patterns in Makefiles and .prj files.  Tuples are faster
    # than lists, use them for storing constants.
    prj_matches = ( \
      '<Module>\s*(.+)\s*</Module>', \
      '<TestPort>\s*(.+)\s*</TestPort>', \
      '<Config>\s*(.+)\s*</Config>', \
      '<Other>\s*(.+)\s*</Other>', \
      '<Other_Source>\s*(.+)\s*</Other_Source>', \
      '<File path="\s*(.+)\s*"', \
      '<File_Group path="\s*(.+)\s*"' )
    
    makefile_matches = ( \
      '^TTCN3_MODULES =\s*(.+)', \
      '^ASN1_MODULES =\s*(.+)', \
      '^USER_SOURCES =\s*(.+)', \
      '^USER_HEADERS =\s*(.+)', \
      '^OTHER_FILES =\s*(.+)' )

    try:
      file = open(filename, 'rt')
    except IOError:
      self._logger.error('File `%s\' cannot be opened for reading' % filename)
      return (None, [])
    
    files = []
    makefile_patch = None
    if re.search('.*[Mm]akefile$', filename):
      multi_line = False
      for line in file:
        line = line.strip()
        if multi_line:
          files.extend(map(lambda f: os.path.join(path, f), line.split()))
          multi_line = line.endswith('\\')
          if multi_line:
            files.pop()
        else:
          for line_match in makefile_matches:
            matched = re.search(line_match, line)
            if matched:
              files.extend(map(lambda f: os.path.join(path, f),
                               matched.group(1).split()))
              multi_line = line.endswith('\\')
              if multi_line:
                files.pop()
    elif re.search('.*\.prj$', filename) or re.search('.*\.grp', filename):
      files_to_exclude = []
      for line in file:
        # Only basic support for Makefile patching, since it doesn't have a
        # bright future in its current form...
        matched = re.search('<ScriptFile_AfterMake>\s*(.+)\s*</ScriptFile_AfterMake>', line)
        if matched:
          makefile_patch = os.path.join(path, matched.group(1))
          continue
        matched = re.search('<UnUsed_List>\s*(.+)\s*</UnUsed_List>', line)
        if matched:
          files_to_exclude.extend(matched.group(1).split(','))
          continue
        for line_match in prj_matches:
          matched = re.search(line_match, line)
          if matched and matched.group(1) not in files_to_exclude:
            file_to_append = os.path.join(path, matched.group(1))
            files_to_append = []
            if file_to_append != filename and file_to_append.endswith('.grp'):
              self._logger.debug('Group file `%s\' found' % file_to_append)
              last_slash = file_to_append.rfind('/')      
              if last_slash != -1:
                grp_dir = file_to_append[:last_slash]
                if path.startswith('/'):
                  grp_dir = os.path.join(path, grp_dir)  
                (not_used, files_to_append) = self.extract_files(grp_dir, file_to_append)
              else:
                self._logger.warning('Skipping contents of `%s\', check ' \
                                     'this file by hand' % file_to_append)
            files.append(file_to_append)
            files.extend(files_to_append)
            break
    else:
      self._logger.error('Unsupported project description file: %s\n' % filename)
    file.close()
    return (makefile_patch, files)
