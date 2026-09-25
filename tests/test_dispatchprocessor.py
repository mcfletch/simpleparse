"""Finding the processing function for a tag in a processor"""
import pytest

from simpleparse.dispatchprocessor import dispatch

TAG = ('word', 0, 5, None)
BUFFER = 'hello'


class Found:
    def word(self, tag, buffer):
        return buffer[tag[1]:tag[2]]


def test_an_attribute_is_called():
    assert dispatch(Found(), TAG, BUFFER) == 'hello'


def test_a_mapping_entry_is_called():
    source = {'word': lambda _tag, buffer: buffer.upper()}
    assert dispatch(source, TAG, BUFFER) == 'HELLO'


@pytest.mark.parametrize('source', [{}, [], object()], ids=['mapping', 'sequence', 'neither'])
def test_a_missing_function_is_an_attribute_error(source):
    with pytest.raises(AttributeError, match='No processing function for tag "word"'):
        dispatch(source, TAG, BUFFER)


class BrokenMapping:
    def __getitem__(self, key):
        raise RuntimeError('lookup failed for %s' % key)


def test_an_error_inside_the_lookup_propagates():
    with pytest.raises(RuntimeError, match='lookup failed for word'):
        dispatch(BrokenMapping(), TAG, BUFFER)
