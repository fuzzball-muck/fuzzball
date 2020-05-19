import asyncio
import asyncio.events
import os.path
import re
import signal
import subprocess
import tempfile
import unittest
import yaml

"""Location of fbmuck source code root directory."""
SOURCE_ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))

"""Path to compiled fbmuck server binary."""
SERVER_PATH = os.path.join(SOURCE_ROOT_DIR, 'src/fbmuck')

"""
Convert bytes into something printable for debugging and string matching.
"""
def _text(b):
    return b.decode('UTF-8', errors='replace')

"""
Call asyncio.run if it's available (recent Python versions), otherwise use
low-level API to do something similar.
"""
def _asyncio_run(coro):
    if hasattr(asyncio, 'run'):
        return asyncio.run(coro)
    else:
        loop = asyncio.events.new_event_loop()
        asyncio.set_event_loop(loop)
        return loop.run_until_complete(coro)

"""
Base class for test cases that run a server.

Contains a setUp and tearDown method that starts a server in a temporary directory.

The primary method for using this class is `_run_command`, which handles starting the
server, connecting to it and retrieving the output of a particular set of commands.

If this is not acceptable, then `_start`, `_write_and_await_prompt`, and `_finish`
provide a lower-level interface.

Many of the methods of this class are coroutines, to be used with asyncio.
"""
class ServerTestBase(unittest.TestCase):
    """Default database file to use for the server, passed as -dbin argument.
    Note that muf files are *not* copied into place."""
    input_database = os.path.join(SOURCE_ROOT_DIR, 'dbs/minimal/data/minimal.db')

    """Output database, passed as -dbout argument."""
    output_database = 'out.db'

    """Commands to send to connect to the server. The default connects as #1 to minimal.db."""
    connect_string = b'connect One potrzebie\n!pose DONECONNECTMARKER\n'

    """Text to expect to see when the command sent by `connect_string` finished."""
    connect_prompt = b'One DONECONNECTMARKER\r\n'

    """Commands to send to create a marker that can be used to tell when a command completes."""
    done_command_command = b'\n!pose DONECOMMANDMARKER\n'

    """The expected output for `done_command_command`."""
    done_command_prompt = b'One DONECOMMANDMARKER\r\n'

    """Commands to send before termination. This needs to shut down the server in some way, since
    the tests wait until end-of-file."""
    finish_string = b'@shutdown\n'

    """@tune parameters to set via the -parmfile argument."""
    params = {}

    """Timezone to run the server in (via the TZ environment variable)."""
    timezone = 'UTC'

    """Generate a placeholder motd.txt file in `self.game_dir` (set by setUp())."""
    def _generate_placeholder_motd(self):
        with open(os.path.join(self.game_dir, 'motd.txt'), 'w') as fh:
            fh.write("### PLACEHOLDER MOTD FILE ###\n")

    """Generate a placeholder welcome.txt file in `self.game_dir` (set by setUp())."""
    def _generate_placeholder_welcome(self):
        with open(os.path.join(self.game_dir, 'welcome.txt'), 'w') as fh:
            fh.write("### PLACEHOLDER WELCOME FILE ###\n")

    """Generate a parmfile in `self.game_dir`/test_parm_file which includes the parameters specified in
    self.params and sets file_motd to motd.txt and file_welcome_screen to welcome.txt (matching
    `_generate_placeholder_motd` and `_generate_placeholder_welcome.`"""
    def _generate_parmfile(self):
        with open(os.path.join(self.game_dir, 'test_parm_file'), 'w') as fh:
            fh.write("file_motd=motd.txt\nfile_welcome_screen=welcome.txt\n")
            for key, value in self.params.items():
                fh.write("{key}={value}\n".format(key=key, value=value))
            fh.close()

    """Generate files and directories for the srever in self.game_dir."""
    def _generate_files(self):
        self._generate_placeholder_motd()
        self._generate_placeholder_welcome()
        self._generate_parmfile()
        try:
            os.makedirs(os.path.join(self.game_dir, 'logs'))
        except:
            pass
        try:
            os.makedirs(os.path.join(self.game_dir, 'muf'))
        except:
            pass

    """Setup tests by creating a temporary game_dir directory (set as self.game_dir)
    and running _generate_files."""
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.game_dir = self.temp_dir.name

    """Delete the temporary directory created by setUp()."""
    def tearDown(self):
        if self._process:
            self._process.send_signal(signal.SIGKILL)
        self.temp_dir.cleanup()

    """
    Start the the MUCK server according to the settings in this object.

    Sets self._process to a asyncio.subprocess.Process instance.

    Initializes self._stderr_future to a asyncio Task that reads from the stderr and returns the result.

    Initializes self._current_stdout and self._current_stderr to empty bytestrings. These are appended to be read/write methods below.
    """
    async def _start_server(self):
        self._generate_files()
        command_line = [
            SERVER_PATH,
           '-dbin', self.input_database,
           '-dbout', self.output_database,
           '-gamedir', self.game_dir,
           '-dbout', os.path.join(self.game_dir, 'dbout'),
           '-console',
           '-parmfile', 'test_parm_file',
        ]
        my_env = os.environ.copy()
        my_env['MALLOC_CHECK_'] = '2'
        if self.timezone:
            my_env['TZ'] = self.timezone
        self._process = await asyncio.create_subprocess_exec(
            command_line[0], *command_line[1:],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=my_env,
        )
        self._current_stdout = b''
        self._current_stderr = b''
        self._stderr_future = asyncio.ensure_future(self._process.stderr.read())


    """Read from the MUCK server started by _start_server until the prompt specified appears.

    If the prompt never occurs, read until EOF.

    The text read is appeneded to self._current_stdout in addition to being returned.

    Returns:
        the text read as a bytes
    """
    async def _read_to_prompt(self, prompt):
        try:
            text = await self._process.stdout.readuntil(prompt)
        except asyncio.IncompleteReadError as e:
            text = e.partial
        print("read from server:\n{}".format(_text(text)))
        self._current_stdout += text
        return text

    """Read from the MUCK server started by _start_server until end-of-file.

    The text read is appeneded to self._current_stdout in addition to being returned.

    Returns:
        the text read as bytes
    """
    async def _read_to_eof(self):
        text = await self._process.stdout.read()
        print("read from server:\n{}".format(_text(text)))
        self._current_stdout += text
        return text

    """Write commands to the MUCK server started by _start_server, then read until
    the specified prompt (or EOF if not prompt is specified or the prompt is not found
    before EOF).

    Args:
       input: commands to write to the server (as bytes)
       prompt: prompt to expect as bytes, or None for no prompt
    Returns:
       text read up until and including the prompt
    """
    async def _write_and_await_prompt(self, input, prompt=None):
        print("writing to server:\n{}".format(_text(input)))
        self._process.stdin.write(input)
        _, text = await asyncio.gather(
            self._process.stdin.drain(),
            self._read_to_prompt(prompt) if prompt != None else self._read_to_eof()
        )
        return text

    """Send `self.finish_string` to the server and wait for the server to terminate.

    Fills `self._current_stderr` with the output accumulated from the server's stderr."""
    async def _finish(self):
        _, _, self._current_stderr = await asyncio.gather(
            self._write_and_await_prompt(self.finish_string),
            self._process.wait(),
            self._stderr_future,
        )
        self._process = None

    """Start the server (with _start_server) and connect (using self._connect_string)
    and wait for connnecting to finish (with self._connect_prompt)."""
    async def _start_and_connect(self):
        await self._start_server()
        await self._write_and_await_prompt(self.connect_string, self.connect_prompt)

    """Start and connect to the server, send a set of commands, and then send commands to shutdown
    the server.

    Uses self.connect_string, self.connect_prompt, self.done_command_command, self.done_command_prompt,
    self.finish_string to handle connecting, isolating the commands' output and terminating the server
    cleanly.

    Args:
       command: bytes to send to the server
    Returns:
       the output from `command` (with self.done_command_prompt added at the end)
    """
    async def _run_command(self, command):
        try:
            await self._start_and_connect()
            output = await self._write_and_await_prompt(command + self.done_command_command, self.done_command_prompt)
            await self._finish()
        finally:
            print("server stderr:\n{}".format(_text(self._current_stderr)))
        return output


