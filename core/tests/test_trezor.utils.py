from common import *

from trezor import utils


class TestUtils(unittest.TestCase):

    def test_chunks(self):
        c = list(utils.chunks(range(100), 7))
        for i in range(15):
            # need to check start, stop, step attrs until https://github.com/micropython/micropython/issues/2600 is resolved
            self.assertEqual(c[i].start, i * 7)
            self.assertEqual(c[i].stop, 100 if (i == 14) else (i + 1) * 7)
            self.assertEqual(c[i].step, 1)

    def test_format_amount(self):
        VECTORS = [
            (123456, 3, "123.456"),
            (4242, 7, "0.0004242"),
            (-123456, 3, "-123.456"),
            (-4242, 7, "-0.0004242"),
        ]
        for v in VECTORS:
            self.assertEqual(utils.format_amount(v[0], v[1]), v[2])

    def test_format_timestamp(self):
        VECTORS = [
            (1577833200, "2020-01-01 00:00:00"),
            (1577836800, "2020-01-01 01:00:00"),
            (1577839000, "2020-01-01 01:36:40"),
            (1575542899, "2019-12-05 11:48:19"),
        ]
        for v in VECTORS:
            self.assertEqual(utils.format_timestamp(v[0]), v[1])


if __name__ == '__main__':
    unittest.main()
