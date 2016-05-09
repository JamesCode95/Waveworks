import sys
import os

# General information about the project.
project = u'NVIDIA WaveWorks'
projectshort = u'NVIDIA WaveWorks'

# product page on developer.nvidia.com
productlink = 'http://developer.nvidia.com/WaveWorks'

# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.
#
# The short X.Y version.
version = '1.6.'
# The full version, including alpha/beta/rc tags.
release = '1.6.1'

# Don't ship rst source
html_copy_source = False

# Pulls in the standard config information
filename='{}\default.conf.py'.format(os.environ['SPHINX_ROOT'])
if os.path.exists(filename):
  execfile(filename)
else:
  sys.stderr.write('ERROR: Please sync //sw/devrel/libdev/external/sphinx/...\n')
  sys.stderr.flush()
  sys.exit(-1)

# Insert overrides of the defaults after this line

# Add replacement macros below
# See http://docutils.sourceforge.net/docs/ref/rst/restructuredtext.html#substitution-definitions for types of substitutions 
#rst_epilog += ".. |SHORT| replace:: Very Long Text\n"
#rst_epilog += '.. _external link: http://link.to.page/\n'

# Use rst_prolog to add content to begining of every rst file
#rst_prolog += "\n"

