/*
#    copyright 2019 Open Interconnect Consortium, Inc. All rights reserved.
#    Redistribution and use in source and binary forms, with or without modification,
#    are permitted provided that the following conditions are met:
#    1.  Redistributions of source code must retain the above copyright notice,
#        this list of conditions and the following disclaimer.
#    2.  Redistributions in binary form must reproduce the above copyright notice,
#        this list of conditions and the following disclaimer in the documentation and/or other materials provided
#        with the distribution.
#
#    THIS SOFTWARE IS PROVIDED BY THE OPEN INTERCONNECT CONSORTIUM, INC. "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
#    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE OR
#    WARRANTIES OF NON-INFRINGEMENT, ARE DISCLAIMED. IN NO EVENT SHALL THE OPEN INTERCONNECT CONSORTIUM, INC. OR
#    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
#    OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#    EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef PKI_CERT_INCLUDE_H
#define PKI_CERT_INCLUDE_H
#if defined(OC_SECURITY) && defined(OC_PKI)

/* PKI certificate data
 input file = pki_certs.zip
 prefix = pki_certs
 date 2021-10-18 11:05:50
*/

const char *my_cert = "-----BEGIN CERTIFICATE-----\r\n"
                      "MIID/TCCA6OgAwIBAgIJAI0K+3tTslN8MAoGCCqGSM49BAMCMFsxDDAKBgNVBAoM\r\n"
                      "A09DRjEiMCAGA1UECwwZS3lyaW8gVGVzdCBJbmZyYXN0cnVjdHVyZTEnMCUGA1UE\r\n"
                      "AwweS3lyaW8gVEVTVCBJbnRlcm1lZGlhdGUgQ0EwMDAyMB4XDTIxMTAxODExMDU1\r\n"
                      "MFoXDTIxMTExNzExMDU1MFowRjEMMAoGA1UECgwDT0NGMSIwIAYDVQQLDBlLeXJp\r\n"
                      "byBUZXN0IEluZnJhc3RydWN0dXJlMRIwEAYDVQQDDAlwa2lfY2VydHMwWTATBgcq\r\n"
                      "hkjOPQIBBggqhkjOPQMBBwNCAAQxWY9uTXQdARS01/FtaouP28ysGYq9KX2IQJqP\r\n"
                      "PlEB+++7eG5Qc7oZ15DtMXV9RMRtaeAKm8MQpK7HoGhH123Yo4ICYzCCAl8wCQYD\r\n"
                      "VR0TBAIwADAOBgNVHQ8BAf8EBAMCA4gwKQYDVR0lBCIwIAYIKwYBBQUHAwIGCCsG\r\n"
                      "AQUFBwMBBgorBgEEAYLefAEGMB0GA1UdDgQWBBTDzP0hu7MtiU4go/Uj7pxxkCJX\r\n"
                      "djAfBgNVHSMEGDAWgBQZc2oEGgsHcE9TeVM2h/wMunyuCzCBlgYIKwYBBQUHAQEE\r\n"
                      "gYkwgYYwXQYIKwYBBQUHMAKGUWh0dHA6Ly90ZXN0cGtpLmt5cmlvLmNvbS9vY2Yv\r\n"
                      "Y2FjZXJ0cy9CQkU2NEY5QTdFRTM3RDI5QTA1RTRCQjc3NTk1RjMwOEJFNDFFQjA3\r\n"
                      "LmNydDAlBggrBgEFBQcwAYYZaHR0cDovL3Rlc3RvY3NwLmt5cmlvLmNvbTBfBgNV\r\n"
                      "HR8EWDBWMFSgUqBQhk5odHRwOi8vdGVzdHBraS5reXJpby5jb20vb2NmL2NybHMv\r\n"
                      "QkJFNjRGOUE3RUUzN0QyOUEwNUU0QkI3NzU5NUYzMDhCRTQxRUIwNy5jcmwwGAYD\r\n"
                      "VR0gBBEwDzANBgsrBgEEAYORVgABAjBlBgorBgEEAYORVgEABFcwVTAJAgECAgEA\r\n"
                      "AgEAMDYMGTEuMy42LjEuNC4xLjUxNDE0LjAuMC4xLjAMGTEuMy42LjEuNC4xLjUx\r\n"
                      "NDE0LjAuMC4yLjAMC0RldmljZU5hbWUxDANPQ0YwKgYKKwYBBAGDkVYBAQQcMBoG\r\n"
                      "CysGAQQBg5FWAQEABgsrBgEEAYORVgEBATAwBgorBgEEAYORVgECBCIwIAwOMS4z\r\n"
                      "LjYuMS40LjEuNzEMCURpc2NvdmVyeQwDMS4wMAoGCCqGSM49BAMCA0gAMEUCIF3d\r\n"
                      "OzSi1QZ4ky43uL5Ag3CdA9/vo/ZhzudR5B7g5hNWAiEA5I5lDJeJUmr5X1a72oxJ\r\n"
                      "737Dz6ppPuY10v+Sf8mD97g=\r\n"
                      "-----END CERTIFICATE-----\r\n";