"""Base class for simple command tests. Used internally by create_command_tests_for."""
class CommandTestCase(ServerTestBase):
    """Command (as bytes) to run to mark the end of the setup section of commands."""
    done_setup_command = b"!pose ENDSETUPMARKER\n"
    """Expected output (as str) of done_setup_command."""
    done_setup_prompt = "One ENDSETUPMARKER\n"

    def _test_one(self, info):
        setup = info.get('setup', '')
        commands = info['commands']
        expect = info['expect']
        output = _asyncio_run(self._run_command(
            setup.encode('UTF-8') +
            self.done_setup_command +
            commands.encode('UTF-8')
        ))
        output = output.decode('UTF-8', errors='replace')
        output = output.replace('\r\n', '\n')
        output = output.split(self.done_setup_prompt, 2)[1]
        for item in info['expect']:
            self.assertRegex(output, item)

    def id(self):
        return self.name

def _create_command_test_from(base_class, source_file, info):
    expect = info['expect']
    if isinstance(expect, str):
        expect = [expect]
    info['expect'] = expect
    setattr(base_class,
        'test_' + info['name'],
        lambda self: self._test_one(info)
    )
"""
Given a YAML file with an array of dictionaries, each of which represents a command-running test,
add methods representing each test to the specified base_class. For these methods to work, the
specified base class should be a subclass of CommandTestCase.

If no settings are changed from CommandTestCase, commands will be run as #1 in minimal.db.

Each item in the YAML file can contain the following keys:
*  name: used to construct the name of the test method
*  setup: initial commands to run, but not inspect the output of
*  commands: final set of commands to run and inspect the output of
*  expect: regular expression pattern or list of regular expression patterns
           to expect to find in the output of "commands". Matching is done after conversion
           to UTF-8 and replacing \r\n by \n

Args:
   * base_class: base class type, which must inherit from CommandTestCase
   * yaml_file: YAML file to open
"""
def create_command_tests_for(base_class, yaml_file):
    with open(yaml_file, 'r') as fh:
        lst = yaml.load(fh)
    for item in lst:
        _create_command_test_from(base_class, yaml_file, item)
