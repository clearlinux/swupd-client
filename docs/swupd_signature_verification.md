# Swupd Signature Verification

## Checking the integrity of a file

Swupd contains multiple subcommands that install files in the system, including but not limited to update, bundle-add, verify. Before installing any file to the system root swupd needs to check the downloaded file integrity.

Swupd uses a chain of files to validate the content and attributes of any file to be installed. A simplified description of the process to install a file F, that belongs to bundle B in version V of Clear Linux is:
Download (if needed) the MoM (Manifest of Manifests) of version V of Clear Linux and its correspondent signature
Check if MoM signature was signed with swupd root certificate installed on the system.
Download (if needed) the Manifest of bundle B and calculate its hash. Check if the listed hash for bundle B in MoM is correct.
Download (if needed) file F and calculate its hash. Check if the listed hash for file F in bundle B Manifest is correct.
Install the file.

The hash used for that operation is a SHA256 of the content and attributes of the file in question (F or Manifest of bundle B). Swupd hashdump command can be used to calculate the hash for debugging purposes.

## MoM signature verification

The Manifest.MoM file used by swupd should have a corresponding PKCS7 file with the suffix .sig that is going to be used to validate the Manifest.MoM integrity. Swupd uses certificates on the certificate store located on /usr/share/clear/update-ca/Swupd_Root.pem to validate the signature in the PKCS7 bundle. Chained certificates inside the PKCS7 bundle is supported if they are signed by any certificate in the Swupd_Root.pem store.

The openssl command line that is equivalent to the operation performed by swupd is:

openssl smime -verify -in Manifest.MoM.sig -inform DER -content Manifest.MoM -purpose any -CAfile /usr/share/clear/update-ca/Swupd_Root.pem

## Build time parameters

 - Disable the signature verification (--disable-signature-verification):
Don’t download .sig files and always assume MoM integrity isn’t compromised. Hashes for Bundle Manifests and files are still going to be performed.
**Warning**: This is not recommended and all swupd operations that install files are going to be insecure.

 - Change certificate path (--with-certpath=PATH)
Change to PATH the default pem file to be used on MoM signature verification. Default value is /usr/share/clear/update-ca/Swupd_Root.pem.

## Runtime parameters

 - Change certificate path (-C, --certpath)
Change in runtime to PATH the default pem file to be used on MoM signature verification. The pem file on PATH will be used instead of the default pem file defined in compile time.

 - Disable signature verification (-n, --nosigcheck)
Proceed even if signature verification fails. Warns the user about that if signature is invalid.
**Warning**: This is not recommended and all swupd operations that install files are going
to be insecure.

Note: Runtime paremeters can also be set using the [swupd config file](https://github.com/clearlinux/swupd-client/blob/master/config)

## Examples

Examples on how to create and keep keys to be used with swupd. This command lines are examples only and we suggest you to check openssl documentation on how to create keys with the level of security expected for your use case.

All examples listed below use a 1 day expiry date which is unrealistic for a real use case. The only purpose of this documentation is to explain what’s supported by swupd and shouldn’t be used as a guide on how to sign Clear Linux based distributions.

### 1 - Using a self-signed certificate

```
# Generate a self-signed certificate
openssl genrsa -out root.key 4096
openssl req -days 1 -new -x509 -key root.key -out Swupd_Root.pem -subj "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost"

# Sign the MoM file
openssl smime -sign -binary -in Manifest.MoM -signer Swupd_Root.pem -inkey root.key -outform DER -out Manifest.MoM.sig

# Check if the signature is correct
openssl smime -verify -in Manifest.MoM.sig -inform DER -content Manifest.MoM -purpose any -CAfile Swupd_Root.pem
```

### 2 - Using a chained certificate with a self-signed root certificate

```
# Generate a self-signed root certificate
openssl genrsa -out root.key 4096
openssl req -days 1 -new -x509 -key root.key -out Swupd_Root.pem -subj "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=localhost"

# Generate an intermediate key and sign it using the root certificate
openssl genrsa -out intermediate.key 4096
openssl req -new -key intermediate.key -out intermediate.csr -subj "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=intermediate"
openssl x509 -days 1 -req -in intermediate.csr -CA Swupd_Root.pem -CAkey root.key -set_serial 0x1 -out intermediate.pem

# Sign the MoM file using the intermediate key
openssl smime -sign -binary -in Manifest.MoM -signer intermediate.pem -inkey intermediate.key -out Manifest.MoM.sig -outform DER

# Check if the signature is correct using the root key
openssl smime -verify -in Manifest.MoM.sig -inform DER -content Manifest.MoM -purpose any -CAfile Swupd_Root.pem

# Note that in this example the key used to sign the updates is the intermediate key and the certificate used by clients to validate the signature is the Swupd_Root.pem
```

### 2b - Rotating the intermediate key

One advantage on using the solution 2 is that it’s possible to rotate the intermediate key if necessary. Instructions on how to perform this operation are following.

```
# Generate and sign the new intermediate key
openssl genrsa -out new_intermediate.key 4096
openssl req -new -key new_intermediate.key -out new_intermediate.csr -subj "/C=US/ST=Oregon/L=Portland/O=Company Name/OU=Org/CN=new_intermediate"
openssl x509 -days 1 -req -in new_intermediate.csr -CA Swupd_Root.pem -CAkey root.key -set_serial 0x1 -out new_intermediate.pem

# Sign the MoM file using the new key
openssl smime -sign -binary -in Manifest.MoM -signer new_intermediate.pem -inkey new_intermediate.key -out Manifest.MoM.sig -outform DER

# Check if the signature is correct using the same root key
openssl smime -verify -in Manifest.MoM.sig -inform DER -content Manifest.MoM -purpose any -CAfile Swupd_Root.pem
```

## Certificate Revocation

### Certificate Revocation List

Usage of Certificate Revocation list is implemented in src/signature.c, but isn’t currently used on swupd. To enable that, calls to signature_init() should set the crl parameter pointing to a file with the Certificate Revocation List.

### OCSP (Online Certificate Status Protocol)

OCSP isn’t yet supported on swupd. If a certificate with OCSP set and flagged as critical is used, swupd will abort with a warning saying that OCSP isn’t supported.

The check of the OCSP flag is in validate_authority() function on src/signature.c and that’s probably the best location to add the OCSP implementation if needed.

