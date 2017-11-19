#ifndef DNSSPECTYPES_H
#define DNSSPECTYPES_H

#include <QJsonObject>
#include <QString>

#define ADD_JSON_SERIALIZE_METHODS       \
    void read(const QJsonObject &json);  \
    void write(QJsonObject &json) const;

/**
 * Domain type, container for top level domains and mappable
 * subdomains
 *     type enum {topLevel, subDomain}
 *     name: QString (add role formatters)
 *     readJson, writeJson
 *
 * map to be implemented as nested Domain objects (under topLevel)
 *      "map": {
 *          "www" : { "alias": "" },
 *          "ftp" : { "ip": ["10.2.3.4", "10.4.3.2"] },
 *          "mail": { "ns": ["ns1.host.net", "ns12.host.net"] }
 *      }
 */
class Domain
{
public:
    enum DomainType {
        TopLevel, SubDomain
    };
    explicit Domain(const QString &name, DomainType type);

    QString name() const;
    void setName(const QString &name);

    void read(const QJsonObject &json);
    void write(QJsonObject &json) const;

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString name;
}

/**
 * A/AAAA record.
 *   "ip"      : "192.168.1.1",
 *   "ip6"     : "2001:4860:0:1001::68",
 *   "tor"     : "eqt5g4fuenphqinx.onion",
 *   "i2p"     : // TODO: decide what to do with this (needs "specifier")
 *   "freenet" : "USK@0I8g...xbZ4,AQACAAE/Example/42/"
 */
class A
{
public:
    enum ResourceType {
        IPv4, IPv6, Tor, I2P, Freenet
    };
    explicit A();
    explicit A(const QString &host, ResourceType type);

    QString host() const;
    void setHost(const QString &host);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString host;
}

/**
 * CNAME:
 *   "alias": "example.bit." // must be a valid dns name
 */
class CNAME
{
public:
    explicit CNAME();
    explicit CNAME(const QString &host);

    QString host() const;
    void setHost(const QString &host);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString host;
}

/**
 * SOA/RP:
 *   "email"   : "hostmaster@example.bit",
 *   "info"    : "Example & Sons Co."
 */
class SOARP
{
public:
    explicit SOARP();
    explicit SOARP(const QString &email, const QString &info);

    QString email() const;
    void setEmail(const QString &email);

    QString info() const;
    void setInfo(const QString &info);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString email;
    QString info;
}

/**
 * DNAME:
 *   "translate": "otherhost.bit."*
 */
class DNAME
{
public:
    explicit DNAME();
    explicit DNAME(const QString &host);

    QString host() const;
    void setHost(const QString &host);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString host;
}

/**
 * NS: [QList of strings]
 *   "ns": ["ns1.example.com.", "ns2.example.com."]
 *   "ns": "ns1.example.com."
 */
class NS
{
public:
    explicit NS();
    explicit NS(QList<QString> hosts);

    QList<QString> hosts() const;
    void addHost(const QString &host);
    void removeHost(const QString &host);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<QString> hosts;
}

/**
 * DS: [QList of 4d-vectors]
 *     "ds": [[12345,8,1,"EfatjsUqKYSrqv18O1FlA3hcIHI="],
 *            [12345,8,2,"LXEWQrcmsEQBYnyp+6wy9chTD7GQPMTbAiWHF5IaSIE="]]
 */
class DS
{
public:
    explicit DS();
    // TODO: reconsider this format
    explicit DS(QList<QList<QString>> records);

    QList<QList<QString>> records() const;
    void addRecord(QList<QString> record);
    void removeRecord(int index);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<QList<QString>> records;
}

/**
 * TLS: [key-value map]
 *   "tls": {
 *     "tcp": {
 *       "443": [ [1, "660008F91...D7621B787", 1] ],
 *       "25": [ [1, "660008F91...D7621B787", 1] ]
 *     }
 *   }
 */
class TLS
{
public:
    explicit TLS();
    explicit TLS(QMap tlsRecords);

    QMap tlsRecords() const;

    ADD_JSON_SERIALIZE_METHODS;
private:
    QMap tlsRecords;
}

/**
 * SRV: [QList of 4d-vectors]
 *   "service" : [ [priorityInt, weightInt, portInt, dnsNameString], ...]
 */
class SRV
{
public:
    explicit SRV();
    explicit SRV(QList<QList<QString>> records);

    QList<QList<QString>> records() const;

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<QList<QString>> records;
}

/**
 * TXT:
 *   "txt": "string" // limit to 255 bytes
 */
class TXT
{
public:
    explicit TXT();
    explicit TXT(const QString &data);

    QString data() const;
    void setData(const QString &data);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QString data;
}

/**
 * IMPORT: [QList of strings]
 *     "import": ["dd/alpha", "dd/beta"]
 */
class IMPORT
{
public:
    explicit IMPORT();
    explicit IMPORT(QList<QString> imports);

    QList<QString> imports() const;
    void setImport(const QString &import);
    void removeImport(const QString &import);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<QString> imports;
}

/**
 * SSHFP: [QList of 3d-vectors]
 *     "sshfp": [[2,1,"EjRWeJq83vZ4kBI0VniavN72eJA="]]
 */
class SSHFP
{
public:
    explicit SSHFP();
    explicit SSHFP(QList<QList<QString>> fingerprints);

    QList<QList<QString>> fingerprints() const;
    void addRecord(QList<QString> fingerprint);
    void removeRecord(int index);

    ADD_JSON_SERIALIZE_METHODS;
private:
    QList<QList<QString>> fingerprints;
}

#endif // DNSSPECTYPES_H
