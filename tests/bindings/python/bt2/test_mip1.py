import typing
import unittest

import bt2

# pyright: strict,  reportPrivateUsage=false


T = typing.TypeVar("T")


# Assert that `obj` is of type `expected_type` and return it as a `T`.
def c(obj: object, expected_type: typing.Type[T]) -> T:
    assert type(obj) is expected_type
    return obj


class TheSourceIterator(bt2._UserMessageIterator):
    def __init__(
        self,
        config: bt2._MessageIteratorConfiguration,
        port: bt2._UserComponentOutputPort,
    ):
        tc = self._component._create_trace_class()
        t = tc(
            namespace="the-trace-namespace", name="the-trace-name", uid="the-trace-uid"
        )

        cc = self._component._create_clock_class(
            origin=bt2.ClockClassOrigin(
                "the-clock-class-origin-namespace",
                "the-clock-class-origin-name",
                "the-clock-class-origin-uid",
            ),
            namespace="the-clock-class-namespace",
            name="the-clock-class-name",
            uid="the-clock-class-uid",
            precision=12,
            accuracy=34,
        )

        sc = tc.create_stream_class(
            namespace="the-stream-class-namespace",
            name="the-stream-class-name",
            uid="the-stream-class-uid",
            default_clock_class=cc,
        )
        s = t.create_stream(sc)

        ec = sc.create_event_class(
            namespace="the-event-class-namespace",
            name="the-event-class-name",
            uid="the-event-class-uid",
            payload_field_class=self._create_payload_fc(tc),
        )
        em = self._create_event_message(ec, s, default_clock_snapshot=100)
        e = em.event
        p = e.payload_field
        assert p is not None
        self._fill_payload_field(p)

        self._msgs = [
            self._create_stream_beginning_message(s),
            em,
            self._create_stream_end_message(s),
        ]

    def _create_payload_fc(self, tc: bt2._TraceClass):
        payload_fc = tc.create_structure_field_class()

        # Static blob
        payload_fc.append_member(
            "static-blob",
            tc.create_static_blob_field_class(4, media_type="application/vnd.rar"),
        )

        # Dynamic blob witout length field
        payload_fc.append_member(
            "dynamic-blob-without-length-field",
            tc.create_dynamic_blob_field_class(),
        )

        # Dynamic blob with length field
        payload_fc.append_member(
            "dynamic-blob-with-length-field-length",
            tc.create_unsigned_integer_field_class(),
        )
        payload_fc.append_member(
            "dynamic-blob-with-length-field",
            tc.create_dynamic_blob_field_class(
                length_field_location=tc.create_field_location(
                    bt2.FieldLocationScope.EVENT_PAYLOAD,
                    ["dynamic-blob-with-length-field-length"],
                )
            ),
        )

        # Bit array (with flags)
        bit_array_fc = tc.create_bit_array_field_class(16)
        bit_array_fc.add_flag("flag1", bt2.UnsignedIntegerRangeSet([0]))
        bit_array_fc += [
            ("flag2", bt2.UnsignedIntegerRangeSet([4, (12, 15)])),
            ("flag3", bt2.UnsignedIntegerRangeSet([(4, 7)])),
        ]
        payload_fc.append_member("bit-array", bit_array_fc)

        # Option without selector field
        payload_fc.append_member(
            "option-without-selector-field",
            tc.create_option_without_selector_field_class(
                tc.create_string_field_class()
            ),
        )

        # Option with bool selector field
        payload_fc.append_member(
            "option-with-bool-selector-field-selector", tc.create_bool_field_class()
        )
        payload_fc.append_member(
            "option-with-bool-selector-field",
            tc.create_option_with_bool_selector_field_class(
                tc.create_string_field_class(),
                selector_field_location=tc.create_field_location(
                    bt2.FieldLocationScope.EVENT_PAYLOAD,
                    ["option-with-bool-selector-field-selector"],
                ),
            ),
        )

        # Option with unsigned integer selector field
        payload_fc.append_member(
            "option-with-unsigned-integer-selector-field-selector",
            tc.create_unsigned_integer_field_class(8),
        )
        payload_fc.append_member(
            "option-with-unsigned-integer-selector-field",
            tc.create_option_with_unsigned_integer_selector_field_class(
                tc.create_string_field_class(),
                selector_field_location=tc.create_field_location(
                    bt2.FieldLocationScope.EVENT_PAYLOAD,
                    ["option-with-unsigned-integer-selector-field-selector"],
                ),
                ranges=bt2.UnsignedIntegerRangeSet([1]),
            ),
        )

        # Option with signed integer selector field
        payload_fc.append_member(
            "option-with-signed-integer-selector-field-selector",
            tc.create_signed_integer_field_class(8),
        )
        payload_fc.append_member(
            "option-with-signed-integer-selector-field",
            tc.create_option_with_signed_integer_selector_field_class(
                tc.create_string_field_class(),
                selector_field_location=tc.create_field_location(
                    bt2.FieldLocationScope.EVENT_PAYLOAD,
                    ["option-with-signed-integer-selector-field-selector"],
                ),
                ranges=bt2.SignedIntegerRangeSet([1]),
            ),
        )

        # Variant without selector field location
        variant_without_selector_field_fc = (
            tc.create_variant_without_selector_field_class()
        )
        variant_without_selector_field_fc.append_option(
            None, tc.create_single_precision_real_field_class()
        )
        variant_without_selector_field_fc.append_option(
            "str", tc.create_string_field_class()
        )
        payload_fc.append_member(
            "variant-without-selector-field",
            variant_without_selector_field_fc,
        )

        # Variant with unsigned integer selector field location
        payload_fc.append_member(
            "variant-with-unsigned-integer-selector-field-selector",
            tc.create_unsigned_integer_field_class(8),
        )
        variant_with_unsigned_integer_selector_field_fc = (
            tc.create_variant_with_unsigned_selector_field_class(
                selector_field_location=tc.create_field_location(
                    bt2.FieldLocationScope.EVENT_PAYLOAD,
                    ["variant-with-unsigned-integer-selector-field-selector"],
                ),
            )
        )
        variant_with_unsigned_integer_selector_field_fc.append_option(
            None,
            tc.create_single_precision_real_field_class(),
            bt2.UnsignedIntegerRangeSet([0]),
        )
        variant_with_unsigned_integer_selector_field_fc.append_option(
            "str", tc.create_string_field_class(), bt2.UnsignedIntegerRangeSet([1])
        )
        payload_fc.append_member(
            "variant-with-unsigned-integer-selector-field",
            variant_with_unsigned_integer_selector_field_fc,
        )

        # Variant with signed integer selector field location
        payload_fc.append_member(
            "variant-with-signed-integer-selector-field-selector",
            tc.create_signed_integer_field_class(8),
        )
        variant_with_signed_integer_selector_field_fc = (
            tc.create_variant_with_signed_selector_field_class(
                selector_field_location=tc.create_field_location(
                    bt2.FieldLocationScope.EVENT_PAYLOAD,
                    ["variant-with-signed-integer-selector-field-selector"],
                ),
            )
        )
        variant_with_signed_integer_selector_field_fc.append_option(
            None,
            tc.create_single_precision_real_field_class(),
            bt2.SignedIntegerRangeSet([0]),
        )
        variant_with_signed_integer_selector_field_fc.append_option(
            "str", tc.create_string_field_class(), bt2.SignedIntegerRangeSet([1])
        )
        payload_fc.append_member(
            "variant-with-signed-integer-selector-field",
            variant_with_signed_integer_selector_field_fc,
        )

        return payload_fc

    def _fill_payload_field(self, p: bt2._StructureField):
        # Static blob
        static_blob = c(p["static-blob"], bt2._StaticBlobField)
        static_blob.data = b"abcd"
        static_blob.data[2] = 0x65  # e

        # Dynamic blob without length field
        dynamic_blob_without_length_field = c(
            p["dynamic-blob-without-length-field"], bt2._DynamicBlobField
        )
        dynamic_blob_without_length_field.length = 5
        dynamic_blob_without_length_field.data = b"salut"

        # Dynamic blob with length field
        c(
            p["dynamic-blob-with-length-field-length"],
            bt2._UnsignedIntegerField,
        ).value = 7
        dynamic_blob_with_length_field = c(
            p["dynamic-blob-with-length-field"],
            bt2._DynamicBlobWithLengthFieldField,
        )
        dynamic_blob_with_length_field.length = 7
        dynamic_blob_with_length_field.data = b"bonjour"

        # Bit array
        c(
            p["bit-array"],
            bt2._BitArrayField,
        ).value_as_integer = 0b10000

        # Option without selector field
        c(
            p["option-without-selector-field"],
            bt2._OptionField,
        ).value = "hi"

        # Option with bool selector field
        c(
            p["option-with-bool-selector-field-selector"],
            bt2._BoolField,
        ).value = True
        c(
            p["option-with-bool-selector-field"],
            bt2._OptionWithBoolSelectorField,
        ).value = "hello"

        # Option with unsigned integer selector field
        c(
            p["option-with-unsigned-integer-selector-field-selector"],
            bt2._UnsignedIntegerField,
        ).value = 1
        c(
            p["option-with-unsigned-integer-selector-field"],
            bt2._OptionWithUnsignedIntegerSelectorField,
        ).value = "world"

        # Option with signed integer selector field
        c(
            p["option-with-signed-integer-selector-field-selector"],
            bt2._SignedIntegerField,
        ).value = 1
        c(
            p["option-with-signed-integer-selector-field"],
            bt2._OptionWithSignedIntegerSelectorField,
        ).value = "option-with-signed-integer-selector-field-value"

        # Variant without selector field
        c(
            p["variant-without-selector-field"],
            bt2._VariantField,
        ).selected_option_index = 1
        c(
            p["variant-without-selector-field"],
            bt2._VariantField,
        ).value = "variant-without-selector-field-value"

        # Variant with unsigned integer selector field
        c(
            p["variant-with-unsigned-integer-selector-field-selector"],
            bt2._UnsignedIntegerField,
        ).value = 1
        variant_field_with_unsigned_integer_selector_field = c(
            p["variant-with-unsigned-integer-selector-field"],
            bt2._VariantFieldWithUnsignedIntegerSelector,
        )
        variant_field_with_unsigned_integer_selector_field.selected_option_index = 1
        variant_field_with_unsigned_integer_selector_field.value = (
            "variant-with-unsigned-integer-selector-field-value"
        )

        # Variant with signed integer selector field
        c(
            p["variant-with-signed-integer-selector-field-selector"],
            bt2._SignedIntegerField,
        ).value = 1
        variant_field_with_signed_integer_selector_field = c(
            p["variant-with-signed-integer-selector-field"],
            bt2._VariantFieldWithSignedIntegerSelector,
        )
        variant_field_with_signed_integer_selector_field.selected_option_index = 1
        variant_field_with_signed_integer_selector_field.value = (
            "variant-with-signed-integer-selector-field-value"
        )

    def __next__(self):
        if len(self._msgs) == 0:
            raise StopIteration

        return self._msgs.pop(0)


