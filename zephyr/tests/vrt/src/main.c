/*
 * SPDX-License-Identifier: MIT
 *
 * libvrt smoke test: write and read back IF data, IF context, VITA 49.2 signal data and command packets.
 */

#include <zephyr/ztest.h>

#include <vrt/vrt_init.h>
#include <vrt/vrt_read.h>
#include <vrt/vrt_types.h>
#include <vrt/vrt_util.h>
#include <vrt/vrt_words.h>
#include <vrt/vrt_write.h>

#define BODY_WORDS 8

ZTEST(libvrt, test_if_data_roundtrip)
{
	uint32_t body[BODY_WORDS], buf[32];
	struct vrt_packet p, r;
	int32_t words;

	for (int i = 0; i < BODY_WORDS; i++) {
		body[i] = 0x01020304U * (uint32_t)(i + 1);
	}
	vrt_init_packet(&p);
	p.header.packet_type = VRT_PT_IF_DATA_WITH_STREAM_ID;
	p.header.has.class_id = true;
	p.header.has.trailer = true;
	p.header.tsi = VRT_TSI_UTC;
	p.header.tsf = VRT_TSF_SAMPLE_COUNT;
	p.header.packet_count = 9;
	p.fields.stream_id = 0x5d5d0001U;
	p.fields.class_id.oui = 0x00ABCDEFU;
	p.fields.class_id.information_class_code = 0x1234U;
	p.fields.class_id.packet_class_code = 0x5678U;
	p.fields.integer_seconds_timestamp = 1759622400U;
	p.fields.fractional_seconds_timestamp = 79999999U;
	p.body = body;
	p.words_body = BODY_WORDS;
	p.trailer.has.valid_data = true;
	p.trailer.valid_data = true;

	words = vrt_words_packet(&p);
	zassert_true(words > 0 && words <= (int32_t)ARRAY_SIZE(buf), "words %d", words);
	p.header.packet_size = (uint16_t)words;

	zassert_equal(vrt_write_packet(&p, buf, ARRAY_SIZE(buf), true), words);
	zassert_equal(vrt_read_packet(buf, ARRAY_SIZE(buf), &r, true), words);

	zassert_equal(r.header.packet_type, VRT_PT_IF_DATA_WITH_STREAM_ID);
	zassert_equal(r.header.packet_count, 9);
	zassert_equal(r.header.packet_size, words);
	zassert_equal(r.fields.stream_id, p.fields.stream_id);
	zassert_equal(r.fields.class_id.oui, p.fields.class_id.oui);
	zassert_equal(r.fields.integer_seconds_timestamp, p.fields.integer_seconds_timestamp);
	zassert_equal(r.fields.fractional_seconds_timestamp,
		      p.fields.fractional_seconds_timestamp);
	zassert_equal(r.words_body, BODY_WORDS);
	zassert_mem_equal(r.body, body, sizeof(body));
	zassert_true(r.trailer.has.valid_data && r.trailer.valid_data);
}

ZTEST(libvrt, test_if_context_roundtrip)
{
	uint32_t buf[64];
	struct vrt_packet p, r;
	int32_t words;

	vrt_init_packet(&p);
	p.header.packet_type = VRT_PT_IF_CONTEXT;
	p.header.tsi = VRT_TSI_UTC;
	p.header.tsf = VRT_TSF_REAL_TIME;
	p.fields.stream_id = 0x5d5d0001U;
	p.if_context.has.bandwidth = true;
	p.if_context.bandwidth = 20e6;
	p.if_context.has.rf_reference_frequency = true;
	p.if_context.rf_reference_frequency = 2412e6;
	p.if_context.has.sample_rate = true;
	p.if_context.sample_rate = 80e6;
	p.if_context.has.reference_level = true;
	p.if_context.reference_level = -10.5F;

	words = vrt_words_packet(&p);
	zassert_true(words > 0 && words <= (int32_t)ARRAY_SIZE(buf), "words %d", words);
	p.header.packet_size = (uint16_t)words;

	zassert_equal(vrt_write_packet(&p, buf, ARRAY_SIZE(buf), true), words);
	zassert_equal(vrt_read_packet(buf, ARRAY_SIZE(buf), &r, true), words);

	zassert_equal(r.header.packet_type, VRT_PT_IF_CONTEXT);
	zassert_true(r.if_context.has.bandwidth && r.if_context.has.rf_reference_frequency &&
		     r.if_context.has.sample_rate && r.if_context.has.reference_level);
	zassert_equal(r.if_context.bandwidth, 20e6);
	zassert_equal(r.if_context.rf_reference_frequency, 2412e6);
	zassert_equal(r.if_context.sample_rate, 80e6);
	zassert_equal(r.if_context.reference_level, -10.5F);
}

/* Packets and command sections are static: together they exceed the ztest thread stack */
static struct vrt_packet p49, r49;
static struct vrt_command c49, rc49;

