from __future__ import with_statement, division

import hashlib

try:
    from hashlib import algorithms_available
except ImportError:  # pragma: no cover
    algorithms_available = [
        "md5",
        "sha1",
        "sha224",
        "sha256",
        "sha384",
        "sha512",
    ]
from functools import partial
import pytest
import sys
import hypothesis.strategies as st
from hypothesis import note, assume, given, settings, example

from .keys import SigningKey
from .keys import BadSignatureError
from .util import sigencode_der, sigencode_string
from .util import sigdecode_der, sigdecode_string
from .curves import curves
from .der import (
    encode_integer,
    encode_bitstring,
    encode_octet_string,
    encode_oid,
    encode_sequence,
    encode_constructed,
)
from .ellipticcurve import CurveEdTw


example_data = b"some data to sign"
"""Since the data is hashed for processing, really any string will do."""


hash_and_size = [
    (name, hashlib.new(name).digest_size) for name in algorithms_available
]
"""Pairs of hash names and their output sizes.
Needed for pairing with curves as we don't support hashes
bigger than order sizes of curves."""


keys_and_sigs = []
"""Name of the curve+hash combination, VerifyingKey and DER signature."""


# for hypothesis strategy shrinking we want smallest curves and hashes first
for curve in sorted(curves, key=lambda x: x.baselen):
    for hash_alg in [
        name
        for name, size in sorted(hash_and_size, key=lambda x: x[1])
        if 0 < size <= curve.baselen
    ]:
        sk = SigningKey.generate(
            curve, hashfunc=partial(hashlib.new, hash_alg)
        )

        keys_and_sigs.append(
            (
                "{0} {1}".format(curve, hash_alg),
                sk.verifying_key,
                sk.sign(example_data, sigencode=sigencode_der),
            )
        )


# first make sure that the signatures can be verified
@pytest.mark.parametrize(
    "verifying_key,signature",
    [pytest.param(vk, sig, id=name) for name, vk, sig in keys_and_sigs],
)
def test_signatures(verifying_key, signature):
    assert verifying_key.verify(
        signature, example_data, sigdecode=sigdecode_der
    )


@st.composite
def st_fuzzed_sig(draw, keys_and_sigs):
    """
    Hypothesis strategy that generates pairs of VerifyingKey and malformed
    signatures created by fuzzing of a valid signature.
    """
    name, verifying_key, old_sig = draw(st.sampled_from(keys_and_sigs))
    note("Configuration: {0}".format(name))

    sig = bytearray(old_sig)

    # decide which bytes should be removed
    to_remove = draw(
        st.lists(st.integers(min_value=0, max_value=len(sig) - 1), unique=True)
    )
    to_remove.sort()
    for i in reversed(to_remove):
        del sig[i]
    note("Remove bytes: {0}".format(to_remove))

    # decide which bytes of the original signature should be changed
    if sig:  # pragma: no branch
        xors = draw(
            st.dictionaries(
                st.integers(min_value=0, max_value=len(sig) - 1),
                st.integers(min_value=1, max_value=255),
            )
        )
        for i, val in xors.items():
            sig[i] ^= val
        note("xors: {0}".format(xors))

    # decide where new data should be inserted
    insert_pos = draw(st.integers(min_value=0, max_value=len(sig)))
    # NIST521p signature is about 140 bytes long, test slightly longer
    insert_data = draw(st.binary(max_size=256))

    sig = sig[:insert_pos] + insert_data + sig[insert_pos:]
    note(
        "Inserted at position {0} bytes: {1!r}".format(insert_pos, insert_data)
    )

    sig = bytes(sig)
    # make sure that there was performed at least one mutation on the data
    assume(to_remove or xors or insert_data)
    # and that the mutations didn't cancel each-other out
    assume(sig != old_sig)

    return verifying_key, sig


params = {}
# not supported in hypothesis 2.0.0
if sys.version_info >= (2, 7):  # pragma: no branch
    from hypothesis import HealthCheck

    # deadline=5s because NIST521p are slow to verify
    params["deadline"] = 5000
    params["suppress_health_check"] = [
        HealthCheck.data_too_large,
        HealthCheck.filter_too_much,
        HealthCheck.too_slow,
    ]

slow_params = dict(params)
slow_params["max_examples"] = 10


@settings(**params)
@given(st_fuzzed_sig(keys_and_sigs))
def test_fuzzed_der_signatures(args):
    verifying_key, sig = args

    with pytest.raises(BadSignatureError):
        verifying_key.verify(sig, example_data, sigdecode=sigdecode_der)


@st.composite
def st_random_der_ecdsa_sig_value(draw):
    """
    Hypothesis strategy for selecting random values and encoding them
    to ECDSA-Sig-Value object::

        ECDSA-Sig-Value ::= SEQUENCE {
            r INTEGER,
            s INTEGER
        }
    """
    name, verifying_key, _ = draw(st.sampled_from(keys_and_sigs))
    note("Configuration: {0}".format(name))
    order = int(verifying_key.curve.order)

    # the encode_integer doesn't suport negative numbers, would be nice
    # to generate them too, but we have coverage for remove_integer()
    # verifying that it doesn't accept them, so meh.
    # Test all numbers around the ones that can show up (around order)
    # way smaller and slightly bigger
    r = draw(
        st.integers(min_value=0, max_value=order << 4)
        | st.integers(min_value=order >> 2, max_value=order + 1)
    )
    s = draw(
        st.integers(min_value=0, max_value=order << 4)
        | st.integers(min_value=order >> 2, max_value=order + 1)
    )

    sig = encode_sequence(encode_integer(r), encode_integer(s))

    return verifying_key, sig


