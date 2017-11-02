""" mxTextTools - A tools package for fast text processing.

    Copyright (c) 2000, Marc-Andre Lemburg; mailto:mal@lemburg.com
    Copyright (c) 2000-2003, eGenix.com Software GmbH; mailto:info@egenix.com
    See the documentation for further information on copyrights,
    or contact the author. All Rights Reserved.
"""
from __future__ import print_function

import types

#
# import the C module and the version number
#
from .mxTextTools import *
from .mxTextTools import __version__

#
# import the symbols needed to write tag tables
#
from .Constants.TagTables import *

#
# import the some handy character sets
#
from .Constants.Sets import *

#
# format and print tables, taglists and joinlists:
#


def format_entry(table, i,

                 TupleType=tuple):
    """ Returns a pp-formatted tag table entry as string 
    """
    e = table[i]
    jne = 0
    je = 1
    t, c, m = e[:3]
    if len(e) > 3:
        jne = e[3]
    if len(e) > 4:
        je = e[4]
    flags, cmd = divmod(c, 256)
    c = id2cmd[cmd]
    if type(m) == TupleType and c in ('Table', 'SubTable'):
        m = '<table>'
    elif m == None:
        m = 'Here/To'
    else:
        m = repr(m)
        if len(m) > 17:
            m = m[:17] + '...'
    return '%-15.15s : %-30s : jne=%+i : je=%+i' % \
           (repr(t), '%-.15s : %s' % (c, m), jne, je)


def format_table(table, i=-1):
    """ Returns a pp-formatted version of the tag table as string """

    l = []
    for j in range(len(table)):
        if i == j:
            l.append('--> ' + format_entry(table, j))
        else:
            l.append('    ' + format_entry(table, j))
    return '\n'.join(l) + '\n'


def print_tagtable(table):
    """ Print the tag table 
    """
    print(format_table(table))


def print_tags(text, tags, indent=0):
    """ Print the taglist tags for text using the given indent level 
    """
    for tag, l, r, subtags in tags:
        tagname = repr(tag)
        if len(tagname) > 20:
            tagname = tagname[:20] + '...'
        target = repr(text[l:r])
        if len(target) > 60:
            target = target[:60] + '...'
        if subtags == None:
            print(' ' + indent * ' |', tagname, ': ', target, (l, r))
        else:
            print(' ' + indent * ' |', tagname, ': ', target, (l, r))
            print_tags(text, subtags, indent + 1)


def print_joinlist(joins, indent=0,

                   StringType=str):
    """ Print the joinlist joins using the given indent level 
    """
    for j in joins:
        if type(j) == StringType:
            text = repr(j)
            if len(text) > 40:
                text = text[:40] + '...'
            print(' ' + indent * ' |', text, ' (len = %i)' % len(j))
        else:
            text = j[0]
            l, r = j[1:3]
            text = repr(text[l:r])
            if len(text) > 40:
                text = text[:40] + '...'
            print(' ' + indent * ' |', text, ' (len = %i)' % (r - l), (l, r))


def normlist(jlist,

             StringType=str):
    """ Return a normalized joinlist.

        All tuples in the joinlist are turned into real strings.  The
        resulting list is a equivalent copy of the joinlist only
        consisting of strings.

    """
    l = [''] * len(jlist)
    for i in range(len(jlist)):
        entry = jlist[i]
        if type(entry) == StringType:
            l[i] = entry
        else:
            l[i] = entry[0][entry[1]:entry[2]]
    return l

#
# aid for matching from a list of words
#


def _lookup_dict(l, index=0):

    d = {}
    for w in l:
        c = w[index]
        if c in d:
            d[c].append(w)
        else:
            d[c] = [w]
    return d


def word_in_list(l):
    """ Creates a lookup table that matches the words in l 
    """
    t = []
    d = _lookup_dict(l)
    keys = list(d.keys())
    if len(keys) < 18:  # somewhat arbitrary bound
        # fast hint for small sets
        t.append((None, IsIn, ''.join(list(d.keys()))))
        t.append((None, Skip, -1))
    # test groups
    for c, group in list(d.items()):
        t.append(None)  # hint will be filled in later
        i = len(t) - 1
        for w in group:
            t.append((None, Word, w[1:], +1, MatchOk))
        t.append((None, Fail, Here))
        # add hint
        t[i] = (None, Is, c, len(t) - i)
    t.append((None, Fail, Here))
    return tuple(t)

