import os
import test_util
import unittest

def load_tests(loader, standard_tests, pattern):
    suite = unittest.TestSuite()
    for name in os.listdir('command-cases'):
        if name.endswith('yml'):
            suite.addTests(test_util.suite_for_commands_in(
                os.path.join('command-cases', name),
                loader=loader
            ))
    return suite

if __name__ == '__main__':
    unittest.main()
