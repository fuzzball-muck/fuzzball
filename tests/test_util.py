import asyncio
import asyncio.events
import os.path
import re
import subprocess
import tempfile
import unittest
import yaml

SOURCE_ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))

SERVER_PATH = os.path.join(SOURCE_ROOT_DIR, 'src/fbmuck')

SHUTDOWN_MESSAGE = b'Going down - Bye'

cached_server_options = None

def _text(b):
    return b.decode('UTF-8', errors='replace')

def _asyncio_run(coro):
    if hasattr(asyncio, 'run'):
        return asyncio.run(coro)
    else:
        loop = asyncio.events.new_event_loop()
        asyncio.set_event_loop(loop)
        return loop.run_until_complete(coro)

class ServerTestBase(unittest.TestCase):
    input_database = os.path.join(SOURCE_ROOT_DIR, 'dbs/minimal/data/minimal.db')
    output_database = 'out.db'
    connect_string = b'connect One potrzebie\n!pose DONECONNECTMARKER\n'
    connect_prompt = b'One DONECONNECTMARKER\r\n'
    done_command_command = b'\n!pose DONECOMMANDMARKER\n'
    done_command_prompt = b'One DONECOMMANDMARKER\r\n'
    finish_string = b'@shutdown\n'
    params = {}
    timezone = 'UTC'
    timeout = 5

    def _generate_empty_motd(self):
        with open(os.path.join(self.game_dir, 'motd.txt'), 'w') as fh:
            fh.write("### EMPTY MOTD FILE ###\n")

    def _generate_empty_welcome(self):
        with open(os.path.join(self.game_dir, 'welcome.txt'), 'w') as fh:
            fh.write("### EMPTY WELCOME FILE ###\n")

    def _generate_parmfile(self):
        with open(os.path.join(self.game_dir, 'test_parm_file'), 'w') as fh:
            fh.write("file_motd=motd.txt\nfile_welcome_screen=welcome.txt\n")
            for key, value in self.params.items():
                fh.write("{key}={value}\n".format(key=key, value=value))
            fh.close()

    def _generate_files(self):
        self._generate_empty_motd()
        self._generate_empty_welcome()
        self._generate_parmfile()
        try:
            os.makedirs(os.path.join(self.game_dir, 'logs'))
        except:
            pass
        try:
            os.makedirs(os.path.join(self.game_dir, 'muf'))
        except:
            pass
    
    
    def setUp(self):
        self.temp_dir= tempfile.TemporaryDirectory()
        self.game_dir = self.temp_dir.name

    def tearDown(self):
        self.temp_dir.cleanup()

    async def _start(self):
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
        self.process = await asyncio.create_subprocess_exec(
            command_line[0], *command_line[1:],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=my_env,
        )
        self.current_stdout = b''
        self.current_stderr = b''
        self.stderr_future = asyncio.ensure_future(self.process.stderr.read())
    

    async def _read_to_prompt(self, prompt):
        try:
            text = await self.process.stdout.readuntil(prompt)
        except asyncio.IncompleteReadError as e:
            text = e.partial
        print("read from server:\n{}".format(_text(text)))
        self.current_stdout += text
        return text
    
    async def _read_to_eof(self):
        text = await self.process.stdout.read()
        print("read from server:\n{}".format(_text(text)))
        self.current_stdout += text
        return text

    async def _write_and_await_prompt(self, input, prompt=None):
        print("writing to server:\n{}".format(_text(input)))
        self.process.stdin.write(input)
        _, text = await asyncio.gather(
            self.process.stdin.drain(),
            self._read_to_prompt(prompt) if prompt != None else self._read_to_eof()
        )
        return text

    async def _finish(self):
        _, _, self.current_stderr = await asyncio.gather(
            self._read_to_eof(),
            self.process.wait(),
            self.stderr_future,
        )
   
    async def _run_command(self, command):
        try:
            await self._start()
            await self._write_and_await_prompt(self.connect_string, self.connect_prompt)
            output = await self._write_and_await_prompt(command + self.done_command_command, self.done_command_prompt)
            await self._write_and_await_prompt(self.finish_string)
            await self._finish()
        finally:
            print("server stderr:\n{}".format(_text(self.current_stderr)))
        return output 


class CommandTestCase(ServerTestBase):
    def _test_one(self, info):
        setup = info.get('setup', '')
        commands = info['commands'] 
        expect = info['expect']
        output = _asyncio_run(self._run_command(
            setup.encode('UTF-8') +
            b'\n!pose ENDSETUP\n' +
            commands.encode('UTF-8')
        ))
        output = output.decode('UTF-8', errors='replace')
        output = output.replace('\r\n', '\n')
        output = output.split('One ENDSETUP\n', 2)[1]
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

def create_command_tests_for(base_class, yaml_file):
    with open(yaml_file, 'r') as fh:
        lst = yaml.load(fh)
    for item in lst:
        _create_command_test_from(base_class, yaml_file, item)
