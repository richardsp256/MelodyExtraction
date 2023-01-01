Overview
--------

pymelex is a python module with experimental bindings to the library. The main
use of these bindings is prototyping.

Tests
-----

A handful of minor tests have been included. The main goal of the tests is to
check whether the bindings break. These tests require `pytest` to be installed.

To run the tests, call

	py.test tests

from the directory where this readme is stored. Note that all unit testing for
the correctness of the underlying C-code is performed separately.