const char *my_key = "-----BEGIN EC PARAMETERS-----\r\n"
                     "BggqhkjOPQMBBw==\r\n"
                     "-----END EC PARAMETERS-----\r\n"
                     "-----BEGIN EC PRIVATE KEY-----\r\n"
                     "MHcCAQEEIOT8CJzE/QngjfBEYWsAHuuab/nkmBmRp5rypOp2ljWzoAoGCCqGSM49\r\n"
                     "AwEHoUQDQgAEMVmPbk10HQEUtNfxbWqLj9vMrBmKvSl9iECajz5RAfvvu3huUHO6\r\n"
                     "GdeQ7TF1fUTEbWngCpvDEKSux6BoR9dt2A==\r\n"
                     "-----END EC PRIVATE KEY-----\r\n";

const char *int_ca = "-----BEGIN CERTIFICATE-----\r\n"
                     "MIIC+jCCAqGgAwIBAgIJAPObjMBXKhG1MAoGCCqGSM49BAMCMFMxDDAKBgNVBAoM\r\n"
                     "A09DRjEiMCAGA1UECwwZS3lyaW8gVGVzdCBJbmZyYXN0cnVjdHVyZTEfMB0GA1UE\r\n"
                     "AwwWS3lyaW8gVEVTVCBST09UIENBMDAwMjAeFw0xODExMzAxODEyMTVaFw0yODEx\r\n"
                     "MjYxODEyMTVaMFsxDDAKBgNVBAoMA09DRjEiMCAGA1UECwwZS3lyaW8gVGVzdCBJ\r\n"
                     "bmZyYXN0cnVjdHVyZTEnMCUGA1UEAwweS3lyaW8gVEVTVCBJbnRlcm1lZGlhdGUg\r\n"
                     "Q0EwMDAyMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEvA+Gn3ofRpH40XuVppBR\r\n"
                     "f78mDtfclOkBd7/32yQcmK2LQ0wm/uyl2cyeABPuN6NFcR9+LYkXZ5P4Ovy9R43Q\r\n"
                     "vqOCAVQwggFQMBIGA1UdEwEB/wQIMAYBAf8CAQAwDgYDVR0PAQH/BAQDAgGGMB0G\r\n"
                     "A1UdDgQWBBQZc2oEGgsHcE9TeVM2h/wMunyuCzAfBgNVHSMEGDAWgBQoSOTlJ1jZ\r\n"
                     "CO4JNOSxuz1ZZh/I9TCBjQYIKwYBBQUHAQEEgYAwfjBVBggrBgEFBQcwAoZJaHR0\r\n"
                     "cDovL3Rlc3Rwa2kua3lyaW8uY29tL29jZi80RTY4RTNGQ0YwRjJFNEY4MEE4RDE0\r\n"
                     "MzhGNkExQkE1Njk1NzEzRDYzLmNydDAlBggrBgEFBQcwAYYZaHR0cDovL3Rlc3Rv\r\n"
                     "Y3NwLmt5cmlvLmNvbTBaBgNVHR8EUzBRME+gTaBLhklodHRwOi8vdGVzdHBraS5r\r\n"
                     "eXJpby5jb20vb2NmLzRFNjhFM0ZDRjBGMkU0RjgwQThEMTQzOEY2QTFCQTU2OTU3\r\n"
                     "MTNENjMuY3JsMAoGCCqGSM49BAMCA0cAMEQCHwXkRYd+u5pOPH544wBmBRJz/b0j\r\n"
                     "ppvUIHx8IUH0CioCIQDC8CnMVTOC5aIoo5Yg4k7BDDNxbRQoPujYes0OTVGgPA==\r\n"
                     "-----END CERTIFICATE-----\r\n";

const char *root_ca = "-----BEGIN CERTIFICATE-----\r\n"
                      "MIIB3zCCAYWgAwIBAgIJAPObjMBXKhGyMAoGCCqGSM49BAMCMFMxDDAKBgNVBAoM\r\n"
                      "A09DRjEiMCAGA1UECwwZS3lyaW8gVGVzdCBJbmZyYXN0cnVjdHVyZTEfMB0GA1UE\r\n"
                      "AwwWS3lyaW8gVEVTVCBST09UIENBMDAwMjAeFw0xODExMzAxNzMxMDVaFw0yODEx\r\n"
                      "MjcxNzMxMDVaMFMxDDAKBgNVBAoMA09DRjEiMCAGA1UECwwZS3lyaW8gVGVzdCBJ\r\n"
                      "bmZyYXN0cnVjdHVyZTEfMB0GA1UEAwwWS3lyaW8gVEVTVCBST09UIENBMDAwMjBZ\r\n"
                      "MBMGByqGSM49AgEGCCqGSM49AwEHA0IABGt1sU2QhQcK/kflKSF9TCrvKaDckLWd\r\n"
                      "ZoyvP6z0OrqNdtBscZgVYsSHMQZ1R19wWxsflvNr8bMVW1K3HWMkpsijQjBAMA8G\r\n"
                      "A1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/BAQDAgGGMB0GA1UdDgQWBBQoSOTlJ1jZ\r\n"
                      "CO4JNOSxuz1ZZh/I9TAKBggqhkjOPQQDAgNIADBFAiAlMUwgVeL8d5W4jZdFJ5Zg\r\n"
                      "clk7XT66LNMfGkExSjU1ngIhANOvTmd32A0kEtIpHbiKA8+RFDCPJWjN4loxrBC7\r\n"
                      "v0JE\r\n"
                      "-----END CERTIFICATE-----\r\n";

#endif /* OC_SECURITY && OC_PKI */
#endif /* PKI_CERT_INCLUDE_H */