ZTEST(libvrt, test_vita49_2_signal_data_nd0)
{
	uint32_t body[BODY_WORDS], buf[32];
	struct vrt_packet *const p = &p49, *const r = &r49;
	int32_t words;

	for (int i = 0; i < BODY_WORDS; i++) {
		body[i] = 0x00010001U * (uint32_t)i;
	}
	vrt_init_packet(p);
	p->header.packet_type = VRT_PT_IF_DATA_WITH_STREAM_ID;
	p->header.has.trailer = true;
	p->header.nd0 = true;
	p->header.spectrum_or_time = VRT_SOT_SPECTRUM;
	p->fields.stream_id = 0x5d5d0002U;
	p->body = body;
	p->words_body = BODY_WORDS;
	p->trailer.has.valid_data = true;
	p->trailer.valid_data = true;

	words = vrt_words_packet(p);
	zassert_equal(words, 1 + 1 + BODY_WORDS + 1);
	zassert_equal(vrt_write_packet(p, buf, ARRAY_SIZE(buf), true), words);
	/* Type 1, trailer (26), Nd0 (25), spectrum (24) */
	zassert_equal(buf[0], 0x1700000bU, "header 0x%08x", buf[0]);

	zassert_equal(vrt_read_packet(buf, words, r, true), words);
	zassert_true(r->header.nd0);
	zassert_equal(r->header.spectrum_or_time, VRT_SOT_SPECTRUM);
	zassert_true(r->header.has.trailer && r->trailer.valid_data);
	zassert_mem_equal(r->body, body, sizeof(body));
}

ZTEST(libvrt, test_vita49_2_command_roundtrip)
{
	uint32_t buf[32];
	struct vrt_packet *const p = &p49, *const r = &r49;
	struct vrt_command *const c = &c49, *const rc = &rc49;
	int32_t words;

	/* Control packet: set RF frequency, request an execution acknowledge with warnings and errors */
	vrt_init_packet(p);
	vrt_init_command(c);
	p->command = c;
	p->header.packet_type = VRT_PT_COMMAND;
	p->fields.stream_id = 0x5d5d0001U;
	c->cam.controllee_enable = true;
	c->cam.action_mode = VRT_AM_EXECUTE;
	c->cam.permit_warnings = true;
	c->cam.permit_errors = true;
	c->cam.execution = true;
	c->cam.warnings = true;
	c->cam.errors = true;
	c->message_id = 1;
	c->controllee_id = 0x5d5d0001U;
	p->if_context.has.rf_reference_frequency = true;
	p->if_context.rf_reference_frequency = 2412e6;

	words = vrt_words_packet(p);
	zassert_equal(words, 8);
	zassert_equal(vrt_write_packet(p, buf, ARRAY_SIZE(buf), true), words);
	/* Same words as SoapyVITA49 Session::setField builds */
	zassert_equal(buf[0], 0x60000008U, "header 0x%08x", buf[0]);
	zassert_equal(buf[2], 0x870b0000U, "cam 0x%08x", buf[2]);
	zassert_equal(buf[5], 0x08000000U, "cif0 0x%08x", buf[5]);

	vrt_init_packet(r);
	r->command = rc;
	zassert_equal(vrt_read_packet(buf, words, r, true), words);
	zassert_equal(r->header.packet_type, VRT_PT_COMMAND);
	zassert_false(r->header.acknowledge);
	zassert_equal(rc->cam.action_mode, VRT_AM_EXECUTE);
	zassert_true(rc->cam.execution && rc->cam.warnings && rc->cam.errors);
	zassert_equal(rc->message_id, 1);
	zassert_equal(rc->controllee_id, 0x5d5d0001U);
	zassert_true(r->if_context.has.rf_reference_frequency);
	zassert_equal(r->if_context.rf_reference_frequency, 2412e6);

	/* Execution acknowledge with an error response for the RF frequency */
	vrt_init_packet(p);
	vrt_init_command(c);
	p->command = c;
	p->header.packet_type = VRT_PT_COMMAND;
	p->header.acknowledge = true;
	p->fields.stream_id = 0x5d5d0001U;
	c->cam.execution = true;
	c->cam.errors = true;
	c->message_id = 1;
	c->errors.has.rf_reference_frequency = true;
	c->errors.rf_reference_frequency = VRT_WEF_PARAMETER_OUT_OF_RANGE;

	words = vrt_words_packet(p);
	zassert_equal(words, 6);
	zassert_equal(vrt_write_packet(p, buf, ARRAY_SIZE(buf), true), words);
	zassert_equal(buf[0], 0x64000006U, "header 0x%08x", buf[0]);
	zassert_equal(buf[2], 0x00090000U, "cam 0x%08x", buf[2]);
	zassert_equal(buf[4], 0x08000000U, "eif0 0x%08x", buf[4]);
	zassert_equal(buf[5], VRT_WEF_PARAMETER_OUT_OF_RANGE);

	/* On the wire in network order, and back */
	vrt_to_big_endian(buf, words);
	zassert_equal(((const uint8_t *)buf)[0], 0x64);
	zassert_equal(((const uint8_t *)buf)[3], 0x06);
	zassert_equal(((const uint8_t *)buf)[20], 0x10);
	vrt_from_big_endian(buf, words);
	zassert_equal(buf[0], 0x64000006U);

	vrt_init_packet(r);
	r->command = rc;
	zassert_equal(vrt_read_packet(buf, words, r, true), words);
	zassert_true(r->header.acknowledge);
	zassert_true(rc->cam.execution && rc->cam.errors);
	zassert_true(rc->errors.has.rf_reference_frequency);
	zassert_equal(rc->errors.rf_reference_frequency, VRT_WEF_PARAMETER_OUT_OF_RANGE);
}

ZTEST_SUITE(libvrt, NULL, NULL, NULL, NULL, NULL);
