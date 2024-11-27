# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2019 EfficiOS Inc.
#

# pyright: strict, reportPrivateUsage=false

import math
import typing

import bt2

bt2.register_plugin(__name__, "complete")

_FieldType = typing.TypeVar("_FieldType", bound=bt2._Field)


def c(field: bt2._Field, expected_type: typing.Type[_FieldType]) -> _FieldType:
    assert type(field) is expected_type
    return field


class CompleteSrcIter(bt2._UserMessageIterator):
    def __init__(
        self,
        config: bt2._MessageIteratorConfiguration,
        output_port: bt2._UserComponentOutputPort,
    ):
        ec = typing.cast(bt2._EventClass, output_port.user_data)
        sc = ec.stream_class
        tc = sc.trace_class

        trace = tc()
        stream = trace.create_stream(sc)

        ev = self._create_event_message(ec, stream, default_clock_snapshot=123)

        payload = ev.event.payload_field
        assert payload

        payload["bool"] = False
        payload["real_single"] = 2.0
        payload["real_double"] = math.pi
        payload["int32"] = 121
        payload["int3"] = -1
        payload["int9_hex"] = -92
        payload["uint32"] = 121
        payload["uint61"] = 299792458
        payload["uint5_oct"] = 29

        struct = c(payload["struct"], bt2._StructureField)
        struct["str"] = "Rotisserie St-Hubert"
        struct["option_real"] = math.pi

        payload["string"] = "🎉"
        payload["dyn_array"] = [1.2, 2 / 3, 42.3, math.pi]
        payload["dyn_array_len"] = 4
        payload["dyn_array_with_len"] = [5.2, 5 / 3, 42.5, math.pi * 12]
        payload["sta_array"] = ["🕰", "🦴", " 🎍"]
        payload["option_none"]
        payload["option_some"] = "NORMANDIN"
        payload["option_bool_selector"] = True
        payload["option_bool"] = "Mike's"
        payload["option_int_selector"] = 1
        payload["option_int"] = "Barbies resto bar grill"

        variant = c(payload["variant"], bt2._VariantField)
        variant.selected_option_index = 0
        variant.value = "Couche-Tard"

        mip = self._component._graph_mip_version

        if mip > 0:
            c(payload["static-blob"], bt2._StaticBlobField).data = b"Masbourian"

            dynamic_blob_without_length_field = c(
                payload["dynamic-blob-without-length-field"], bt2._DynamicBlobField
            )
            dynamic_blob_without_length_field.length = 3
            dynamic_blob_without_length_field.data = b"mdr"

            c(
                payload["dynamic-blob-with-length-field-length"],
                bt2._UnsignedIntegerField,
            ).value = 5
            dynamic_blob_with_length_field = c(
                payload["dynamic-blob-with-length-field"],
                bt2._DynamicBlobWithLengthFieldField,
            )
            dynamic_blob_with_length_field.length = 5
            dynamic_blob_with_length_field.data = b"buick"

        self._msgs = [
            self._create_stream_beginning_message(stream),
            ev,
            self._create_stream_end_message(stream),
        ]

    def __next__(self):
        if len(self._msgs) > 0:
            return self._msgs.pop(0)
        else:
            raise StopIteration


