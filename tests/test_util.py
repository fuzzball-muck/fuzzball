import asyncio
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
        self.stderr_future = self.process.stderr.read()
    

    async def _read_to_prompt(self, prompt):
        text = await self.process.stdout.readuntil(prompt)
        self.current_stdout += text
        return text
    
    async def _read_to_eof(self):
        text = await self.process.stdout.read()
        self.current_stdout += text
        return text

    async def _write_and_await_prompt(self, input, prompt=None):
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
        await self._start()
        await self._write_and_await_prompt(self.connect_string, self.connect_prompt)
        output = await self._write_and_await_prompt(command + self.done_command_command, self.done_command_prompt)
        await self._write_and_await_prompt(self.finish_string)
        await self._finish()
        print("server stdout:\n{}\nserver stderr:\n{}".format(
            _text(self.current_stdout), _text(self.current_stderr)
        ))
        return output 


class CommandTestCase(ServerTestBase):
    def test_command(self):
        output = asyncio.run(self._run_command(
            self._setup.encode('UTF-8') +
            b'\n!pose ENDSETUP\n' +
            self._commands.encode('UTF-8')
        ))
        output = output.decode('UTF-8', errors='replace')
        output = output.replace('\r\n', '\n')
        output = output.split('One ENDSETUP\n', 2)[1]
        for item in self._expect:
            self.assertRegex(output, item)

def _generate_command_test(base, info):
    expect = info['expect']
    if isinstance(expect, str):
        expect = [expect]
    return type(
        base + ':' + info['name'],
        (CommandTestCase,),
        {
            '_commands': info['commands'],
            '_setup': info.get('setup', ''),
            '_expect': expect
        }
    )
        

def suite_for_commands_in(yaml_file, loader=None):
    suite = unittest.TestSuite()
    if not loader:
        loader = unittest.TestLoader()
    with open(yaml_file, 'r') as fh:
        lst = yaml.load(fh)
    for item in lst:
        suite.addTests(
            loader.loadTestsFromTestCase(_generate_command_test(yaml_file, item))
        )
    return suite

        
