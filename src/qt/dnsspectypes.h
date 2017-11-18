#ifndef DNSSPECTYPES_H
#define DNSSPECTYPES_H

/**
 * Domain
 *     type enum {topLevel, subDomain}
 *     name: QString (add role formatters)
 *     readJson, writeJson
 *
 * A:
 *     "ip"      : "192.168.1.1",
 *     "ip6"     : "2001:4860:0:1001::68",
 *     "tor"     : "eqt5g4fuenphqinx.onion",
 *     "i2p"     : // TODO: decide what to do with this (needs "specifier")
 *     "freenet" : "USK@0I8g...xbZ4,AQACAAE/Example/42/"
 *
 * CNAME:
 *     "alias": "example.bit." // must be a valid dns name
 *
 * SOA/RP:
 *      "email"   : "hostmaster@example.bit",
 *      "info"    : "Example & Sons Co."
 *
 * DNAME:
 *     "translate": "otherhost.bit."
 *
 * NS: [QList of strings]
 *     "ns": ["ns1.example.com.", "ns2.example.com."]
 *     "ns": "ns1.example.com."
 *
 * DS: [QList of 4d-vectors]
 *     "ds": [[12345,8,1,"EfatjsUqKYSrqv18O1FlA3hcIHI="],
 *            [12345,8,2,"LXEWQrcmsEQBYnyp+6wy9chTD7GQPMTbAiWHF5IaSIE="]]
 *
 * TLS: [key-value map]
 *      "tls": {
 *          "tcp": {
 *              "443": [ [1, "660008F91C07DCF9058CDD5AD2BAF6CC9EAE0F912B8B54744CB7643D7621B787", 1] ],
 *              "25": [ [1, "660008F91C07DCF9058CDD5AD2BAF6CC9EAE0F912B8B54744CB7643D7621B787", 1] ]
 *          }
 *      }
 *
 * SRV: [QList of 4d-vectors]
 *      "service" : [ [priorityInt, weightInt, portInt, dnsNameString], ...]
 *
 * TXT:
 *     "txt": "string" // limit to 255 bytes
 *
 * IMPORT: [QList of strings]
 *     "import": ["dd/alpha", "dd/beta"]
 *
 * SSHFP: [QList of 3d-vectors]
 *     "sshfp": [[2,1,"EjRWeJq83vZ4kBI0VniavN72eJA="]]
 *
 * map to be implemented as nested Domain objects (under topLevel)
 *      "map": {
 *          "www" : { "alias": "" },
 *          "ftp" : { "ip": ["10.2.3.4", "10.4.3.2"] },
 *          "mail": { "ns": ["ns1.host.net", "ns12.host.net"] }
 *      }
 */

#endif // DNSSPECTYPES_H
