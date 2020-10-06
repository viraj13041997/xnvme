// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvmec.h>
#include <omp.h>

#define MAX_LISTINGS 1024
#define MAX_HANDLES 1024

static int
test_enum(struct xnvmec *cli)
{
	struct xnvme_enumeration *listing[MAX_LISTINGS] = { 0 };
	uint64_t nlistings = 2;
	int nerr = 0, err;

	nlistings = XNVME_MAX(nlistings, cli->args.count);
	if (nlistings > MAX_LISTINGS) {
		xnvmec_pinf("--count: %ld > %d", cli->args.count, MAX_LISTINGS);
		return -EINVAL;
	}
	xnvmec_pinf("Will enumerate %ld times", nlistings);

	for (uint64_t i = 0; i < nlistings; ++i) {
		err = xnvme_enumerate(&listing[i], cli->args.sys_uri, cli->args.flags);
		if (err) {
			nerr += 1;
			xnvmec_perr("xnvme_enumerate()", err);
			xnvmec_pinf("The %ld' xnvme_enumerate() failed", i + 1);
		}

		if (i && (listing[i]->nentries != listing[i - 1]->nentries)) {
			nerr += 1;
			xnvmec_pinf("The enumeration %ld did not match the prev", i);

			xnvme_enumeration_pr(listing[i], XNVME_PR_DEF);
			xnvme_enumeration_pr(listing[i - 1], XNVME_PR_DEF);
		}
	}
	if (nerr) {
		goto exit;
	}

exit:
	printf("\n");
	if (nerr) {
		xnvmec_pinf("--={[ Got Errors - see details above ]}=--");
		xnvmec_pinf("nerr: %d", nerr);
	} else {
		xnvmec_pinf("LGMT: xnvme_enumerate()");
	}
	printf("\n");

	for (uint64_t i = 0; i < nlistings; ++i) {
		free(listing[i]);
	}

	return nerr ? -ENOMEM : 0;
}

static int
test_enum_open(struct xnvmec *cli)
{
	struct xnvme_enumeration *listing = NULL;
	int count = 1;
	int nerr = 0, err;

	err = xnvme_enumerate(&listing, cli->args.sys_uri, cli->args.flags);
	if (err) {
		nerr += 1;
		xnvmec_perr("xnvme_enumerate()", err);
		goto exit;
	}
	xnvme_enumeration_pr(listing, XNVME_PR_DEF);

	if (cli->args.count) {
		count = cli->args.count > MAX_HANDLES ? MAX_HANDLES : cli->args.count;
	}

	for (int64_t i = 0; i < listing->nentries; ++i) {
		struct xnvme_dev *dev[MAX_HANDLES] = { 0 };

		for (int hidx = 0; hidx < count; ++hidx) {
			dev[hidx] = xnvme_dev_open(listing->entries[i].uri);
			if (!dev[hidx]) {
				nerr += 1;
				xnvmec_pinf("xnvme_dev_open(%s)",
					    listing->entries[i].uri);
				xnvmec_perr("xnvme_dev_open()", errno);
			}
			if (cli->args.verbose) {
				xnvme_dev_pr(dev[i], XNVME_PR_DEF);
			}
		}

		for (int hidx = 0; hidx < count; ++hidx) {
			xnvme_dev_close(dev[hidx]);
			dev[hidx] = NULL;
		}

		if (nerr) {
			goto exit;
		}
	}

exit:
	printf("\n");
	if (nerr) {
		xnvmec_pinf("--={[ Got Errors - see details above ]}=--");
		xnvmec_pinf("nerr: %d", nerr);
	} else {
		xnvmec_pinf("LGMT: xnvme_enumerate() + xnvme_dev_open() * count");
	}
	printf("\n");

	free(listing);

	return nerr ? -ENOMEM : 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub g_subs[] = {
	{
		"multi",
		"Call xnvme_enumerate() 'count' times",
		"Call xnvme_enumerate() 'count' times",
		test_enum, {
			{XNVMEC_OPT_SYS_URI, XNVMEC_LOPT},
			{XNVMEC_OPT_COUNT, XNVMEC_LOPT},
			{XNVMEC_OPT_VERBOSE, XNVMEC_LFLG}
		},
	},
	{
		"open",
		"Call xnvme_enumerate() once, open each device 'count' times",
		"Call xnvme_enumerate() once, open each device 'count' times\n"
		"Dump info on each opened device with --verbose",
		test_enum_open, {
			{XNVMEC_OPT_SYS_URI, XNVMEC_LOPT},
			{XNVMEC_OPT_COUNT, XNVMEC_LOPT},
			{XNVMEC_OPT_VERBOSE, XNVMEC_LFLG}
		},
	},
};

static struct xnvmec g_cli = {
	.title = "Test xNVMe enumeration",
	.descr_short = "Test xNVMe enumeration",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
