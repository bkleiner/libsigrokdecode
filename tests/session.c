/*
 * This file is part of the libsigrokdecode project.
 *
 * Copyright (C) 2013 Uwe Hermann <uwe@hermann-uwe.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <libsigrokdecode-internal.h> /* First, to avoid compiler warning. */
#include <libsigrokdecode.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include "lib.h"

struct match_result {
	uint64_t sample;
	char *matched;
};

static void match_output_callback(struct srd_proto_data *pdata, void *cb_data)
{
	GArray *results;
	struct match_result result;
	struct srd_proto_data_annotation *annotation;

	results = cb_data;
	annotation = pdata->data;
	result.sample = pdata->start_sample;
	result.matched = g_strdup(annotation->ann_text[0]);
	g_array_append_val(results, result);
}

static void match_results_clear(GArray *results)
{
	struct match_result *result;
	guint i;

	for (i = 0; i < results->len; i++) {
		result = &g_array_index(results, struct match_result, i);
		g_free(result->matched);
	}
	g_array_free(results, TRUE);
}

/*
 * Check whether srd_session_new() works.
 * If it returns != SRD_OK (or segfaults) this test will fail.
 */
START_TEST(test_session_new)
{
	int ret;
	struct srd_session *sess;

	srd_init(NULL);
	ret = srd_session_new(&sess);
	fail_unless(ret == SRD_OK, "srd_session_new() failed: %d.", ret);
	srd_exit();
}
END_TEST

/*
 * Check whether srd_session_new() fails for bogus parameters.
 * If it returns SRD_OK (or segfaults) this test will fail.
 */
START_TEST(test_session_new_bogus)
{
	int ret;

	srd_init(NULL);
	ret = srd_session_new(NULL);
	fail_unless(ret != SRD_OK, "srd_session_new(NULL) worked.");
	srd_exit();
}
END_TEST

/*
 * Check whether multiple srd_session_new() calls work.
 * If any call returns != SRD_OK (or segfaults) this test will fail.
 */
START_TEST(test_session_new_multiple)
{
	int ret;
	struct srd_session *sess1, *sess2, *sess3;

	sess1 = sess2 = sess3 = NULL;

	srd_init(NULL);

	/* Multiple srd_session_new() calls must work. */
	ret = srd_session_new(&sess1);
	fail_unless(ret == SRD_OK, "srd_session_new() 1 failed: %d.", ret);
	ret = srd_session_new(&sess2);
	fail_unless(ret == SRD_OK, "srd_session_new() 2 failed: %d.", ret);
	ret = srd_session_new(&sess3);
	fail_unless(ret == SRD_OK, "srd_session_new() 3 failed: %d.", ret);

	/* The returned session pointers must all be non-NULL. */
	fail_unless(sess1 != NULL);
	fail_unless(sess2 != NULL);
	fail_unless(sess3 != NULL);

	/* The returned session pointers must not be the same. */
	fail_unless(sess1 != sess2);
	fail_unless(sess1 != sess3);
	fail_unless(sess2 != sess3);

	/* Each session must have another ID than any other session. */
	fail_unless(sess1->session_id != sess2->session_id);
	fail_unless(sess1->session_id != sess3->session_id);
	fail_unless(sess2->session_id != sess3->session_id);

	/* Destroying any of the sessions must work. */
	ret = srd_session_destroy(sess1);
	fail_unless(ret == SRD_OK, "srd_session_destroy() 1 failed: %d.", ret);
	ret = srd_session_destroy(sess2);
	fail_unless(ret == SRD_OK, "srd_session_destroy() 2 failed: %d.", ret);
	ret = srd_session_destroy(sess3);
	fail_unless(ret == SRD_OK, "srd_session_destroy() 3 failed: %d.", ret);

	srd_exit();
}
END_TEST

/*
 * Check whether srd_session_destroy() works.
 * If it returns != SRD_OK (or segfaults) this test will fail.
 */
START_TEST(test_session_destroy)
{
	int ret;
	struct srd_session *sess;

	srd_init(NULL);
	srd_session_new(&sess);
	ret = srd_session_destroy(sess);
	fail_unless(ret == SRD_OK, "srd_session_destroy() failed: %d.", ret);
	srd_exit();
}
END_TEST

/*
 * Check whether srd_session_destroy() fails for bogus sessions.
 * If it returns SRD_OK (or segfaults) this test will fail.
 */
