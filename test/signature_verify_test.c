/*
 *   Software Updater - client side
 *
 *      Copyright © 2014-2016 Intel Corporation.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, version 2 or later of the License.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Authors:
 *         Tom Keel <thomas.keel@intel.com>
 *
 */

/*
 * Usage: thisprogram <data> <data-sig> <ca-cert>
 *
 *   <data> - file containing some arbitrary data
 *   <data-sig> - file containing the purported signature of the data --
 *       a PEM-encoded PKCS#7 structure containing both the signature and
 *       a certificate containing the key used to do the signing. That
 *       certificate is signed with the private key of the <ca-cert>.
 *   <ca-cert> - a CA certificate containing a public key for validating the
 *       public key in the certificate in <data-sig>
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "signature.h"

static void usage(char *);

int main(int argc, char **argv)
{
	if (argc != 4) {
		usage(argv[0]);
	}

	if (download_and_verify_signature(argv[1], argv[2], 10, false)) {
		fprintf(stderr, "Verification successful!\n");
	} else {
		fprintf(stderr, "Verification failed!\n");
	}

	return 0;
}

static void usage(char *prog_name)
{
	fprintf(stderr, "Usage: %s data signature rootcert\n", prog_name);
	exit(-1);
}
