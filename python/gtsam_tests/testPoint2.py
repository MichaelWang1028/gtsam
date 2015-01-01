import unittest
from numpy.testing import *
from gtsam import *
import numpy_eigen as npe
import numpy

#https://docs.python.org/2/library/unittest.html
class TestPoint2(unittest.TestCase):

    def setUp(self):
        self.point = Point2()

    def test_get_methods(self):
        x = self.point.x()
        y = self.point.y()
        self.assertEqual(x, 0.0)
        self.assertEqual(y, 0.0)

    def test_overloaded_constructor(self):
        p = Point2(1.0, 1.0)
        x = p.x()
        y = p.y()
        self.assertEqual(x, 1.0)
        self.assertEqual(y, 1.0)

    def test_group_methods(self):
        p = Point2(2.0, 5.0)
        actual = self.point.compose(p)
        x = actual.x()
        y = actual.y()
        self.assertEqual(x, 2.0)
        self.assertEqual(y, 5.0)

        actual = p.inverse()
        x = actual.x()
        y = actual.y()
        self.assertEqual(x, -2.0)
        self.assertEqual(y, -5.0)

        p2 = Point2(1.0, 4.0)
        actual = p2.between(p)
        x = actual.x()
        y = actual.y()
        self.assertEqual(x, 1.0)
        self.assertEqual(y, 1.0)

    def test_vector(self):
        actual = self.point.vector()
        assert_array_equal(actual, [0.0, 0.0]) #numpy.testing
        p = Point2(2.0, 5.0)
        actual = p.vector()
        assert_array_equal(actual, [2.0, 5.0]) #numpy.testing

    def test_manifold(self):
        actual = self.point.dim()
        self.assertEqual(actual, 2)

        #Causes error
        v = numpy.array([1.0, 1.0])
        actual = self.point.retract(v)

    def test_npe(self):
        v = numpy.array([[1.0], [1.0]])
        p = Point2(v)


if __name__ == '__main__':
    unittest.main()