START_TEST(test_session_destroy_bogus)
{
	int ret;

	srd_init(NULL);
	ret = srd_session_destroy(NULL);
	fail_unless(ret != SRD_OK, "srd_session_destroy() failed: %d.", ret);
	srd_exit();
}
END_TEST

static void conf_check_ok(struct srd_session *sess, int key, uint64_t x)
{
	int ret;

	ret = srd_session_metadata_set(sess, key, g_variant_new_uint64(x));
	fail_unless(ret == SRD_OK, "srd_session_metadata_set(%p, %d, %"
		PRIu64 ") failed: %d.", sess, key, x, ret);
}

static void conf_check_fail(struct srd_session *sess, int key, uint64_t x)
{
	int ret;
	GVariant *value = g_variant_new_uint64(x);

	ret = srd_session_metadata_set(sess, key, value);
	fail_unless(ret != SRD_OK, "srd_session_metadata_set(%p, %d, %"
		PRIu64 ") worked.", sess, key, x);
	if (ret != SRD_OK)
		g_variant_unref(value);
}

static void conf_check_fail_null(struct srd_session *sess, int key)
{
	int ret;

	ret = srd_session_metadata_set(sess, key, NULL);
	fail_unless(ret != SRD_OK,
		"srd_session_metadata_set(NULL) for key %d worked.", key);
}

static void conf_check_fail_str(struct srd_session *sess, int key, const char *s)
{
	int ret;
	GVariant *value = g_variant_new_string(s);

	ret = srd_session_metadata_set(sess, key, value);
	fail_unless(ret != SRD_OK, "srd_session_metadata_set() for key %d "
		"failed: %d.", key, ret);
	if (ret != SRD_OK)
		g_variant_unref(value);
}

/*
 * Check whether srd_session_metadata_set() works.
 * If it returns != SRD_OK (or segfaults) this test will fail.
 */
START_TEST(test_session_metadata_set)
{
	uint64_t i;
	struct srd_session *sess;

	srd_init(NULL);
	srd_session_new(&sess);
	/* Try a bunch of values. */
	for (i = 0; i < 1000; i++)
		conf_check_ok(sess, SRD_CONF_SAMPLERATE, i);
	/* Try the max. possible value. */
	conf_check_ok(sess, SRD_CONF_SAMPLERATE, UINT64_MAX);
	srd_session_destroy(sess);
	srd_exit();
}
END_TEST

/*
 * Check whether srd_session_metadata_set() fails with invalid input.
 * If it returns SRD_OK (or segfaults) this test will fail.
 */
START_TEST(test_session_metadata_set_bogus)
{
	struct srd_session *sess;

	srd_init(NULL);
	srd_session_new(&sess);

	/* Incorrect GVariant type (currently only uint64 is used). */
	conf_check_fail_str(sess, SRD_CONF_SAMPLERATE, "");
	conf_check_fail_str(sess, SRD_CONF_SAMPLERATE, "Foo");

	/* NULL data pointer. */
	conf_check_fail_null(sess, SRD_CONF_SAMPLERATE);

	/* NULL session. */
	conf_check_fail(NULL, SRD_CONF_SAMPLERATE, 0);

	/* Invalid keys. */
	conf_check_fail(sess, -1, 0);
	conf_check_fail(sess, 9, 0);
	conf_check_fail(sess, 123, 0);

	srd_session_destroy(sess);
	srd_exit();
}
END_TEST

/*
 * Check whether srd_session_terminate_reset() succeeds on newly created
 * sessions, as well as after calling start() and meta(). No data is fed
 * to decoders here.
 */
START_TEST(test_session_reset_nodata)
{
	struct srd_session *sess;
	int ret;
	GVariant *data;

	srd_init(NULL);
	srd_session_new(&sess);
	ret = srd_session_terminate_reset(sess);
	fail_unless(ret == SRD_OK, "srd_session_terminate_reset() failed: %d.", ret);
	ret = srd_session_start(sess);
	fail_unless(ret == SRD_OK, "srd_session_start() failed: %d.", ret);
	ret = srd_session_terminate_reset(sess);
	fail_unless(ret == SRD_OK, "srd_session_terminate_reset() failed: %d.", ret);
	data = g_variant_new_uint64(1000000);
	ret = srd_session_metadata_set(sess, SRD_CONF_SAMPLERATE, data);
	fail_unless(ret == SRD_OK, "srd_session_metadata_set() failed: %d.", ret);
	ret = srd_session_terminate_reset(sess);
	fail_unless(ret == SRD_OK, "srd_session_terminate_reset() failed: %d.", ret);
	ret = srd_session_destroy(sess);
	fail_unless(ret == SRD_OK, "srd_session_destroy() failed: %d.", ret);
	srd_exit();
}
END_TEST

