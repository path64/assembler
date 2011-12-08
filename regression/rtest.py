#! /usr/bin/env python
# Regression test runner
#
#  Copyright (C) 2010  Peter Johnson
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
import binascii
import os
import subprocess
import sys
import time

def lprint(*args, **kwargs):
    sep = kwargs.pop("sep", ' ')
    end = kwargs.pop("end", '\n')
    file = kwargs.pop("file", sys.stdout)
    file.write(sep.join(args))
    file.write(end)

yasmexe = None
outdir = None

def path_splitall(path):
    """Split all components of a path into a list."""
    outpath = []
    while True:
        (head, tail) = os.path.split(path)
        if not head:
            break
        path = head
        outpath.append(tail)
    outpath.append(path)
    outpath.reverse()
    return outpath

class Test(object):
    def __init__(self, name, fullpath):
        self.name = name
        self.fullpath = fullpath

        # Default parser based on extension
        self.parser = self.name.endswith(".s") and "gas" or "nasm"

        self.basefn = os.path.splitext("_".join(path_splitall(self.name)))[0]
        self.outfn = self.basefn + ".out"
        self.ewfn = self.basefn + ".ew"

        # Read the input file in its entirety.  We use this for various things.
        f = open(self.fullpath)
        try:
            self.inputfile = f.read()
        finally:
            f.close()
        self.inputlines = self.inputfile.splitlines()

    def save_ew(self, stderrdata):
        f = open(os.path.join(outdir, self.ewfn), "w")
        try:
            f.write(stderrdata)
        finally:
            f.close()

    def compare_ew(self, stderrdata):
        """Check error/warnings output."""
        # If there's a .ew file, use it.
        golden = []
        try:
            f = open(os.path.splitext(self.fullpath)[0] + ".ew")
            try:
                golden = [l.rstrip() for l in f.readlines()]
            finally:
                f.close()
        except IOError:
            pass

        result = [l for l in enumerate(stderrdata.splitlines())
                  if not l[1].startswith(" ")]

        match = True
        if len(golden) != len(result):
            lprint("%s: error/warning mismatches" % self.ewfn)
            match = False
        for (i, o), g in zip(result, golden):
            if o != g:
                lprint("%s:%d: mismatch on error/warning" % (self.ewfn, i+1))
                lprint(" Expected: %s" % g)
                lprint(" Actual: %s" % o)
                match = False

        if not match:
            self.save_ew(stderrdata)

        return match

    def compare_out(self):
        """Check output file."""
        # If there's a .hex file, use it; otherwise scan the input file
        # for comments starting with "out:" followed by hex digits.
        golden = []
        try:
            f = open(os.path.splitext(self.fullpath)[0] + ".hex")
            try:
                golden = [l.strip() for l in f.readlines()]
            finally:
                f.close()
        except IOError:
            for l in self.inputlines:
                comment = l.partition(self.commentsep)[2].strip()
                if not comment.startswith('out:'):
                    continue
                golden.extend(comment[4:].split())
        ngolden = []
        for x in golden:
            if not x:
                continue
            # Currently only does little endian
            ngolden.extend(ord(y) for y in reversed(binascii.a2b_hex(x)))
        golden = ngolden

        goldenfn = self.basefn + ".gold"

        # check result file
        f = open(os.path.join(outdir, self.outfn), "rb")
        try:
            result = f.read()
        finally:
            f.close()
        match = True
        if len(golden) != len(result):
            lprint("%s: output length %d (expected %d)"
                    % (self.outfn, len(result), len(golden)))
            match = False
        for i, (o, g) in enumerate(zip([ord(x) for x in result], golden)):
            if o != g:
                # XXX Check FreeBSD OSABI in ELF header.
                if i == 7 and o == 9 and g == 0 and os.uname()[0] == "FreeBSD":
                    continue
                lprint("%s:%d: mismatch: %s (expected %s)"
                        % (self.outfn, i, hex(o), hex(g)))
                lprint("  (only the first mismatch is reported)")
                match = False
                break

        if not match:
            # save golden version to binary file
            lprint("Expected output: %s" % goldenfn)
            f = open(os.path.join(outdir, goldenfn), "wb")
            try:
                f.write("".join([chr(x) for x in golden]))
            finally:
                f.close()

            # save golden hex
            f = open(os.path.join(outdir, self.basefn + ".goldhex"), "w")
            try:
                f.writelines(["%02x\n" % x for x in golden])
            finally:
                f.close()

            # save result hex
            f = open(os.path.join(outdir, self.basefn + ".outhex"), "w")
            try:
                f.writelines(["%02x\n" % ord(x) for x in result])
            finally:
                f.close()

        return match

    def get_option(self, option, default=None):
        """Get test-specific option from the first line of the input file.
        Returns None if option not present, otherwise option string."""
        firstline = self.inputlines[0]

        start = firstline.find("[" + option)
        if start == -1:
            return default

        # get everything between the []
        str = firstline[start+1:]
        return str[len(option)+1:str.index(']')]

    def run(self):
        """Run test.  Returns false if test failed."""
        # Expected failure: "[fail]"
        expectfail = self.get_option("fail") is not None

        # Parser override: "[parser X]"
        self.parser = self.get_option("parser", self.parser)

        # We default to bin output, but can override with "[oformat X]"
        format = self.get_option("oformat", "bin")

        yasmargs = ["yasm", "-f", format, "-p", self.parser]

        # full command line override: "[yasm <args>]"
        yasmoverride = self.get_option("yasm")
        if yasmoverride is not None:
            import shlex
            yasmargs = shlex.split("yasm "+yasmoverride)

        # full command line override: "[ygas <args>]"
        ygasoverride = self.get_option("ygas")
        if ygasoverride is not None:
            import shlex
            yasmargs = shlex.split("ygas "+ygasoverride)
            self.parser = "gas"

        # Set comment separator based on parser
        self.commentsep = (self.parser == "gas") and "#" or ";"

        # Notify start of test
        lprint("[ RUN      ] %s (%s%s)%s" %
               (self.name, yasmargs[0] == "ygas" and "ygas " or "",
                " ".join(yasmargs[1:]), expectfail and "{fail}" or ""))

        # Specify the output filename as we pipe the input.
        yasmargs.extend(["-o", os.path.join(outdir, self.outfn)])

        # We pipe the input, so append "-" to the command line for stdin input.
        yasmargs.append("-")

        # Run yasm!
        start = time.time()
        env = os.environ.copy()
        env["YASM_TEST_SUITE"] = "1"
        proc = subprocess.Popen(yasmargs, bufsize=4096,
                                executable=(ygasoverride and ygasexe or yasmexe),
                                stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, env=env)
        (stdoutdata, stderrdata) = proc.communicate(self.inputfile)
        end = time.time()

        ok = False
        if proc.returncode == 0 and not expectfail:
            ok = True
        elif proc.returncode < 0:
            lprint(" CRASHED: received signal %d" % (-proc.returncode))
            self.save_ew(stderrdata)
        elif expectfail:
            ok = True
        else:
            lprint("Error: return code mismatch.")
            lprint(" Expected: %d" % (expectfail and 1 or 0))
            lprint(" Actual: %d" % proc.returncode)
            if proc.returncode != 0:
                self.save_ew(stderrdata)

        # Check results
        if ok:
            match = self.compare_ew(stderrdata)
            if not match:
                ok = False

            if not expectfail:
                match = self.compare_out()
                if not match:
                    ok = False

        # Summarize test result
        if ok:
            result = "      OK"
        else:
            result = " FAILED "
        lprint("[ %s ] %s (%d ms)" % (result, self.name, int((end-start)*1000)))
        return ok