class TheSource(bt2._UserSourceComponent, message_iterator_class=TheSourceIterator):
    def __init__(
        self,
        config: bt2._UserSourceComponentConfiguration,
        params: bt2._MapValueConst,
        obj: object,
    ):
        self._add_output_port("out")


class TheSink(bt2._UserSinkComponent):
    def __init__(
        self,
        config: bt2._UserSinkComponentConfiguration,
        params: bt2._MapValueConst,
        obj: object,
    ):
        self._in = self._add_input_port("in")
        self._test = typing.cast(unittest.TestCase, obj)

    def _user_graph_is_configured(self):
        self._iter = self._create_message_iterator(self._in)

    def _test_blob(
        self,
        static_blob: bt2._StaticBlobFieldConst,
        dynamic_blob_without_length_field: bt2._DynamicBlobFieldConst,
        dynamic_blob_with_length_field: bt2._DynamicBlobWithLengthFieldFieldConst,
    ):
        # Test `_BlobFieldClassConst.media_type`
        self._test.assertEqual(static_blob.cls.media_type, "application/vnd.rar")

        # Test `_StaticBlobFieldClassConst.length`
        self._test.assertEqual(static_blob.cls.length, 4)

        # Test `_DynamicBlobWithLengthFieldFieldClassConst.length_field_location`
        fl = dynamic_blob_with_length_field.cls.length_field_location
        self._test.assertEqual(fl.root_scope, bt2.FieldLocationScope.EVENT_PAYLOAD)

        # Test `_FieldLocationConst.__len__`
        self._test.assertEqual(len(fl), 1)

        # Test `_FieldLocationConst.__iter__`
        self._test.assertEqual(list(fl), ["dynamic-blob-with-length-field-length"])

        self._test.assertEqual(static_blob.data, b"abed")
        self._test.assertEqual(len(static_blob), 4)

        self._test.assertEqual(dynamic_blob_without_length_field.data, b"salut")
        self._test.assertEqual(dynamic_blob_without_length_field.length, 5)

    def _test_bit_array(self, bit_array: bt2._BitArrayFieldConst):
        bit_array_fc = bit_array.cls
        self._test.assertIsInstance(bit_array_fc, bt2._BitArrayFieldClassConst)

        # Test `_BitArrayFieldClassConst.length`
        self._test.assertEqual(bit_array_fc.length, 16)

        # Test `_BitArrayFieldClassConst.active_flag_labels_for_value_as_integer`
        self._test.assertEqual(
            bit_array_fc.active_flag_labels_for_value_as_integer(0b1), ["flag1"]
        )
        self._test.assertEqual(
            bit_array_fc.active_flag_labels_for_value_as_integer(0b100000000), []
        )

        # Test `_BitArrayFieldClassConst.flags`
        flags = bit_array_fc.flags
        self._test.assertIsInstance(flags, bt2._BitArrayFlags)

        # Test `_BitArrayFlags.__len__`
        self._test.assertEqual(len(flags), 3)

        def flag_to_tuple(flag: bt2._BitArrayFlag):
            self._test.assertIsInstance(flag.label, str)
            self._test.assertIsInstance(flag.ranges, bt2._UnsignedIntegerRangeSetConst)
            return (flag.label, [(rng.lower, rng.upper) for rng in flag.ranges])

        def flags_to_list(flags: bt2._BitArrayFlags):
            return [flag_to_tuple(flag) for flag in flags]

        # Test `_BitArrayFlags.__iter__`, `_BitArrayFlag.label` and
        # `_BitArrayFlag.ranges`
        self._test.assertEqual(
            flags_to_list(flags),
            [
                ("flag1", [(0, 0)]),
                ("flag2", [(4, 4), (12, 15)]),
                ("flag3", [(4, 7)]),
            ],
        )

        # Test `_BitArrayFlags.__getitem__`
        self._test.assertEqual(flag_to_tuple(flags["flag1"]), ("flag1", [(0, 0)]))
        self._test.assertEqual(
            flag_to_tuple(flags["flag2"]), ("flag2", [(4, 4), (12, 15)])
        )
        self._test.assertEqual(flag_to_tuple(flags["flag3"]), ("flag3", [(4, 7)]))

        # Test `_BitArrayFieldConst.active_flag_labels`
        self._test.assertEqual(bit_array.active_flag_labels, ["flag2", "flag3"])

    def _test_option_with_bool_selector_field(
        self, field: bt2._OptionWithBoolSelectorFieldConst
    ):
        sfl = field.cls.selector_field_location
        self._test.assertEqual(sfl.root_scope, bt2.FieldLocationScope.EVENT_PAYLOAD)
        self._test.assertEqual(list(sfl), ["option-with-bool-selector-field-selector"])

    def _test_option_with_unsigned_integer_selector_field(
        self, field: bt2._OptionWithUnsignedIntegerSelectorFieldConst
    ):
        sfl = field.cls.selector_field_location
        self._test.assertEqual(sfl.root_scope, bt2.FieldLocationScope.EVENT_PAYLOAD)
        self._test.assertEqual(
            list(sfl), ["option-with-unsigned-integer-selector-field-selector"]
        )

    def _test_option_with_signed_integer_selector_field(
        self, field: bt2._OptionWithSignedIntegerSelectorFieldConst
    ):
        sfl = field.cls.selector_field_location
        self._test.assertEqual(sfl.root_scope, bt2.FieldLocationScope.EVENT_PAYLOAD)
        self._test.assertEqual(
            list(sfl), ["option-with-signed-integer-selector-field-selector"]
        )

    def _test_variant_without_selector_field(self, field: bt2._VariantFieldConst):
        self._test.assertEqual(field.selected_option_index, 1)
        self._test.assertIs(type(field.selected_option), bt2._StringFieldConst)
        self._test.assertEqual(
            field.selected_option, "variant-without-selector-field-value"
        )
        self._test.assertEqual(list(field.cls), [None, "str"])
        self._test.assertIsNone(field.cls.option_at_index(0).name, None)
        self._test.assertEqual(field.cls.option_at_index(1).name, "str")

    def _test_variant_with_unsigned_integer_selector_field(
        self, field: bt2._VariantField
    ):
        self._test.assertEqual(field.selected_option_index, 1)

    def _user_consume(self):
        msg = next(self._iter)

        if type(msg) is bt2._StreamBeginningMessageConst:
            s = msg.stream
            t = s.trace
            sc = s.cls

            self._test.assertEqual(t.namespace, "the-trace-namespace")
            self._test.assertEqual(t.name, "the-trace-name")
            self._test.assertEqual(t.uid, "the-trace-uid")

            cc = sc.default_clock_class
            assert cc is not None
            self._test.assertEqual(cc.namespace, "the-clock-class-namespace")
            self._test.assertEqual(cc.name, "the-clock-class-name")
            self._test.assertEqual(cc.uid, "the-clock-class-uid")
            self._test.assertEqual(cc.opt_precision, 12)
            self._test.assertEqual(cc.accuracy, 34)

            origin = cc.origin
            self._test.assertEqual(origin.namespace, "the-clock-class-origin-namespace")
            self._test.assertEqual(origin.name, "the-clock-class-origin-name")
            self._test.assertEqual(origin.uid, "the-clock-class-origin-uid")

            sc = msg.stream.cls
            self._test.assertEqual(sc.namespace, "the-stream-class-namespace")
            self._test.assertEqual(sc.name, "the-stream-class-name")
            self._test.assertEqual(sc.uid, "the-stream-class-uid")

            ec = sc[0]
            self._test.assertEqual(ec.namespace, "the-event-class-namespace")
            self._test.assertEqual(ec.name, "the-event-class-name")
            self._test.assertEqual(ec.uid, "the-event-class-uid")
        elif type(msg) is bt2._EventMessageConst:
            p = msg.event.payload_field
            assert p is not None

            self._test_blob(
                c(p["static-blob"], bt2._StaticBlobFieldConst),
                c(
                    p["dynamic-blob-without-length-field"],
                    bt2._DynamicBlobFieldConst,
                ),
                c(
                    p["dynamic-blob-with-length-field"],
                    bt2._DynamicBlobWithLengthFieldFieldConst,
                ),
            )
            self._test_bit_array(c(p["bit-array"], bt2._BitArrayFieldConst))
            self._test_option_with_bool_selector_field(
                c(
                    p["option-with-bool-selector-field"],
                    bt2._OptionWithBoolSelectorFieldConst,
                )
            )
            self._test_option_with_unsigned_integer_selector_field(
                c(
                    p["option-with-unsigned-integer-selector-field"],
                    bt2._OptionWithUnsignedIntegerSelectorFieldConst,
                )
            )
            self._test_option_with_signed_integer_selector_field(
                c(
                    p["option-with-signed-integer-selector-field"],
                    bt2._OptionWithSignedIntegerSelectorFieldConst,
                )
            )
            self._test_variant_without_selector_field(
                c(p["variant-without-selector-field"], bt2._VariantFieldConst)
            )


class TestMIP1(unittest.TestCase):
    def test_everything(self):
        g = bt2.Graph(1)
        src = g.add_component(TheSource, "the-source")
        snk = g.add_component(TheSink, "the-sink", None, self)
        g.connect_ports(src.output_ports["out"], snk.input_ports["in"])
        g.run()


if __name__ == "__main__":
    unittest.main()