START_TEST(test_wait_match_semantics)
{
	static const uint8_t samples[] = { 0x00, 0x00, 0x01, 0x01, 0x03, 0x03 };
	static const uint64_t expected_samples[] = { 2, 4, 4, 5 };
	static const char *expected_matches[] = { "11", "1", "1", "01" };
	struct srd_decoder_inst *inst;
	struct srd_session *sess;
	struct match_result *result;
	GHashTable *channels;
	GArray *results;
	guint i;
	int ret;

	srd_init(TEST_DECODERS_DIR);
	ret = srd_decoder_load("wait_match");
	fail_unless(ret == SRD_OK, "Failed to load wait_match decoder: %d.", ret);
	srd_session_new(&sess);
	inst = srd_inst_new(sess, "wait_match", NULL);
	fail_unless(inst != NULL, "Failed to create wait_match instance.");

	channels = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
			(GDestroyNotify)g_variant_unref);
	g_hash_table_insert(channels, g_strdup("a"), g_variant_ref_sink(g_variant_new_int32(0)));
	g_hash_table_insert(channels, g_strdup("b"), g_variant_ref_sink(g_variant_new_int32(1)));
	ret = srd_inst_channel_set_all(inst, channels);
	g_hash_table_destroy(channels);
	fail_unless(ret == SRD_OK, "Failed to set wait_match channels: %d.", ret);

	results = g_array_new(FALSE, FALSE, sizeof(struct match_result));
	srd_pd_output_callback_add(sess, SRD_OUTPUT_ANN, match_output_callback, results);
	ret = srd_session_start(sess);
	fail_unless(ret == SRD_OK, "Failed to start wait_match session: %d.", ret);

	/* Split immediately before the first match to exercise chunk continuity. */
	ret = srd_session_send(sess, 0, 2, samples, 2, 1);
	fail_unless(ret == SRD_OK, "Failed to send first sample chunk: %d.", ret);
	ret = srd_session_send(sess, 2, G_N_ELEMENTS(samples), samples + 2,
			G_N_ELEMENTS(samples) - 2, 1);
	fail_unless(ret == SRD_OK, "Failed to send second sample chunk: %d.", ret);
	ret = srd_session_send_eof(sess);
	fail_unless(ret == SRD_OK, "Failed to send EOF: %d.", ret);

	fail_unless(results->len == G_N_ELEMENTS(expected_samples),
			"Expected %u matches, got %u.",
			(unsigned int)G_N_ELEMENTS(expected_samples), results->len);
	for (i = 0; i < results->len; i++) {
		result = &g_array_index(results, struct match_result, i);
		fail_unless(result->sample == expected_samples[i],
				"Match %u occurred at sample %" PRIu64 ", expected %" PRIu64 ".",
				i, result->sample, expected_samples[i]);
		fail_unless(!strcmp(result->matched, expected_matches[i]),
				"Match %u was %s, expected %s.", i,
				result->matched, expected_matches[i]);
	}

	match_results_clear(results);
	srd_session_destroy(sess);
	srd_exit();
}
END_TEST

Suite *suite_session(void)
{
	Suite *s;
	TCase *tc;

	s = suite_create("session");

	tc = tcase_create("new_destroy");
	tcase_add_checked_fixture(tc, srdtest_setup, srdtest_teardown);
	tcase_add_test(tc, test_session_new);
	tcase_add_test(tc, test_session_new_bogus);
	tcase_add_test(tc, test_session_new_multiple);
	tcase_add_test(tc, test_session_destroy);
	tcase_add_test(tc, test_session_destroy_bogus);
	suite_add_tcase(s, tc);

	tc = tcase_create("config");
	tcase_add_checked_fixture(tc, srdtest_setup, srdtest_teardown);
	tcase_add_test(tc, test_session_metadata_set);
	tcase_add_test(tc, test_session_metadata_set_bogus);
	suite_add_tcase(s, tc);

	tc = tcase_create("reset");
	tcase_add_test(tc, test_session_reset_nodata);
	suite_add_tcase(s, tc);

	tc = tcase_create("wait_match");
	tcase_add_checked_fixture(tc, srdtest_setup, srdtest_teardown);
	tcase_add_test(tc, test_wait_match_semantics);
	suite_add_tcase(s, tc);

	return s;
}