#
# Extra stuff useful in combination with the C functions
#


def replace(text, what, with_what, start=0, stop=None,

            SearchObject=TextSearch, join=join, joinlist=joinlist, tag=tag,
            string_replace=str.replace, type=type,
            StringType=str):
    """A fast replacement for string.replace.

       what can be given as string or search object.

       This function is a good example for the AppendTagobj-flag usage
       (the taglist can be used directly as joinlist).

    """
    if type(what) is not TextSearchType:
        so = SearchObject(what)
    else:
        so = what
        what = so.match
    if stop is None:
        if start == 0 and len(what) < 2:
            return string_replace(text, what, with_what)
        stop = len(text)
    t = ((text, sWordStart, so, +2),
         # Found something, replace and continue searching
         (with_what, Skip + AppendTagobj, len(what), -1, -1),
         # Rest of text
         (text, Move, ToEOF)
         )
    found, taglist, last = tag(text, t, start, stop)
    if not found:
        return text
    return join(taglist)

# Alternative (usually slower) versions using different techniques:


def _replace2(text, what, with_what, start=0, stop=None,

              join=join, joinlist=joinlist, tag=tag,
              TextSearchType=TextSearchType, TextSearch=TextSearch):
    """Analogon to string.replace; returns a string with_what all occurences
       of what in text[start:stop] replaced by with_what.

       This version uses a one entry tag-table and a
       Boyer-Moore-Search-object.  what can be a string or a
       TextSearch search object.  It's faster than string.replace in
       those cases, where the what-string gets long and/or many
       replacements are found; faster meaning from a few percent up to
       many times as fast

       start and stop define the slice of text to work in.  stop
       defaults to len(text).

    """
    if stop is None:
        stop = len(text)
    if type(what) is not TextSearchType:
        what = TextSearch(what)
    t = ((with_what, sFindWord, what, +1, +0),)
    found, taglist, last = tag(text, t, start, stop)
    if not found:
        return text
    return join(joinlist(text, taglist))


def _replace3(text, what, with_what,

              join=str.join, TextSearch=TextSearch,
              TextSearchType=TextSearchType):

    if type(what) is not TextSearchType:
        what = TextSearch(what)
    slices = what.findall(text)
    if not slices:
        return text
    l = []
    x = 0
    for left, right in slices:
        l.append(text[x:left] + with_what)
        x = right
    l.append(text[x:])
    return join(l, '')


def _replace4(text, what, with_what,

              join=join, joinlist=joinlist, tag=tag, TextSearch=TextSearch,
              TextSearchType=TextSearchType):

    if type(what) is not TextSearchType:
        what = TextSearch(what)
    slices = what.findall(text)
    if not slices:
        return text
    repl = [None] * len(slices)
    for i in range(len(slices)):
        repl[i] = (with_what,) + slices[i]
    return join(joinlist(text, repl))


def multireplace(text, replacements, start=0, stop=None,

                 join=join, joinlist=joinlist):
    """ Apply multiple replacement to a text at once.

        replacements must be list of tuples (replacement, left,
        right).  It is used to replace the slice text[left:right] with_what
        the string replacement.

        Note that the replacements do not affect one another.  Indices
        always refer to the original text string.

        Replacements must not overlap. Otherwise a ValueError is
        raised.

    """
    if stop is not None:
        return join(joinlist(text, replacements, start, stop))
    else:
        return join(joinlist(text, replacements, start))


def find(text, what, start=0, stop=None,

         SearchObject=TextSearch):
    """ A faster replacement for string.find().

        Uses a search object for the task. Returns the position of the
        first occurance of what in text[start:stop]. stop defaults to
        len(text).  Returns -1 in case no occurance was found.

    """
    if stop is not None:
        return SearchObject(what).find(text, start, stop)
    else:
        return SearchObject(what).find(text, start)


