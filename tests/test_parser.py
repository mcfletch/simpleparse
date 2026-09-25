"""A Parser's generator is what a library of shared productions is built from"""
from simpleparse import objectgenerator
from simpleparse.parser import Parser


def test_a_production_is_shared_through_the_generator():
    library = Parser("digits := [0-9]+")
    shared = objectgenerator.LibraryElement(
        generator=library.generator,
        production="digits",
    )
    user = Parser("number := digits", "number", prebuilts=[("digits", shared)])
    success, children, next = user.parse("2026")
    assert success
    assert next == 4
    assert [child[:3] for child in children] == [("digits", 0, 4)]


def test_the_underscore_name_is_the_same_generator():
    made = Parser("digits := [0-9]+")
    assert made._generator is made.generator  # noqa: SLF001 the name under test
