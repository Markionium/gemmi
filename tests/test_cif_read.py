#!/usr/bin/env python

import gc
import unittest
from gemmi import cif

class TestBlock(unittest.TestCase):
    def test_find(self):
        block = cif.read_string("""
            data_test
            _nonloop_a alpha
            _nonloop_b beta
            loop_ _la _lb A B C D
        """).sole_block()
        gc.collect()
        rows = list(block.find('_la'))
        self.assertEqual(list(rows[0]), ['A'])

        rows = list(block.find(['_lb', '_la']))  # changed order
        gc.collect()
        self.assertEqual(list(rows[0]), ['B', 'A'])
        self.assertEqual(rows[1][1], 'C')

        rows = list(block.find('_nonloop_', ['a', 'b']))
        gc.collect()
        self.assertEqual([list(r) for r in rows], [['alpha', 'beta']])

if __name__ == '__main__':
    unittest.main()
