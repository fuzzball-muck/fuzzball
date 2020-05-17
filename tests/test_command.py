import os
import test_util
import unittest

for name in os.listdir('command-cases'):
    if name.endswith('yml'):
        class_name = name.replace('.yml', '')
        base_class = globals()[class_name] =  type(class_name, (test_util.CommandTestCase,), {})
        test_util.create_command_tests_for(base_class, os.path.join('command-cases', name))

if __name__ == '__main__':
    unittest.main()