def run_all(bpath):
    ntests = 0
    ndirs = 0
    npassed = 0
    failed = []
    lprint("[==========] Running tests.")
    start = time.time()
    for root, dirs, files in os.walk(bpath):
        somefiles = [f for f in files if f.endswith(".asm") or f.endswith(".s")]
        if not somefiles:
            continue
        ndirs += 1
        lprint("[----------] %d tests from %s"
                % (len(somefiles), root[len(bpath)+1:]))
        for name in somefiles:
            fullpath = os.path.join(root, name)
            name = fullpath[len(bpath)+1:]
            test = Test(name, fullpath)
            ok = test.run()
            ntests += 1
            if ok:
                npassed += 1
            else:
                failed.append(name)
        lprint("[----------] %d tests from %s"
                % (len(somefiles), root[len(bpath)+1:]))
    end = time.time()
    lprint("[==========] %d tests from %d directories ran. (%d ms total)"
            % (ntests, ndirs, int((end-start)*1000)))
    lprint("[  PASSED  ] %d tests." % npassed)
    if failed:
        lprint("[  FAILED  ] %d tests, listed below:" % len(failed))
        for name in failed:
            lprint("[  FAILED  ] %s" % name)
        lprint(" %d FAILED TESTS" % len(failed))
        return False
    return True

if __name__ == "__main__":
    if len(sys.argv) != 5:
        lprint("Usage: rtest.py <path to regression tree>", file=sys.stderr)
        lprint("    <path to output directory>", file=sys.stderr)
        lprint("    <path to yasm executable>", file=sys.stderr)
        lprint("    <path to ygas executable>", file=sys.stderr)
        sys.exit(2)
    outdir = sys.argv[2]
    yasmexe = sys.argv[3]
    ygasexe = sys.argv[4]
    all_ok = run_all(sys.argv[1])
    if all_ok:
        sys.exit(0)
    else:
        sys.exit(1)