@settings(**slow_params)
@given(st_random_der_ecdsa_sig_value())
def test_random_der_ecdsa_sig_value(params):
    """
    Check if random values encoded in ECDSA-Sig-Value structure are rejected
    as signature.
    """
    verifying_key, sig = params

    with pytest.raises(BadSignatureError):
        verifying_key.verify(sig, example_data, sigdecode=sigdecode_der)


def st_der_integer(*args, **kwargs):
    """
    Hypothesis strategy that returns a random positive integer as DER
    INTEGER.
    Parameters are passed to hypothesis.strategy.integer.
    """
    if "min_value" not in kwargs:  # pragma: no branch
        kwargs["min_value"] = 0
    return st.builds(encode_integer, st.integers(*args, **kwargs))


@st.composite
def st_der_bit_string(draw, *args, **kwargs):
    """
    Hypothesis strategy that returns a random DER BIT STRING.
    Parameters are passed to hypothesis.strategy.binary.
    """
    data = draw(st.binary(*args, **kwargs))
    if data:
        unused = draw(st.integers(min_value=0, max_value=7))
        data = bytearray(data)
        data[-1] &= -(2 ** unused)
        data = bytes(data)
    else:
        unused = 0
    return encode_bitstring(data, unused)


def st_der_octet_string(*args, **kwargs):
    """
    Hypothesis strategy that returns a random DER OCTET STRING object.
    Parameters are passed to hypothesis.strategy.binary
    """
    return st.builds(encode_octet_string, st.binary(*args, **kwargs))


def st_der_null():
    """
    Hypothesis strategy that returns DER NULL object.
    """
    return st.just(b"\x05\x00")


@st.composite
def st_der_oid(draw):
    """
    Hypothesis strategy that returns DER OBJECT IDENTIFIER objects.
    """
    first = draw(st.integers(min_value=0, max_value=2))
    if first < 2:
        second = draw(st.integers(min_value=0, max_value=39))
    else:
        second = draw(st.integers(min_value=0, max_value=2 ** 512))
    rest = draw(
        st.lists(st.integers(min_value=0, max_value=2 ** 512), max_size=50)
    )
    return encode_oid(first, second, *rest)


def st_der():
    """
    Hypothesis strategy that returns random DER structures.

    A valid DER structure is any primitive object, an octet encoding
    of a valid DER structure, sequence of valid DER objects or a constructed
    encoding of any of the above.
    """
    return st.recursive(
        st.just(b"")
        | st_der_integer(max_value=2 ** 4096)
        | st_der_bit_string(max_size=1024 ** 2)
        | st_der_octet_string(max_size=1024 ** 2)
        | st_der_null()
        | st_der_oid(),
        lambda children: st.builds(
            lambda x: encode_octet_string(x), st.one_of(children)
        )
        | st.builds(lambda x: encode_bitstring(x, 0), st.one_of(children))
        | st.builds(
            lambda x: encode_sequence(*x), st.lists(children, max_size=200)
        )
        | st.builds(
            lambda tag, x: encode_constructed(tag, x),
            st.integers(min_value=0, max_value=0x3F),
            st.one_of(children),
        ),
        max_leaves=40,
    )


@settings(**params)
@given(st.sampled_from(keys_and_sigs), st_der())
def test_random_der_as_signature(params, der):
    """Check if random DER structures are rejected as signature"""
    name, verifying_key, _ = params

    with pytest.raises(BadSignatureError):
        verifying_key.verify(der, example_data, sigdecode=sigdecode_der)


@settings(**params)
@given(st.sampled_from(keys_and_sigs), st.binary(max_size=1024 ** 2))
@example(
    keys_and_sigs[0], encode_sequence(encode_integer(0), encode_integer(0))
)
@example(
    keys_and_sigs[0],
    encode_sequence(encode_integer(1), encode_integer(1)) + b"\x00",
)
@example(keys_and_sigs[0], encode_sequence(*[encode_integer(1)] * 3))
def test_random_bytes_as_signature(params, der):
    """Check if random bytes are rejected as signature"""
    name, verifying_key, _ = params

    with pytest.raises(BadSignatureError):
        verifying_key.verify(der, example_data, sigdecode=sigdecode_der)


keys_and_string_sigs = [
    (
        name,
        verifying_key,
        sigencode_string(
            *sigdecode_der(sig, verifying_key.curve.order),
            order=verifying_key.curve.order
        ),
    )
    for name, verifying_key, sig in keys_and_sigs
    if not isinstance(verifying_key.curve.curve, CurveEdTw)
]
"""
Name of the curve+hash combination, VerifyingKey and signature as a
byte string.
"""


keys_and_string_sigs += [
    (name, verifying_key, sig,)
    for name, verifying_key, sig in keys_and_sigs
    if isinstance(verifying_key.curve.curve, CurveEdTw)
]


@settings(**params)
@given(st_fuzzed_sig(keys_and_string_sigs))
def test_fuzzed_string_signatures(params):
    verifying_key, sig = params

    with pytest.raises(BadSignatureError):
        verifying_key.verify(sig, example_data, sigdecode=sigdecode_string)
