#!/bin/bash
CERTIFICATE=$1
DATA=$2
SIGNATURE=$3


if [ -z "$CERTIFICATE" ]; then
	echo "ERROR: Missing certificate parameter";
	exit 1
fi
if [ -z "$DATA" ]; then
	echo "ERROR: Missing data parameter";
	exit 1
fi
if [ -z "$SIGNATURE" ]; then
	echo "ERROR: Missing signature parameter";
	exit 1
fi

openssl_verify()
{
	openssl smime -verify -in "$1" -inform der -content "$2" -CAfile "$3" > /dev/null 2>&1

}

create_fake_signature()
{
	openssl genpkey -algorithm RSA -out key.pem > /dev/null 2>&1
	openssl dgst -sha256 -sign key.pem -out fake.sig "$DATA"
	mv fake.sig "$TEMPSIG"
	rm -f key.pem
}

corrupt_certificate()
{
	sed -i '10,$ d' "$TEMPCERT"
}

corrupt_data()
{
	echo "deadcafe" >> "$TEMPDATA"
}

set_tempfiles()
{
	cp "$CERTIFICATE" "$TEMPCERT"
	cp "$DATA" "$TEMPDATA"
	cp "$SIGNATURE" "$TEMPSIG"
}

clean_tempfiles()
{
	rm "$TEMPCERT" "$TEMPDATA" "$TEMPSIG"
}
# The tests follow this truth table to decide that the certificate and
# signature are valid. In this table 1 refers to ERROR or INVALID, while 0
# refers to SUCCESS or VALID, so the first line in the table means the
# certificate, data, and signature are invalid and we expect an ERROR return
#
# +------+------+-----++-------+
# | Cert | Data | Sig || Error |
# +------+------+-----++-------+
# |    1 |    1 |   1 ||     1 |
# |    1 |    1 |   0 ||     1 |
# |    1 |    0 |   1 ||     1 |
# |    1 |    0 |   0 ||     1 |
# |    0 |    1 |   1 ||     1 |
# |    0 |    1 |   0 ||     1 |
# |    0 |    0 |   1 ||     1 |
# |    0 |    0 |   0 ||     0 |
# +------+------+-----++-------+

# Row 8, This is the only test that should pass, i.e return 0, as shown in the last row of the table above
openssl_verify "$SIGNATURE" "$DATA" "$CERTIFICATE"
if [ $? -ne 0 ]; then
	echo -e "!!!!Failed to verify data, all inputs were expected to be valid!!!!\n"
	exit 1
fi

# All tests below should NOT return 0, and are testing OpenSSL correctness rather than the content passed in
TEMPCERT=$(mktemp)
TEMPDATA=$(mktemp)
TEMPSIG=$(mktemp)

# Row 1, invalid cert, invalid data, invalid signature provided
set_tempfiles
corrupt_certificate
corrupt_data
create_fake_signature
openssl_verify "$TEMPSIG" "$TEMPDATA" "$TEMPCERT"
if [ $? -eq 0 ]; then
	echo -e "ERROR: Invalid Certificate, Data, and Signature, expected FAIL, returned SUCCESS\n"
	clean_tempfiles
	exit 1
fi

# Row 2, Invalid cert, invalid data, valid signature
set_tempfiles
corrupt_certificate
corrupt_data
openssl_verify "$TEMPSIG" "$TEMPDATA" "$TEMPCERT"
if [ $? -eq 0 ]; then
	echo -e "ERROR: Invalid Certificate and Data, expected FAIL, returned SUCCESS\n"
	clean_tempfiles
	exit 1
fi

# Row 3, Invalid cert, valid data, invalid signature
set_tempfiles
corrupt_certificate
create_fake_signature
openssl_verify "$TEMPSIG" "$TEMPDATA" "$TEMPCERT"
if [ $? -eq 0 ]; then
	echo -e "ERROR: Invalid Certificate and Signature, expected FAIL, returned SUCCESS\n"
	clean_tempfiles
	exit 1
fi

# Row 4, invalid cert, valid data, valid signature
set_tempfiles
corrupt_certificate
openssl_verify "$TEMPSIG" "$TEMPDATA" "$TEMPCERT"
if [ $? -eq 0 ]; then
	echo -e "ERROR: Invalid Certificate, expected FAIL, returned SUCCESS\n"
	clean_tempfiles
	exit 1
fi

# Row 5, valid cert, invalid data, invalid signature
set_tempfiles
corrupt_data
create_fake_signature
openssl_verify "$TEMPSIG" "$TEMPDATA" "$TEMPCERT"
if [ $? -eq 0 ]; then
	echo -e "ERROR: Invalid Data and Signature, expected FAIL, returned SUCCESS\n"
	clean_tempfiles
	exit 1
fi

# Row 6, valid cert, invalid data, valid signature
set_tempfiles
corrupt_data
create_fake_signature
openssl_verify "$TEMPSIG" "$TEMPDATA" "$TEMPCERT"
if [ $? -eq 0 ]; then
	echo -e "ERROR: Invalid Data, expected FAIL, returned SUCCESS\n"
	clean_tempfiles
	exit 1
fi

# Row 7, valid cert, valid data, invalid signature
set_tempfiles
create_fake_signature
openssl_verify "$TEMPSIG" "$TEMPDATA" "$TEMPCERT"
if [ $? -eq 0 ]; then
	echo -e "ERROR: Invalid Signature, expected FAIL, returned SUCCESS\n"
	clean_tempfiles
	exit 1
fi

# The certificate, data, and signature are valid
echo "ALL CLEAN!"
clean_tempfiles
exit 0