def findall(text, what, start=0, stop=None,

            SearchObject=TextSearch):
    """ Find all occurances of what in text.

        Uses a search object for the task. Returns a list of slice
        tuples (l,r) marking the all occurances in
        text[start:stop]. stop defaults to len(text).  Returns an
        empty list in case no occurance was found.

    """
    if stop is not None:
        return SearchObject(what).findall(text, start, stop)
    else:
        return SearchObject(what).findall(text, start)


def split(text, sep, start=0, stop=None, translate=None,

          SearchObject=TextSearch):
    """ A faster replacement for string.split().

        Uses a search object for the task. Returns the result of
        cutting the text[start:stop] string into snippets at every sep
        occurance in form of a list of substrings. translate is passed
        to the search object as translation string if given.

        XXX Undocumented.

    """
    if translate:
        so = SearchObject(sep, translate)
    else:
        so = SearchObject(sep)
    if stop:
        cuts = so.findall(text, start, stop)
    else:
        cuts = so.findall(text, start)
    l = 0
    list = []
    append = list.append
    for left, right in cuts:
        append(text[l:left])
        l = right
    append(text[l:])
    return list

# helper for tagdict


def _tagdict(text, dict, prefix, taglist):

    for o, l, r, s in taglist:
        pfx = prefix + str(o)
        dict[pfx] = text[l:r]
        if s:
            _tagdict(text, dict, pfx + '.', s)


def tagdict(text, *args):
    """ Tag a text just like the function tag() and then convert
        its output into a dictionary where the tagobjects reference
        their respective strings

        This function emulates the interface of tag().  In contrast to
        tag() this funtion *does* make copies of the found stings,
        though.

        Returns a tuple (rc,tagdict,next) with_what the same meaning of rc
        and next as tag(); tagdict is the new dictionary or None in
        case rc is 0.

    """
    rc, taglist, next = tag(*(text,) + args)
    if not rc:
        return (rc, None, next)
    d = {}
    tagdict = _tagdict
    for o, l, r, s in taglist:
        pfx = str(o)
        d[pfx] = text[l:r]
        if s:
            tagdict(text, d, pfx + '.', s)
    return (rc, d, next)


def invset(chars):
    """ Return a set with_what all characters *except* the ones in chars.
    """
    return set(chars, 0)


def is_whitespace(text, start=0, stop=None,

                  charset=nonwhitespace_charset):
    """ Return 1 iff text[start:stop] only contains whitespace
        characters (as defined in Constants/Sets.py), 0 otherwise.

    """
    if stop is None:
        stop = len(text)
    return (charset.search(text, 1, start, stop) is None)


def collapse(text, separator=' ',

             join=join, charset=CharSet(newline + whitespace)):
    """ Eliminates newline characters and compresses whitespace
        characters into one space.

        The result is a one line text string. Tim Peters will like
        this function called with_what '-' separator ;-)

    """
    return join(charset.split(text), separator)


_linesplit_table = (
    (None, Is, '\r', +1),
    (None, Is, '\n', +1),
    ('line', AllInCharSet + AppendMatch, CharSet('^\r\n'), +1, -2),
    (None, EOF, Here, +1, MatchOk),
    ('empty line', Skip + AppendMatch, 0, 0, -4),
)


def splitlines(text,

               tag=tag, linesplit_table=_linesplit_table):
    """ Split text into a list of single lines.

        The following combinations are considered to be line-ends:
        '\r', '\r\n', '\n'; they may be used in any combination.  The
        line-end indicators are removed from the strings prior to
        adding them to the list.

        This function allows dealing with text files from Macs, PCs
        and Unix origins in a portable way.

    """
    return tag(text, linesplit_table)[1]


# Alias for backward compatibility
linesplit = splitlines

_linecount_table = (
    (None, Is, '\r', +1),
    (None, Is, '\n', +1),
    ('line', AllInCharSet, CharSet('^\r\n'), +1, -2),
    (None, EOF, Here, +1, MatchOk),
    ('empty line', Skip, 0, 0, -4),
)


def countlines(text,

               linecount_table=_linecount_table):
    """ Returns the number of lines in text.

        Line ends are treated just like for splitlines() in a
        portable way.

    """
    return len(tag(text, linecount_table)[1])


