from  __future__ import print_function

import os
import re
import sys
import imp
import shutil
import unittest
import subprocess
import tempfile
from contextlib import contextmanager
from functools import wraps

from nose.tools import assert_true, assert_equal
from nose.plugins.attrib import attr
from nose.plugins.skip import SkipTest

from cyclus import lib as libcyclus

if sys.version_info[0] >= 3:
    basestring = str

unit = attr('unit')
integration = attr('integration')

INPUT = os.path.join(os.path.dirname(__file__), "input")

def cleanfs(paths):
    """Removes the paths from the file system."""
    for p in paths:
        p = os.path.join(*p)
        if os.path.isfile(p):
            os.remove(p)
        elif os.path.isdir(p):
            shutil.rmtree(p)


def check_cmd(args, cwd, holdsrtn):
    """Runs a command in a subprocess and verifies that it executed properly.
    """
    if not isinstance(args, basestring):
        args = " ".join(args)
    print("TESTING: running command in {0}:\n\n{1}\n".format(cwd, args))
    f = tempfile.NamedTemporaryFile()
    env = dict(os.environ)
    env['_'] = cp = subprocess.check_output(['which', 'cyclus'], cwd=cwd,
                                            universal_newlines=True).strip()
    rtn = subprocess.call(args, shell=True, cwd=cwd, stdout=f, stderr=f, env=env)
    if rtn != 0:
        f.seek(0)
        print('CYCLUS COMMAND: ' + cp)
        print("STDOUT + STDERR:\n\n" + f.read().decode())
    f.close()
    holdsrtn[0] = rtn
    assert_equal(rtn, 0)


@contextmanager
def clean_import(name, paths=None):
    """Imports and returns a module context manager and then removes
    all modules which didn't originally exist when exiting the block.
    Be sure to delete any references to the returned module prior to
    exiting the context.
    """
    sys.path = paths + sys.path
    origmods = set(sys.modules.keys())
    mod = imp.load_module(name, *imp.find_module(name, paths))
    yield mod
    sys.path = sys.path[len(paths):]
    del mod
    newmods = set(sys.modules.keys()) - origmods
    for newmod in newmods:
        del sys.modules[newmod]


TESTNAME_RE = re.compile('(?:^|[\\b_\\.-])[Tt]est')

def modtests(mod):
    """Finds all of the tests in a module."""
    tests = []
    for name in dir(mod):
        if TESTNAME_RE.match(name) is None:
            continue
        test = getattr(mod, name)
        if test is unittest.TestCase:
            continue
        tests.append(test)
    return tests


def dirtests(d):
    """Finds all of the test files in a directory."""
    files = os.listdir(d)
    filenames = []
    for file in files:
        if not file.endswith('.py'):
            continue
        if TESTNAME_RE.match(file) is None:
            continue
        filenames.append(file[:-3])
    return filenames


def skip_then_continue(msg=""):
    """A simple function to yield such that a test is marked as skipped
    and we may continue on our merry way. A message may be optionally passed
    to this function.
    """
    raise SkipTest(msg)

@contextmanager
def indir(d):
    """Context manager for switching directorties and then switching back."""
    cwd = os.getcwd()
    os.chdir(d)
    yield
    os.chdir(cwd)

#
# Some Database API test helpers
#

LIBCYCLUS_HAS_BEEN_RUN = False
DBS = [('libcyclus-test.h5', 'libcyclus-orig.h5', libcyclus.Hdf5Back),
       #('libcyclus-test.sqlite', 'libcyclus-orig.sqlite', libcyclus.SqliteBack)
       ]


def safe_call(cmd, shell=False, *args, **kwargs):
    """Checks that a command successfully runs with/without shell=True.
    Returns the process return code.
    """
    try:
        rtn = subprocess.call(cmd, shell=False, *args, **kwargs)
    except (subprocess.CalledProcessError, OSError):
        cmd = ' '.join(cmd)
        rtn = subprocess.call(cmd, shell=True, *args, **kwargs)
    return rtn


def libcyclus_setup():
    global LIBCYCLUS_HAS_BEEN_RUN
    if not LIBCYCLUS_HAS_BEEN_RUN:
        LIBCYCLUS_HAS_BEEN_RUN = True
        for fname, oname, _ in DBS:
            if os.path.isfile(fname):
                os.remove(fname)
            if os.path.isfile(oname):
                os.remove(oname)
    for fname, oname, _ in DBS:
        if os.path.isfile(oname):
            continue
        safe_call(['cyclus', '-o' + oname,
                   os.path.join(INPUT, 'inventory.xml')])


def dbtest(f):
    @wraps(f)
    def wrapper():
        for fname, oname, backend in DBS:
            if os.path.exists(fname):
                os.remove(fname)
            shutil.copy(oname, fname)
            db = backend(fname)
            yield f, db, fname, backend
    return wrapper




#
# Here there be Hackons!
#

# hack to make sure that we are actually in the tests dir when we start running
# tests.  This works because this file is imported by many of the other test
# files.
_fdir = os.path.dirname(__file__)
if os.getcwd() != _fdir:
    os.chdir(_fdir)
del _fdir
