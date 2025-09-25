#!/usr/bin/env python3
#
# test_lfst.py -- Unit tests for lfst
#
# Copyright (C) 2025 Timo Kokkonen <tjko@iki.fi>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

"""littlefs-toy unit tester"""

import os
import subprocess
import io
import shutil
import hashlib
import tempfile
import inspect
import unittest


class LFSTests(unittest.TestCase):
    """lfs test cases"""

    program = '../build/lfst'
    tmpdir = None
    workdir = None
    debug = False
    testfiles = [ 'test1.bin', 'test2.bin', 'test3.bin', 'test4.bin', 'test5.bin' ]


    def setUp(self):
        if "LFS" in os.environ:
            self.program = os.environ["LFS"]
        if "DEBUG" in os.environ:
            self.debug = True
        self.tmpdir = tempfile.mkdtemp()

    def tearDown(self):
        if self.tmpdir:
            shutil.rmtree(self.tmpdir)


    def run_test(self, args, check=True, directory=False):
        """execute lfs command for a test"""
        command = [self.program] + args
        if directory:
            func_name  = inspect.currentframe().f_code.co_name
            workdir = self.tmpdir + '/' + func_name
            if not os.path.isdir(workdir):
                os.makedirs(workdir)
            self.workdir = workdir
            command.extend(['-C', workdir])
        if self.debug:
            print(f'\nRun command: {" ".join(command)}')
        res = subprocess.run(command, encoding="utf-8", check=check,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        output = res.stdout
        if self.debug:
            print(f'Result: {res.returncode}')
            print(f'---\n{output}\n---\n')
        return output, res.returncode

    def get_hash(self, filename, tmpdir=False):
        if tmpdir:
            filename = self.workdir + '/' + filename
            #print(f'Calculating hash for file: {filename}')
        with open(filename, "rb") as f:
            digest = hashlib.file_digest(f, "sha256")
        return digest.hexdigest()


    def test_version(self):
        """test version information output"""
        output, _ = self.run_test(['--version'])
        self.assertIn('GNU General Public License', output)
        self.assertRegex(output, r'lfst v\d+\.\d+\.\d+')

    def test_noarguments(self):
        """test running without arguments"""
        output, res = self.run_test([], check=False)
        self.assertEqual(1, res)
        self.assertRegex(output, r'no command specified')

    def test_list_contents(self):
        """test listing image contents"""
        testfile = self.testfiles[-1]
        output, res = self.run_test(['-tvf','lfs_4096.img'])
        self.assertRegex(output, r'\s\./' + testfile + '\n')

    def test_block_size_autodetect(self):
        """test blocksize auto detection"""
        testfile = self.testfiles[-1]
        output, res = self.run_test(['-tvf','lfs_512.img'])
        self.assertRegex(output, r'\s\./' + testfile + '\n')
        self.assertRegex(output, r'blocksize is 512\s')

    def test_extract(self):
        """test extracting files from image"""
        testfiles = self.testfiles[0:5:2]
        for blocksize in ['4096', '512']:
            output, res = self.run_test(['-xvf','lfs_' + blocksize + '.img','-b',blocksize,
                                         '-O'] + testfiles, directory=True)
            for fname in testfiles:
                self.assertRegex(output, r'\./' + fname + '\n')
                h_orig = self.get_hash(fname, tmpdir=False)
                h_new = self.get_hash(fname, tmpdir=True)
                #print(f'Checking {fname}: {h_orig} vs {h_new}')
                self.assertEqual(h_orig, h_new)

    def test_create(self):
        """test creating image from files"""
        testfiles = self.testfiles
        image = self.tmpdir + '/lfs.img'
        # create new filesystem image from the files
        output, res = self.run_test(['-cvf', image, '-s', '256M'] + testfiles)
        # extract filed from the filesystem image to temp directory
        output2, res2 = self.run_test(['-xvf', image], directory=True)
        # compare that extracted files match the source files
        for fname in testfiles:
            self.assertRegex(output2, r'\./' + fname + '\n')
            h_orig = self.get_hash(fname, tmpdir=False)
            h_new = self.get_hash(fname, tmpdir=True)
            #print(f'Checking {fname}: {h_orig} vs {h_new}')
            self.assertEqual(h_orig, h_new)

    def test_delete(self):
        """test deleting files from filesystem image"""
        testfiles = self.testfiles
        image = self.tmpdir + '/lfs.img'
        # create new filesystem image from the files
        output, res = self.run_test(['-cvf', image, '-s', '64M'] + testfiles)
        # delete files from the image
        delete_list = self.testfiles[2:4]
        output2, res2 = self.run_test(['-dvf', image] + delete_list)
        # check that deleted files are no longer on the image
        output3, res3 = self.run_test(['-tvf', image])
        for fname in testfiles:
            if fname in delete_list:
                self.assertNotRegex(output3, r'\s\./' + fname + '\n')
            else:
                self.assertRegex(output3, r'\s\./' + fname + '\n')

    def test_offset(self):
        """test accessing filesystem image at an offset"""
        output, res = self.run_test(['-tvf','lfs_offset_64K.img'], check=False)
        self.assertEqual(1, res)
        output2, res2 = self.run_test(['-tvf','lfs_offset_64K.img','-o','64K'])
        for fname in self.testfiles:
            self.assertRegex(output2, r'\s\./' + fname + '\n')



if __name__ == '__main__':
    unittest.main()

# eof :-)