_wordsplit_table = (
    (None, AllInCharSet, whitespace_charset, +1),
    ('word', AllInCharSet + AppendMatch, nonwhitespace_charset, +1, -1),
    (None, EOF, Here, +1, MatchOk),
)


def splitwords(text,

               charset=whitespace_charset):
    """ Split text into a list of single words.

        Words are separated by whitespace. The whitespace is stripped
        before adding the words to the list.

    """
    return charset.split(text)

#
# Testing and benchmarking
#


# Taken from my hack.py module:
import time


class _timer:

    """ timer class with a quite obvious interface
        - .start() starts a fairly accurate CPU-time timer plus an
          absolute timer
        - .stop() stops the timer and returns a tuple: the CPU-time in seconds
          and the absolute time elapsed since .start() was called
    """

    utime = 0
    atime = 0

    def start(self,

              clock=time.clock, time=time.time):

        self.atime = time()
        self.utime = clock()

    def stop(self,

             clock=time.clock, time=time.time):

        self.utime = clock() - self.utime
        self.atime = time() - self.atime
        return self.utime, self.atime

    def usertime(self,

                 clock=time.clock, time=time.time):

        self.utime = clock() - self.utime
        self.atime = time() - self.atime
        return self.utime

    def abstime(self,

                clock=time.clock, time=time.time):

        self.utime = clock() - self.utime
        self.atime = time() - self.atime
        return self.utime

    def __str__(self):

        return '%0.2fu %0.2fa sec.' % (self.utime, self.atime)


