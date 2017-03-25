#!/usr/bin/env python3

import argparse
from collections import defaultdict
import difflib
import os
import subprocess
import sys

try:
    from termcolor import colored
except ImportError:
    def colored(text, *args, **kwargs):
        return text


class RunnerError(Exception):
    pass


class TestRunnerBase(object):

    def __init__(self, path, verbosity):
        self.path = path
        self.verbosity = verbosity
        self.__captured_output = defaultdict(list)
        self.__current_test = None

    @staticmethod
    def success(text):
        return colored(text, 'green')

    @staticmethod
    def error(text, worse=False):
        return colored(text, 'red', attrs=(['bold'] if worse else []))

    @staticmethod
    def warn(text):
        return colored(text, 'yellow')

    @staticmethod
    def blockquote(text, first=True, last=True):
        return (
            (colored('=' * 20 + ' START ' + '=' * 20, 'blue') + '\n' if first else '') +
            text + ('' if text.endswith('\n') else '\n') +
            (colored('=' * 21 + ' END ' + '=' * 21, 'blue') if last else colored('-' * 47, 'blue'))
        )

    @staticmethod
    def log(*args, **kwargs):
        print(*args, file=sys.stderr, flush=True, **kwargs)

    def capture_output(self, output):
        assert isinstance(output, str)
        if self.verbosity >= 4:
            self.__captured_output[self.__current_test].append(output)

    def run(self):
        succeeded_count = 0
        failed_count = 0
        test_count = 0
        error_count = 0

        for dirpath, dirnames, filenames in os.walk(self.path):
            for filename in filenames:
                fpath = os.path.join(dirpath, filename)

                if not self.is_test(fpath):
                    continue

                test_count += 1

                if self.verbosity >= 3:
                    self.log(fpath + ': ', end='')

                self.__current_test = fpath
                succeeded = False
                try:
                    succeeded = self.run_test(fpath)
                    if isinstance(succeeded, str):
                        self.capture_output(succeeded)
                finally:
                    self.__current_test = None
                    # Only keep the captured output if the test failed.
                    if succeeded is True and fpath in self.__captured_output:
                        del self.__captured_output[fpath]

                if succeeded is True:
                    succeeded_count += 1
                elif succeeded is False:
                    failed_count += 1
                else:
                    error_count += 1

                if self.verbosity >= 3:
                    if succeeded is True:
                        self.log(self.success('PASSED'))
                    elif succeeded is False:
                        self.log(self.error('FAILED'))
                    else:
                        self.log(self.error('ERROR', worse=True))
                elif self.verbosity >= 1:
                    if succeeded is True:
                        self.log(self.success('.'), end='')
                    elif succeeded is False:
                        self.log(self.error('F'), end='')
                    else:
                        self.log(self.error('E', worse=True), end='')

        if self.verbosity >= 1:
            self.log()

        if self.verbosity >= 4 and self.__captured_output:
            self.log('Captured output from tests that failed or had errors:')
            for fpath, captured_output in self.__captured_output.items():
                self.log(fpath)
                last_output = len(captured_output) - 1
                for i, output in enumerate(captured_output):
                    self.log(self.blockquote(output, first=(i == 0), last=(i == last_output)))

        if self.verbosity >= 2:
            self.log('{} success + {} failure + {} error = {} tests'.format(
                succeeded_count, failed_count, error_count, test_count
            ))

        if failed_count or error_count:
            if self.verbosity >= 1:
                self.log(self.error('FAILED'))
            return 1

        if self.verbosity >= 1:
            self.log(self.success('PASSED'))
        return 0

    def is_test(self, fpath):
        return False

    def run_test(self, fpath):
        return False


class DiffTestRunner(TestRunnerBase):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.__runners_by_extension = {}

    @staticmethod
    def diff(reference_output, output):
        diff = difflib.unified_diff(
            reference_output.splitlines(), output.splitlines(),
            fromfile='reference output', tofile='test output',
            lineterm=''
        )
        return '\n'.join(
            colored(line, 'red') if line.startswith('-') else
            colored(line, 'green') if line.startswith('+') else
            colored(line, 'cyan') if line.startswith('@') else
            line
            for line in diff
        )

    def register_runner(self, extension, runner):
        self.__runners_by_extension[extension] = runner

    def is_test(self, fpath):
        root, ext = os.path.splitext(fpath)
        if ext in self.__runners_by_extension:
            return True
        return False

    def run_test(self, fpath):
        root, ext = os.path.splitext(fpath)
        runner = self.__runners_by_extension[ext]

        try:
            runner.run(fpath)
        except RunnerError as e:
            if runner.stdout:
                self.capture_output(colored('STDOUT:\n', 'magenta') + runner.stdout)
            if runner.stderr:
                self.capture_output(colored('STDERR:\n', 'magenta') + runner.stderr)
            self.capture_output(str(e))
            return 'Could not perform diff due to error(s) above.'

        if runner.stderr:
            self.capture_output(colored('STDERR:\n', 'magenta') + runner.stderr)

        output = runner.stdout
        reference_path = root + '.txt'

        try:
            with open(reference_path) as reference_file:
                reference_output = reference_file.read()
        except FileNotFoundError:
            return 'File not found: {}'.format(reference_path)

        succeeded = (output == reference_output)

        if not succeeded:
            self.capture_output(self.diff(reference_output, output))

        return succeeded


class RunnerBase(object):

    def __init__(self):
        self.stdout = ''
        self.stderr = ''

    def run(self, fpath):
        pass


class ScriptRunner(RunnerBase):

    def __init__(self, interpreter_path, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.__interpreter_path = interpreter_path

    def run(self, fpath):
        try:
            with open(fpath) as f:
                first_line = f.readline()
        except FileNotFoundError:
            raise RunnerError('could not open file {}'.format(fpath))

        options = []

        if '!' in first_line:
            options = first_line.split('!', 1)[1].split()

        proc = subprocess.Popen(
            [self.__interpreter_path] + options + [fpath],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        outs, errs = b'', b''
        try:
            outs, errs = proc.communicate(timeout=15)
        except subprocess.TimeoutExpired:
            proc.kill()
            outs, errs = proc.communicate()
            raise RunnerError('Script timed out.')
        finally:
            self.stdout = outs.decode('utf-8')
            self.stderr = errs.decode('utf-8')


def main():
    this_dir = os.path.dirname(__file__)

    parser = argparse.ArgumentParser(description='Recursively find tests and run them.')
    parser.add_argument(
        'path', nargs='?', default=os.path.join(this_dir, 'tests'),
        help='Path to a test or a directory with tests in it.'
    )
    parser.add_argument('--verbose', '-v', default=0, action='count', dest='verbosity')

    test_runner = DiffTestRunner(**vars(parser.parse_args()))
    test_runner.register_runner('.lua', ScriptRunner(
        os.path.join(this_dir, 'bin', 'exe')
    ))

    sys.exit(test_runner.run())


if __name__ == '__main__':
    main()