@bt2.plugin_component_class
class CompleteSrc(bt2._UserSourceComponent, message_iterator_class=CompleteSrcIter):
    @staticmethod
    def _user_get_supported_mip_versions(
        params: bt2._MapValueConst, obj: object, log_level: bt2.LoggingLevel
    ):
        return [0, 1]

    def __init__(
        self,
        config: bt2._UserSourceComponentConfiguration,
        params: bt2._MapValueConst,
        obj: object,
    ):
        tc = self._create_trace_class()
        cc = self._create_clock_class()
        sc = tc.create_stream_class(default_clock_class=cc)
        mip = self._graph_mip_version

        dyn_array_with_len_fc = tc.create_unsigned_integer_field_class(19)

        if mip == 0:
            dyn_array_fc = tc.create_dynamic_array_field_class(
                tc.create_double_precision_real_field_class(),
                length_fc=dyn_array_with_len_fc,
            )
        else:
            dyn_array_fc = tc.create_dynamic_array_field_class(
                tc.create_double_precision_real_field_class(),
                length_field_location=tc.create_field_location(
                    bt2.FieldLocationScope.EVENT_PAYLOAD, ["dyn_array_len"]
                ),
            )

        variant_fc = (
            tc.create_variant_field_class()
            if mip == 0
            else tc.create_variant_without_selector_field_class()
        )
        variant_fc.append_option(
            name="var_str", field_class=tc.create_string_field_class()
        )

        option_bool_selector_fc = tc.create_bool_field_class()

        if mip == 0:
            option_bool_fc = tc.create_option_with_bool_selector_field_class(
                tc.create_string_field_class(), selector_fc=option_bool_selector_fc
            )
            option_bool_reversed_fc = tc.create_option_with_bool_selector_field_class(
                tc.create_string_field_class(),
                selector_fc=option_bool_selector_fc,
                selector_is_reversed=True,
            )
        else:
            option_bool_selector_fl = tc.create_field_location(
                bt2.FieldLocationScope.EVENT_PAYLOAD, ["option_bool_selector"]
            )
            option_bool_fc = tc.create_option_with_bool_selector_field_class(
                tc.create_string_field_class(),
                selector_field_location=option_bool_selector_fl,
            )
            option_bool_reversed_fc = tc.create_option_with_bool_selector_field_class(
                tc.create_string_field_class(),
                selector_field_location=option_bool_selector_fl,
                selector_is_reversed=True,
            )

        option_int_selector_fc = tc.create_unsigned_integer_field_class(8)
        option_int_ranges = bt2.UnsignedIntegerRangeSet([(1, 3), (18, 44)])

        if mip == 0:
            option_int_fc = tc.create_option_with_integer_selector_field_class(
                tc.create_string_field_class(),
                selector_fc=option_int_selector_fc,
                ranges=option_int_ranges,
            )
        else:
            option_int_fc = tc.create_option_with_unsigned_integer_selector_field_class(
                tc.create_string_field_class(),
                selector_field_location=tc.create_field_location(
                    bt2.FieldLocationScope.EVENT_PAYLOAD, ["option_int_selector"]
                ),
                ranges=option_int_ranges,
            )

        payload = tc.create_structure_field_class(
            members=(
                ("bool", tc.create_bool_field_class()),
                ("real_single", tc.create_single_precision_real_field_class()),
                ("real_double", tc.create_double_precision_real_field_class()),
                ("int32", tc.create_signed_integer_field_class(32)),
                ("int3", tc.create_signed_integer_field_class(3)),
                (
                    "int9_hex",
                    tc.create_signed_integer_field_class(
                        9,
                        preferred_display_base=bt2.IntegerDisplayBase.HEXADECIMAL,
                    ),
                ),
                ("uint32", tc.create_unsigned_integer_field_class(32)),
                ("uint61", tc.create_unsigned_integer_field_class(61)),
                (
                    "uint5_oct",
                    tc.create_unsigned_integer_field_class(
                        5, preferred_display_base=bt2.IntegerDisplayBase.OCTAL
                    ),
                ),
                (
                    "struct",
                    tc.create_structure_field_class(
                        members=(
                            ("str", tc.create_string_field_class()),
                            (
                                "option_real",
                                tc.create_option_without_selector_field_class(
                                    tc.create_double_precision_real_field_class()
                                ),
                            ),
                        )
                    ),
                ),
                ("string", tc.create_string_field_class()),
                (
                    "dyn_array",
                    tc.create_dynamic_array_field_class(
                        tc.create_double_precision_real_field_class()
                    ),
                ),
                ("dyn_array_len", dyn_array_with_len_fc),
                (
                    "dyn_array_with_len",
                    dyn_array_fc,
                ),
                (
                    "sta_array",
                    tc.create_static_array_field_class(
                        tc.create_string_field_class(), 3
                    ),
                ),
                (
                    "option_none",
                    tc.create_option_without_selector_field_class(
                        tc.create_double_precision_real_field_class()
                    ),
                ),
                (
                    "option_some",
                    tc.create_option_without_selector_field_class(
                        tc.create_string_field_class()
                    ),
                ),
                ("option_bool_selector", option_bool_selector_fc),
                ("option_bool", option_bool_fc),
                (
                    "option_bool_reversed",
                    option_bool_reversed_fc,
                ),
                ("option_int_selector", option_int_selector_fc),
                (
                    "option_int",
                    option_int_fc,
                ),
                ("variant", variant_fc),
            )
        )

        if mip > 0:
            payload += (
                (
                    "static-blob",
                    tc.create_static_blob_field_class(
                        10, media_type="application/x-gameboy-rom"
                    ),
                ),
                (
                    "dynamic-blob-without-length-field",
                    tc.create_dynamic_blob_field_class(
                        media_type="application/x-shockwave-flash"
                    ),
                ),
                (
                    "dynamic-blob-with-length-field-length",
                    tc.create_unsigned_integer_field_class(8),
                ),
                (
                    "dynamic-blob-with-length-field",
                    tc.create_dynamic_blob_field_class(
                        length_field_location=tc.create_field_location(
                            bt2.FieldLocationScope.EVENT_PAYLOAD,
                            ["dynamic-blob-with-length-field-length"],
                        ),
                        media_type="application/x-shockwave-flash",
                    ),
                ),
            )

        ec = sc.create_event_class(name="my-event", payload_field_class=payload)
        self._add_output_port("some-name", ec)