def _bench(file='mxTextTools/mxTextTools.c'):

    def mismatch(orig, new):
        print()
        for i in range(len(orig)):
            if orig[i] != new[i]:
                break
        else:
            print('Length mismatch: orig=%i new=%i' % (len(orig), len(new)))
            if len(orig) > len(new):
                print('Missing chars:' + repr(orig[len(new):]))
            else:
                print('Excess chars:' + repr(new[len(orig):]))
            print()
            return
        print('Mismatch at offset %i:' % i)
        print((orig[i - 100:i]
               + '<- %s != %s ->' % (repr(orig[i]), repr(new[i]))
               + orig[i + 1:i + 100]))
        print()

    text = open(file).read()
    import string

    t = _timer()
    print('Working on a %i byte string' % len(text))

    if 0:
        print()
        print('Replacing strings')
        print('-' * 72)
        print()
        for what, with_what in (('m', 'M'), ('mx', 'MX'), ('mxText', 'MXTEXT'),
                                ('hmm', 'HMM'), ('hmmm', 'HMM'), ('hmhmm', 'HMM')):
            print('Replace "%s" with "%s"' % (what, with_what))
            t.start()
            for i in range(100):
                rtext = text.replace(what, with_what)
            print('with string.replace:', t.stop(), 'sec.')
            t.start()
            for i in range(100):
                ttext = replace(text, what, with_what)
            print('with tag.replace:', t.stop(), 'sec.')
            if ttext != rtext:
                print('results are NOT ok !')
                print('-' * 72)
                mismatch(rtext, ttext)
            t.start()
            for i in range(100):
                ttext = _replace2(text, what, with_what)
            print('with tag._replace2:', t.stop(), 'sec.')
            if ttext != rtext:
                print('results are NOT ok !')
                print('-' * 72)
                print(rtext)
            t.start()
            for i in range(100):
                ttext = _replace3(text, what, with_what)
            print('with tag._replace3:', t.stop(), 'sec.')
            if ttext != rtext:
                print('results are NOT ok !')
                print('-' * 72)
                print(rtext)
            t.start()
            for i in range(100):
                ttext = _replace4(text, what, with_what)
            print('with tag._replace4:', t.stop(), 'sec.')
            if ttext != rtext:
                print('results are NOT ok !')
                print('-' * 72)
                print(rtext)
            print()

    if 0:
        print()
        print('String lower/upper')
        print('-' * 72)
        print()

        op = str.lower
        t.start()
        for i in range(1000):
            op(text)
        t.stop()
        print(' string.lower:', t)

        op = str.upper
        t.start()
        for i in range(1000):
            op(text)
        t.stop()
        print(' string.upper:', t)

        op = upper
        t.start()
        for i in range(1000):
            op(text)
        t.stop()
        print(' TextTools.upper:', t)

        op = lower
        t.start()
        for i in range(1000):
            op(text)
        t.stop()
        print(' TextTools.lower:', t)

        print('Testing...', end=' ')
        ltext = text.lower()
        assert ltext == lower(text)
        utext = text.upper()
        assert utext == upper(text)
        print('ok.')

    if 0:
        print()
        print('Joining lists')
        print('-' * 72)
        print()

        l = setsplit(text, whitespace_set)

        op = str.join
        t.start()
        for i in range(1000):
            op(l)
        t.stop()
        print(' string.join:', t)

        op = join
        t.start()
        for i in range(1000):
            op(l)
        t.stop()
        print(' TextTools.join:', t)

        op = str.join
        t.start()
        for i in range(1000):
            op(l, ' ')
        t.stop()
        print(' string.join with separator:', t)

        op = join
        t.start()
        for i in range(1000):
            op(l, ' ')
        t.stop()
        print(' TextTools.join with separator:', t)

    if 0:
        print()
        print('Creating join lists')
        print('-' * 72)
        print()

        repl = []
        for i in range(0, len(text), 10):
            repl.append((str(i), i, i + 1))

        op = joinlist
        t.start()
        for i in range(1000):
            op(text, repl)
        t.stop()
        print(' TextTools.joinlist:', t)

    if 0:
        print()
        print('Splitting text')
        print('-' * 72)
        print()

        op = str.split
        t.start()
        for i in range(100):
            op(text)
        t.stop()
        print(' string.split whitespace:', t, '(', len(op(text)), 'snippets )')

        op = setsplit
        ws = whitespace_set
        t.start()
        for i in range(100):
            op(text, ws)
        t.stop()
        print(' TextTools.setsplit whitespace:', t,
              '(', len(op(text, ws)), 'snippets )')

        assert text.split() == setsplit(text, ws)

        op = str.split
        sep = 'a'
        t.start()
        for i in range(100):
            op(text, sep)
        t.stop()
        print(' string.split at "a":', t, '(', len(op(text, sep)), 'snippets )')

        op = split
        sep = 'a'
        t.start()
        for i in range(100):
            op(text, sep)
        t.stop()
        print(' TextTools.split at "a":', t,
              '(', len(op(text, sep)), 'snippets )')

        op = charsplit
        sep = 'a'
        t.start()
        for i in range(100):
            op(text, sep)
        t.stop()
        print(' TextTools.charsplit at "a":', t,
              '(', len(op(text, sep)), 'snippets )')

        op = setsplit
        sep = set('a')
        t.start()
        for i in range(100):
            op(text, sep)
        t.stop()
        print(' TextTools.setsplit at "a":', t,
              '(', len(op(text, sep)), 'snippets )')

        # Note: string.split and setsplit don't work identically !

        op = str.split
        sep = 'int'
        t.start()
        for i in range(100):
            op(text, sep)
        t.stop()
        print(' string.split at "int":', t,
              '(', len(op(text, sep)), 'snippets )')

        op = split
        sep = 'int'
        t.start()
        for i in range(100):
            op(text, sep)
        t.stop()
        print(' TextTools.split at "int":', t,
              '(', len(op(text, sep)), 'snippets )')

        op = setsplit
        sep = set('int')
        t.start()
        for i in range(100):
            op(text, sep)
        t.stop()
        print(' TextTools.setsplit at "i", "n", "t":',
              t, '(', len(op(text, sep)), 'snippets )')

        op = str.split
        sep = 'register'
        t.start()
        for i in range(100):
            op(text, sep)
        t.stop()
        print(' string.split at "register":', t,
              '(', len(op(text, sep)), 'snippets )')

        op = split
        sep = 'register'
        t.start()
        for i in range(100):
            op(text, sep)
        t.stop()
        print(' TextTools.split at "register":', t,
              '(', len(op(text, sep)), 'snippets )')


if __name__ == '__main__':
    _bench()
